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

#ifndef GTR_PIECES_CELL_RENDERER_H
#define GTR_PIECES_CELL_RENDERER_H

#include <gtk/gtkcellrenderer.h>

#define GTR_TYPE_PIECES_CELL_RENDERER ( gtr_pieces_cell_renderer_get_type( ) )
GType gtr_pieces_cell_renderer_get_type( void );
#define GTR_PIECES_CELL_RENDERER( obj ) \
    ( G_TYPE_CHECK_INSTANCE_CAST( ( obj ), GTR_TYPE_PIECES_CELL_RENDERER, \
                                  GtrPiecesCellRenderer ) )
#define GTR_PIECES_CELL_RENDERER_CLASS( obj ) \
    ( G_TYPE_CHECK_CLASS_CAST( ( obj ), GTR_PIECES_CELL_RENDERER, \
                               GtrPiecesCellRendererClass ) )
#define GTR_IS_PIECES_CELL_RENDERER( obj ) \
    ( G_TYPE_CHECK_INSTANCE_TYPE( ( obj ), GTR_TYPE_PIECES_CELL_RENDERER ) )
#define GTR_IS_PIECES_CELL_RENDERER_CLASS( obj ) \
    ( G_TYPE_CHECK_CLASS_TYPE( ( obj ), GTR_TYPE_PIECES_CELL_RENDERER ) )
#define GTR_PIECES_CELL_RENDERER_GET_CLASS \
    ( G_TYPE_INSTANCE_GET_CLASS( ( obj ), GTR_TYPE_PIECES_CELL_RENDERER, \
                                 GtrPiecesCellRendererClass ) )

typedef struct _GtrPiecesCellRenderer      GtrPiecesCellRenderer;
typedef struct _GtrPiecesCellRendererClass GtrPiecesCellRendererClass;

struct _GtrPiecesCellRenderer
{
    GtkCellRenderer             parent;
};

struct _GtrPiecesCellRendererClass
{
    GtkCellRendererClass parent_class;
};

GtkCellRenderer * gtr_pieces_cell_renderer_new( void );

#endif /* GTR_PIECES_CELL_RENDERER_H */
