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
#include <stdlib.h>
#include <string.h> /* memmove */

#include "ptrarray.h"
#include "utils.h"

#define FLOOR 32
#define PTR_ARRAY_MAGIC 0xff1a2b3c

const tr_ptrArray TR_PTR_ARRAY_INIT = { PTR_ARRAY_MAGIC, NULL, 0, 0 };

tr_bool
tr_isPtrArray( const tr_ptrArray * a )
{
    return ( a != NULL && a->magic == PTR_ARRAY_MAGIC
             && ( a->items != NULL || ( a->n_items == 0 && a->n_alloc == 0 ) ) );
}

void
tr_ptrArrayDestruct( tr_ptrArray * p, PtrArrayForeachFunc func )
{
    assert( tr_isPtrArray( p ) );

    if( func )
        tr_ptrArrayForeach( p, func );

    tr_free( p->items );

    memset( p, 0, sizeof( tr_ptrArray ) );
}

void
tr_ptrArrayForeach( tr_ptrArray *       t,
                    PtrArrayForeachFunc func )
{
    int i;

    assert( tr_isPtrArray( t ) );
    assert( func );

    for( i = 0; i < t->n_items; ++i )
        func( t->items[i] );
}

void**
tr_ptrArrayPeek( tr_ptrArray * t,
                 int *         size )
{
    assert( tr_isPtrArray( t ) );
    *size = t->n_items;
    return t->items;
}

void*
tr_ptrArrayNth( tr_ptrArray* t,
                int          i )
{
    assert( tr_isPtrArray( t ) );
    assert( i >= 0 );
    assert( i < t->n_items );

    return t->items[i];
}

int
tr_ptrArrayInsert( tr_ptrArray * t,
                   void        * ptr,
                   int           pos )
{
    assert( tr_isPtrArray( t ) );
    if( t->n_items >= t->n_alloc )
    {
        t->n_alloc = MAX( FLOOR, t->n_alloc * 2 );
        t->items = tr_renew( void*, t->items, t->n_alloc );
    }

    if( pos < 0 || pos > t->n_items )
        pos = t->n_items;
    else
        memmove( t->items + pos + 1,
                 t->items + pos,
                 sizeof( void* ) * ( t->n_items - pos ) );

    t->items[pos] = ptr;
    t->n_items++;
    return pos;
}

void*
tr_ptrArrayPop( tr_ptrArray* t )
{
    void * ret = NULL;

    assert( tr_isPtrArray( t ) );

    if( t->n_items )
        ret = t->items[--t->n_items];

    return ret;
}

void
tr_ptrArrayErase( tr_ptrArray * t,
                  int           begin,
                  int           end )
{
    assert( tr_isPtrArray( t ) );
    assert( begin >= 0 );
    if( end < 0 ) end = t->n_items;
    assert( begin < end );
    assert( end <= t->n_items );

    memmove( t->items + begin,
            t->items + end,
            sizeof( void* ) * ( t->n_items - end ) );

    t->n_items -= ( end - begin );
}

/**
***
**/

int
tr_ptrArrayLowerBound( const tr_ptrArray *                t,
                       const void *                       ptr,
                       int                 compare( const void *,
                                                    const void * ),
                       tr_bool *                    exact_match )
{
    int len, first = 0;

    assert( tr_isPtrArray( t ) );
    len = t->n_items;

    while( len > 0 )
    {
        int       half = len / 2;
        int       middle = first + half;
        const int c = compare( t->items[middle], ptr );
        if( c < 0 )
        {
            first = middle + 1;
            len = len - half - 1;
        }
        else if( !c )
        {
            if( exact_match )
                *exact_match = TRUE;
            return middle;
            break;
        }
        else
        {
            len = half;
        }
    }

    if( exact_match )
        *exact_match = FALSE;

    return first;
}

#ifdef NDEBUG
#define assertSortedAndUnique(a,b)
#else
static void
assertSortedAndUnique( const tr_ptrArray * t,
                    int compare(const void*, const void*) )
{
    int i;
    assert( tr_isPtrArray( t ) );

    for( i = 0; i < t->n_items - 2; ++i )
        assert( compare( t->items[i], t->items[i + 1] ) <= 0 );
}
#endif

int
tr_ptrArrayInsertSorted( tr_ptrArray * t,
                         void *        ptr,
                         int           compare(const void*, const void*) )
{
    int pos, ret;
    assert( tr_isPtrArray( t ) );
    pos = tr_ptrArrayLowerBound( t, ptr, compare, NULL );
    ret = tr_ptrArrayInsert( t, ptr, pos );

    //assertSortedAndUnique( t, compare );
    return ret;
}

void*
tr_ptrArrayFindSorted( tr_ptrArray * t,
                       const void *  ptr,
                       int           compare(const void*, const void*) )
{
    tr_bool   match;
    int pos;

    assert( tr_isPtrArray( t ) );
    pos = tr_ptrArrayLowerBound( t, ptr, compare, &match );

    return match ? t->items[pos] : NULL;
}

void*
tr_ptrArrayRemoveSorted( tr_ptrArray * t,
                         const void  * ptr,
                         int           compare(const void*, const void*) )
{
    void *    ret = NULL;
    tr_bool   match;
    int pos;

    assert( tr_isPtrArray( t ) );
    pos = tr_ptrArrayLowerBound( t, ptr, compare, &match );

    if( match )
    {
        ret = t->items[pos];
        tr_ptrArrayErase( t, pos, pos + 1 );
    }
    assertSortedAndUnique( t, compare );
    return ret;
}
