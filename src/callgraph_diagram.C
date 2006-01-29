/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2005 Greg Banks <gnb@alphalink.com.au>
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

#include "callgraph_diagram.H"
#include "tok.H"

CVSID("$Id: callgraph_diagram.C,v 1.2 2006-01-29 00:32:35 gnb Exp $");

#define BOX_WIDTH  	    4.0
#define BOX_HEIGHT  	    1.0
#define RANK_GAP 	    2.0
#define FILE_GAP  	    0.1
#define ARROW_SIZE	    0.5
#define HUGE    	    1.0e10

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

callgraph_diagram_t::node_t::node_t(cov_callnode_t *cn)
{
    callnode_ = cn;
    if (cn->function != 0)
	scope_ = new cov_function_scope_t(cn->function);
    cn->userdata = (void *)this;
}

callgraph_diagram_t::node_t::~node_t()
{
    delete scope_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

callgraph_diagram_t::callgraph_diagram_t()
{
}

callgraph_diagram_t::~callgraph_diagram_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
callgraph_diagram_t::name()
{
    return "callgraph";
}

const char *
callgraph_diagram_t::title()
{
    return "Call Graph";
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraph_diagram_t::adjust_rank(callgraph_diagram_t::node_t *n, int delta)
{
    GList *iter;

    if (n->generation_ == generation_)
    {
    	dprintf1(D_GRAPH2WIN, "callgraph_diagram_t::adjust_rank: avoiding loop at \"%s\"\n",
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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* 
 * Recursive descent from the main() node, building node_t's
 * and calculating of node rank (which can be O(N^2) as we may
 * have to adjust the ranks of subtrees up to the entire tree).
 */
callgraph_diagram_t::node_t *
callgraph_diagram_t::build_node(cov_callnode_t *cn, int rank)
{
    node_t *n;
    GList *iter;

    dprintf1(D_GRAPH2WIN, "callgraph_diagram_t::build_node(\"%s\")\n", cn->name.data());

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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraph_diagram_t::add_spread(callgraph_diagram_t::node_t *n)
{
    GList *iter;
    gboolean first;

    if (n->generation_ == generation_)
    	return; /* been here this time */

    first = (n->spread_ == 0);
    n->spread_++;
    n->generation_ = generation_;

    dprintf3(D_GRAPH2WIN, "callgraph_diagram_t::add_spread(\"%s\") => spread_=%d generation_=%lu\n",
    	    n->callnode_->name.data(), n->spread_, n->generation_);

    for (iter = n->callnode_->in_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *a = (cov_callarc_t *)iter->data;
	node_t *parent = node_t::from_callnode(a->from);
	
	if (first && parent != 0 && parent->rank_ == n->rank_-1)
    	    add_spread(parent);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraph_diagram_t::build_ranks(callgraph_diagram_t::node_t *n)
{
    GList *iter;
    rank_t *r;
    int ndirect = 0;

    if (n->file_)
    	return;     /* already been here on another branch in the graph */

    dprintf1(D_GRAPH2WIN, "callgraph_diagram_t::build_ranks(\"%s\")\n",
    	    	n->callnode_->name.data());

    if (n->rank_ >= ranks_->length())
    {
	dprintf1(D_GRAPH2WIN, "callgraph_diagram_t::build_ranks: expanding ranks to %d\n",
	    	    n->rank_);
	r = new rank_t();
	ranks_->set(n->rank_, r);
    }
    else
    {
	r = ranks_->nth(n->rank_);
    }
    r->nodes_.append(n);
    n->file_ = r->nodes_.length();
    if (n->file_ > max_file_)
    	max_file_ = n->file_;
    
    for (iter = n->callnode_->out_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *a = (cov_callarc_t *)iter->data;
	node_t *child = node_t::from_callnode(a->to);

    	build_ranks(child);
	
	if (child->rank_ == n->rank_+1)
	    ndirect++;
    }

    if (ndirect == 0)
    {
	++generation_;
	add_spread(n);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraph_diagram_t::assign_geometry(node_t *n, double ystart, double yend)
{
    cov_callnode_t *cn = n->callnode_;
    GList *iter;
    double ny, yperspread;

    if (n->have_geom_)
    	return;     /* already been here */
    n->have_geom_ = TRUE;

    dprintf5(D_GRAPH2WIN, "callgraph_diagram_t::assign_geometry: %s:%s y={%g,%g} spread_=%d\n",
    	(cn->function == 0 ? "library" : cn->function->file()->minimal_name()),
	cn->name.data(),
	ystart, yend,
	n->spread_);

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
    	    "callgraph_diagram_t::assign_geometry {x=%g, y=%g, w=%g, h=%g}\n",
    		n->x_, n->y_, n->x_+BOX_WIDTH, n->y_+BOX_HEIGHT);
    dprintf4(D_GRAPH2WIN|D_VERBOSE,
    	    "callgraph_diagram_t::assign_geometry: bounds={x1=%g y1=%g x2=%g y2=%g}\n",
    	    	bounds_.x1, bounds_.y1, bounds_.x2, bounds_.y2);


    ny = ystart + FILE_GAP;
    yperspread = (n->spread_ == 0 ? 0.0 : (yend-ystart-FILE_GAP)/n->spread_);
    
    for (iter = cn->out_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *ca = (cov_callarc_t *)iter->data;
	node_t *child = node_t::from_callnode(ca->to);
	double nyend;

    	nyend = ny + child->spread_ * yperspread;
    	assign_geometry(child, ny, nyend);
	ny = nyend;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraph_diagram_t::prepare()
{
    dprintf0(D_LEGOWIN, "callgraph_diagram_t::prepare\n");

    main_node_ = build_node(cov_callnode_t::find("main"), 0);

    ranks_ = new ptrarray_t<rank_t>;
    max_file_ = 0;

    bounds_.x1 = HUGE;
    bounds_.y1 = HUGE;
    bounds_.x2 = -HUGE;
    bounds_.y2 = -HUGE;

    build_ranks(main_node_);
    assign_geometry(main_node_,
		    FILE_GAP,
		    (BOX_HEIGHT + FILE_GAP) * (max_file_+1));

    bounds_.x1 -= FILE_GAP;
    bounds_.x2 += FILE_GAP;
    bounds_.y1 -= RANK_GAP;
    bounds_.y2 += RANK_GAP;
    dprintf4(D_GRAPH2WIN, "callgraph_diagram_t::populate: bounds={x1=%g y1=%g x2=%g y2=%g}\n",
    	    	bounds_.x1, bounds_.y1, bounds_.x2, bounds_.y2);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraph_diagram_t::show_node(node_t *n, scenegen_t *sg)
{
    cov_callnode_t *cn = n->callnode_;
    GList *iter;
    string_var label;
    unsigned int rgb;

    if (n->shown_)
    	return;     /* already been here */
    n->shown_ = TRUE;

    if (cn->function != 0)
    {
	label = g_strdup_printf("%s\n%s\n%4.2f%%",
	    cn->name.data(),
    	    cn->function->file()->minimal_name(),
	    100.0 * n->scope_->get_stats()->blocks_fraction());
	rgb = bg_rgb_by_status_[n->scope_->status()];
    }
    else
    {
	label = g_strdup_printf("%s", cn->name.data());
	rgb = bg_rgb_by_status_[cov::UNINSTRUMENTED];
    }

    sg->function(cn->function);
    sg->fill(rgb);
    sg->border(RGB(0,0,0));
    sg->box(n->x_, n->y_, BOX_WIDTH, BOX_HEIGHT);
    sg->fill(RGB(0,0,0));
    sg->textbox(n->x_, n->y_, BOX_WIDTH, BOX_HEIGHT, label);

    for (iter = cn->out_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *ca = (cov_callarc_t *)iter->data;
	node_t *child = node_t::from_callnode(ca->to);

    	show_node(child, sg);

	sg->arrow_size(ARROW_SIZE);
	sg->fill(fg_rgb_by_status_[ca->count ? cov::COVERED : cov::UNCOVERED]);
	sg->polyline_begin(FALSE);
	sg->polyline_point(n->x_+ BOX_WIDTH, n->y_ + BOX_HEIGHT/2.0);
	sg->polyline_point(child->x_, child->y_ + BOX_HEIGHT/2.0);
	sg->polyline_end(TRUE);
    }
}

void
callgraph_diagram_t::render(scenegen_t *sg)
{
    show_node(main_node_, sg);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraph_diagram_t::get_bounds(dbounds_t *db)
{
    *db = bounds_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
