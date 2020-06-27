/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2002-2020 Greg Banks <gnb@fastmail.fm>
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
#include "estring.H"
#include "ui.h"

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
	GtkWidget *text;    /* GtkText in gtk1.2, GtkTextView in gtk2.0 */

	licence_window = glade_xml_get_widget(xml, "licence");
	text = glade_xml_get_widget(xml, "licence_text");

	ui_text_setup(text);
	ui_text_begin(text);
	ui_text_add(text, /*tag*/0, licence_str, sizeof(licence_str)-1);
	ui_text_end(text);
	ui_text_ensure_visible(text, 1);    /* show the start of the text */
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

	about_window = glade_xml_get_widget(xml, "about");
	ui_window_set_title(about_window, "");

	about_label = glade_xml_get_widget(xml, "about_label");
	gtk_label_get(GTK_LABEL(about_label), &blurb_proto);

	estring blurb;
	blurb.append_string(blurb_proto);
	blurb.replace_all("@VERSION@", VERSION);
	blurb.replace_all("@AUTHOR@", "Greg Banks <gnb@users.sourceforge.net>");
	blurb.replace_all("@WARRANTY@", _(warranty_str));
	gtk_label_set_text(GTK_LABEL(about_label), blurb.data());
    }

    gtk_widget_show(about_window);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
