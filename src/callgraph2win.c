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

#include "callgraph2win.h"
#include "sourcewin.h"
#include "cov.h"
#include "estring.h"
#include <libgnomeui/libgnomeui.h>

CVSID("$Id: callgraph2win.c,v 1.1 2002-12-12 00:11:01 gnb Exp $");

static const char callgraph2win_window_key[] = "callgraph2win_key";

static void callgraph2win_populate(callgraph2win_t*);

#define CANVAS_WIDTH	    30.0
#define CANVAS_HEIGHT	    30.0
#define BOX_WIDTH  	    4.0
#define BOX_HEIGHT  	    1.0
#define CHILD_STEP  	    5.0
#define ARROW_SIZE	    0.5
#define HUGE    	    1.0e8

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

callgraph2win_t *
callgraph2win_new(void)
{
    callgraph2win_t *cw;
    GladeXML *xml;
    
    cw = new(callgraph2win_t);
    cw->zoom = 1.0;

    /* load the interface & connect signals */
    xml = ui_load_tree("callgraph2");
    
    cw->window = glade_xml_get_widget(xml, "callgraph2");
    ui_register_window(cw->window);
    
    cw->canvas = glade_xml_get_widget(xml, "callgraph2_canvas");
    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(cw->canvas), cw->zoom);
    
    ui_register_windows_menu(ui_get_dummy_menu(xml, "callgraph2_windows_dummy"));

    gtk_object_set_data(GTK_OBJECT(cw->window), callgraph2win_window_key, cw);
    
    callgraph2win_populate(cw);
    
    return cw;
}

void
callgraph2win_delete(callgraph2win_t *cw)
{
    /* JIC of strange gui stuff */
    if (cw->deleting)
    	return;
    cw->deleting = TRUE;
    
    gtk_widget_destroy(cw->window);
    g_free(cw);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static callgraph2win_t *
callgraph2win_from_widget(GtkWidget *w)
{
    w = ui_get_window(w);
    return (w == 0 ? 0 : gtk_object_get_data(GTK_OBJECT(w), callgraph2win_window_key));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
callgraph2win_zoom_to(callgraph2win_t *cw, double factor)
{
    cw->zoom = factor;
    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(cw->canvas), cw->zoom);
}

static void
callgraph2win_zoom_all(callgraph2win_t *cw)
{
    double zoomx, zoomy;

    zoomx = cw->canvas->allocation.width / (cw->bounds.x2 - cw->bounds.x1);
    zoomy = cw->canvas->allocation.height / (cw->bounds.y2 - cw->bounds.y1);
    cw->zoom = MIN(zoomx, zoomy);
    fprintf(stderr, "callgraph2win_zoom_all: zoomx=%g zoomy=%g zoom=%g\n",
    	    	zoomx, zoomy, cw->zoom);

    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(cw->canvas), cw->zoom);
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


static void
add_callnode(
    callgraph2win_t *cw,
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
    GnomeCanvasGroup *root = gnome_canvas_root(GNOME_CANVAS(cw->canvas));


    fprintf(stderr, "add_callnode: %s:%s\n",
    	(cn->function == 0 ? "library" : cn->function->file->name),
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
    	    cn->function->file->name,
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

    dbounds_update(&cw->bounds, x, y, x+BOX_WIDTH, y+BOX_HEIGHT);    
    
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

    	add_callnode(cw, ca->to, nx, ny, ny+ystep);

	ny += ystep;
    }
}

static void
callgraph2win_populate(callgraph2win_t *cw)
{
#if DEBUG
    fprintf(stderr, "callgraph2win_populate\n");
#endif
    cw->main_node = cov_callnode_find("main");
    
    gnome_canvas_set_scroll_region(GNOME_CANVAS(cw->canvas),
    				   0.0, 0.0, CANVAS_WIDTH, CANVAS_HEIGHT);

    cw->bounds.x1 = HUGE;
    cw->bounds.y1 = HUGE;
    cw->bounds.x2 = -HUGE;
    cw->bounds.y2 = -HUGE;

    add_callnode(cw, cw->main_node, 0.0, 0.0, CANVAS_HEIGHT);
    fprintf(stderr, "callgraph2win_populate: bounds={x1=%g y1=%g x2=%g y2=%g}\n",
    	    	cw->bounds.x1, cw->bounds.y1, cw->bounds.x2, cw->bounds.y2);
    
    gnome_canvas_set_scroll_region(GNOME_CANVAS(cw->canvas),
				   cw->bounds.x1, cw->bounds.y1,
				   cw->bounds.x2, cw->bounds.y2);
    callgraph2win_zoom_all(cw);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if 0
static void
callgraph2win_update(callgraph2win_t *cw)
{
    cov_callnode_t *cn = ui_combo_get_current_data(
	    	    	    	GTK_COMBO(cw->function_combo));
    
#if DEBUG
    fprintf(stderr, "callgraph2win_update\n");
#endif
    gtk_widget_set_sensitive(cw->function_view, (cn->function != 0));
    ui_window_set_title(cw->window, cn->name);

    callgraph2win_update_clist(cw->ancestors_clist, cn->in_arcs, TRUE);
    callgraph2win_update_clist(cw->descendants_clist, cn->out_arcs, FALSE);
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: this needs to be common code too */
static void
callgraph2win_show_source(
    const char *filename,
    unsigned long startline,
    unsigned long endline)
{
    sourcewin_t *srcw = sourcewin_new();
    
    sourcewin_set_filename(srcw, filename);
    if (startline > 0)
    {
	sourcewin_ensure_visible(srcw, startline);
	sourcewin_select_region(srcw, startline, endline);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_callgraph2_close_activate(GtkWidget *w, gpointer data)
{
    callgraph2win_t *cw = callgraph2win_from_widget(w);
    
    if (cw != 0)
	callgraph2win_delete(cw);
}

GLADE_CALLBACK void
on_callgraph2_exit_activate(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}


GLADE_CALLBACK void
on_callgraph2_zoom_in_activate(GtkWidget *w, gpointer data)
{
    callgraph2win_t *cw = callgraph2win_from_widget(w);

#if DEBUG
    fprintf(stderr, "on_callgraph2_zoom_in_activate\n");
#endif

    callgraph2win_zoom_to(cw, cw->zoom*2.0);
}


GLADE_CALLBACK void
on_callgraph2_zoom_out_activate(GtkWidget *w, gpointer data)
{
    callgraph2win_t *cw = callgraph2win_from_widget(w);

#if DEBUG
    fprintf(stderr, "on_callgraph2_zoom_out_activate\n");
#endif

    callgraph2win_zoom_to(cw, cw->zoom/2.0);
}


GLADE_CALLBACK void
on_callgraph2_show_all_activate(GtkWidget *w, gpointer data)
{
    callgraph2win_t *cw = callgraph2win_from_widget(w);

#if DEBUG
    fprintf(stderr, "on_callgraph2_show_all_activate\n");
#endif

    callgraph2win_zoom_all(cw);
}


#if 0
GLADE_CALLBACK void
on_callgraph2_function_view_clicked(GtkWidget *w, gpointer data)
{
    callgraph2win_t *cw = callgraph2win_from_widget(w);
    cov_callnode_t *cn = ui_combo_get_current_data(
	    	    	    	GTK_COMBO(cw->function_combo));
    const cov_location_t *start, *end;

    start = cov_function_get_first_location(cn->function);
    end = cov_function_get_last_location(cn->function);

    callgraph2win_show_source(start->filename, start->lineno, end->lineno);
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
