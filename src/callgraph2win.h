/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001 Greg Banks <gnb@alphalink.com.au>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _ggcov_callgraph2win_h_
#define _ggcov_callgraph2win_h_ 1

#include "ui.h"
#include "cov.h"

typedef struct
{
    double x1, y1, x2, y2;
} dbounds_t;

typedef struct
{
    GtkWidget *window;
    gboolean deleting:1;    /* handle possible GUI recursion crap */

    GtkWidget *canvas;
    cov_callnode_t *main_node;
    double zoom;
    dbounds_t bounds;
} callgraph2win_t;


callgraph2win_t *callgraph2win_new(void);
void callgraph2win_delete(callgraph2win_t *cw);


#endif /* _ggcov_callgraphwin_h_ */
