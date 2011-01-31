/******************************************************************************
 * $Id$
 *
 * Copyright (c) 2005-2011 Transmission authors and contributors
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

#include <libtransmission/transmission.h>
#include <libtransmission/utils.h>
#include "pieces-cell-renderer.h"
#include "pieces-common.h"
#include "tr-torrent.h"
#include "util.h"


static const gint MIN_BAR_WIDTH   = 100;
static const gint MIN_BAR_HEIGHT  = 20;
static const gint PROGRESS_HEIGHT = 4;
static const gint BORDER_WIDTH    = 1;

/*
 * Defines:
 *   static gpointer gtr_pieces_cell_renderer_parent_class;
 *   GType gtr_pieces_cell_renderer_get_type( void );
 */
G_DEFINE_TYPE( GtrPiecesCellRenderer, gtr_pieces_cell_renderer,
               GTK_TYPE_CELL_RENDERER );

struct _GtrPiecesCellRendererPrivate
{
    TrTorrent       * gtor;
    cairo_surface_t * offscreen;
    int               offscreen_w;
    int               offscreen_h;
};

typedef struct _GtrPiecesCellRendererPrivate GtrPiecesCellRendererPrivate;

#define GTR_PIECES_CELL_RENDERER_GET_PRIVATE( obj ) \
    ( G_TYPE_INSTANCE_GET_PRIVATE( ( obj ), GTR_TYPE_PIECES_CELL_RENDERER, \
                                   GtrPiecesCellRendererPrivate ) )

enum
{
    PROP_0,
    PROP_TORRENT
};

static void
gtr_pieces_cell_renderer_get_size( GtkCellRenderer * cell,
                                   GtkWidget       * widget UNUSED,
                                   GdkRectangle    * cell_area,
                                   gint            * x_offset,
                                   gint            * y_offset,
                                   gint            * width,
                                   gint            * height )
{
    if( width )
        *width = MIN_BAR_WIDTH + cell->xpad * 2;
    if( height )
        *height = MIN_BAR_HEIGHT + cell->ypad * 2;

    if( cell_area )
    {
        if( width )
            *width = cell_area->width;
        if( height )
            *height = cell_area->height;
    }

    if( x_offset )
        *x_offset = 0;
    if( y_offset )
        *y_offset = 0;
}

static cairo_t *
get_offscreen_context( GtrPiecesCellRendererPrivate * priv,
                       cairo_t * cr, int w, int h )
{
    if( !priv->offscreen || priv->offscreen_w != w
        || priv->offscreen_h != h )
    {
        cairo_surface_t * target = cairo_get_target( cr ), * s;
        if( priv->offscreen )
            cairo_surface_destroy( priv->offscreen );
        s = cairo_surface_create_similar( target,
                                          CAIRO_CONTENT_COLOR_ALPHA,
                                          w, h );
        priv->offscreen = s;
        priv->offscreen_w = w;
        priv->offscreen_h = h;
    }
    return cairo_create( priv->offscreen );
}

static void
render_progress( GtrPiecesCellRendererPrivate * priv,
                 cairo_t * cr, int x, int y, int w, int h )
{
    const GtrPieceStyle * pstyle = gtr_get_piece_style( );
    const tr_stat * st = tr_torrent_stat( priv->gtor );
    const GdkColor * bg_color, * bar_color;
    double progress;

    if( !st )
    {
        gdk_cairo_set_source_color( cr, &pstyle->progress_bg_color );
        cairo_rectangle( cr, x, y, w, h );
        cairo_fill( cr );
        return;
    }

    if( st->percentDone >= 1.0 )
    {
        progress = MIN( 1.0, MAX( 0.0, st->seedRatioPercentDone ) );
        bg_color = &pstyle->ratio_bg_color;
        bar_color = &pstyle->ratio_bar_color;
    }
    else
    {
        progress = MIN( 1.0, MAX( 0.0, st->percentDone ) );
        bg_color = &pstyle->progress_bg_color;
        bar_color = &pstyle->progress_bar_color;
    }

    if( st->activity == TR_STATUS_STOPPED )
    {
        bg_color = &pstyle->progress_bg_color;
        bar_color = &pstyle->progress_stopped_color;
    }

    if( progress < 1.0 )
    {
        gdk_cairo_set_source_color( cr, bg_color );
        cairo_rectangle( cr, x, y, w, h );
        cairo_fill( cr );
    }

    if( progress > 0.0 )
    {
        gdk_cairo_set_source_color( cr, bar_color );
        cairo_rectangle( cr, x, y, progress * w, h );
        cairo_fill( cr );
    }
}

static void
gtr_pieces_cell_renderer_render( GtkCellRenderer      * cell,
                                 GdkDrawable          * window,
                                 GtkWidget            * widget UNUSED,
                                 GdkRectangle         * background_area UNUSED,
                                 GdkRectangle         * cell_area,
                                 GdkRectangle         * expose_area,
                                 GtkCellRendererState   flags UNUSED )
{
    GtrPiecesCellRenderer * self;
    GtrPiecesCellRendererPrivate * priv;
    const GtrPieceStyle * pstyle = gtr_get_piece_style( );
    gint x, y, w, h, xo, yo, wo, ho, hp;
    cairo_t * cr, * cro;

    self = GTR_PIECES_CELL_RENDERER( cell );
    priv = GTR_PIECES_CELL_RENDERER_GET_PRIVATE( self );

    x = cell_area->x + cell->xpad;
    y = cell_area->y + cell->ypad;
    w = cell_area->width - cell->xpad * 2;
    h = cell_area->height - cell->ypad * 2;

    cr = gdk_cairo_create( window );

    gdk_cairo_rectangle( cr, expose_area );
    cairo_clip( cr );

    cro = get_offscreen_context( priv, cr, w, h );
    xo = 0;
    yo = 0;
    wo = w;
    ho = h;

    gdk_cairo_set_source_color( cro, &pstyle->border_color );
    cairo_paint( cro );
    xo += BORDER_WIDTH;
    yo += BORDER_WIDTH;
    wo -= 2 * BORDER_WIDTH;
    ho -= 2 * BORDER_WIDTH;
    hp = PROGRESS_HEIGHT;

    render_progress( priv, cro, xo, yo, wo, hp );
    gtr_draw_pieces( cro, priv->gtor, xo, yo + hp, wo, ho - hp );
    cairo_destroy( cro );

    cairo_set_source_surface( cr, priv->offscreen, x, y );
    cairo_paint( cr );

    cairo_destroy( cr );
}

static void
gtr_pieces_cell_renderer_set_property( GObject      * object,
                                       guint          property_id,
                                       const GValue * v,
                                       GParamSpec   * pspec )
{
    GtrPiecesCellRenderer * self;
    GtrPiecesCellRendererPrivate * priv;

    self = GTR_PIECES_CELL_RENDERER( object );
    priv = GTR_PIECES_CELL_RENDERER_GET_PRIVATE( self );

    switch( property_id )
    {
        case PROP_TORRENT:
            priv->gtor = g_value_get_object( v );
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec );
            break;
    }
}

static void
gtr_pieces_cell_renderer_get_property( GObject    * object,
                                       guint        property_id,
                                       GValue     * v,
                                       GParamSpec * pspec )
{
    const GtrPiecesCellRenderer * self;
    GtrPiecesCellRendererPrivate * priv;

    self = GTR_PIECES_CELL_RENDERER( object );
    priv = GTR_PIECES_CELL_RENDERER_GET_PRIVATE( self );

    switch( property_id )
    {
        case PROP_TORRENT:
            g_value_take_object( v, priv->gtor );
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec );
            break;
    }
}

static void
gtr_pieces_cell_renderer_finalize( GObject * object )
{
    GtrPiecesCellRenderer * self = GTR_PIECES_CELL_RENDERER( object );
    GtrPiecesCellRendererPrivate * priv;

    priv =  GTR_PIECES_CELL_RENDERER_GET_PRIVATE( self );

    if( priv->offscreen )
    {
        cairo_surface_destroy( priv->offscreen );
        priv->offscreen = NULL;
    }

    G_OBJECT_CLASS( gtr_pieces_cell_renderer_parent_class )->finalize( object );
}

static void
gtr_pieces_cell_renderer_class_init( GtrPiecesCellRendererClass * klass )
{
    GtkCellRendererClass * cell_class = GTK_CELL_RENDERER_CLASS( klass );
    GObjectClass * gobject_class = G_OBJECT_CLASS( klass );

    g_type_class_add_private( klass, sizeof( GtrPiecesCellRendererPrivate ) );

    cell_class->render          = gtr_pieces_cell_renderer_render;
    cell_class->get_size        = gtr_pieces_cell_renderer_get_size;
    gobject_class->set_property = gtr_pieces_cell_renderer_set_property;
    gobject_class->get_property = gtr_pieces_cell_renderer_get_property;
    gobject_class->finalize     = gtr_pieces_cell_renderer_finalize;

    g_object_class_install_property( gobject_class, PROP_TORRENT,
                                     g_param_spec_object( "torrent", NULL,
                                                          "TrTorrent*",
                                                          TR_TORRENT_TYPE,
                                                          G_PARAM_READWRITE ) );
}

static void
gtr_pieces_cell_renderer_init( GtrPiecesCellRenderer * self )
{
    GtrPiecesCellRendererPrivate * priv;

    priv = GTR_PIECES_CELL_RENDERER_GET_PRIVATE( self );
    priv->gtor = NULL;
    priv->offscreen = NULL;
}

GtkCellRenderer *
gtr_pieces_cell_renderer_new( void )
{
    return g_object_new( GTR_TYPE_PIECES_CELL_RENDERER, NULL );
}
