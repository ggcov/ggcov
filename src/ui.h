/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2003 Greg Banks <gnb@alphalink.com.au>
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
#include "uicommon.h"
#include <glade/glade.h>
#include <gdk/gdkkeysyms.h>

#ifdef __cplusplus
#define GLADE_CALLBACK extern "C"
#else
#define GLADE_CALLBACK
#endif

GladeXML *ui_load_tree(const char *root);
extern const char *ui_glade_path;
GtkWidget *ui_get_dummy_menu(GladeXML *xml, const char *name);


/* combobox get/set current item using index */
int gtk_combo_get_current(GtkCombo *combo);
void gtk_combo_set_current(GtkCombo *combo, int n);

/* combobox add/get current item using data */
void ui_combo_add_data(GtkCombo *combo, const char *label, gpointer data);
gpointer ui_combo_get_current_data(GtkCombo *combo);
void ui_combo_set_current_data(GtkCombo *combo, gpointer data);

void ui_combo_clear(GtkCombo *combo);

/* Get the nearest enclosing dialog or toplevel window */
GtkWidget *ui_get_window(GtkWidget *w);

/* when the last registered window is destroyed, quit the main loop */
void ui_register_window(GtkWidget *w);
/* remember a menu which is updated when windows are registered & destroyed */
void ui_register_windows_menu(GtkWidget *menu);
/* add a predefined menu entry to all Windows menus */
void ui_register_windows_entry(const char *label,
    	    	    	       void (*callback)(GtkWidget *, gpointer userdata),
			       gpointer userdata);

/*
 * Set the icon that will be used for all toplevel windows.
 */
void ui_set_default_icon(char **xpm_data);


/*
 * Update the window's title to reflect `filename'.  The
 * magic string @FILE@ is used in the glade file to mark
 * where the filename will be inserted; @VERSION@ works too.
 */
void ui_window_set_title(GtkWidget *w, const char *filename);

/* delete every item in the menu except a tearoff */
void ui_delete_menu_items(GtkWidget *menu);
GtkWidget *ui_menu_add_simple_item(GtkWidget *menu, const char *label,
	       void (*callback)(GtkWidget*, gpointer), gpointer calldata);
GtkWidget *ui_menu_add_seperator(GtkWidget *menu);

/* Functions for making the column labels in a clist have sort arrows */
void ui_clist_init_column_arrow(GtkCList *, int col);
void ui_clist_set_sort_column(GtkCList *, int col);
void ui_clist_set_sort_type(GtkCList *, GtkSortType ty);

/* handles clist & ctree */
gpointer ui_clist_double_click_data(GtkCList *, GdkEvent *);
#if GTK2
gpointer ui_tree_view_double_click_data(GtkTreeView *, GdkEvent *, int column);
#endif


#endif /* _ggcov_ui_h_ */
