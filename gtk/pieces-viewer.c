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
#include "pieces-viewer.h"
#include "util.h"


#define BAR_HEIGHT 16

/*
 * Defines:
 *   static gpointer gtr_pieces_viewer_parent_class;
 *   GType gtr_pieces_viewer_get_type( void );
 */
G_DEFINE_TYPE( GtrPiecesViewer, gtr_pieces_viewer, GTK_TYPE_DRAWING_AREA );

typedef struct _GtrPiecesViewerPrivate GtrPiecesViewerPrivate;
struct _GtrPiecesViewerPrivate
{
    TrTorrent * gtor; /* not owned */
    TrCore    * core; /* not owned */
};

#define GTR_PIECES_VIEWER_GET_PRIVATE( obj ) \
    ( G_TYPE_INSTANCE_GET_PRIVATE( ( obj ), GTR_TYPE_PIECES_VIEWER, \
                                   GtrPiecesViewerPrivate ) )

static void
paint( GtrPiecesViewer * self, cairo_t * cr )
{
    GtrPiecesViewerPrivate * priv = GTR_PIECES_VIEWER_GET_PRIVATE( self );
    const GtrPieceStyle * pstyle = gtr_get_piece_style( );
    GtkAllocation a;

    if( !priv->gtor )
        return;

    gdk_cairo_set_source_color( cr, &pstyle->border_color );
    cairo_paint( cr );

    gtr_widget_get_allocation( GTK_WIDGET( self ), &a );
    gtr_draw_pieces( cr, priv->gtor, 1, 1, a.width - 2, a.height - 2 );
}

static gboolean
gtr_pieces_viewer_expose( GtkWidget * widget, GdkEventExpose * event)
{
    GtrPiecesViewer * self = GTR_PIECES_VIEWER( widget );
    cairo_t * cr;

    g_return_val_if_fail( self != NULL, FALSE );

    cr = gdk_cairo_create( widget->window );
    gdk_cairo_rectangle( cr, &event->area );
    cairo_clip( cr );
    paint( self, cr );
    cairo_destroy( cr );

    return FALSE;
}

static void
gtr_pieces_viewer_size_request( GtkWidget * w, GtkRequisition * req )
{
    g_return_if_fail( w != NULL );
    g_return_if_fail( GTR_IS_PIECES_VIEWER( w ) );

    req->height = BAR_HEIGHT;
}

static void
gtr_pieces_viewer_dispose( GObject * object )
{
    GtrPiecesViewer * self = GTR_PIECES_VIEWER( object );
    GtrPiecesViewerPrivate * priv = GTR_PIECES_VIEWER_GET_PRIVATE( self );

    if( priv->gtor )
        priv->gtor = NULL;
    G_OBJECT_CLASS( gtr_pieces_viewer_parent_class )->dispose( object );
}

static void
gtr_pieces_viewer_class_init( GtrPiecesViewerClass * klass )
{
    GtkWidgetClass * widget_class = GTK_WIDGET_CLASS( klass );
    GObjectClass * gobject_class = G_OBJECT_CLASS( klass );

    g_type_class_add_private( klass, sizeof( GtrPiecesViewerPrivate ) );

    widget_class->expose_event = gtr_pieces_viewer_expose;
    widget_class->size_request = gtr_pieces_viewer_size_request;
    gobject_class->dispose = gtr_pieces_viewer_dispose;
}

static void
gtr_pieces_viewer_init( GtrPiecesViewer * self )
{
    GtrPiecesViewerPrivate * priv = GTR_PIECES_VIEWER_GET_PRIVATE( self );

    priv->gtor = NULL;
}

GtkWidget *
gtr_pieces_viewer_new( TrCore * core )
{
    GtrPiecesViewer * self;
    GtrPiecesViewerPrivate * priv;

    self = g_object_new( GTR_TYPE_PIECES_VIEWER, NULL );
    priv = GTR_PIECES_VIEWER_GET_PRIVATE( self );
    priv->core = core;

    return GTK_WIDGET( self );
}

void
gtr_pieces_viewer_set_gtorrent( GtrPiecesViewer * self, TrTorrent * gtor )
{
    GtrPiecesViewerPrivate * priv = GTR_PIECES_VIEWER_GET_PRIVATE( self );
    priv->gtor = gtor;
}

void
gtr_pieces_viewer_set_torrent_by_id( GtrPiecesViewer * self, int id )
{
    GtrPiecesViewerPrivate * priv = GTR_PIECES_VIEWER_GET_PRIVATE( self );
    priv->gtor = tr_core_get_handle_by_id( priv->core, id );
}
