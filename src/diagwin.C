/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2005 Greg Banks <gnb@users.sourceforge.net>
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

#include "diagwin.H"
#include "cov.H"
#include "canvas_scenegen.H"
#include "prefs.H"

CVSID("$Id: diagwin.C,v 1.4 2010-05-09 05:37:15 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

diagwin_t::diagwin_t(diagram_t *di)
{
    GladeXML *xml;
    
    zoom_ = 1.0;
    diagram_ = di;

    /* load the interface & connect signals */
    xml = ui_load_tree("diag");
    
    set_window(glade_xml_get_widget(xml, "diag"));
    set_title(diagram_->title());
    
    canvas_ = glade_xml_get_widget(xml, "diag_canvas");
    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(canvas_), zoom_);
    
    ui_register_windows_menu(ui_get_dummy_menu(xml, "diag_windows_dummy"));
}

diagwin_t::~diagwin_t()
{
    delete diagram_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
diagwin_t::zoom_to(double factor)
{
    zoom_ = factor;
    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(canvas_), zoom_);
}

void
diagwin_t::zoom_all()
{
    double zoomx, zoomy;
    dbounds_t bounds;

    diagram_->get_bounds(&bounds);

    zoomx = canvas_->allocation.width / bounds.width();
    zoomy = canvas_->allocation.height / bounds.height();
    zoom_ = MIN(zoomx, zoomy);
    dprintf3(D_DIAGWIN, "diagwin_t::zoom_all: zoomx=%g zoomy=%g zoom=%g\n",
    	    	zoomx, zoomy, zoom_);

    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(canvas_), zoom_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define GDK_TO_RGB(gdkcol) \
	RGB(((gdkcol)->red>>8), ((gdkcol)->green>>8), ((gdkcol)->blue>>8))

static void
set_diagram_colors(diagram_t *di)
{
    int i;

    for (i = 0 ; i < cov::NUM_STATUS ; i++)
    {
	di->set_fg((cov::status_t)i, GDK_TO_RGB(foregrounds_by_status[i]));
	di->set_bg((cov::status_t)i, GDK_TO_RGB(backgrounds_by_status[i]));
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
diagwin_t::populate()
{
    GnomeCanvasGroup *root = gnome_canvas_root(GNOME_CANVAS(canvas_));

    dprintf0(D_DIAGWIN, "diagwin_t::populate\n");

    while (root->item_list != 0)
    {
    	gtk_object_destroy(GTK_OBJECT(root->item_list->data));
    }

    set_diagram_colors(diagram_);
    diagram_->prepare();

    scenegen_t *sg = new canvas_scenegen_t(GNOME_CANVAS(canvas_));
    diagram_->render(sg);
    delete sg;

    /* setup the canvas to show the whole diagram */
    dbounds_t bounds;
    diagram_->get_bounds(&bounds);
    gnome_canvas_set_scroll_region(GNOME_CANVAS(canvas_),
				   bounds.x1, bounds.y1,
				   bounds.x2, bounds.y2);
    zoom_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
diagwin_t::name() const
{
    return diagram_->name();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
diagwin_t::on_zoom_in_activate()
{
    dprintf0(D_DIAGWIN, "diagwin_t::on_zoom_in_activate\n");
    zoom_to(zoom_*2.0);
}


GLADE_CALLBACK void
diagwin_t::on_zoom_out_activate()
{
    dprintf0(D_DIAGWIN, "diagwin_t::on_zoom_out_activate\n");
    zoom_to(zoom_/2.0);
}


GLADE_CALLBACK void
diagwin_t::on_show_all_activate()
{
    dprintf0(D_DIAGWIN, "diagwin_t::on_show_all_activate\n");
    zoom_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
