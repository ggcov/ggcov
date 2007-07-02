/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2005 Greg Banks <gnb@alphalink.com.au>
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

GladeXML *ui_load_tree(const char *root);
void ui_prepend_glade_path(const char *dir);
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

#if !GTK2
/* Functions for making the column labels in a clist have sort arrows */
void ui_clist_init_column_arrow(GtkCList *, int col);
void ui_clist_set_sort_column(GtkCList *, int col);
void ui_clist_set_sort_type(GtkCList *, GtkSortType ty);
#endif

/*
 * Functions to abstract some of the differences between
 * gtk1's GtkCList/GtkCTree and gtk2's GtkTreeView.
 */
gpointer ui_list_double_click_data(GtkWidget *, GdkEvent *,
    	    	    	    	   int col/*gtk2 closure column*/);
void ui_list_set_column_visibility(GtkWidget *, int col, gboolean vis);

/*
 * Abstraction of GtkText (gtk 1.2) and GtkTextView et al (gtk 2.0).
 * Text windows get a fixed width font and support colour coding by
 * the use of "tags".  Other features of gtk 2.0 are not supported
 * through this interface.
 */
/* set up a text window, force the font etc */
void ui_text_setup(GtkWidget *w);
/* return the current font width, e.g. for aligning title columns */
int ui_text_font_width(GtkWidget *w);
/*
 * Adjust the text size in the given text widget according to `dirn':
 *
 *  < 0: decrease text size by 10%
 * == 0: set text size to normal text size for the window
 *  > 0: increase text size by 10%
 */
void ui_text_adjust_text_size(GtkWidget *w, int dirn);

#if GTK2
typedef GtkTextTag ui_text_tag;
#else
typedef struct ui_text_tag_s ui_text_tag;
#endif
/* create a colour coding tag */
ui_text_tag *ui_text_create_tag(GtkWidget *w, const char *name, GdkColor *fg);

/* sample and restore the vertical scroll setting */
gfloat ui_text_vscroll_sample(GtkWidget *w);
void ui_text_vscroll_restore(GtkWidget *w, gfloat);

/* add text to the window in streaming mode. tag may be NULL. len may be -1 for strlen */
void ui_text_begin(GtkWidget *w);
void ui_text_add(GtkWidget *w, ui_text_tag *tag, const char *s, int len);
void ui_text_end(GtkWidget *w);

/* select all the lines from `start' to `end' inclusive */
void ui_text_select_lines(GtkWidget *w, unsigned long start, unsigned long end);
/* ensure the given line is visible, if necessary scrolling vertically */
void ui_text_ensure_visible(GtkWidget *w, unsigned long line);
/* return the range of lines currently selected */
void ui_text_get_selected_lines(GtkWidget *w, unsigned long *startp,
    	    	    	    	unsigned long *endp);
/* get the entire contents of the window, needs to be g_free()d */
char *ui_text_get_contents(GtkWidget *w);

typedef struct
{
    const char *name;
    GCallback func;
} ui_named_callback_t;

#endif /* _ggcov_ui_h_ */
