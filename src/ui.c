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

#include "ui.h"
#include "estring.h"

CVSID("$Id: ui.c,v 1.3 2001-11-25 07:32:12 gnb Exp $");

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
    char *path_save, *dir, *buf, *file;
    struct stat sb;
    
    buf = path_save = g_strdup(path);
    while ((dir = strtok(buf, ":")) != 0)
    {
    	buf = 0;
	file = g_strconcat(dir, "/", base, 0);

	if (stat(file, &sb) == 0 && S_ISREG(sb.st_mode))
	{
	    g_free(path_save);
	    return file;
	}
	g_free(file);
    }
    
    g_free(path_save);
    return 0;
}

GladeXML *
ui_load_tree(const char *root)
{
    GladeXML *xml = 0;
    /* load & create the interface */
    static char *filename = 0;
    static const char gladefile[] = PACKAGE ".glade";
    
    if (filename == 0)
    {
	filename = find_file(ui_glade_path, gladefile);
	if (filename == 0)
	    fatal("can't find %s in path %s\n", gladefile, ui_glade_path);
#if DEBUG
	fprintf(stderr, "Loading Glade UI from file \"%s\"\n", filename);
#endif
    }
    xml = glade_xml_new(filename, root);
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

static GList *ui_windows;
static GList *ui_windows_menus;

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
	
	ui_add_simple_menu_item(menu, title,
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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char ui_title_key[] = "ui_title_key";

static void
ui_replace_all(estring *e, const char *from, const char *to)
{
    char *p;

    while ((p = strstr(e->data, from)) != 0)
    	estring_replace_string(e, (p - e->data), strlen(from), to);
}

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
    if ((proto = gtk_object_get_data(GTK_OBJECT(w), ui_title_key)) == 0)
    {
    	proto = g_strdup(GTK_WINDOW(w)->title);
	gtk_object_set_data_full(GTK_OBJECT(w), ui_title_key, proto,
	    	    	    	 ui_window_title_destroy);
    }

    estring_init(&title);
    
    estring_append_string(&title, proto);
    ui_replace_all(&title, "@FILE@", filename);
    ui_replace_all(&title, "@VERSION@", VERSION);
    
    gtk_window_set_title(GTK_WINDOW(w), title.data);
        
    estring_free(&title);
    
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
ui_add_simple_menu_item(
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
     
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
