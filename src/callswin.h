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

#ifndef _ggcov_callswin_h_
#define _ggcov_callswin_h_ 1

#include "ui.h"

typedef struct
{
    GtkWidget *window;
    gboolean deleting:1;    /* handle possible GUI recursion crap */

    GtkWidget *from_function_combo;
    GtkWidget *from_function_view;

    GtkWidget *to_function_combo;
    GtkWidget *to_function_view;

    GtkWidget *clist;
    GList *functions;
} callswin_t;


callswin_t *callswin_new(void);
void callswin_delete(callswin_t *sw);


#endif /* _ggcov_callswin_h_ */
