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

CVSID("$Id: ui.c,v 1.1 2001-11-23 09:09:25 gnb Exp $");

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


const char *ui_glade_path = 
#if DEBUG
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
	    fatal("can't find %s in path %s", gladefile, ui_glade_path);
#if DEBUG
	fprintf(stderr, "Loading Glade UI from file \"%s\"\n", filename);
#endif
    }
    xml = glade_xml_new(filename, root);
    if (xml == 0)
    	fatal("can't load Glade UI from file \"%s\"", filename);
    
    /* connect the signals in the interface */
    glade_xml_signal_autoconnect(xml);

    return xml;
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

static int ui_num_registered_windows = 0;

static void
ui_registered_window_destroy(GtkWidget *w, gpointer userdata)
{
    if (--ui_num_registered_windows == 0)
    	gtk_main_quit();
}

void
ui_register_window(GtkWidget *w)
{
    ++ui_num_registered_windows;
    gtk_signal_connect(GTK_OBJECT(w), "destroy", 
    	GTK_SIGNAL_FUNC(ui_registered_window_destroy), 0);
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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
