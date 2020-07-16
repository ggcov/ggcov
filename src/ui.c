/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>
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
#include "window.H"
#include "confsection.H"
#include "logging.H"

static logging::logger_t &_log = logging::find_logger("uicore");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#define COL_LABEL   0
#define COL_DATA    1
#define COL_ICON    2
#define COLUMN_TYPES \
    3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING
static const char ui_combo_sep_key[] = "ui_combo_sep_key";

ui_combo_t *
init(ui_combo_t *cbox, const char *sep)
{
    GtkCellRenderer *rend;
    if (sep)
    {
	g_object_set_data(G_OBJECT(cbox), ui_combo_sep_key, (gpointer)sep);
	GtkTreeStore *store = gtk_tree_store_new(COLUMN_TYPES);
	gtk_combo_box_set_model(cbox, GTK_TREE_MODEL(store));

	rend = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbox), rend, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cbox), rend, "stock-id", COL_ICON, (char *)0);

	rend = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(rend), "xalign", 0.0, (char *)0);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbox), rend, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cbox), rend, "text", COL_LABEL, (char *)0);
    }
    else
    {
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
	gtk_combo_box_set_model(cbox, GTK_TREE_MODEL(store));

	GtkCellRenderer *rend = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbox), rend, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cbox), rend, "text", COL_LABEL, (char *)0);
    }
    return cbox;
}

static const char pending_model_key[] = "ui-pending-model";

void
clear(ui_combo_t *cbox)
{
    GtkTreeModel *model = gtk_combo_box_get_model(cbox);
    /* Detach the model from the cbox temporarily.  This was found
     * experimentally to have a huge performance impact, at least
     * for 2.16, due to the GtkTreeView's signal handlers not being
     * called on every row add, which had horrible O(N^2) behaviour */
    g_object_ref(G_OBJECT(model));
    gtk_combo_box_set_model(cbox, NULL);
    if (GTK_IS_TREE_STORE(model))
	gtk_tree_store_clear(GTK_TREE_STORE(model));
    else
	gtk_list_store_clear(GTK_LIST_STORE(model));
    g_object_set_data(G_OBJECT(cbox), pending_model_key, model);
}

void
add(ui_combo_t *cbox, const char *label, gpointer data)
{
    GtkTreeModel *model = (GtkTreeModel *)
	g_object_get_data(G_OBJECT(cbox), pending_model_key);

    if (GTK_IS_TREE_STORE(model))
    {
	const char *sep = (const char *)
		    g_object_get_data(G_OBJECT(cbox), ui_combo_sep_key);
	tok_t tok(label, sep);
	GtkTreeIter itr;
	gboolean itr_valid = gtk_tree_model_get_iter_first(model, &itr);
	GtkTreeIter parent;
	gboolean parent_valid = FALSE;
	while (const char *comp = tok.next())
	{
	    while (itr_valid)
	    {
		char *ilabel = 0;
		gtk_tree_model_get(model, &itr, COL_LABEL, &ilabel, -1);
		assert(ilabel);
		boolean done = !strcmp(ilabel, comp);
		g_free(ilabel);
		if (done)
		    break;
		itr_valid = gtk_tree_model_iter_next(model, &itr);
	    }
	    if (!itr_valid)
	    {
		gtk_tree_store_append(GTK_TREE_STORE(model), &itr,
				      (parent_valid ? &parent : 0));
		gtk_tree_store_set(GTK_TREE_STORE(model), &itr,
				   COL_LABEL, comp,
				   COL_ICON, GTK_STOCK_DIRECTORY,
				   -1);
		itr_valid = TRUE;
	    }
	    parent = itr;
	    parent_valid = itr_valid;   // always TRUE
	}
	gtk_tree_store_set(GTK_TREE_STORE(model), &itr,
			   COL_DATA, data,
			   COL_ICON, GTK_STOCK_FILE,
			   -1);
    }
    else
    {
	GtkListStore *store = GTK_LIST_STORE(model);
	GtkTreeIter treeitr;
	/* Atomically (w.r.t. gtk signals) append a new column.
	 * Experiment indicates that appending has exactly the
	 * same performance as prepending, at least for 2.16. */
	gtk_list_store_insert_with_values(store, &treeitr, 0x7fffffff,
					  COL_LABEL, label, COL_DATA, data, -1);
    }
}

void
done(ui_combo_t *cbox)
{
    GtkTreeModel *model = (GtkTreeModel *)
	g_object_steal_data(G_OBJECT(cbox), pending_model_key);
    gtk_combo_box_set_model(cbox, model);
    g_object_unref(G_OBJECT(model));
}

gpointer
get_active(ui_combo_t *cbox)
{
    GtkTreeModel *model = gtk_combo_box_get_model(cbox);
    GtkTreeIter treeitr;
    void *data = 0;
    if (gtk_combo_box_get_active_iter(cbox, &treeitr))
	gtk_tree_model_get(model, &treeitr, COL_DATA, &data, -1);
    return data;
}

void
set_active(ui_combo_t *cbox, gpointer data)
{
    GtkTreeModel *model = gtk_combo_box_get_model(cbox);
    GtkTreeIter treeitr;

    gboolean valid = gtk_tree_model_get_iter_first(model, &treeitr);

    while (valid)
    {
	void *dd = 0;
	gtk_tree_model_get(model, &treeitr, COL_DATA, &dd, -1);
	if (data == dd)
	{
	    gtk_combo_box_set_active_iter(cbox, &treeitr);
	    return;
	}
	valid = gtk_tree_model_iter_next(model, &treeitr);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#ifndef UI_DEBUG
#define UI_DEBUG 0
#endif

static string_var ui_glade_path = PKGDATADIR;

static char *
find_file(const char *path, const char *base)
{
    tok_t tok(path, ":");
    string_var file;
    const char *dir;
    struct stat sb;

    while ((dir = tok.next()) != 0)
    {
	file = g_strconcat(dir, "/", base, (char *)0);

	if (stat(file, &sb) == 0 && S_ISREG(sb.st_mode))
	    return file.take();
    }

    return 0;
}

void
ui_prepend_glade_path(const char *dir)
{
    ui_glade_path = g_strconcat(dir, ":", ui_glade_path.data(), (char *)0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern ui_class_callback_t ui_class_callbacks[];    /* in glade_callbacks.c */
extern ui_simple_callback_t ui_simple_callbacks[];  /* in glade_callbacks.c */

/*
 * Connect up named functions for signal handlers.
 */
void
ui_register_callbacks(GladeXML *xml)
{
    int i;

    for (i = 0 ; ui_simple_callbacks[i].name ; i++)
	glade_xml_signal_connect(xml,
				 ui_simple_callbacks[i].name,
				 ui_simple_callbacks[i].func);
    for (i = 0 ; ui_class_callbacks[i].name ; i++)
	glade_xml_signal_connect_data(xml,
				      ui_class_callbacks[i].name,
				      ui_class_callbacks[i].tramp,
				      &ui_class_callbacks[i]);
}

/*
 * Unfortunately, we can't use glade_xml_signal_connect()
 * to hook up our custom widget creation functions, we have
 * to do it this way instead.  Fmeh.
 */
static GtkWidget *
ui_custom_widget_handler(
    GladeXML *xml,
    gchar *func_name,
    gchar *name,
    gchar *string1,
    gchar *string2,
    gint int1,
    gint int2,
    gpointer user_data)
{
    int i;

    for (i = 0 ; ui_simple_callbacks[i].name ; i++)
    {
	if (!strcmp(func_name, ui_simple_callbacks[i].name))
	{
	    /* TODO: pass down some of the string1 et al arguments */
	    return ((GtkWidget *(*)(void))ui_simple_callbacks[i].func)();
	}
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GladeXML *
ui_load_tree(const char *root)
{
    GladeXML *xml = 0;
    /* load & create the interface */
    static char *filename = 0;
    static const char gladefile[] = PACKAGE "-glade2.glade";

    if (filename == 0)
    {
	filename = find_file(ui_glade_path, gladefile);
	if (filename == 0)
	    fatal("can't find %s in path %s\n", gladefile, ui_glade_path.data());
	_log.debug("Loading Glade UI from file \"%s\"\n", filename);
    }

    glade_set_custom_handler(ui_custom_widget_handler, (gpointer)0);

    xml = glade_xml_new(filename, root, /* translation domain */PACKAGE);

    if (xml == 0)
	fatal("can't load Glade UI from file \"%s\"\n", filename);

    /* connect the signals in the interface */
    ui_register_callbacks(xml);

    return xml;
}


GtkWidget *
ui_get_dummy_menu(GladeXML *xml, const char *name)
{
    GtkWidget *dummy;
    GtkWidget *menu;
    GtkWidget *tearoff;

    dummy = glade_xml_get_widget(xml, name);

    menu = gtk_widget_get_parent(dummy);

    tearoff = gtk_tearoff_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), tearoff);
    gtk_widget_show(tearoff);

    return menu;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static GtkWidget *
_ui_virtual_parent(GtkWidget *w)
{
    if (GTK_IS_MENU(w))
	return gtk_menu_get_attach_widget(GTK_MENU(w));
    return gtk_widget_get_parent(GTK_WIDGET(w));
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
    const char * const *xpm;
    GdkPixmap *pm;
    GdkBitmap *mask;
} ui_default_icon;

static void
ui_window_set_default_icon(GtkWidget *w)
{
    if (!gtk_widget_get_realized(w))
    {
	_log.debug2("ui_window_set_default_icon(w=0x%08lx): "
		    "delaying until realized\n", (unsigned long)w);
	/* delay self until realized */
	g_signal_connect(G_OBJECT(w), "realize",
	    G_CALLBACK(ui_window_set_default_icon), 0);
	/* don't bother disconnecting the signal, it will only ever fire once */
	return;
    }

    _log.debug2("ui_window_set_default_icon(w=0x%08lx)\n", (unsigned long)w);

    if (ui_default_icon.pm == 0)
	ui_default_icon.pm = gdk_pixmap_create_from_xpm_d(
                                gtk_widget_get_window(w),
				&ui_default_icon.mask, 0,
				(char **)ui_default_icon.xpm);
    gdk_window_set_icon(
        gtk_widget_get_window(w), 0,
	ui_default_icon.pm, ui_default_icon.mask);
}

void
ui_set_default_icon(const char * const *xpm_data)
{
    ui_default_icon.xpm = xpm_data;
    ui_default_icon.pm = 0;     /* JIC */
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

    _log.debug("ui_on_windows_menu_activate: %s\n",
               gtk_window_get_title(GTK_WINDOW(win)));

    GdkWindow *window = gtk_widget_get_window(win);
    gdk_window_show(window);
    gdk_window_raise(window);
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

	if ((title = strchr(gtk_window_get_title(GTK_WINDOW(win)), ':')) != 0)
	{
	    title++;    /* skip colon */
	    while (*title && isspace(*title))
		title++;            /* skip whitespace */
	}
	if (title == 0 || *title == '\0')
	    title = gtk_window_get_title(GTK_WINDOW(win));

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
    g_signal_connect(G_OBJECT(w), "destroy",
	G_CALLBACK(ui_on_window_destroy), 0);
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
    g_signal_connect(G_OBJECT(menu), "destroy",
	G_CALLBACK(ui_on_windows_menu_destroy), 0);
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
    if ((proto = (char *)g_object_get_data(G_OBJECT(w), ui_title_key)) == 0)
    {
	proto = g_strdup(gtk_window_get_title( GTK_WINDOW(w)));
	g_object_set_data_full(G_OBJECT(w), ui_title_key, proto,
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

    for (list = gtk_container_get_children(GTK_CONTAINER(menu)) ;
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
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), butt);
    g_signal_connect(G_OBJECT(butt), "activate",
		     G_CALLBACK(callback), calldata);
    gtk_widget_show(butt);

    return butt;
}

GtkWidget *
ui_menu_add_seperator(GtkWidget *menu)
{
    GtkWidget *butt;

    butt = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), butt);
    gtk_widget_show(butt);

    return butt;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gpointer
ui_list_double_click_data(GtkWidget *w, GdkEvent *event, int column )
{
    GtkTreeView *tv = GTK_TREE_VIEW(w);
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
	if (_log.is_enabled(logging::DEBUG))
	{
	    string_var path_str = gtk_tree_path_to_string(path);
	    _log.debug("ui_list_double_click_data: path=\"%s\"\n", path_str.data());
	}
	model = gtk_tree_view_get_model(tv);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, column, &data, -1);
	gtk_tree_path_free(path);

	return data;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
ui_list_set_column_visibility(GtkWidget *w, int col, gboolean vis)
{
    gtk_tree_view_column_set_visible(
	    gtk_tree_view_get_column(GTK_TREE_VIEW(w), col),
	    vis);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static PangoFontDescription *ui_text_font_desc;
static gboolean ui_text_font_dirty = FALSE;

#define UI_TEXT_SCALE_FACTOR    1.1
#define UI_TEXT_DEFAULT_SIZE    10240

void
ui_text_setup(GtkWidget *w)
{
    assert(GTK_IS_TEXT_VIEW(w));

    /*
     * Override the font in the text window: it needs to be
     * fixedwidth so the source aligns properly.
     */
    if (ui_text_font_dirty)
    {
	/* User changed text size */
	g_assert(ui_text_font_desc != 0);
	pango_font_description_free(ui_text_font_desc);
	ui_text_font_desc = 0;
	ui_text_font_dirty = FALSE;
    }
    if (ui_text_font_desc == 0)
    {
	ui_text_font_desc = pango_font_description_from_string("monospace");
	confsection_t *cs = confsection_t::get("general");
	gint size = cs->get_int("text_size", UI_TEXT_DEFAULT_SIZE);
	pango_font_description_set_size(ui_text_font_desc, size);
    }
    gtk_widget_modify_font(w, ui_text_font_desc);

    /* Suppress wrap: it screws up our pretence of being multi-column */
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(w), GTK_WRAP_NONE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
ui_text_font_width(GtkWidget *w)
{
    PangoLayout *layout = gtk_widget_create_pango_layout(w, "W");
    PangoRectangle ink, logical;
    pango_layout_get_pixel_extents(layout, &ink, &logical);
    g_object_unref(layout);
    return logical.width;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
ui_text_adjust_text_size(GtkWidget *w, int dirn)
{
    static const char normal_size_key[] = "ui-text-normal-size";

    PangoFontDescription *font_desc = w->style->font_desc;
    gint size = pango_font_description_get_size(font_desc);

    gint normal_size = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), normal_size_key));
    if (normal_size == 0)
    {
	normal_size = pango_font_description_get_size(ui_text_font_desc);
	g_object_set_data(G_OBJECT(w), normal_size_key,
			  GINT_TO_POINTER(normal_size));
    }

    if (dirn < 0)
	size = (gint)((double)size / UI_TEXT_SCALE_FACTOR + 0.5);
    else if (dirn == 0)
	size = normal_size;
    else
	size = (gint)((double)size * UI_TEXT_SCALE_FACTOR + 0.5);

    font_desc = pango_font_description_copy(font_desc);
    pango_font_description_set_size(font_desc, size);
    gtk_widget_modify_font(w, font_desc);

    confsection_t::get("general")->set_int("text_size", size);
    confsection_t::sync();
    ui_text_font_dirty = TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

ui_text_tag *
ui_text_create_tag(GtkWidget *w, const char *name, GdkColor *fg)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));

    return gtk_text_buffer_create_tag(buffer, name,
		"foreground-gdk",       fg,
		(char *)0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gfloat
ui_text_vscroll_sample(GtkWidget *w)
{
    return GTK_TEXT_VIEW(w)->vadjustment->value;
}

void
ui_text_vscroll_restore(GtkWidget *w, gfloat vs)
{
    GtkTextView *tv = GTK_TEXT_VIEW(w);

    /* Work around rounding bug in gtk 2.0.2 */
    if (vs + tv->vadjustment->page_size + 0.5 > tv->vadjustment->upper)
	vs = tv->vadjustment->upper - tv->vadjustment->page_size - 0.5;

    gtk_adjustment_set_value(tv->vadjustment, vs);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
ui_text_begin(GtkWidget *w)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));

    gtk_text_buffer_set_text(buffer, "", -1);
}

void
ui_text_add(GtkWidget *w, ui_text_tag *tag, const char *str, int len)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
    GtkTextIter end;

    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert_with_tags(buffer, &end, str, len,
				     tag, (char*)0);
}

void
ui_text_end(GtkWidget *w)
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
ui_text_select_lines(GtkWidget *w, unsigned long startline, unsigned long endline)
{
#ifdef HAVE_GTK_TEXT_BUFFER_SELECT_RANGE
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
    GtkTextIter start, end;

    if (startline > 1)
	startline--;        /* workaround gtk bug */

    gtk_text_buffer_get_iter_at_line(buffer, &start, startline);
    gtk_text_buffer_get_iter_at_line(buffer, &end, endline);

    /* this function appeared sometime after 2.0.2 */
    gtk_text_buffer_select_range(buffer, &start, &end);
#else /* !HAVE_GTK_TEXT_BUFFER_SELECT_RANGE */
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
#endif /* !HAVE_GTK_TEXT_BUFFER_SELECT_RANGE */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
ui_text_ensure_visible(GtkWidget *w, unsigned long line)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
    GtkTextIter iter;

    gtk_text_buffer_get_iter_at_line(buffer, &iter, line);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(w), &iter,
				 0.0, FALSE, 0.0, 0.0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


void
ui_text_get_selected_lines(
    GtkWidget *w,
    unsigned long *startp,
    unsigned long *endp)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
    GtkTextIter start_iter, end_iter;

    if (!gtk_text_buffer_get_selection_bounds(buffer, &start_iter, &end_iter))
    {
	if (startp != 0)
	    *startp = 0;
	if (endp != 0)
	    *endp = 0;
    }
    else
    {
	if (startp != 0)
	    *startp = gtk_text_iter_get_line(&start_iter)+1;
	if (endp != 0)
	    *endp = gtk_text_iter_get_line(&end_iter)+1;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
ui_text_get_contents(GtkWidget *w)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
    GtkTextIter start, end;

    gtk_text_buffer_get_bounds(buffer, &start, &end);
    return gtk_text_buffer_get_text(buffer, &start, &end,
				    /*include_hidden_chars*/FALSE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static logging::level_t
log_level(GLogLevelFlags level)
{
    switch (level & G_LOG_LEVEL_MASK)
    {
    case G_LOG_LEVEL_ERROR: return logging::ERROR;
    case G_LOG_LEVEL_CRITICAL: return logging::FATAL;
    case G_LOG_LEVEL_WARNING: return logging::WARNING;
    case G_LOG_LEVEL_MESSAGE: return logging::INFO;
    case G_LOG_LEVEL_INFO: return logging::INFO;
    case G_LOG_LEVEL_DEBUG: return logging::DEBUG;
    default: return logging::INFO;
    }
}

static void
log_func(
    const char *domain,
    GLogLevelFlags level,
    const char *msg,
    gpointer user_data)
{
    _log.message(log_level(level), "%s: %s\n",
	(domain == 0 ? PACKAGE : domain),
	msg);
    if (level & G_LOG_FLAG_FATAL)
	exit(1);
}

void
ui_log_init(void)
{
    static const char * const domains[] =
	{ "GLib", "GLib-GObject", "Gtk", "Gnome", "libglade", /*application*/0 };
    unsigned int i;

    for (i = 0 ; i < sizeof(domains)/sizeof(domains[0]) ; i++)
	g_log_set_handler(domains[i],
		      (GLogLevelFlags)(G_LOG_LEVEL_MASK|G_LOG_FLAG_FATAL),
		      log_func, /*user_data*/0);
    uix_log_init();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
