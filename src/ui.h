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

#ifndef _ggcov_ui_h_
#define _ggcov_ui_h_ 1

#include "common.h"
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gdk/gdkkeysyms.h>
#include <gdk_imlib.h>

#define GLADE_CALLBACK

GladeXML *ui_load_tree(const char *root);
extern const char *ui_glade_path;

int gtk_combo_get_current(GtkCombo *combo);
void gtk_combo_set_current(GtkCombo *combo, int n);

/* Get the nearest enclosing dialog or toplevel window */
GtkWidget *ui_get_window(GtkWidget *w);

/* when the last registered window is destroyed, quit the main loop */
void ui_register_window(GtkWidget *w);

void ui_delete_menu_items(GtkWidget *menu);

#endif /* _ggcov_ui_h_ */
