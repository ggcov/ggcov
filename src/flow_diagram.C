/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2006 Greg Banks <gnb@alphalink.com.au>
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

#include "flow_diagram.H"
#include "tok.H"

CVSID("$Id: flow_diagram.C,v 1.1 2006-07-10 10:24:53 gnb Exp $");

#define NODE_WIDTH	6.0
#define HMARGIN		0.5
#define HGAP		6.0
#define LINE_HEIGHT	6.0
#define VMARGIN		0.5
#define VGAP		6.0
#define SLOTGAP		3.0
#define ARROW_SIZE	3.0
#define ARROW_SHAPE	1.0, 1.0, 1.0

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

flow_diagram_t::flow_diagram_t(cov_function_t *fn)
 :  function_(fn)
{
    title_ = g_strdup_printf("Block Diagram %s", fn->name());
}

flow_diagram_t::~flow_diagram_t()
{
    nodes_.delete_all();

    unsigned int i;
    for (i = 0 ; i < num_lines_ ; i++)
	nodes_by_line_[i].remove_all();
    delete[] nodes_by_line_;

    delete nodes_by_bindex_;

    if (arcs_ != 0)
    {
	for (i = 0 ; i < arcs_->length() ; i++)
	    delete arcs_->nth(i);
	delete arcs_;
    }

}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
flow_diagram_t::name()
{
    return "block";
}

const char *
flow_diagram_t::title()
{
    return title_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * First pass: create nodes
 */

unsigned int
flow_diagram_t::generate_nodes()
{
    const cov_location_t *first = function_->get_first_location();
    const cov_location_t *last = function_->get_last_location();

    dprintf0(D_DLEGO, "flow_diagram_t::prepare\n");

    if (first == 0 ||
	first->lineno == 0 ||
        last == 0 ||
	last->lineno == 0 ||
	last->lineno < first->lineno)
    {
	fprintf(stderr, "flow_diagram_t::prepare: can't generate "
			"diagram for function \"%s\"\n",
		    function_->name());
	return 0;
    }

    num_lines_ = last->lineno - first->lineno + 1;
    unsigned int num_nodes = 0;
    nodes_by_line_ = new list_t<node_t>[num_lines_];
    nodes_by_bindex_ = new ptrarray_t<node_t>();
    nodes_by_bindex_->resize(function_->num_blocks());

    unsigned int i;
    for (i = 0 ; i < function_->num_blocks() ; i++)
    {
	cov_block_t *b = function_->nth_block(i);

	node_t *node = new node_t(b);
	node->first_ = node->next_ = node->prev_ = node;
	nodes_.append(node);
	nodes_by_bindex_->set(b->bindex(), node);
	num_nodes++;

	list_iterator_t<cov_location_t> liter;
	for (liter = b->location_iterator() ; liter != (cov_location_t *)0 ; ++liter)
	{
	    cov_location_t *loc = *liter;

	    if (safe_strcmp(loc->filename, first->filename))
		continue;
	    if (loc->lineno < first->lineno ||
		loc->lineno > last->lineno)
		continue;
	    int idx = loc->lineno - first->lineno;
	    if (node->first_idx_ < 0)
	    {
		/* first location for this node */
		node->first_idx_ = node->last_idx_ = idx;
	    }
	    else if (idx == node->last_idx_+1)
	    {
		/* consecutive location for this node, update last_idx_ */
		node->last_idx_ = idx;
	    }
	    else
	    {
		/* nonconsecutive location for this node, split it*/
		node_t *newnode = new node_t(b);
		newnode->first_ = newnode->next_ = node->first_;
		newnode->prev_ = node;
		node->next_ = newnode;
		node->first_->prev_ = newnode;
		nodes_.append(newnode);
		num_nodes++;
		newnode->first_idx_ = newnode->last_idx_ = idx;
		node = newnode;
	    }
	    nodes_by_line_[idx].append(node);
	}

	/* `node' is now the last node in the chain for this block */
    }

    return num_nodes;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Second pass: assign ranks to the nodes
 */

void
flow_diagram_t::assign_ranks()
{
    max_rank_ = -1;

    unsigned int i;
    for (i = 0 ; i < num_lines_ ; i++)
    {
	int lrank = -1;
	list_iterator_t<node_t> niter;
	/* calculate lrank as maximum of ranks for this line */
	for (niter = nodes_by_line_[i].first() ; niter != (node_t *)0 ; ++niter)
	{
	    node_t *node = (*niter)->first_;

	    if (node->rank_ > lrank)
		lrank = node->rank_;
	}
	/* allocate ranks for unranked nodes on this line */
	for (niter = nodes_by_line_[i].first() ; niter != (node_t *)0 ; ++niter)
	{
	    node_t *node = (*niter)->first_;

	    if (node->rank_ < 0)
		node->rank_ = ++lrank;
	}
	/* maintain the global maximum rank */
	if (lrank > max_rank_)
	    max_rank_ = lrank;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Third pass: setup the arcs and sort them in increasing order of
 * length, so shorter arcs are allocated slots closer to the main
 * sequence.  This minimises crossings.
 */

int
flow_diagram_t::arc_t::compare(const arc_t **a1p, const arc_t **a2p)
{
    /* sort by increasing length of slot needed */
    return (*a1p)->slot_needed() - (*a2p)->slot_needed();
}

gboolean
flow_diagram_t::no_intervening_nodes(node_t *from, node_t *to) const
{
    int idx;

    if (to->first_idx_ <= from->last_idx_)
	return FALSE;
    for (idx = from->last_idx_+1 ; idx < to->first_idx_ ; idx++)
    {
	if (nodes_by_line_[idx].head() != 0)
	    return FALSE;
    }
    return TRUE;
}

flow_diagram_t::arc_case_t
flow_diagram_t::classify_arc(node_t *from, node_t *to) const
{
    if (no_intervening_nodes(from, to))
	return AC_DOWN1;
    if (no_intervening_nodes(to, from))
	return AC_UP1;
    if (from->last_idx_ == to->first_idx_ &&
	to->first_->rank_ == from->first_->rank_+1)
	return AC_ACROSS1;
    if (from->last_idx_ < to->first_idx_)
	return AC_DOWN;
    if (from->first_idx_ > to->last_idx_)
	return AC_UP;
    return AC_ACROSS;
}

void
flow_diagram_t::add_arc(cov_arc_t *a, node_t *from, node_t *to)
{
    arc_t *arc = new arc_t();

    arc->arc_ = a;
    arc->from_ = from;
    arc->to_ = to;

    switch (arc->case_ = classify_arc(from, to))
    {
    case AC_DOWN:
	arc->first_ = from->last_idx_;
	arc->last_ = to->first_idx_;
	break;
    case AC_UP:
	arc->first_ = to->last_idx_;
	arc->last_ = from->first_idx_;
	break;
    default:
	/* don't need first_ & last_ because we won't be allocating slots */
	arc->first_ = arc->last_ = 0;
	break;
    }

    arcs_->append(arc);
}

void
flow_diagram_t::generate_arcs()
{
    arcs_ = new ptrarray_t<arc_t>();

    list_iterator_t<node_t> niter;
    for (niter = nodes_.first() ; niter != (node_t *)0 ; ++niter)
    {
	node_t *node = *niter;

	if (!node->have_geometry())
	    continue;

	if (node->first_->prev_ == node)
	{
	    /* last node for this block, show outgoing arcs */

	    list_iterator_t<cov_arc_t> aiter;
	    for (aiter = node->block_->out_arc_iterator() ;
		 aiter != (cov_arc_t *)0 ;
		 ++aiter)
	    {
		cov_arc_t *a = *aiter;
		node_t *tonode = nodes_by_bindex_->nth(a->to()->bindex());

		if (tonode->have_geometry())
		    add_arc(a, node, tonode);
	    }
	}
	else
	{
	    /* non-last node for this block, add a pretend fallthrough arc */
	    add_arc(0, node, node->next_);
	}
    }
    arcs_->sort(arc_t::compare);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Third pass: allocate arc slots
 */

void
flow_diagram_t::slot_distance(node_t *node, int idx, int *distances) const
{
    gboolean side = 0;

    list_iterator_t<node_t> niter;
    for (niter = nodes_by_line_[idx].first() ;
	 niter != (node_t *)0 ;
	 ++niter)
    {
	if (*niter == node)
	    side = 1;
	else
	    distances[side]++;
    }
    dprintf4(D_DLEGO|D_VERBOSE,
	    "slot_distance(node=%p,idx=%d,distances=[%d,%d]\n",
	    node, idx, distances[0], distances[1]);
}

int
flow_diagram_t::choose_side(arc_t *arc) const
{
    int distances[2];
    distances[0] = distances[1] = 0;

    switch (arc->case_)
    {
    case AC_DOWN:
	slot_distance(arc->from_, arc->from_->last_idx_, distances);
	slot_distance(arc->to_, arc->to_->last_idx_, distances);
	break;
    case AC_UP:
	slot_distance(arc->from_, arc->from_->first_idx_, distances);
	slot_distance(arc->to_, arc->to_->last_idx_, distances);
	break;
    default:
	assert(0);
	return -1;
    }
    return (distances[0] <= distances[1] ? 0 : 1);
}

void
flow_diagram_t::allocate_arc_slots()
{
    unsigned int *next_slot[2];
    next_slot[0] = new unsigned int[num_lines_];
    next_slot[1] = new unsigned int[num_lines_];
    num_slots_[0] = 0;
    num_slots_[1] = 0;

    unsigned int i;
    for (i = 0 ; i < arcs_->length() ; i++)
    {
	arc_t *arc = arcs_->nth(i);

	if (arc->slot_needed() == 0)
	    continue;
	int side = choose_side(arc);

	unsigned int slot = 0;
	int idx;
	for (idx = arc->first_+1 ; idx < arc->last_ ; idx++)
	{
	    if (next_slot[side][idx] > slot)
		slot = next_slot[side][idx];
	}
	for (idx = arc->first_+1 ; idx < arc->last_ ; idx++)
	{
	    next_slot[side][idx] = slot+1;
	}
	if (slot >= num_slots_[side])
	    num_slots_[side] = slot+1;

	arc->slot_ = (2*side-1) * (slot + 1);
    }
    delete[] next_slot[0];
    delete[] next_slot[1];

    if (debug_enabled(D_DLEGO|D_VERBOSE))
    {
	duprintf0("flow_diagram_t::arcs_[] = {\n");
	for (i = 0 ; i < arcs_->length() ; i++)
	{
	    arc_t *arc = arcs_->nth(i);
	    duprintf3("flow_diagram_t::    {from=%u,to=%u,slot=%d}\n",
		    arc->from_->block_->bindex(),
		    arc->to_->block_->bindex(),
		    arc->slot_);
	}
	duprintf0("flow_diagram_t::}\n");
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Fourth pass: calculate global geometry and assign geometry to the nodes
 */
void
flow_diagram_t::assign_geometry()
{
    xpos_[0] = 0.0;
    xpos_[1] = xpos_[0] + HMARGIN;
    xpos_[2] = xpos_[1] + (num_slots_[0] > 1 ? SLOTGAP * (num_slots_[0]-1) : 0.0);
    xpos_[3] = xpos_[2] + (num_slots_[0] ? SLOTGAP : 0.0);
    xpos_[4] = xpos_[3] + (NODE_WIDTH + HGAP) * max_rank_ + NODE_WIDTH;
    xpos_[5] = xpos_[4] + (num_slots_[1] ? SLOTGAP : 0.0);
    xpos_[6] = xpos_[5] + (num_slots_[1] > 1 ? SLOTGAP * (num_slots_[1]-1) : 0.0);
    xpos_[7] = xpos_[6] + HMARGIN;

    ypos_[0] = 0.0;
    ypos_[1] = ypos_[0] + VMARGIN;
    ypos_[2] = ypos_[1] + (LINE_HEIGHT + VGAP) * num_lines_;
    ypos_[3] = ypos_[2] + VMARGIN;

    if (debug_enabled(D_DLEGO|D_VERBOSE))
    {
	unsigned int i;
	duprintf0("flow_diagram_t::xpos_[] = {\n");
	for (i = 0 ; i < 8 ; i++)
	    duprintf1("flow_diagram_t::    %g\n", xpos_[i]);
	duprintf0("flow_diagram_t::}\n");

	duprintf0("flow_diagram_t::ypos_[] = {\n");
	for (i = 0 ; i < 4 ; i++)
	    duprintf1("flow_diagram_t::    %g\n", ypos_[i]);
	duprintf0("flow_diagram_t::}\n");
    }

    list_iterator_t<node_t> niter;
    for (niter = nodes_.first() ; niter != (node_t *)0 ; ++niter)
    {
	node_t *node = *niter;

	if (node->first_->rank_ < 0 || node->first_idx_ < 0)
	    continue;
	node->x_ = xpos_[3] + (NODE_WIDTH + HGAP) * node->first_->rank_;
	node->y_ = ypos_[1] + (LINE_HEIGHT + VGAP) * node->first_idx_;
	node->w_ = NODE_WIDTH;
	node->h_ = (LINE_HEIGHT + VGAP) * (node->last_idx_ - node->first_idx_ + 1) - VGAP;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
flow_diagram_t::prepare()
{
    if (!generate_nodes())
	return;
    assign_ranks();
    generate_arcs();
    allocate_arc_slots();
    assign_geometry();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
flow_diagram_t::show_debug_grid(scenegen_t *sg)
{
    unsigned int i;
    unsigned num_ranks = max_rank_+1;

    /* draw a light blue background behind the whole diagram */
    sg->fill(RGB(0xc0,0xc0,0xff));
    sg->noborder();
    sg->box(xpos_[0], ypos_[0],
	    xpos_[7]-xpos_[0], ypos_[3]-ypos_[0]);

    /* draw light magenta rectangles behind the arc slot areas */
    sg->fill(RGB(0xff,0xc0,0xff));
    if (num_slots_[0] > 1)
	sg->box(xpos_[1], ypos_[1],
		xpos_[2]-xpos_[1], ypos_[2]-ypos_[1]);
    if (num_slots_[1] > 1)
	sg->box(xpos_[5], ypos_[1],
		xpos_[6]-xpos_[5], ypos_[2]-ypos_[1]);

    /* draw a dark magenta showing slots */
    sg->fill(RGB(0x80,0x00,0x80));
    sg->border(RGB(0x80,0x00,0x80));

    /* vertical grid lines over left slot area */
    if (num_slots_[0])
    {
	for (i = 0 ; i < num_slots_[0] ; i++)
	{
	    double x = xpos_[1] + SLOTGAP * i;
	    sg->polyline_begin(FALSE);
	    sg->polyline_point(x, ypos_[1]);
	    sg->polyline_point(x, ypos_[2]);
	    sg->polyline_end(FALSE);
	}
    }

    /* vertical grid lines over right slot area */
    if (num_slots_[1])
    {
	for (i = 0 ; i < num_slots_[1] ; i++)
	{
	    double x = xpos_[6] - SLOTGAP * i;
	    sg->polyline_begin(FALSE);
	    sg->polyline_point(x, ypos_[1]);
	    sg->polyline_point(x, ypos_[2]);
	    sg->polyline_end(FALSE);
	}
    }

    /* draw a dark blue showing lines and ranks */
    sg->fill(RGB(0x00,0x00,0x80));
    sg->border(RGB(0x00,0x00,0x80));

    /* horizontal grid lines over the node area */
    for (i = 0 ; i < num_lines_ ; i++)
    {
	double y = ypos_[1] + (LINE_HEIGHT + VGAP) * i;

	sg->polyline_begin(FALSE);
	sg->polyline_point(xpos_[3], y);
	sg->polyline_point(xpos_[4], y);
	sg->polyline_end(FALSE);

	sg->polyline_begin(FALSE);
	sg->polyline_point(xpos_[3], y+LINE_HEIGHT);
	sg->polyline_point(xpos_[4], y+LINE_HEIGHT);
	sg->polyline_end(FALSE);
    }
    /* vertical grid lines over the node area */
    for (i = 0 ; i < num_ranks ; i++)
    {
	double x = xpos_[3] + (NODE_WIDTH + HGAP) * i;

	sg->polyline_begin(FALSE);
	sg->polyline_point(x, ypos_[1]);
	sg->polyline_point(x, ypos_[2]);
	sg->polyline_end(FALSE);

	sg->polyline_begin(FALSE);
	sg->polyline_point(x+NODE_WIDTH, ypos_[1]);
	sg->polyline_point(x+NODE_WIDTH, ypos_[2]);
	sg->polyline_end(FALSE);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

double
flow_diagram_t::slotx(int slot) const
{
    assert(slot != 0);
    if (slot < 0)
    {
	/* left slot area */
	slot = -slot - 1;    /* slot now [0..num_slots_[0]-1] */
	return xpos_[2] - SLOTGAP * slot;
    }
    else
    {
	/* right slot area */
	slot = slot - 1;    /* slot now [0..num_slots_[1]-1] */
	return xpos_[5] + SLOTGAP * slot;
    }
}

double
flow_diagram_t::node_t::slotx(int slot) const
{
    if (slot < 0)
    {
	/* left slot area */
	return x_;
    }
    else if (slot == 0)
    {
	/* straight up or down */
	return x_ + w_/2.0;
    }
    else
    {
	/* right slot area */
	return x_ + w_;
    }
}

void
flow_diagram_t::show_arc(arc_t *arc, scenegen_t *sg)
{
    double vx, vy;

    cov::status_t status = (arc->arc_ == 0 ?
			    arc->from_->block_->status() :
			    arc->arc_->status());
    sg->fill(fg_rgb_by_status_[status]);

    node_t *from = arc->from_;
    node_t *to = arc->to_;

    sg->polyline_begin(FALSE);
    switch (arc->case_)
    {
    case AC_DOWN1:
	/* arc down from one line to the next */
	sg->polyline_point(from->slotx(0), from->y_ + from->h_);
	sg->polyline_point(to->slotx(0), to->y_);
	break;
    case AC_UP1:
	/* arc up from one line to the previous */
	sg->polyline_point(from->slotx(0), from->y_);
	sg->polyline_point(to->slotx(0), to->y_ + to->h_);
	break;
    case AC_ACROSS1:
	/* arc from one rank to the next */
	sg->polyline_point(from->slotx(1), from->y_ + from->h_/2.0);
	sg->polyline_point(to->slotx(-1), to->y_ + to->h_/2.0);
	break;
    case AC_DOWN:
	/* more general arc downwards */
	vx = slotx(arc->slot_);
	dprintf4(D_DLEGO|D_VERBOSE,
	    "arc downwards from=%u to=%u slot=%d vx=%g\n",
	    from->block_->bindex(),
	    to->block_->bindex(),
	    arc->slot_, vx);
	sg->polyline_point(from->slotx(arc->slot_), from->y_ + from->h_);
	sg->polyline_point(vx, from->y_ + from->h_ + VGAP/2.0);
	sg->polyline_point(vx, to->y_ - VGAP/2.0);
	sg->polyline_point(to->slotx(arc->slot_), to->y_);
	break;
    case AC_UP:
	/* more general arc upwards */
	vx = slotx(arc->slot_);
	dprintf4(D_DLEGO|D_VERBOSE,
	    "arc upwards from=%u to=%u slot=%d vx=%g\n",
	    from->block_->bindex(),
	    to->block_->bindex(),
	    arc->slot_, vx);
	sg->polyline_point(from->slotx(arc->slot_), from->y_);
	sg->polyline_point(vx, from->y_ - VGAP/2.0);
	sg->polyline_point(vx, to->y_ + to->h_ + VGAP/2.0);
	sg->polyline_point(to->slotx(arc->slot_), to->y_ + to->h_);
	break;
    case AC_ACROSS:
	/* arc between vertically overlapping nodes */
	vx = (from->slotx(0) + to->slotx(0))/2.0;
	vy = MAX(from->y_ + from->h_, to->y_ + to->h_) + VGAP/2.0;
	sg->polyline_point(from->slotx(0), from->y_ + from->h_);
	sg->polyline_point(vx, vy);
	sg->polyline_point(to->slotx(0), to->y_ + to->h_);
	break;
    }
    sg->polyline_end(TRUE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
flow_diagram_t::show_node(node_t *node, scenegen_t *sg)
{
    if (!node->have_geometry())
	return;

    sg->fill(bg_rgb_by_status_[node->block_->status()]);
    sg->border(RGB(0,0,0));
//     sg->object(node->block_);
    sg->box(node->x_, node->y_, node->w_, node->h_);

#if 0
    char label[64];
    snprintf(label, sizeof(label), "%u\n%llu",
	    node->block_->bindex(),
	    node->block_->count());
    dprintf1(D_DLEGO|D_VERBOSE, "flow_diagram_t::node_t::show label=\"%s\"\n",
		label);
    sg->textbox(node->x_, node->y_, node->w_, node->h_, label);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
flow_diagram_t::render(scenegen_t *sg)
{
    if (debug_enabled(D_DLEGO))
	show_debug_grid(sg);

    /*
     * Draw arcs between nodes
     */
    sg->arrow_size(ARROW_SIZE);
    sg->arrow_shape(ARROW_SHAPE);
    unsigned int i;
    for (i = 0 ; i < arcs_->length() ; i++)
	show_arc(arcs_->nth(i), sg);

    /*
     * Draw nodes last, so their black border
     * is visible even if arrows overlap it
     */
    list_iterator_t<node_t> niter;
    for (niter = nodes_.first() ; niter != (node_t *)0 ; ++niter)
	show_node(*niter, sg);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
flow_diagram_t::get_bounds(dbounds_t *db)
{
    db->x1 = xpos_[0];
    db->y1 = ypos_[0];
    db->x2 = xpos_[7];
    db->y2 = ypos_[3];
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
