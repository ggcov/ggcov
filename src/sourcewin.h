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

#ifndef _ggcov_sourcewin_h_
#define _ggcov_sourcewin_h_ 1

#include "ui.h"

typedef struct
{
    GtkWidget *window;
    GtkWidget *text;
    GtkWidget *number_check;
    GtkWidget *count_check;
    GtkWidget *source_check;
    GtkWidget *colors_check;
    GtkWidget *filenames_menu;
    GtkWidget *functions_menu;
    char *title_string;
    char *filename;
    unsigned int max_lineno;	/* the largest linenumber */
    GArray *offsets_by_line;
    gboolean deleting:1;    /* handle possible GUI recursion crap */
} sourcewin_t;


sourcewin_t *sourcewin_new(void);
void sourcewin_delete(sourcewin_t *sw);
void sourcewin_set_filename(sourcewin_t *sw, const char *filename);


#endif /* _ggcov_sourcewin_h_ */
