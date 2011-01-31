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

#ifndef GTR_PIECES_COMMON_H
#define GTR_PIECES_COMMON_H

#include <gtk/gtk.h>
#include "tr-torrent.h"

typedef struct _GtrPieceStyle
{
    GdkColor piece_bg_color;
    GdkColor piece_have_color;
    GdkColor piece_missing_color;
    GdkColor piece_seeding_color;
    GdkColor piece_stopped_color;
    GdkColor progress_bg_color;
    GdkColor progress_bar_color;
    GdkColor ratio_bg_color;
    GdkColor ratio_bar_color;
    GdkColor border_color;
    GdkColor progress_stopped_color;
    GdkColor magnet_color;
}
GtrPieceStyle;

const GtrPieceStyle * gtr_get_piece_style( void );

void gtr_draw_pieces( cairo_t * cr, TrTorrent * gtor, int x, int y, int w, int h );

#endif /* GTR_PIECES_COMMON_H */

