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

#include "callgraph2win.H"
#include "cov.h"
#include <libgnomeui/libgnomeui.h>

CVSID("$Id: callgraph2win.C,v 1.2 2002-12-22 01:40:37 gnb Exp $");

#define CANVAS_WIDTH	    30.0
#define CANVAS_HEIGHT	    30.0
#define BOX_WIDTH  	    4.0
#define BOX_HEIGHT  	    1.0
#define CHILD_STEP  	    5.0
#define ARROW_SIZE	    0.5
#define HUGE    	    1.0e8

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

callgraph2win_t::callgraph2win_t()
{
    GladeXML *xml;
    
    zoom_ = 1.0;

    /* load the interface & connect signals */
    xml = ui_load_tree("callgraph2");
    
    set_window(glade_xml_get_widget(xml, "callgraph2"));
    
    canvas_ = glade_xml_get_widget(xml, "callgraph2_canvas");
    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(canvas_), zoom_);
    
    ui_register_windows_menu(ui_get_dummy_menu(xml, "callgraph2_windows_dummy"));
}

callgraph2win_t::~callgraph2win_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraph2win_t::zoom_to(double factor)
{
    zoom_ = factor;
    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(canvas_), zoom_);
}

void
callgraph2win_t::zoom_all()
{
    double zoomx, zoomy;

    zoomx = canvas_->allocation.width / (bounds_.x2 - bounds_.x1);
    zoomy = canvas_->allocation.height / (bounds_.y2 - bounds_.y1);
    zoom_ = MIN(zoomx, zoomy);
    fprintf(stderr, "callgraph2win_t::zoom_all: zoomx=%g zoomy=%g zoom=%g\n",
    	    	zoomx, zoomy, zoom_);

    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(canvas_), zoom_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static void
dbounds_update(
    dbounds_t *b,
    double x1,
    double y1,
    double x2,
    double y2)
{
    if (x1 < b->x1)
    	b->x1 = x1;
    if (y1 < b->y1)
    	b->y1 = y1;
    if (x2 > b->x2)
    	b->x2 = x2;
    if (y2 > b->y2)
    	b->y2 = y2;
}


void
callgraph2win_t::add_callnode(
    cov_callnode_t *cn,
    double x,
    double ystart,
    double yend)
{
    GnomeCanvasItem *ctext, *cline;
    GList *iter;
    double y;
    double ystep;
    double nx, ny;
    GnomeCanvasPoints *points;
    cov_stats_t stats;
    char *label;
    const char *fn_color = 0;
    GnomeCanvasGroup *root = gnome_canvas_root(GNOME_CANVAS(canvas_));


    fprintf(stderr, "callgraph2win_t::add_callnode: %s:%s\n",
    	(cn->function == 0 ? "library" : cov_file_minimal_name(cn->function->file)),
	cn->name);
    
    if (cn->function != 0)
    {
    	double lines_pc;
	
	cov_stats_init(&stats);
	cov_function_calc_stats(cn->function, &stats);
    	lines_pc = (stats.lines ? 
	    	    100.0 *(double)stats.lines_executed / (double)stats.lines : 
		    0.0);
	
	label = g_strdup_printf("%s\n%s\n%g%%",
	    cn->name,
    	    cov_file_minimal_name(cn->function->file),
	    lines_pc);
	if (stats.lines_executed == stats.lines)
	    fn_color = "green";
	else if (stats.lines_executed > 0)
	    fn_color = "yellow";
	else
	    fn_color = "red";
    }
    else
    {
	label = g_strdup_printf("%s", cn->name);
    	fn_color = "grey50";
    }

    y = (ystart + yend)/2.0;
    gnome_canvas_item_new(
    	    	root,
    	    	GNOME_TYPE_CANVAS_RECT,
		"x1", 	    	x,
		"y1",	    	y,
		"x2", 	    	x+BOX_WIDTH,
		"y2",	    	y+BOX_HEIGHT,
		"fill_color",	fn_color,
		"outline_color","black",
		0);
    ctext = gnome_canvas_item_new(
    	    	root,
    	    	GNOME_TYPE_CANVAS_TEXT,
		"text",     	label,
		"font",     	"fixed",
		"fill_color",	"black",
		"x", 	    	x,
		"y",	    	y,
		"clip",     	TRUE,
		"clip_width",	BOX_WIDTH,
		"clip_height",	BOX_HEIGHT,
		"anchor",   	GTK_ANCHOR_NORTH_WEST,
		0);
    cn->userdata = ctext;
    g_free(label);

    dbounds_update(&bounds_, x, y, x+BOX_WIDTH, y+BOX_HEIGHT);    
    
    nx = x + CHILD_STEP;
    ny = ystart;
    ystep = (cn->out_arcs == 0 ?
    	    	0.0 :
		(yend-ystart)/(double)g_list_length(cn->out_arcs));
    for (iter = cn->out_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *ca = (cov_callarc_t *)iter->data;

    	assert(ca->from == cn);
	if (ca->to->userdata != 0)
	    continue;

    	points = gnome_canvas_points_new(2);
	points->coords[0] = x+BOX_WIDTH;
	points->coords[1] = y + BOX_HEIGHT/2.0;
	points->coords[2] = nx,
	points->coords[3] = ny + ystep/2.0 + BOX_HEIGHT/2.0;

	ctext = gnome_canvas_item_new(
    	    	    root,
    	    	    GNOME_TYPE_CANVAS_LINE,
		    "points", 	    	points,
		    "last_arrowhead",	TRUE,
		    "arrow_shape_a",	ARROW_SIZE,
		    "arrow_shape_b",	ARROW_SIZE,
		    "arrow_shape_c",	ARROW_SIZE/4.0,
		    "fill_color",   	(ca->count ? "green" : "red"),
		    0);

    	add_callnode(ca->to, nx, ny, ny+ystep);

	ny += ystep;
    }
}

void
callgraph2win_t::populate()
{
#if DEBUG
    fprintf(stderr, "callgraph2win_t::populate\n");
#endif
    main_node_ = cov_callnode_find("main");
    
    gnome_canvas_set_scroll_region(GNOME_CANVAS(canvas_),
    				   0.0, 0.0, CANVAS_WIDTH, CANVAS_HEIGHT);

    bounds_.x1 = HUGE;
    bounds_.y1 = HUGE;
    bounds_.x2 = -HUGE;
    bounds_.y2 = -HUGE;

    add_callnode(main_node_, 0.0, 0.0, CANVAS_HEIGHT);
    fprintf(stderr, "callgraph2win_t::populate: bounds={x1=%g y1=%g x2=%g y2=%g}\n",
    	    	bounds_.x1, bounds_.y1, bounds_.x2, bounds_.y2);
    
    gnome_canvas_set_scroll_region(GNOME_CANVAS(canvas_),
				   bounds_.x1, bounds_.y1,
				   bounds_.x2, bounds_.y2);
    zoom_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_callgraph2_close_activate(GtkWidget *w, gpointer data)
{
    callgraph2win_t *cw = callgraph2win_t::from_widget(w);
    
    assert(cw != 0);
    delete(cw);
}

GLADE_CALLBACK void
on_callgraph2_exit_activate(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}


GLADE_CALLBACK void
on_callgraph2_zoom_in_activate(GtkWidget *w, gpointer data)
{
    callgraph2win_t *cw = callgraph2win_t::from_widget(w);

#if DEBUG
    fprintf(stderr, "on_callgraph2_zoom_in_activate\n");
#endif

    cw->zoom_to(cw->zoom_*2.0);
}


GLADE_CALLBACK void
on_callgraph2_zoom_out_activate(GtkWidget *w, gpointer data)
{
    callgraph2win_t *cw = callgraph2win_t::from_widget(w);

#if DEBUG
    fprintf(stderr, "on_callgraph2_zoom_out_activate\n");
#endif

    cw->zoom_to(cw->zoom_/2.0);
}


GLADE_CALLBACK void
on_callgraph2_show_all_activate(GtkWidget *w, gpointer data)
{
    callgraph2win_t *cw = callgraph2win_t::from_widget(w);

#if DEBUG
    fprintf(stderr, "on_callgraph2_show_all_activate\n");
#endif

    cw->zoom_all();
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
