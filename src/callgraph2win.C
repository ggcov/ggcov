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

#include "callgraph2win.H"
#include "cov.H"
#include "prefs.H"

CVSID("$Id: callgraph2win.C,v 1.15 2004-02-23 11:08:43 gnb Exp $");

#define BOX_WIDTH  	    4.0
#define BOX_HEIGHT  	    1.0
#define RANK_GAP 	    2.0
#define FILE_GAP  	    0.1
#define ARROW_SIZE	    0.5
#define HUGE    	    1.0e10

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

callgraph2win_t::node_t::node_t(cov_callnode_t *cn)
{
    callnode_ = cn;
    if (cn->function != 0)
	scope_ = new cov_function_scope_t(cn->function);
    cn->userdata = (void *)this;
}

callgraph2win_t::node_t::~node_t()
{
    delete scope_;
}

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


void
callgraph2win_t::show_box(node_t *n, double ystart, double yend)
{
    cov_callnode_t *cn = n->callnode_;
    GList *iter;
    double ny, yperspread;
    GnomeCanvasPoints *points;
    char *label;
    const GdkColor *fn_color = 0;
    GnomeCanvasGroup *root = gnome_canvas_root(GNOME_CANVAS(canvas_));

    if (n->rect_item_ != 0)
    	return;     /* already been here */

    dprintf5(D_GRAPH2WIN, "callgraph2win_t::show_box: %s:%s y={%g,%g} spread_=%d\n",
    	(cn->function == 0 ? "library" : cn->function->file()->minimal_name()),
	cn->name.data(),
	ystart, yend,
	n->spread_);

    if (cn->function != 0)
    {
	label = g_strdup_printf("%s\n%s\n%4.2f%%",
	    cn->name.data(),
    	    cn->function->file()->minimal_name(),
	    100.0 * n->scope_->get_stats()->blocks_fraction());
	fn_color = backgrounds_by_status[n->scope_->status()];
    }
    else
    {
	label = g_strdup_printf("%s", cn->name.data());
	fn_color = backgrounds_by_status[cov::UNINSTRUMENTED];
    }

    n->x_ = RANK_GAP + n->rank_ * (BOX_WIDTH + RANK_GAP);
    n->y_ = (ystart + yend)/2.0;


    /* adjust the bounds */
    if (n->x_ < bounds_.x1)
    	bounds_.x1 = n->x_;
    if (n->y_ < bounds_.y1)
    	bounds_.y1 = n->y_;
    if (n->x_+BOX_WIDTH > bounds_.x2)
    	bounds_.x2 = n->x_+BOX_WIDTH;
    if (n->y_+BOX_HEIGHT > bounds_.y2)
    	bounds_.y2 = n->y_+BOX_HEIGHT;
    dprintf4(D_GRAPH2WIN|D_VERBOSE,
    	    "callgraph2win_t::show_box {x=%g, y=%g, w=%g, h=%g}\n",
    		n->x_, n->y_, n->x_+BOX_WIDTH, n->y_+BOX_HEIGHT);
    dprintf4(D_GRAPH2WIN|D_VERBOSE,
    	    "callgraph2win_t::show_box: bounds={x1=%g y1=%g x2=%g y2=%g}\n",
    	    	bounds_.x1, bounds_.y1, bounds_.x2, bounds_.y2);

    n->rect_item_ = gnome_canvas_item_new(
    	    	root,
    	    	GNOME_TYPE_CANVAS_RECT,
		"x1", 	    	n->x_,
		"y1",	    	n->y_,
		"x2", 	    	n->x_+BOX_WIDTH,
		"y2",	    	n->y_+BOX_HEIGHT,
		"fill_color_gdk",fn_color,
		"outline_color","black",
#if GTK2
		"width_pixels", 1,
#endif
		0);
    n->text_item_ = gnome_canvas_item_new(
    	    	root,
    	    	GNOME_TYPE_CANVAS_TEXT,
		"text",     	label,
		"font",     	"fixed",
		"fill_color",	"black",
		"x", 	    	n->x_,
		"y",	    	n->y_,
		"clip",     	TRUE,
		"clip_width",	BOX_WIDTH,
		"clip_height",	BOX_HEIGHT,
		"anchor",   	GTK_ANCHOR_NORTH_WEST,
		0);

    g_free(label);


    ny = ystart + FILE_GAP;
    yperspread = (n->spread_ == 0 ? 0.0 : (yend-ystart-FILE_GAP)/n->spread_);
    
    points = gnome_canvas_points_new(2);
    for (iter = cn->out_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *ca = (cov_callarc_t *)iter->data;
	node_t *child = node_t::from_callnode(ca->to);
	double nyend;

    	nyend = ny + child->spread_ * yperspread;
    	show_box(child, ny, nyend);
	ny = nyend;

	points->coords[0] = n->x_+ BOX_WIDTH;
	points->coords[1] = n->y_ + BOX_HEIGHT/2.0;
	points->coords[2] = child->x_,
	points->coords[3] = child->y_ + BOX_HEIGHT/2.0;

    	fn_color = foregrounds_by_status[ca->count ? cov::COVERED :
	    	    	    	    	    	     cov::UNCOVERED];

	gnome_canvas_item_new(
    	    	    root,
    	    	    GNOME_TYPE_CANVAS_LINE,
		    "points", 	    	points,
		    "last_arrowhead",	TRUE,
		    "arrow_shape_a",	ARROW_SIZE,
		    "arrow_shape_b",	ARROW_SIZE,
		    "arrow_shape_c",	ARROW_SIZE/4.0,
		    "fill_color_gdk",	fn_color,
		    0);
    }
    gnome_canvas_points_free(points);
}


void
callgraph2win_t::adjust_rank(callgraph2win_t::node_t *n, int delta)
{
    GList *iter;

    if (n->generation_ == generation_)
    {
    	dprintf1(D_GRAPH2WIN, "callgraph2win_t::adjust_rank: avoiding loop at \"%s\"\n",
	    	    n->callnode_->name.data());
    	return;
    }

    n->generation_ = generation_;
    n->rank_ += delta;
    
    for (iter = n->callnode_->out_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *a = (cov_callarc_t *)iter->data;
	node_t *nto = node_t::from_callnode(a->to);

    	if (nto != 0 &&
	    nto != n &&
	    nto->rank_ >= (n->rank_ - delta))
    	    adjust_rank(nto, delta);
    }
}

/* 
 * Recursive descent from the main() node, building node_t's
 * and calculating of node rank (which can be O(N^2) as we may
 * have to adjust the ranks of subtrees up to the entire tree).
 */
callgraph2win_t::node_t *
callgraph2win_t::build_node(cov_callnode_t *cn, int rank)
{
    node_t *n;
    GList *iter;

    dprintf1(D_GRAPH2WIN, "callgraph2win_t::build_node(\"%s\")\n", cn->name.data());

    if ((n = node_t::from_callnode(cn)) != 0)
    {
    	/* already seen at an earlier rank...demote to this rank */
    	if (n->on_path_)
	{
	    /* loop avoidance */
	    fprintf(stderr, "build_node: avoided loop at %s\n",
	    	    	n->callnode_->name.data());
	    return n;
	}
	++generation_;
	adjust_rank(n, (rank - n->rank_));
    }
    else
    {
    	n = new node_t(cn);
	n->rank_ = rank;
    }
    
    n->on_path_ = TRUE;
    for (iter = cn->out_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *a = (cov_callarc_t *)iter->data;
    	
    	build_node(a->to, rank+1);	
    }
    n->on_path_ = FALSE;
    
    return n;
}

void
callgraph2win_t::add_spread(callgraph2win_t::node_t *n)
{
    GList *iter;
    gboolean first;

    if (n->generation_ == generation_)
    	return; /* been here this time */

    first = (n->spread_ == 0);
    n->spread_++;
    n->generation_ = generation_;

    dprintf3(D_GRAPH2WIN, "callgraph2win_t::add_spread(\"%s\") => spread_=%d generation_=%lu\n",
    	    n->callnode_->name.data(), n->spread_, n->generation_);

    for (iter = n->callnode_->in_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *a = (cov_callarc_t *)iter->data;
	node_t *parent = node_t::from_callnode(a->from);
	
	if (first && parent != 0 && parent->rank_ == n->rank_-1)
    	    add_spread(parent);
    }
}

void
callgraph2win_t::build_ranks(callgraph2win_t::node_t *n, GPtrArray *ranks)
{
    GList *iter;
    GList **rlp;
    int ndirect = 0;

    if (n->file_)
    	return;     /* already been here on another branch in the graph */

    dprintf1(D_GRAPH2WIN, "callgraph2win_t::build_ranks(\"%s\")\n",
    	    	n->callnode_->name.data());

    if (n->rank_ >= ranks->len)
    {
	dprintf1(D_GRAPH2WIN, "callgraph2win_t::build_ranks: expanding ranks to %d\n",
	    	    n->rank_);
    	g_ptr_array_set_size(ranks, n->rank_+1);
    }
    rlp = (GList **)&g_ptr_array_index(ranks, n->rank_);
    *rlp = g_list_append(*rlp, n);
    n->file_ = g_list_length(*rlp);
    if (n->file_ > max_file_)
    	max_file_ = n->file_;
    
    for (iter = n->callnode_->out_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *a = (cov_callarc_t *)iter->data;
	node_t *child = node_t::from_callnode(a->to);

    	build_ranks(child, ranks);
	
	if (child->rank_ == n->rank_+1)
	    ndirect++;
    }

    if (ndirect == 0)
    {
	++generation_;
	add_spread(n);
    }
}


void
callgraph2win_t::populate()
{
    dprintf0(D_GRAPH2WIN, "callgraph2win_t::populate\n");
    main_node_ = build_node(cov_callnode_t::find("main"), 0);
    
    GPtrArray *ranks = g_ptr_array_new();
    max_file_ = 0;

    bounds_.x1 = HUGE;
    bounds_.y1 = HUGE;
    bounds_.x2 = -HUGE;
    bounds_.y2 = -HUGE;

    build_ranks(main_node_, ranks);
    show_box(main_node_, FILE_GAP, (BOX_HEIGHT + FILE_GAP) * (max_file_+1));

    bounds_.x1 -= FILE_GAP;
    bounds_.x2 += FILE_GAP;
    bounds_.y1 -= RANK_GAP;
    bounds_.y2 += RANK_GAP;
    dprintf4(D_GRAPH2WIN, "callgraph2win_t::populate: bounds={x1=%g y1=%g x2=%g y2=%g}\n",
    	    	bounds_.x1, bounds_.y1, bounds_.x2, bounds_.y2);

    gnome_canvas_set_scroll_region(GNOME_CANVAS(canvas_),
				   bounds_.x1, bounds_.y1,
				   bounds_.x2, bounds_.y2+RANK_GAP);
    zoom_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_callgraph2_zoom_in_activate(GtkWidget *w, gpointer data)
{
    callgraph2win_t *cw = callgraph2win_t::from_widget(w);

    dprintf0(D_GRAPH2WIN, "on_callgraph2_zoom_in_activate\n");

    cw->zoom_to(cw->zoom_*2.0);
}


GLADE_CALLBACK void
on_callgraph2_zoom_out_activate(GtkWidget *w, gpointer data)
{
    callgraph2win_t *cw = callgraph2win_t::from_widget(w);

    dprintf0(D_GRAPH2WIN, "on_callgraph2_zoom_out_activate\n");

    cw->zoom_to(cw->zoom_/2.0);
}


GLADE_CALLBACK void
on_callgraph2_show_all_activate(GtkWidget *w, gpointer data)
{
    callgraph2win_t *cw = callgraph2win_t::from_widget(w);

    dprintf0(D_GRAPH2WIN, "on_callgraph2_show_all_activate\n");

    cw->zoom_all();
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
