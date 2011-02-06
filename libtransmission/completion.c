/*
 * This file Copyright (C) Mnemosyne LLC
 *
 * This file is licensed by the GPL version 2. Works owned by the
 * Transmission project are granted a special exemption to clause 2(b)
 * so that the bulk of its code can remain under the MIT license.
 * This exemption does not extend to derived works not owned by
 * the Transmission project.
 *
 * $Id$
 */

#include <assert.h>
#include <string.h>

#include "transmission.h"
#include "completion.h"
#include "torrent.h"
#include "torrent-magnet.h"
#include "utils.h"

static void
tr_cpReset( tr_completion * cp )
{
    tr_bitfieldClear( &cp->pieceBitfield );
    tr_bitfieldClear( &cp->blockBitfield );
    memset( cp->completeBlocks, 0, sizeof( uint16_t ) * cp->tor->info.pieceCount );
    cp->sizeNow = 0;
    cp->sizeWhenDoneIsDirty = 1;
    cp->haveValidIsDirty = 1;
}

tr_completion *
tr_cpConstruct( tr_completion * cp, tr_torrent * tor )
{
    cp->tor = tor;
    cp->completeBlocks  = tr_new( uint16_t, tor->info.pieceCount );
    tr_bitfieldConstruct( &cp->blockBitfield, tor->block_count );
    tr_bitfieldConstruct( &cp->pieceBitfield, tor->info.pieceCount );
    tr_cpReset( cp );
    return cp;
}

tr_completion*
tr_cpDestruct( tr_completion * cp )
{
    tr_free( cp->completeBlocks );
    tr_bitfieldDestruct( &cp->pieceBitfield );
    tr_bitfieldDestruct( &cp->blockBitfield );
    return cp;
}

void
tr_cpInvalidateDND( tr_completion * cp )
{
    cp->sizeWhenDoneIsDirty = 1;
}

uint64_t
tr_cpSizeWhenDone( const tr_completion * ccp )
{
    if( ccp->sizeWhenDoneIsDirty )
    {
        tr_completion *    cp = (tr_completion *) ccp; /* mutable */
        const tr_torrent * tor = cp->tor;
        const tr_info *    info = &tor->info;
        tr_piece_index_t   i;
        uint64_t           size = 0;

        for( i = 0; i < info->pieceCount; ++i )
        {
            if( !info->pieces[i].dnd )
            {
                /* we want the piece... */
                size += tr_torPieceCountBytes( tor, i );
            }
            else if( tr_cpPieceIsComplete( cp, i ) )
            {
                /* we have the piece... */
                size += tr_torPieceCountBytes( tor, i );
            }
            else if( cp->completeBlocks[i] )
            {
                /* we have part of the piece... */
                const tr_block_index_t b = tr_torPieceFirstBlock( tor, i );
                const tr_block_index_t e = b + tr_torPieceCountBlocks( tor, i );
                tr_block_index_t j;
                for( j = b; j < e; ++j )
                    if( tr_cpBlockIsCompleteFast( cp, j ) )
                        size += tr_torBlockCountBytes( tor, j );
            }
        }

        cp->sizeWhenDoneLazy = size;
        cp->sizeWhenDoneIsDirty = 0;
    }

    assert( ccp->sizeWhenDoneLazy <= ccp->tor->info.totalSize );
    assert( ccp->sizeWhenDoneLazy >= ccp->sizeNow );
    return ccp->sizeWhenDoneLazy;
}

void
tr_cpPieceAdd( tr_completion *  cp,
               tr_piece_index_t piece )
{
    const tr_torrent *     tor = cp->tor;
    const tr_block_index_t start = tr_torPieceFirstBlock( tor, piece );
    const tr_block_index_t end = start + tr_torPieceCountBlocks( tor, piece );
    tr_block_index_t       i;

    for( i = start; i < end; ++i )
        tr_cpBlockAdd( cp, i );
}

void
tr_cpPieceRem( tr_completion *  cp,
               tr_piece_index_t piece )
{
    const tr_torrent *     tor = cp->tor;
    const tr_block_index_t start = tr_torPieceFirstBlock( tor, piece );
    const tr_block_index_t end = start + tr_torPieceCountBlocks( tor, piece );
    tr_block_index_t       block;

    assert( cp );
    assert( piece < tor->info.pieceCount );
    assert( start < tor->block_count );
    assert( start <= end );
    assert( end <= tor->block_count );

    for( block = start; block < end; ++block )
        if( tr_cpBlockIsCompleteFast( cp, block ) )
            cp->sizeNow -= tr_torBlockCountBytes( tor, block );

    cp->sizeWhenDoneIsDirty = 1;
    cp->haveValidIsDirty = 1;
    cp->completeBlocks[piece] = 0;
    tr_bitfieldRemRange ( &cp->blockBitfield, start, end );
    tr_bitfieldRem( &cp->pieceBitfield, piece );
}

void
tr_cpBlockAdd( tr_completion * cp, tr_block_index_t block )
{
    const tr_torrent * tor = cp->tor;

    if( !tr_cpBlockIsComplete( cp, block ) )
    {
        const tr_piece_index_t piece = tr_torBlockPiece( tor, block );
        const int              blockSize = tr_torBlockCountBytes( tor,
                                                                  block );

        ++cp->completeBlocks[piece];

        if( tr_cpPieceIsComplete( cp, piece ) )
            tr_bitfieldAdd( &cp->pieceBitfield, piece );

        tr_bitfieldAdd( &cp->blockBitfield, block );

        cp->sizeNow += blockSize;

        cp->haveValidIsDirty = 1;
        cp->sizeWhenDoneIsDirty = 1;
    }
}


void
tr_cpSetHaveAll( tr_completion * cp )
{
    tr_piece_index_t i;
    tr_torrent * tor = cp->tor;

    tr_cpReset( cp );

    cp->sizeNow = tor->info.totalSize;
    tr_bitfieldAddRange( &cp->blockBitfield, 0, tor->block_count );
    tr_bitfieldAddRange( &cp->pieceBitfield, 0, tor->info.pieceCount );
    for( i=0; i<tor->info.pieceCount; ++i )
        cp->completeBlocks[i] = tr_torPieceCountBlocks( tor, i );
    cp->sizeWhenDoneIsDirty = 1;
    cp->haveValidIsDirty = 1;
}

/* Initialize a completion object from a bitfield indicating which
 * blocks we have. */
tr_bool
tr_cpBlockBitfieldSet( tr_completion * cp, tr_bitfield * blockBitfield )
{
    tr_torrent * tor = cp->tor;
    tr_block_index_t b = 0;
    tr_piece_index_t p = 0;
    int pieceBlock = 0, blocksInCurrentPiece, completeBlocksInPiece = 0;

    assert( cp != NULL );
    assert( blockBitfield != NULL );

    /* The bitfield of block flags is typically loaded from a resume
     * file. Test the bitfield's length in case the resume file somehow
     * got corrupted. */
    if( blockBitfield->byteCount != cp->blockBitfield.byteCount )
        return FALSE;

    /* Start cp with a state where it thinks we have nothing. */
    tr_cpReset( cp );

    /* Initialize our block bitfield from the one passed in. */
    memcpy( cp->blockBitfield.bits, blockBitfield->bits,
            blockBitfield->byteCount );

    /* To set the remaining fields, we walk through every block. */
    blocksInCurrentPiece = tr_torPieceCountBlocks( tor, p );
    while( b < cp->tor->block_count )
    {
        if( tr_bitfieldHasFast( blockBitfield, b ) )
        {
            ++completeBlocksInPiece;
            cp->sizeNow += tr_torBlockCountBytes( tor, b );
        }

        ++b;
        ++pieceBlock;

        /* By the time we reach the end of a piece, we have enough
         * info to update that piece's slot in cp.completeBlocks
         * and cp.pieceBitfield. */
        if( pieceBlock == blocksInCurrentPiece )
        {
            cp->completeBlocks[p] = completeBlocksInPiece;
            if( completeBlocksInPiece == blocksInCurrentPiece )
                tr_bitfieldAdd( &cp->pieceBitfield, p );

            /* Reset the per-piece counters because we're starting on
             * a new piece now. */
            ++p;
            completeBlocksInPiece = 0;
            pieceBlock = 0;
            blocksInCurrentPiece = tr_torPieceCountBlocks( tor, p );
        }
    }

    return TRUE;
}

/***
****
***/

tr_completeness
tr_cpGetStatus( const tr_completion * cp )
{
    if( !tr_torrentHasMetadata( cp->tor ) ) return TR_LEECH;
    if( cp->sizeNow == cp->tor->info.totalSize ) return TR_SEED;
    if( cp->sizeNow == tr_cpSizeWhenDone( cp ) ) return TR_PARTIAL_SEED;
    return TR_LEECH;
}

static uint64_t
calculateHaveValid( const tr_completion * ccp )
{
    uint64_t b = 0;
    tr_piece_index_t i;
    const tr_torrent * tor = ccp->tor;
    const uint64_t pieceSize = tor->info.pieceSize;
    const tr_piece_index_t lastPiece = tor->info.pieceCount - 1;
    const uint64_t lastPieceSize = tr_torPieceCountBytes( tor, lastPiece );

    if( !tr_torrentHasMetadata( tor ) )
        return 0;

    for( i=0; i!=lastPiece; ++i )
        if( tr_cpPieceIsComplete( ccp, i ) )
            b += pieceSize;

    if( tr_cpPieceIsComplete( ccp, lastPiece ) )
        b += lastPieceSize;

    return b;
}

uint64_t
tr_cpHaveValid( const tr_completion * ccp )
{
    if( ccp->haveValidIsDirty )
    {
        tr_completion * cp = (tr_completion *) ccp; /* mutable */
        cp->haveValidLazy = calculateHaveValid( ccp );
        cp->haveValidIsDirty = 0;
    }

    return ccp->haveValidLazy;
}

void
tr_cpGetAmountDone( const tr_completion * cp,
                    float *               tab,
                    int                   tabCount )
{
    int                i;
    const tr_torrent * tor = cp->tor;
    const float        interval = tor->info.pieceCount / (float)tabCount;
    const int          isSeed = tr_cpGetStatus( cp ) == TR_SEED;

    for( i = 0; i < tabCount; ++i )
    {
        const tr_piece_index_t piece = i * interval;

        if( tor == NULL )
            tab[i] = 0.0f;
        else if( isSeed || tr_cpPieceIsComplete( cp, piece ) )
            tab[i] = 1.0f;
        else
            tab[i] = (float)cp->completeBlocks[piece] /
                     tr_torPieceCountBlocks( tor, piece );
    }
}

int
tr_cpMissingBlocksInPiece( const tr_completion * cp, tr_piece_index_t piece )
{
    return tr_torPieceCountBlocks( cp->tor, piece ) - cp->completeBlocks[piece];
}

uint64_t
tr_cpMissingBytesInPiece( const tr_completion * cp, tr_piece_index_t pi )
{
    const tr_torrent * tor = cp->tor;
    tr_block_index_t bs, be, bi;
    uint64_t s = 0;

    if( tr_cpMissingBlocksInPiece( cp, pi ) == 0 )
        return 0;

    bs = tr_torPieceFirstBlock( tor, pi );
    be = bs + tr_torPieceCountBlocks( tor, pi );
    for( bi = bs; bi < be; ++bi )
        if( tr_cpBlockIsCompleteFast( cp, bi ) )
            s += tr_torBlockCountBytes( tor, bi );
    return s;
}

tr_bool
tr_cpPieceIsComplete( const tr_completion * cp, tr_piece_index_t piece )
{
    return cp->completeBlocks[piece] == tr_torPieceCountBlocks( cp->tor, piece );
}

tr_bool
tr_cpFileIsComplete( const tr_completion * cp, tr_file_index_t fileIndex )
{
    const tr_file * file = &cp->tor->info.files[fileIndex];
    tr_piece_index_t pi;
    for( pi = file->firstPiece; pi <= file->lastPiece; ++pi )
        if( !tr_cpPieceIsComplete( cp, pi ) )
            return FALSE;
    return TRUE;
}
