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
    PROP_TORRENT,
    PROP_SHOW_PIECES
};

#define MIN_BAR_WIDTH 100
#define MIN_BAR_HEIGHT 20

typedef struct _PiecesCellRendererClassPrivate
{
    GdkColor          piece_bg_color;
    GdkColor          piece_have_color;
    GdkColor          piece_missing_color;
    GdkColor          piece_seeding_color;
    GdkColor          progress_bg_color;
    GdkColor          progress_bar_color;
    GdkColor          ratio_bg_color;
    GdkColor          ratio_bar_color;
} PiecesCellRendererClassPrivate;

static PiecesCellRendererClassPrivate cpriv_data;
static PiecesCellRendererClassPrivate * cpriv = &cpriv_data;

struct _PiecesCellRendererPrivate
{
    tr_torrent      * tor;
    gboolean          show_pieces;
    int8_t          * tab;
    int               tabsize;
    cairo_surface_t * offscreen;
    int               offscreen_w;
    int               offscreen_h;
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

static void render_progress( PiecesCellRendererPrivate * priv, cairo_t * cr,
                             int x, int y, int w, int h )
{
    tr_torrent * tor = priv->tor;
    const tr_stat * st;
    GdkColor * bg_color, * bar_color;
    double progress;

    if( !tor )
    {
        gdk_cairo_set_source_color( cr, &cpriv->progress_bg_color );
        cairo_rectangle( cr, x, y, w, h );
        cairo_fill( cr );
        return;
    }

    st = tr_torrentStatCached( tor );
    if( st->percentDone >= 1.0 )
    {
        progress = MIN( 1.0, MAX( 0.0, st->seedRatioPercentDone ) );
        bg_color = &cpriv->ratio_bg_color;
        bar_color = &cpriv->ratio_bar_color;
    }
    else
    {
        progress = MIN( 1.0, MAX( 0.0, st->percentDone ) );
        bg_color = &cpriv->progress_bg_color;
        bar_color = &cpriv->progress_bar_color;
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

static void render_pieces( PiecesCellRendererPrivate * priv, cairo_t * cr,
                           int x, int y, int w, int h )
{
    tr_torrent * tor = priv->tor;
    int8_t * pieces = NULL;
    int pieceCount = 0;

    if (w < 1 || h < 1)
        return;

    gdk_cairo_set_source_color( cr, &cpriv->piece_bg_color );
    cairo_rectangle( cr, x, y, w, h );
    cairo_fill( cr );

    pieces = get_pieces_tab( priv, &pieceCount );
    if( tor && pieces && pieceCount > 0 )
    {
        const tr_stat * st = tr_torrentStatCached( tor );
        const tr_bool connected = ( st->peersConnected > 0 );
        const tr_bool seeding = ( st->percentDone >= 1.0 );
        const double pw = (double) w / (double) pieceCount;
        GdkColor * piece_have_color;
        GdkColor * piece_missing_color;
        int i, j;
        int8_t avail;

        if( seeding )
            piece_have_color = &cpriv->piece_seeding_color;
        else
            piece_have_color = &cpriv->piece_have_color;

        if( connected )
            piece_missing_color = &cpriv->piece_missing_color;
        else
            piece_missing_color = &cpriv->piece_bg_color;

        tr_torrentAvailability( tor, pieces, pieceCount );

        for( i = 0; i < pieceCount; )
        {
            if( pieces[i] > 0 )
            {
                ++i;
                continue;
            }
            avail = pieces[i];
            for( j = i + 1; j < pieceCount; ++j )
                if( pieces[j] != avail )
                    break;
            if( avail == 0 )
                gdk_cairo_set_source_color( cr, piece_missing_color );
            else
                gdk_cairo_set_source_color( cr, piece_have_color );
            cairo_rectangle( cr, x + pw * i, y, pw * (j - i), h );
            cairo_fill( cr );
            i = j;
        }
    }
}

static void
pieces_cell_renderer_render( GtkCellRenderer       * cell,
                             GdkDrawable           * window,
                             GtkWidget             * widget UNUSED,
                             GdkRectangle          * background_area UNUSED,
                             GdkRectangle          * cell_area,
                             GdkRectangle          * expose_area,
                             GtkCellRendererState    flags UNUSED )
{
    PiecesCellRenderer        * self = PIECES_CELL_RENDERER( cell );
    PiecesCellRendererPrivate * priv = self->priv;
    gint x, y, w, h;

    x = cell_area->x + cell->xpad;
    y = cell_area->y + cell->ypad;
    w = cell_area->width - cell->xpad * 2;
    h = cell_area->height - cell->ypad * 2;

    if( priv->show_pieces )
    {
        static const gint PBH = 4;
        cairo_t * cr, * cro;

        cr = gdk_cairo_create( window );

        gdk_cairo_rectangle( cr, expose_area );
        cairo_clip( cr );

        cro = get_offscreen_context( priv, cr, w, h );

        render_progress( priv, cro, 0, 0, w, PBH );
        render_pieces( priv, cro, 0, PBH, w, h - PBH );
        cairo_destroy( cro );

        cairo_set_source_surface( cr, priv->offscreen, x, y );
        cairo_paint( cr );

        cairo_destroy( cr );
    }
    else
    {
        gtk_paint_box (widget->style, window,
                       GTK_STATE_NORMAL, GTK_SHADOW_IN,
                       NULL, widget, NULL,
                       x, y, w, h);

        if ( priv->tor )
        {
            const tr_stat * st = tr_torrentStatCached( priv->tor );
            GdkRectangle clip;
            clip.x = x;
            clip.y = y;
            clip.width = st->percentDone * w;
            clip.height = h;
            gtk_paint_box (widget->style, window,
                           GTK_STATE_SELECTED, GTK_SHADOW_OUT,
                           &clip, widget, "bar", clip.x, clip.y,
                           clip.width, clip.height );
        }
    }
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
        case PROP_TORRENT:     priv->tor         = g_value_get_pointer( v ); break;
        case PROP_SHOW_PIECES: priv->show_pieces = g_value_get_boolean( v ); break;
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
        case PROP_TORRENT:     g_value_set_pointer( v, priv->tor ); break;
        case PROP_SHOW_PIECES: g_value_set_boolean( v, priv->show_pieces ); break;
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

    g_object_class_install_property( gobject_class, PROP_SHOW_PIECES,
                                     g_param_spec_boolean( "show-pieces", NULL,
                                                           "Show Pieces",
                                                           FALSE,
                                                           G_PARAM_READWRITE ) );

    gdk_color_parse( "#efefff", &cpriv->piece_bg_color );
    gdk_color_parse( "#2975d6", &cpriv->piece_have_color );
    gdk_color_parse( "#d90000", &cpriv->piece_missing_color );
    gdk_color_parse( "#30b027", &cpriv->piece_seeding_color );
    gdk_color_parse( "#dadada", &cpriv->progress_bg_color );
    gdk_color_parse( "#314e6c", &cpriv->progress_bar_color );
    gdk_color_parse( "#a6e3b4", &cpriv->ratio_bg_color );
    gdk_color_parse( "#448632", &cpriv->ratio_bar_color );
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
