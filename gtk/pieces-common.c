/******************************************************************************
 * $Id$
 *
 * Copyright (c) 2011 Transmission authors and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#include "pieces-common.h"


static void
style_init( GtrPieceStyle * pstyle )
{
    gdk_color_parse( "#efefff", &pstyle->piece_bg_color );
    gdk_color_parse( "#2975d6", &pstyle->piece_have_color );
    gdk_color_parse( "#d90000", &pstyle->piece_missing_color );
    gdk_color_parse( "#30b027", &pstyle->piece_seeding_color );
    gdk_color_parse( "#aaaaaa", &pstyle->piece_stopped_color );
    gdk_color_parse( "#dadada", &pstyle->progress_bg_color );
    gdk_color_parse( "#314e6c", &pstyle->progress_bar_color );
    gdk_color_parse( "#777777", &pstyle->progress_stopped_color );
    gdk_color_parse( "#a6e3b4", &pstyle->ratio_bg_color );
    gdk_color_parse( "#448632", &pstyle->ratio_bar_color );
    gdk_color_parse( "#888888", &pstyle->border_color );
    gdk_color_parse( "#a33dac", &pstyle->magnet_color );
}

const GtrPieceStyle *
gtr_get_piece_style( void )
{
    static GtrPieceStyle style;
    static gboolean initialized = FALSE;

    if( !initialized )
    {
        style_init( &style );
        initialized = TRUE;
    }
    return &style;
}

void
gtr_draw_pieces( cairo_t * cr, TrTorrent * gtor, int x, int y, int w, int h )
{
    const GtrPieceStyle * pstyle = gtr_get_piece_style( );
    const tr_stat * st;
    const int8_t * avtab;

    if (w < 1 || h < 1)
        return;

    gdk_cairo_set_source_color( cr, &pstyle->piece_bg_color );
    cairo_rectangle( cr, x, y, w, h );
    cairo_fill( cr );

    st = tr_torrent_stat( gtor );
    avtab = tr_torrent_availability( gtor, w );
    if( st && avtab )
    {
        const tr_torrent * tor = tr_torrent_handle( gtor );
        const tr_bool magnet = !tr_torrentHasMetadata( tor );
        const tr_bool stopped = ( st->activity == TR_STATUS_STOPPED );
        const tr_bool connected = ( st->peersConnected > 0 );
        const tr_bool seeding = ( st->percentDone >= 1.0 );
        const GdkColor * piece_have_color, * piece_missing_color;
        int i, j;
        int8_t avail;

        if( stopped )
            piece_have_color = &pstyle->piece_stopped_color;
        else if( seeding )
            piece_have_color = &pstyle->piece_seeding_color;
        else
            piece_have_color = &pstyle->piece_have_color;

        if( connected )
        {
            if( magnet )
                piece_missing_color = &pstyle->magnet_color;
            else
                piece_missing_color = &pstyle->piece_missing_color;
        }
        else
        {
            piece_missing_color = &pstyle->piece_bg_color;
        }

        for( i = 0; i < w; )
        {
            if( avtab[i] > 0 )
            {
                ++i;
                continue;
            }
            avail = avtab[i];
            for( j = i + 1; j < w; ++j )
                if( avtab[j] != avail )
                    break;
            if( avail == 0 )
                gdk_cairo_set_source_color( cr, piece_missing_color );
            else
                gdk_cairo_set_source_color( cr, piece_have_color );
            cairo_rectangle( cr, x + i, y, j - i, h );
            cairo_fill( cr );
            i = j;
        }
    }
}
