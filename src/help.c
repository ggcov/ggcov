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

#include "common.h"
#include "estring.h"
#include "ui.h"

CVSID("$Id: help.c,v 1.2 2002-01-17 04:55:06 gnb Exp $");

static GtkWidget *about_window;
static GtkWidget *licence_window;

static const char licence_str[] = 
#include "licence.c"
;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_about_close_clicked(GtkWidget *w, gpointer data)
{
    gtk_widget_hide(about_window);
}

GLADE_CALLBACK void
on_licence_close_clicked(GtkWidget *w, gpointer data)
{
    gtk_widget_hide(licence_window);
}

GLADE_CALLBACK void
on_about_licence_clicked(GtkWidget *w, gpointer data)
{
    if (licence_window == 0)
    {
	GladeXML *xml = ui_load_tree("licence");
    	GtkWidget *licence_text;

	licence_window = glade_xml_get_widget(xml, "licence");

	licence_text = glade_xml_get_widget(xml, "licence_text");
	gtk_text_freeze(GTK_TEXT(licence_text));
	gtk_editable_delete_text(GTK_EDITABLE(licence_text), 0, -1);
	gtk_text_insert(GTK_TEXT(licence_text), /*font*/0,
	    	    	/*fore*/0, /*back*/0,
			licence_str, sizeof(licence_str)-1);
	gtk_text_thaw(GTK_TEXT(licence_text));
    }
    
    gtk_widget_show(licence_window);
}

static const char warranty_str[] = N_("\
GGCov comes with ABSOLUTELY NO WARRANTY.\n\
This is free software and you are welcome\n\
to redistribute it under certain conditions.\n\
For details, press the Licence button.\
");

GLADE_CALLBACK void
on_about_activate(GtkWidget *w, gpointer data)
{
    if (about_window == 0)
    {
	GladeXML *xml = ui_load_tree("about");
    	GtkWidget *about_label;
	char *blurb_proto = 0;
	estring blurb;

	about_window = glade_xml_get_widget(xml, "about");
	ui_window_set_title(about_window, "");

	about_label = glade_xml_get_widget(xml, "about_label");
	gtk_label_get(GTK_LABEL(about_label), &blurb_proto);
	
	estring_init(&blurb);
	estring_append_string(&blurb, blurb_proto);
	estring_replace_all(&blurb, "@VERSION@", VERSION);
	estring_replace_all(&blurb, "@AUTHOR@", "Greg Banks <gnb@alphalink.com.au>");
	estring_replace_all(&blurb, "@WARRANTY@", _(warranty_str));
	gtk_label_set_text(GTK_LABEL(about_label), blurb.data);
	estring_free(&blurb);
    }
    
    gtk_widget_show(about_window);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
