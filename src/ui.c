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
#include "uix.h"
#include "estring.H"
#include "string_var.H"
#include "tok.H"

CVSID("$Id: ui.c,v 1.20 2003-07-19 06:26:13 gnb Exp $");

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

#if GTK2
static PangoFontDescription *ui_text_font_desc;
#else
static GdkFont *ui_text_font;

struct ui_text_tag_s
{
    char *name;
    GdkColor foreground;
};

typedef struct
{
    GList *tags;	    	/* list of tags */
    int lineno;     	    	/* current line number */
    GArray *offsets_by_line;	/* for selecting by line number */
} ui_text_data;

static const char ui_text_data_key[] = "ui_text_data_key";

static void
ui_text_on_destroy(GtkWidget *w, gpointer closure)
{
    ui_text_data *td = (ui_text_data *)closure;

    while (td->tags != 0)
    {
    	ui_text_tag *tag = (ui_text_tag *)td->tags->data;
	
	g_free(tag->name);
	g_free(tag);
	
    	td->tags = g_list_remove_link(td->tags, td->tags);
    }
    g_array_free(td->offsets_by_line, /*free_segment*/TRUE);
    g_free(td);
}

static void
ui_text_line_start(GtkText *text, ui_text_data *td)
{
    unsigned int offset = gtk_text_get_length(text);
#if DEBUG > 5
    fprintf(stderr, "offsets_by_line[%d] = %d\n",
    	td->offsets_by_line->len, offset);
#endif
    g_array_append_val(td->offsets_by_line, offset);
}

#endif /* !GTK2 */

void
ui_text_setup(GtkWidget *w)
{
#if GTK2
    assert(GTK_IS_TEXT_VIEW(w));

    /*
     * Override the font in the text window: it needs to be
     * fixedwidth so the source aligns properly.
     */
    if (ui_text_font_desc == 0)
    	ui_text_font_desc = pango_font_description_from_string("monospace");
    gtk_widget_modify_font(w, ui_text_font_desc);

#ifndef HAVE_GTK_TEXT_BUFFER_SELECT_RANGE
    /* hacky select_region() implementation relies on not having line wrapping */
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(w), GTK_WRAP_NONE);
#endif /* !HAVE_GTK_TEXT_BUFFER_SELECT_RANGE */

#else /* !GTK2 */
    ui_text_data *td;
    
    assert(GTK_IS_TEXT(w));

    td = new(ui_text_data);

    if (ui_text_font == 0)
	ui_text_font = uix_fixed_width_font(gtk_widget_get_style(w)->font);

    td->offsets_by_line = g_array_new(/*zero_terminated*/TRUE,
				      /*clear*/TRUE,
				      sizeof(unsigned int));
    gtk_object_set_data(GTK_OBJECT(w), ui_text_data_key, td);
    gtk_signal_connect(GTK_OBJECT(w), "destroy", 
    	GTK_SIGNAL_FUNC(ui_text_on_destroy), td);
#endif /* !GTK2 */
}


int
ui_text_font_width(GtkWidget *w)
{
#if GTK2
    /*
     * Yes this is a deprecated function...but there doesn't
     * seem to be any other way to get a font width.
     */
    GdkFont *font = gtk_style_get_font(gtk_widget_get_style(w));
    return uix_font_width(font);
#else /* !GTK2 */
    return uix_font_width(ui_text_font);
#endif /* !GTK2 */
}


ui_text_tag *
ui_text_create_tag(GtkWidget *w, const char *name, GdkColor *fg)
{
#if GTK2
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));

    return gtk_text_buffer_create_tag(buffer, name,
    		"foreground-gdk",   	fg,
		(char *)0);
#else /* !GTK2 */
    ui_text_data *td = (ui_text_data *)gtk_object_get_data(GTK_OBJECT(w),
    	    	    	    	    	    	    	   ui_text_data_key);
    ui_text_tag *tag;
    
    tag = new(ui_text_tag);
    tag->name = g_strdup(name);
    tag->foreground = *fg;
    td->tags = g_list_prepend(td->tags, tag);
    
    return tag;
#endif /* !GTK2 */
}   


gfloat
ui_text_vscroll_sample(GtkWidget *w)
{
#if GTK2
    return GTK_TEXT_VIEW(w)->vadjustment->value;
#else /* !GTK2 */
    return GTK_TEXT(w)->vadj->value;
#endif /* !GTK2 */
}

void
ui_text_vscroll_restore(GtkWidget *w, gfloat vs)
{
#if GTK2
    GtkTextView *tv = GTK_TEXT_VIEW(w);

    /* Work around rounding bug in gtk 2.0.2 */
    if (vs + tv->vadjustment->page_size + 0.5 > tv->vadjustment->upper)
    	vs = tv->vadjustment->upper - tv->vadjustment->page_size - 0.5;

    gtk_adjustment_set_value(tv->vadjustment, vs);
#else /* !GTK2 */
    gtk_adjustment_set_value(GTK_TEXT(w)->vadj, vs);
#endif /* !GTK2 */
}

void
ui_text_begin(GtkWidget *w)
{
#if GTK2
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));

    gtk_text_buffer_set_text(buffer, "", -1);
#else /* !GTK2 */
    GtkText *text = GTK_TEXT(w);
    ui_text_data *td = (ui_text_data *)gtk_object_get_data(GTK_OBJECT(w),
    	    	    	    	    	    	    	   ui_text_data_key);

    gtk_text_freeze(text);
    gtk_editable_delete_text(GTK_EDITABLE(text), 0, -1);
    g_array_set_size(td->offsets_by_line, 0);
    ui_text_line_start(text, td);   /* skip 0th entry */
#endif /* !GTK2 */
}

void
ui_text_add(GtkWidget *w, ui_text_tag *tag, const char *str, int len)
{
#if GTK2
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
    GtkTextIter end;

    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert_with_tags(buffer, &end, str, len,
	    	    	    	     tag, (char*)0);
#else /* !GTK2 */
    GtkText *text = GTK_TEXT(w);
    ui_text_data *td = (ui_text_data *)gtk_object_get_data(GTK_OBJECT(w),
    	    	    	    	    	    	    	   ui_text_data_key);
    const char *s, *e;
    GdkColor *fg = (tag == 0 ? 0 : &tag->foreground);
    GdkColor *bg = 0;

    if (len < 0)
    	len = strlen(str);

    /* parse the string for newlines so we can track line offsets */
    s = str;
    while (len > 0 && (e = strchr(s, '\n')) != 0)
    {
    	e++;
	gtk_text_insert(text, ui_text_font, fg, bg, s, (e-s));
    	ui_text_line_start(text, td);
	len -= (e-s);
	s = e;
    }
    if (len > 0)
	gtk_text_insert(text, ui_text_font, fg, bg, s, len);
#endif /* !GTK2 */
}

void
ui_text_end(GtkWidget *w)
{
#if !GTK2
    GtkText *text = GTK_TEXT(w);

    gtk_text_thaw(text);
#endif /* !GTK2 */
}


void
ui_text_select_lines(GtkWidget *w, unsigned long startline, unsigned long endline)
{
#if GTK2
#ifdef HAVE_GTK_TEXT_BUFFER_SELECT_RANGE
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
    GtkTextIter start, end;
    
    gtk_text_buffer_get_iter_at_line(buffer, &start, startline);
    gtk_text_buffer_get_iter_at_line(buffer, &end, endline);

    /* this function appeared sometime after 2.0.2 */
    gtk_text_buffer_select_range(buffer, &start, &end);
#else /* GTK2 && !HAVE_GTK_TEXT_BUFFER_SELECT_RANGE */
    /*
     * In GTK late 1.99 there is no way I can see to use the official
     * API to select a range of rows.  However, we can fake it by
     * calling TextView class methods as if the user had entered the
     * keystrokes for cursor movement and selection...this is a HACK!
     * Unfortunately we can only move down by display lines not
     * logical text lines, so this technique relies on them being
     * identical, i.e. no line wrap.
     */
    GtkTextViewClass *klass = GTK_TEXT_VIEW_GET_CLASS(w);
    GtkTextView *tview = GTK_TEXT_VIEW(w);
    /* first move the cursor to the start of the buffer */
    (*klass->move_cursor)(tview, GTK_MOVEMENT_BUFFER_ENDS, -1, FALSE);
    /* move the cursor down to the start line */
    if (startline > 1)
	(*klass->move_cursor)(tview, GTK_MOVEMENT_DISPLAY_LINES,
    	    	    	      (gint)startline-1, FALSE);
    /* select down to the end line */
    if (endline == startline)
	(*klass->move_cursor)(tview, GTK_MOVEMENT_DISPLAY_LINE_ENDS,
    	    	    	      1, /*extend_selection*/TRUE);
    else
	(*klass->move_cursor)(tview, GTK_MOVEMENT_DISPLAY_LINES,
    	    	    	      (gint)(endline - startline + 1),
			      /*extend_selection*/TRUE);
#endif /* GTK2 && !HAVE_GTK_TEXT_BUFFER_SELECT_RANGE */
#else /* !GTK2 */
    ui_text_data *td = (ui_text_data *)gtk_object_get_data(GTK_OBJECT(w),
    	    	    	    	    	    	    	   ui_text_data_key);
    int endoff;
    
    assert(td->offsets_by_line->len > 0);

    if (startline < 1)
    	startline = 1;
    if (endline < 1)
    	endline = startline;
    if (startline > td->offsets_by_line->len)
    	startline = td->offsets_by_line->len;
    if (endline > td->offsets_by_line->len)
    	endline = td->offsets_by_line->len;
    if (startline > endline)
    	return;
	
    assert(startline >= 1);
    assert(startline <= td->offsets_by_line->len);
    assert(endline >= 1);
    assert(endline <= td->offsets_by_line->len);
    
    /* set endoff to the first location after the last line to be selected */
    if (endline == td->offsets_by_line->len)
	endoff = -1;
    else
	endoff = g_array_index(td->offsets_by_line, unsigned int, endline)-1;

    gtk_editable_select_region(GTK_EDITABLE(w),
    	    g_array_index(td->offsets_by_line, unsigned int, startline-1),
    	    endoff);
#endif /* !GTK2 */
}


void
ui_text_ensure_visible(GtkWidget *w, unsigned long line)
{
#if GTK2
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
    GtkTextIter iter;
    
    gtk_text_buffer_get_iter_at_line(buffer, &iter, line);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(w), &iter,
                                 0.0, FALSE, 0.0, 0.0);
#else
    /* This mostly works.  Not totally predictable but good enough for now */
    ui_text_data *td = (ui_text_data *)gtk_object_get_data(GTK_OBJECT(w),
    	    	    	    	    	    	    	   ui_text_data_key);
    GtkAdjustment *adj = GTK_TEXT(w)->vadj;

    gtk_adjustment_set_value(adj,
	    	adj->upper * (double)line / (double)td->offsets_by_line->len
		    - adj->page_size/2.0);
#endif
}


#if !GTK2
static unsigned long
ui_text_offset_to_lineno(ui_text_data *td, unsigned int offset)
{
    unsigned int top, bottom;
    
    if (offset == 0)
    	return 0;

    top = td->offsets_by_line->len-1;
    bottom = 0;
    
#if DEBUG > 5
    fprintf(stderr, "ui_text_offset_to_lineno: { offset=%u top=%u bottom=%u\n",
    	    offset, top, bottom);
#endif

    while (top - bottom > 1)
    {
    	unsigned int mid = (top + bottom)/2;
	unsigned int midoff = g_array_index(td->offsets_by_line, unsigned int, mid);
	
#if DEBUG > 5
    	fprintf(stderr, "ui_text_offset_to_lineno:     top=%d bottom=%d mid=%d midoff=%u\n",
	    	    	 top, bottom, mid, midoff);
#endif

	if (midoff == offset)
	    top = bottom = mid;
    	else if (midoff < offset)
	    bottom = mid;
	else
	    top = mid;
    }

#if DEBUG > 5
    fprintf(stderr, "ui_text_offset_to_lineno: offset=%u line=%u }\n",
    	    offset, bottom);
#endif

    return (unsigned long)bottom+1;
}
#endif /* !GTK2 */


void
ui_text_get_selected_lines(
    GtkWidget *w,
    unsigned long *startp,
    unsigned long *endp)
{
#if GTK2
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
    GtkTextIter start_iter, end_iter;
    
    if (gtk_text_buffer_get_selection_bounds(buffer, &start_iter, &end_iter))
    {
	if (startp != 0)
	    *startp = 0;
	if (endp != 0)
	    *endp = 0;
    }
    else
    {
	if (startp != 0)
	    *startp = gtk_text_iter_get_line(&start_iter);
	if (endp != 0)
	    *endp = gtk_text_iter_get_line(&end_iter);
    }
#else /* !GTK2 */
    ui_text_data *td = (ui_text_data *)gtk_object_get_data(GTK_OBJECT(w),
    	    	    	    	    	    	    	   ui_text_data_key);

    if (GTK_EDITABLE(w)->selection_start_pos == 0 &&
    	GTK_EDITABLE(w)->selection_end_pos == 0)
    {
	if (startp != 0)
	    *startp = 0;
	if (endp != 0)
	    *endp = 0;
    }
    else
    {
	if (startp != 0)
	    *startp = ui_text_offset_to_lineno(td,
	    	    		GTK_EDITABLE(w)->selection_start_pos);
	if (endp != 0)
	    *endp = ui_text_offset_to_lineno(td,
	    	    		GTK_EDITABLE(w)->selection_end_pos-1);
    }
#endif /* !GTK2 */
}


char *
ui_text_get_contents(GtkWidget *w)
{
#if GTK2
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
    GtkTextIter start, end;

    gtk_text_buffer_get_bounds(buffer, &start, &end);
    return gtk_text_buffer_get_text(buffer, &start, &end,
                                    /*include_hidden_chars*/FALSE);
#else /* !GTK2 */
    return gtk_editable_get_chars(GTK_EDITABLE(w), 0, -1);
#endif /* !GTK2 */
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
