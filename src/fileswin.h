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

#ifndef _ggcov_fileswin_h_
#define _ggcov_fileswin_h_ 1

#include "ui.h"

typedef struct
{
    GtkWidget *window;
    gboolean deleting:1;    /* handle possible GUI recursion crap */

    GList *files;	    /* list of file_rec_t */
    
    GtkWidget *lines_check;
    GtkWidget *calls_check;
    GtkWidget *branches_check;
    GtkWidget *percent_check;
    GtkWidget *clist;
} fileswin_t;


fileswin_t *fileswin_new(void);
void fileswin_delete(fileswin_t *fw);


#endif /* _ggcov_fileswin_h_ */
