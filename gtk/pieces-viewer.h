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

#ifndef _GTR_PIECES_VIEWER_H_
#define _GTR_PIECES_VIEWER_H_

#include <gtk/gtkdrawingarea.h>
#include "tr-torrent.h"

#define GTR_TYPE_PIECES_VIEWER ( gtr_pieces_viewer_get_type( ) )
#define GTR_PIECES_VIEWER( obj ) \
    ( G_TYPE_CHECK_INSTANCE_CAST( ( obj ), GTR_TYPE_PIECES_VIEWER, GtrPiecesViewer ) )
#define GTR_PIECES_VIEWER_CLASS( obj ) \
    ( G_TYPE_CHECK_CLASS_CAST( ( obj ), GTR_PIECES_VIEWER,  GtrPiecesViewerClass ) )
#define GTR_IS_PIECES_VIEWER( obj ) \
    ( G_TYPE_CHECK_INSTANCE_TYPE( ( obj ), GTR_TYPE_PIECES_VIEWER ) )
#define GTR_IS_PIECES_VIEWER_CLASS( obj ) \
    ( G_TYPE_CHECK_CLASS_TYPE( ( obj ), GTR_TYPE_PIECES_VIEWER ) )
#define GTR_PIECES_VIEWER_GET_CLASS \
    ( G_TYPE_INSTANCE_GET_CLASS( ( obj ), GTR_TYPE_PIECES_VIEWER, GtrPiecesViewerClass ) )

typedef struct _GtrPiecesViewer      GtrPiecesViewer;
typedef struct _GtrPiecesViewerClass GtrPiecesViewerClass;

struct _GtrPiecesViewer
{
    GtkDrawingArea parent;
};

struct _GtrPiecesViewerClass
{
    GtkDrawingAreaClass parent_class;
};

GType gtr_pieces_viewer_get_type( void );
GtkWidget * gtr_pieces_viewer_new( void );

/** @note Increases the reference count of @a gtor. */
void gtr_pieces_viewer_set_gtorrent( GtrPiecesViewer * pv, TrTorrent * gtor );

#endif /* _GTR_PIECES_VIEWER_H_ */
