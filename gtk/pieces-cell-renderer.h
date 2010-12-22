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

#ifndef PIECES_CELL_RENDERER_H
#define PIECES_CELL_RENDERER_H

#include <gtk/gtkcellrenderer.h>

#define PIECES_CELL_RENDERER_TYPE ( pieces_cell_renderer_get_type( ) )

#define PIECES_CELL_RENDERER( o ) \
    ( G_TYPE_CHECK_INSTANCE_CAST( ( o ), \
                                 PIECES_CELL_RENDERER_TYPE, \
                                 PiecesCellRenderer ) )

typedef struct _PiecesCellRenderer        PiecesCellRenderer;
typedef struct _PiecesCellRendererClass   PiecesCellRendererClass;
typedef struct _PiecesCellRendererPrivate PiecesCellRendererPrivate;

struct _PiecesCellRenderer
{
    GtkCellRenderer             parent;
    PiecesCellRendererPrivate * priv;
};

struct _PiecesCellRendererClass
{
    GtkCellRendererClass parent_class;
};

GType             pieces_cell_renderer_get_type( void );

GtkCellRenderer * pieces_cell_renderer_new( void );

#endif /* PIECES_CELL_RENDERER_H */
