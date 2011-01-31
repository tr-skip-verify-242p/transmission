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

#include <stdlib.h>

#include "pieces-common.h"
#include "pieces-viewer.h"
#include "tr-prefs.h"
#include "util.h"


#define BAR_HEIGHT 20
#define BORDER_WIDTH 1

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
    guint       timer;
};

#define GTR_PIECES_VIEWER_GET_PRIVATE( obj ) \
    ( G_TYPE_INSTANCE_GET_PRIVATE( ( obj ), GTR_TYPE_PIECES_VIEWER, \
                                   GtrPiecesViewerPrivate ) )

enum gtr_pieces_viewer_signals
{
    SIGNAL_FILE_SELECTED,
    NUM_SIGNALS
};

static guint signals[NUM_SIGNALS];

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
    gtr_draw_pieces( cr, priv->gtor, BORDER_WIDTH, BORDER_WIDTH,
                     a.width - 2 * BORDER_WIDTH,
                     a.height - 2 * BORDER_WIDTH );
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

static gboolean
timer_callback( gpointer user_data )
{
    GtrPiecesViewer * self;
    GtrPiecesViewerPrivate * priv;

    if( !GTR_IS_PIECES_VIEWER( user_data ) )
        return FALSE;

    self = GTR_PIECES_VIEWER( user_data );
    priv = GTR_PIECES_VIEWER_GET_PRIVATE( self );

    if( priv->gtor )
        gtk_widget_queue_draw( GTK_WIDGET( user_data ) );
    return TRUE;
}

static void
gtr_pieces_viewer_size_request( GtkWidget * w, GtkRequisition * req )
{
    g_return_if_fail( w != NULL );
    g_return_if_fail( GTR_IS_PIECES_VIEWER( w ) );

    req->height = BAR_HEIGHT;
}

static int
compare_piece_index_to_file( const void * a, const void * b )
{
    const tr_piece_index_t pi = *(const tr_piece_index_t *) a;
    const tr_file * file = b;
    if( pi < file->firstPiece )
        return -1;
    if( pi > file->lastPiece )
        return 1;
    return 0;
}

static void
emit_file_select_signal( GtrPiecesViewer * self, int x, int y )
{
    GtrPiecesViewerPrivate * priv;
    const tr_info * info;
    const tr_file * file;
    tr_piece_index_t pi;
    tr_file_index_t fi;
    GtkAllocation a;
    float interval;
    int w, h;

    priv = GTR_PIECES_VIEWER_GET_PRIVATE( self );
    if( !( info = tr_torrent_info( priv->gtor ) ) )
        return;

    if( !info->fileCount )
        return;

    gtr_widget_get_allocation( GTK_WIDGET( self ), &a );
    x -= BORDER_WIDTH;
    y -= BORDER_WIDTH;
    w = a.width - 2 * BORDER_WIDTH;
    h = a.height - 2 * BORDER_WIDTH;

    if( w < 1 || h < 1 || y > h )
        return;

    /* FIXME: Same problem as with tr_peerMgrTorrentAvailability(). */
    interval = (float) info->pieceCount / (float) w;
    pi = interval * x;
    if( pi >= info->pieceCount )
        return;

    file = bsearch( &pi, info->files, info->fileCount, sizeof( tr_file ),
                    compare_piece_index_to_file );
    if( !file )
        return;
    fi = file - info->files;

    g_signal_emit( self, signals[SIGNAL_FILE_SELECTED], 0, fi );
}

static gboolean
gtr_pieces_viewer_button_press( GtkWidget * w, GdkEventButton * ev )
{
    emit_file_select_signal( GTR_PIECES_VIEWER( w ), ev->x, ev->y );
    return TRUE;
}

static void
gtr_pieces_viewer_dispose( GObject * object )
{
    GtrPiecesViewer * self = GTR_PIECES_VIEWER( object );
    GtrPiecesViewerPrivate * priv = GTR_PIECES_VIEWER_GET_PRIVATE( self );

    if( priv->gtor )
        priv->gtor = NULL;
    g_source_remove( priv->timer );

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
    widget_class->button_press_event = gtr_pieces_viewer_button_press;
    gobject_class->dispose = gtr_pieces_viewer_dispose;

    signals[SIGNAL_FILE_SELECTED] = g_signal_new(
        "file-selected",
        G_OBJECT_CLASS_TYPE( gobject_class ),
        G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET( GtrPiecesViewerClass, file_selected ),
        NULL, NULL,
        g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1, G_TYPE_UINT );
}

static void
gtr_pieces_viewer_init( GtrPiecesViewer * self )
{
    GtrPiecesViewerPrivate * priv = GTR_PIECES_VIEWER_GET_PRIVATE( self );

    gtk_widget_add_events( GTK_WIDGET( self ),
                           GDK_BUTTON_PRESS_MASK );

    priv->gtor = NULL;
    priv->core = NULL;
}

GtkWidget *
gtr_pieces_viewer_new( TrCore * core )
{
    GtrPiecesViewer * self;
    GtrPiecesViewerPrivate * priv;

    self = g_object_new( GTR_TYPE_PIECES_VIEWER, NULL );
    priv = GTR_PIECES_VIEWER_GET_PRIVATE( self );
    priv->core = core;
    priv->timer = gtr_timeout_add_seconds( SECONDARY_WINDOW_REFRESH_INTERVAL_SECONDS,
                                           timer_callback, self );

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
