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

#include "ui.h"
#include "estring.H"
#include "string_var.H"
#include "tok.H"

CVSID("$Id: ui.c,v 1.19 2003-06-09 05:26:02 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
gtk_combo_get_current(GtkCombo *combo)
{
    GtkList *listw = GTK_LIST(combo->list);
    
    if (listw->selection == 0)
    	return 0;
    return gtk_list_child_position(listw, (GtkWidget*)listw->selection->data);
}

void
gtk_combo_set_current(GtkCombo *combo, int n)
{
    gtk_list_select_item(GTK_LIST(combo->list), n);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char ui_combo_item_key[] = "ui_combo_item_key";

void
ui_combo_add_data(GtkCombo *combo, const char *label, gpointer data)
{
    GtkWidget *item;
    
    item = gtk_list_item_new_with_label(label);
    gtk_object_set_data(GTK_OBJECT(item), ui_combo_item_key, data);
    gtk_widget_show(item);
    gtk_container_add(GTK_CONTAINER(combo->list), item);
}

gpointer
ui_combo_get_current_data(GtkCombo *combo)
{
    GtkList *listw = GTK_LIST(combo->list);

    if (listw->selection == 0)
    	return 0;
    return gtk_object_get_data(GTK_OBJECT(listw->selection->data),
    	    	    	       ui_combo_item_key);
}

void
ui_combo_set_current_data(GtkCombo *combo, gpointer data)
{
    GtkList *listw = GTK_LIST(combo->list);
    GList *iter;
    
    for (iter = listw->children ; iter != 0 ; iter = iter->next)
    {
    	GtkWidget *item = (GtkWidget *)iter->data;
	
	if (gtk_object_get_data(GTK_OBJECT(item), ui_combo_item_key) == data)
	{
	    gtk_list_select_child(listw, item);
	    return;
	}
    }
}

void
ui_combo_clear(GtkCombo *combo)
{
    gtk_list_clear_items(GTK_LIST(combo->list), 0, -1);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#ifndef UI_DEBUG
#define UI_DEBUG 0
#endif

const char *ui_glade_path = 
#if DEBUG || UI_DEBUG
"ui:../../ui:"
#endif
PKGDATADIR;

static char *
find_file(const char *path, const char *base)
{
    tok_t tok(path, ":");
    string_var file;
    const char *dir;
    struct stat sb;
    
    while ((dir = tok.next()) != 0)
    {
	file = g_strconcat(dir, "/", base, 0);

	if (stat(file, &sb) == 0 && S_ISREG(sb.st_mode))
	    return file.take();
    }
    
    return 0;
}

#if GTK2
#define LIBGLADE    "-glade2"
#else
#define LIBGLADE
#endif

GladeXML *
ui_load_tree(const char *root)
{
    GladeXML *xml = 0;
    /* load & create the interface */
    static char *filename = 0;
    static const char gladefile[] = PACKAGE LIBGLADE ".glade";
    
    if (filename == 0)
    {
	filename = find_file(ui_glade_path, gladefile);
	if (filename == 0)
	    fatal("can't find %s in path %s\n", gladefile, ui_glade_path);
#if DEBUG
	fprintf(stderr, "Loading Glade UI from file \"%s\"\n", filename);
#endif
    }

#if GTK2
    xml = glade_xml_new(filename, root, /* translation domain */PACKAGE);
#else
    xml = glade_xml_new(filename, root);
#endif

    if (xml == 0)
    	fatal("can't load Glade UI from file \"%s\"\n", filename);
    
    /* connect the signals in the interface */
    glade_xml_signal_autoconnect(xml);

    return xml;
}


GtkWidget *
ui_get_dummy_menu(GladeXML *xml, const char *name)
{
    GtkWidget *dummy;
    GtkWidget *menu;
    GtkWidget *tearoff;

    dummy = glade_xml_get_widget(xml, name);
    
    menu = dummy->parent;
    
    tearoff = gtk_tearoff_menu_item_new();
    gtk_menu_append(GTK_MENU(menu), tearoff);
    gtk_widget_show(tearoff);
    
    return menu;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GtkWidget *ui_get_window(GtkWidget *w);

static GtkWidget *
_ui_virtual_parent(GtkWidget *w)
{
    if (GTK_IS_MENU(w))
    	return GTK_MENU(w)->parent_menu_item;
    return GTK_WIDGET(w)->parent;
}

GtkWidget *
ui_get_window(GtkWidget *w)
{
    for ( ; 
    	 w != 0 && !GTK_IS_WINDOW(w) ;
	 w = _ui_virtual_parent(w))
    	;
    return w;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static struct
{
    char **xpm;
    GdkPixmap *pm;
    GdkBitmap *mask;
} ui_default_icon;

static void
ui_window_set_default_icon(GtkWidget *w)
{
    if (!GTK_WIDGET_REALIZED(w))
    {
#if UI_DEBUG > 10
    	fprintf(stderr, "ui_window_set_default_icon(w=0x%08lx): delaying until realized\n",
	    	(unsigned)w);
#endif
    	/* delay self until realized */
	gtk_signal_connect(GTK_OBJECT(w), "realize",
	    GTK_SIGNAL_FUNC(ui_window_set_default_icon), 0);
	/* don't bother disconnecting the signal, it will only ever fire once */
    	return;
    }
    
#if UI_DEBUG > 10
    fprintf(stderr, "ui_window_set_default_icon(w=0x%08lx)\n",
	    (unsigned)w);
#endif
	    
    if (ui_default_icon.pm == 0)
    	ui_default_icon.pm = gdk_pixmap_create_from_xpm_d(w->window,
	    	    	    	&ui_default_icon.mask, 0,
				ui_default_icon.xpm);
    gdk_window_set_icon(w->window, 0,
	ui_default_icon.pm, ui_default_icon.mask);
}

void
ui_set_default_icon(char **xpm_data)
{
    ui_default_icon.xpm = xpm_data;
    ui_default_icon.pm = 0; 	/* JIC */
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    char *label;
    void (*callback)(GtkWidget*, gpointer);
    void *userdata;
} ui_windows_entry_t;

static GList *ui_windows;
static GList *ui_windows_menus;
static GList *ui_windows_entries;   /* predefined entries, ui_windows_entry_t */

static void
ui_on_windows_menu_activate(GtkWidget *w, gpointer userdata)
{
    GtkWidget *win = (GtkWidget *)userdata;
        
#if DEBUG
    fprintf(stderr, "ui_on_windows_menu_activate: %s\n",
    	    	    	GTK_WINDOW(win)->title);
#endif
			
    gdk_window_show(win->window);
    gdk_window_raise(win->window);
}

static void
ui_update_windows_menu(GtkWidget *menu)
{
    GList *iter;
    
    ui_delete_menu_items(menu);
    
    for (iter = ui_windows_entries ; iter != 0 ; iter = iter->next)
    {
    	ui_windows_entry_t *we = (ui_windows_entry_t *)iter->data;
	
	/*
	 * Presumably a side effect of the callback will be
	 * the creation of a new window which is registered
	 * by calling ui_register_window().
	 */
	ui_menu_add_simple_item(menu, we->label,
	    	    we->callback, we->userdata);
    }
    
    if (ui_windows_entries != 0)
	ui_menu_add_seperator(menu);
    
    for (iter = ui_windows ; iter != 0 ; iter = iter->next)
    {
    	GtkWidget *win = (GtkWidget *)iter->data;
	const char *title;
	
	if ((title = strchr(GTK_WINDOW(win)->title, ':')) != 0)
	{
	    title++;	/* skip colon */
	    while (*title && isspace(*title))
	    	title++;    	    /* skip whitespace */
	}
	if (title == 0 || *title == '\0')
	    title = GTK_WINDOW(win)->title;
	
	ui_menu_add_simple_item(menu, title,
	    	    ui_on_windows_menu_activate, win);
    }
}

static void
ui_update_windows_menus(void)
{
    GList *iter;
    
    for (iter = ui_windows_menus ; iter != 0 ; iter = iter->next)
    	ui_update_windows_menu((GtkWidget *)iter->data);
}

static void
ui_on_window_destroy(GtkWidget *w, gpointer userdata)
{
    ui_windows = g_list_remove(ui_windows, w);
    if (ui_windows == 0)
    	gtk_main_quit();
    else
	ui_update_windows_menus();
}

void
ui_register_window(GtkWidget *w)
{
    ui_windows = g_list_append(ui_windows, w);
    gtk_signal_connect(GTK_OBJECT(w), "destroy", 
    	GTK_SIGNAL_FUNC(ui_on_window_destroy), 0);
    ui_update_windows_menus();
    
    if (GTK_WINDOW(w)->type == GTK_WINDOW_TOPLEVEL &&
    	ui_default_icon.xpm != 0)
    	ui_window_set_default_icon(w);
}

static void
ui_on_windows_menu_destroy(GtkWidget *w, gpointer userdata)
{
    ui_windows_menus = g_list_remove(ui_windows_menus, w);
}

void
ui_register_windows_menu(GtkWidget *menu)
{
    ui_windows_menus = g_list_append(ui_windows_menus, menu);
    gtk_signal_connect(GTK_OBJECT(menu), "destroy", 
    	GTK_SIGNAL_FUNC(ui_on_windows_menu_destroy), 0);
    ui_update_windows_menu(menu);
}

void
ui_register_windows_entry(
    const char *label,
    void (*callback)(GtkWidget *w, gpointer userdata),
    gpointer userdata)
{
    ui_windows_entry_t *we;
    
    we = new(ui_windows_entry_t);
    we->label = g_strdup(label);
    we->callback = callback;
    we->userdata = userdata;
    
    ui_windows_entries = g_list_append(ui_windows_entries, we);
    ui_update_windows_menus();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char ui_title_key[] = "ui_title_key";

static void
ui_window_title_destroy(void *data)
{
    g_free(data);
}

void
ui_window_set_title(GtkWidget *w, const char *filename)
{
    char *proto;
    estring title;

    /* Grab the original title, derived from the glade file */
    if ((proto = (char *)gtk_object_get_data(GTK_OBJECT(w), ui_title_key)) == 0)
    {
    	proto = g_strdup(GTK_WINDOW(w)->title);
	gtk_object_set_data_full(GTK_OBJECT(w), ui_title_key, proto,
	    	    	    	 ui_window_title_destroy);
    }

    title.append_string(proto);
    title.replace_all("@FILE@", filename);
    title.replace_all("@VERSION@", VERSION);
    
    gtk_window_set_title(GTK_WINDOW(w), title.data());
        
    /* update windows menus if necessary */
    if (g_list_find(ui_windows, w))
    	ui_update_windows_menus();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
ui_delete_menu_items(GtkWidget *menu)
{
    GList *list, *next;
    
    for (list = gtk_container_children(GTK_CONTAINER(menu)) ;
    	 list != 0 ;
	 list = next)
    {
    	GtkWidget *child = (GtkWidget *)list->data;
	next = list->next;

    	if (GTK_IS_TEAROFF_MENU_ITEM(child))
	    continue;
	    
	gtk_widget_destroy(child);	
    }
}

GtkWidget *
ui_menu_add_simple_item(
    GtkWidget *menu,
    const char *label,
    void (*callback)(GtkWidget*, gpointer),
    gpointer calldata)
{
    GtkWidget *butt;

    butt = gtk_menu_item_new_with_label(label);
    gtk_menu_append(GTK_MENU(menu), butt);
    gtk_signal_connect(GTK_OBJECT(butt), "activate", 
    		       GTK_SIGNAL_FUNC(callback),
		       (gpointer)calldata);
    gtk_widget_show(butt);
    
    return butt;
}

GtkWidget *
ui_menu_add_seperator(GtkWidget *menu)
{
    GtkWidget *butt;

    butt = gtk_menu_item_new();
    gtk_menu_append(GTK_MENU(menu), butt);
    gtk_widget_show(butt);
    
    return butt;
}
     
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char ui_clist_arrow_key[] = "ui_clist_arrow_key";

static gboolean
ui_clist_is_sortable_column(GtkCList *clist, int col)
{
    GArray *sortables;
    unsigned int i;

    sortables = (GArray *)gtk_object_get_data(GTK_OBJECT(clist), ui_clist_arrow_key);
    assert(sortables != 0);
    
    for (i = 0 ; i < sortables->len ; i++)
    	if (g_array_index(sortables, int, i) == col)
	    return TRUE;
	    
    return FALSE;   /* this column not sortable */
}

static void
ui_clist_sortables_destroy(gpointer userdata)
{
    g_array_free((GArray *)userdata, /*free_segment*/TRUE);
}

static void
ui_on_clist_click_column(GtkCList *clist, int col, gpointer userdata)
{
    /* first, check this is a sortable column */
    if (!ui_clist_is_sortable_column(clist, col))
    	return;     	/* this column not sortable */

    /* now we can update the sort specifications */
    if (col == clist->sort_column)
	/* toggle the sort direction */
    	ui_clist_set_sort_type(clist, (clist->sort_type == GTK_SORT_ASCENDING ?
	    	    	    	       GTK_SORT_DESCENDING : GTK_SORT_ASCENDING));
    else
	/* make this the sortable column */
    	ui_clist_set_sort_column(clist, col);
	
    /* update the order in the list */
    gtk_clist_sort(clist);
}

void
ui_clist_init_column_arrow(GtkCList *clist, int col)
{
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *arrow;
    GtkWidget *oldlabel;
    char *oldstr = 0;
    GArray *sortables;
    
    oldlabel = gtk_clist_get_column_widget(clist, col);
    if (GTK_IS_BIN(oldlabel))
    	oldlabel = GTK_BIN(oldlabel)->child;
    if (!GTK_IS_LABEL(oldlabel))
    	return;
    gtk_label_get(GTK_LABEL(oldlabel), &oldstr);
    
    hbox = gtk_hbox_new(/*homogeneous*/FALSE, /*spacing*/4);
    gtk_widget_show(hbox);
    
    label = gtk_label_new(oldstr);
    gtk_box_pack_start(GTK_BOX(hbox), label,
    	    	       /*expand*/TRUE, /*fill*/TRUE, /*padding*/0);
    gtk_widget_show(label);
    
    arrow = gtk_arrow_new(GTK_ARROW_DOWN,
    	       (col == clist->sort_column ? GTK_SHADOW_OUT : GTK_SHADOW_NONE));
    gtk_box_pack_start(GTK_BOX(hbox), arrow,
    	    	      /*expand*/FALSE, /*fill*/TRUE, /*padding*/0);
    gtk_widget_show(arrow);
    
    gtk_clist_set_column_widget(clist, col, hbox);
    gtk_clist_column_title_active(clist, col);
    
    /* Setup signal handler and update sortables array */
    sortables = (GArray *)gtk_object_get_data(GTK_OBJECT(clist), ui_clist_arrow_key);
    if (sortables == 0)
    {
    	sortables = g_array_new(/*zero_terminated*/FALSE, /*clear*/FALSE, sizeof(int));
    	gtk_object_set_data_full(GTK_OBJECT(clist), ui_clist_arrow_key,
	    	    	    	 sortables, ui_clist_sortables_destroy);
	gtk_signal_connect(GTK_OBJECT(clist), "click_column",
    	    	    	GTK_SIGNAL_FUNC(ui_on_clist_click_column), 0);
    }
    g_array_append_val(sortables, col);
}

static GtkWidget *
ui_clist_get_column_arrow(GtkCList *clist, int col)
{
    GtkWidget *button = clist->column[col].button;
    GtkWidget *hbox = GTK_BIN(button)->child;
    GtkBoxChild *boxchild = (GtkBoxChild *)g_list_nth_data(GTK_BOX(hbox)->children, 1);
    return boxchild->widget;
}

void
ui_clist_set_sort_column(GtkCList *clist, int col)
{
    GtkWidget *arrow;
    
    if (!ui_clist_is_sortable_column(clist, col))
    	return;
	
    if (ui_clist_is_sortable_column(clist, clist->sort_column))
    {
	arrow = ui_clist_get_column_arrow(clist, clist->sort_column);
	gtk_arrow_set(GTK_ARROW(arrow), GTK_ARROW_DOWN, GTK_SHADOW_NONE);
    }
    
    gtk_clist_set_sort_column(clist, col);

    arrow = ui_clist_get_column_arrow(clist, clist->sort_column);
    gtk_arrow_set(GTK_ARROW(arrow),
      (clist->sort_type == GTK_SORT_ASCENDING ? GTK_ARROW_DOWN : GTK_ARROW_UP),
      GTK_SHADOW_OUT);
}

void
ui_clist_set_sort_type(GtkCList *clist, GtkSortType type)
{
    GtkWidget *arrow;

    if (!ui_clist_is_sortable_column(clist, clist->sort_column))
    	return;

    gtk_clist_set_sort_type(clist, type);
    arrow = ui_clist_get_column_arrow(clist, clist->sort_column);
    gtk_arrow_set(GTK_ARROW(arrow),
      (clist->sort_type == GTK_SORT_ASCENDING ? GTK_ARROW_DOWN : GTK_ARROW_UP),
      GTK_SHADOW_OUT);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gpointer
ui_clist_double_click_data(GtkCList *clist, GdkEvent *event)
{
    int row, col;

    if (event->type == GDK_2BUTTON_PRESS &&
	gtk_clist_get_selection_info(clist,
	    	    	    	     (int)event->button.x,
				     (int)event->button.y,
				     &row, &col))
    {
#if DEBUG
    	fprintf(stderr, "ui_clist_double_click_data: row=%d col=%d\n",
	    	    	row, col);
#endif
	return gtk_clist_get_row_data(clist, row);
    }
    return 0;
}

#if GTK2
gpointer
ui_tree_view_double_click_data(GtkTreeView *tv, GdkEvent *event, int column)
{
    GtkTreeModel *model;
    GtkTreePath *path = 0;
    GtkTreeIter iter;
    gpointer *data = 0;

    if (event->type == GDK_2BUTTON_PRESS &&
    	gtk_tree_view_get_path_at_pos(tv,
	    	    	    	     (int)event->button.x,
				     (int)event->button.y,
				     &path, (GtkTreeViewColumn **)0,
				     (gint *)0, (gint *)0))
    {
#if DEBUG
    	string_var path_str = gtk_tree_path_to_string(path);
    	fprintf(stderr, "ui_tree_view_double_click_data: path=\"%s\"\n",
	    	    	path_str.data());
#endif
    	model = gtk_tree_view_get_model(tv);
    	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, column, &data, -1);
	gtk_tree_path_free(path);
	
	return data;
    }
    return 0;
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
