/******************************************************************************
 * $Id$
 *
 * Copyright (c) 2005-2008 Transmission authors and contributors
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
#include "util.h"

enum
{
    PROP_0,
    PROP_TORRENT
};

#define MIN_BAR_WIDTH 100
#define MIN_BAR_HEIGHT 20

struct _PiecesCellRendererPrivate
{
    tr_torrent      * tor;
    int8_t          * tab;
    int               tabsize;
    cairo_surface_t * offscreen;
    int               offscreen_w;
    int               offscreen_h;
    GdkColor          have_color;
    GdkColor          missing_color;
    GdkColor          bg_color;
    GdkColor          seed_color;
};

static void
pieces_cell_renderer_get_size( GtkCellRenderer  * cell,
                               GtkWidget        * widget UNUSED,
                               GdkRectangle     * cell_area,
                               gint             * x_offset,
                               gint             * y_offset,
                               gint             * width,
                               gint             * height )
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
get_offscreen_context( PiecesCellRendererPrivate * priv, cairo_t * cr, int w, int h )
{
    if( !priv->offscreen || priv->offscreen_w != w || priv->offscreen_h != h )
    {
        cairo_surface_t * target = cairo_get_target( cr );
        if( priv->offscreen )
            cairo_surface_destroy( priv->offscreen );
        priv->offscreen = cairo_surface_create_similar( target, CAIRO_CONTENT_COLOR_ALPHA, w, h );
        priv->offscreen_w = w;
        priv->offscreen_h = h;
    }
    return cairo_create( priv->offscreen );
}

static int8_t *
get_pieces_tab( PiecesCellRendererPrivate * priv, int * setmePieceCount )
{
    tr_torrent * tor = priv->tor;
    int count;

    if( !tor )
    {
        *setmePieceCount = 0;
        return NULL;
    }

    count = tr_torrentInfo( tor )->pieceCount;
    if( count < 1 )
    {
        *setmePieceCount = 0;
        return NULL;
    }

    if( !priv->tab || priv->tabsize < count )
    {
        tr_free( priv->tab );
        priv->tab = tr_malloc( count );
        priv->tabsize = count;
    }
    *setmePieceCount = count;
    return priv->tab;
}

static void
pieces_cell_renderer_render( GtkCellRenderer       * cell,
                             GdkDrawable           * window,
                             GtkWidget             * widget,
                             GdkRectangle          * background_area UNUSED,
                             GdkRectangle          * cell_area,
                             GdkRectangle          * expose_area,
                             GtkCellRendererState    flags UNUSED )
{
    PiecesCellRenderer        * self = PIECES_CELL_RENDERER( cell );
    PiecesCellRendererPrivate * priv = self->priv;
    tr_torrent * tor = priv->tor;
    gint x, y, w, h, xt, yt;
    cairo_t * cr, * wincr;
    int8_t * pieces = NULL;
    int pieceCount = 0;

    x = cell_area->x + cell->xpad;
    y = cell_area->y + cell->ypad;
    w = cell_area->width - cell->xpad * 2;
    h = cell_area->height - cell->ypad * 2;

    gtk_paint_box (widget->style, window,
                   GTK_STATE_NORMAL, GTK_SHADOW_IN,
                   NULL, widget, NULL,
                   x, y, w, h);
    xt = widget->style->xthickness;
    yt = widget->style->ythickness;
    x += xt;
    y += yt;
    w -= 2 * xt;
    h -= 2 * yt;

    wincr = gdk_cairo_create( window );
    cr = get_offscreen_context( priv, wincr, w, h );

    gdk_cairo_set_source_color( cr, &priv->bg_color );
    cairo_paint( cr );

    pieces = get_pieces_tab( priv, &pieceCount );

    if( tor && pieces && pieceCount > 0 )
    {
        const tr_stat * st      = tr_torrentStatCached( tor );
        const tr_bool connected = ( st->peersConnected > 0 );
        const tr_bool seeding   = ( st->percentDone >= 1.0 );
        const double pw         = (double) w / (double) pieceCount;
        GdkColor * piece_color  = ( seeding ? &priv->seed_color : &priv->have_color );
        int i, j;
        int8_t avail;

        tr_torrentAvailability( tor, pieces, pieceCount );

        for( i = 0; i < pieceCount; )
        {
            if( pieces[i] > 0 || ( !connected && pieces[i] == 0 ) )
            {
                ++i;
                continue;
            }
            avail = pieces[i];
            for( j = i + 1; j < pieceCount; ++j )
                if( pieces[j] != avail )
                    break;
            if( avail == 0 )
                gdk_cairo_set_source_color( cr, &priv->missing_color );
            else
                gdk_cairo_set_source_color( cr, piece_color );
            cairo_rectangle( cr, pw * i, 0, pw * (j - i), h );
            cairo_fill( cr );
            i = j;
        }
    }
    cairo_destroy( cr );

    gdk_cairo_rectangle( wincr, expose_area );
    cairo_clip( wincr );

    cairo_set_source_surface( wincr, priv->offscreen, x, y );
    cairo_paint( wincr );
    cairo_destroy( wincr );
}

static void
pieces_cell_renderer_set_property( GObject      * object,
                                   guint          property_id,
                                   const GValue * v,
                                   GParamSpec   * pspec )
{
    PiecesCellRenderer        * self = PIECES_CELL_RENDERER( object );
    PiecesCellRendererPrivate * priv = self->priv;

    switch( property_id )
    {
        case PROP_TORRENT: priv->tor = g_value_get_pointer( v ); break;
        default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
    }
}

static void
pieces_cell_renderer_get_property( GObject     * object,
                                   guint         property_id,
                                   GValue      * v,
                                   GParamSpec  * pspec )
{
    const PiecesCellRenderer  * self = PIECES_CELL_RENDERER( object );
    PiecesCellRendererPrivate * priv = self->priv;

    switch( property_id )
    {
        case PROP_TORRENT: g_value_set_pointer( v, priv->tor ); break;
        default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
    }
}

static void
pieces_cell_renderer_finalize( GObject * object )
{
    PiecesCellRenderer        * self = PIECES_CELL_RENDERER( object );
    PiecesCellRendererPrivate * priv = self->priv;
    GObjectClass              * parent;

    tr_free( priv->tab );
    priv->tab = NULL;
    priv->tabsize = 0;
    if( priv->offscreen )
    {
        cairo_surface_destroy( priv->offscreen );
        priv->offscreen = NULL;
    }

    parent = g_type_class_peek( g_type_parent( PIECES_CELL_RENDERER_TYPE ) ) ;
    parent->finalize( object );
}

static void
pieces_cell_renderer_class_init( PiecesCellRendererClass * klass )
{
    GObjectClass         * gobject_class = G_OBJECT_CLASS( klass );
    GtkCellRendererClass * cell_class = GTK_CELL_RENDERER_CLASS( klass );

    g_type_class_add_private( klass, sizeof( PiecesCellRendererPrivate ) );

    cell_class->render          = pieces_cell_renderer_render;
    cell_class->get_size        = pieces_cell_renderer_get_size;
    gobject_class->set_property = pieces_cell_renderer_set_property;
    gobject_class->get_property = pieces_cell_renderer_get_property;
    gobject_class->finalize     = pieces_cell_renderer_finalize;

    g_object_class_install_property( gobject_class, PROP_TORRENT,
                                     g_param_spec_pointer( "torrent", NULL,
                                                           "tr_torrent*",
                                                           G_PARAM_READWRITE ) );
}

static void
pieces_cell_renderer_init( GTypeInstance * instance,
                           gpointer g_class UNUSED )
{
    PiecesCellRenderer        * self = PIECES_CELL_RENDERER( instance );
    PiecesCellRendererPrivate * priv;

    priv = G_TYPE_INSTANCE_GET_PRIVATE(self, PIECES_CELL_RENDERER_TYPE,
                                       PiecesCellRendererPrivate );
    priv->tor = NULL;
    priv->offscreen = NULL;
    priv->tab = NULL;
    priv->tabsize = 0;

    gdk_color_parse( "#efefef", &priv->bg_color );
    gdk_color_parse( "#2975d6", &priv->have_color );
    gdk_color_parse( "#6b0000", &priv->missing_color );
    gdk_color_parse( "#30b027", &priv->seed_color );

    self->priv = priv;
}

GType
pieces_cell_renderer_get_type( void )
{
    static GType type = 0;

    if( !type )
    {
        static const GTypeInfo info =
        {
            sizeof( PiecesCellRendererClass ),
            NULL,                                       /* base_init */
            NULL,                                       /* base_finalize */
            (GClassInitFunc) pieces_cell_renderer_class_init,
            NULL,                                       /* class_finalize */
            NULL,                                       /* class_data */
            sizeof( PiecesCellRenderer ),
            0,                                          /* n_preallocs */
            (GInstanceInitFunc) pieces_cell_renderer_init,
            NULL
        };

        type = g_type_register_static( GTK_TYPE_CELL_RENDERER,
                                       "PiecesCellRenderer",
                                       &info, (GTypeFlags) 0 );
    }

    return type;
}

GtkCellRenderer *
pieces_cell_renderer_new( void )
{
    return (GtkCellRenderer *) g_object_new( PIECES_CELL_RENDERER_TYPE,
                                             NULL );
}
