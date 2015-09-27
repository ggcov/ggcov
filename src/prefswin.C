/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2002-2004 Greg Banks <gnb@users.sourceforge.net>
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
#include "prefswin.H"
#include "prefs.H"
#include <libgnomeui/libgnomeui.h>

CVSID("$Id: prefswin.C,v 1.8 2010-05-09 05:37:15 gnb Exp $");

prefswin_t *prefswin_t::instance_ = 0;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

prefswin_t::prefswin_t()
{
    GladeXML *xml;

    assert(instance_ == 0);
    instance_ = this;

    /* load the interface & connect signals */
    xml = ui_load_tree("preferences");

    set_window(glade_xml_get_widget(xml, "preferences"));

    reuse_srcwin_check_ = glade_xml_get_widget(xml,
				    "preferences_general_reuse_srcwin_check");
    reuse_summwin_check_ = glade_xml_get_widget(xml,
				    "preferences_general_reuse_summwin_check");
    color_pickers_[0] = glade_xml_get_widget(xml,
				"preferences_colors_covered_foreground");
    color_pickers_[1] = glade_xml_get_widget(xml,
				"preferences_colors_covered_background");
    color_pickers_[2] = glade_xml_get_widget(xml,
				"preferences_colors_partcovered_foreground");
    color_pickers_[3] = glade_xml_get_widget(xml,
				"preferences_colors_partcovered_background");
    color_pickers_[4] = glade_xml_get_widget(xml,
				"preferences_colors_uncovered_foreground");
    color_pickers_[5] = glade_xml_get_widget(xml,
				"preferences_colors_uncovered_background");
    color_pickers_[6] = glade_xml_get_widget(xml,
				"preferences_colors_uninstrumented_foreground");
    color_pickers_[7] = glade_xml_get_widget(xml,
				"preferences_colors_uninstrumented_background");
    color_pickers_[8] = glade_xml_get_widget(xml,
				"preferences_colors_suppressed_foreground");
    color_pickers_[9] = glade_xml_get_widget(xml,
				"preferences_colors_suppressed_background");
}


prefswin_t::~prefswin_t()
{
    assert(instance_ == this);
    instance_ = 0;
}

prefswin_t *
prefswin_t::instance()
{
    if (instance_ == 0)
	new prefswin_t;
    return instance_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
prefswin_t::update_picker(int i, const GdkColor *col)
{
    gnome_color_picker_set_i16(GNOME_COLOR_PICKER(color_pickers_[i]),
			       col->red, col->green, col->blue, 65535);
}


void
prefswin_t::update()
{
    dprintf0(D_PREFSWIN, "prefswin_t::update\n");

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(reuse_srcwin_check_),
				prefs.reuse_srcwin);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(reuse_summwin_check_),
				prefs.reuse_summwin);

    update_picker(0, &prefs.covered_foreground);
    update_picker(1, &prefs.covered_background);
    update_picker(2, &prefs.partcovered_foreground);
    update_picker(3, &prefs.partcovered_background);
    update_picker(4, &prefs.uncovered_foreground);
    update_picker(5, &prefs.uncovered_background);
    update_picker(6, &prefs.uninstrumented_foreground);
    update_picker(7, &prefs.uninstrumented_background);
    update_picker(8, &prefs.suppressed_foreground);
    update_picker(9, &prefs.suppressed_background);
}

void
prefswin_t::populate()
{
    update();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
prefswin_t::grey_items()
{
    dprintf0(D_PREFSWIN, "prefswin_t::grey_items\n");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
prefswin_t::apply_picker(int i, GdkColor *col)
{
    gushort dummy;

    gnome_color_picker_get_i16(GNOME_COLOR_PICKER(color_pickers_[i]),
			       &col->red, &col->green, &col->blue, &dummy);
}

void
prefswin_t::apply()
{
    dprintf0(D_PREFSWIN, "prefswin_t::apply\n");

    prefs.reuse_srcwin = GTK_TOGGLE_BUTTON(reuse_srcwin_check_)->active;
    prefs.reuse_summwin = GTK_TOGGLE_BUTTON(reuse_summwin_check_)->active;

    apply_picker(0, &prefs.covered_foreground);
    apply_picker(1, &prefs.covered_background);
    apply_picker(2, &prefs.partcovered_foreground);
    apply_picker(3, &prefs.partcovered_background);
    apply_picker(4, &prefs.uncovered_foreground);
    apply_picker(5, &prefs.uncovered_background);
    apply_picker(6, &prefs.uninstrumented_foreground);
    apply_picker(7, &prefs.uninstrumented_background);
    apply_picker(8, &prefs.suppressed_foreground);
    apply_picker(9, &prefs.suppressed_background);

    prefs.save();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
prefswin_t::on_ok_clicked()
{
    apply();
    delete this;
}

GLADE_CALLBACK void
prefswin_t::on_apply_clicked()
{
    apply();
}

GLADE_CALLBACK void
prefswin_t::on_cancel_clicked()
{
    delete this;
}

GLADE_CALLBACK void
on_preferences_activate(GtkWidget *w, gpointer data)
{
    prefswin_t::instance()->show();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
