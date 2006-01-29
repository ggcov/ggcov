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
#include "estring.H"

CVSID("$Id: callgraph_diagram.C,v 1.12 2006-01-29 01:14:44 gnb Exp $");

#define MARGIN		    0.2
#define BOX_WIDTH  	    4.0
#define BOX_HEIGHT  	    1.0
#define RANK_GAP 	    2.0
#define FILE_GAP  	    0.1
#define ARROW_SIZE	    0.5

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

unsigned int
callgraph_diagram_t::node_t::nup()
{
    if (!(flags_ & HAVE_NUP))
    {
    	GList *iter;

	for (iter = callnode_->in_arcs ; iter != 0 ; iter = iter->next)
	{
	    cov_callarc_t *a = (cov_callarc_t *)iter->data;
	    node_t *from = node_t::from_callnode(a->from);

	    if (from != 0 && from->rank_ < rank_)
		nup_++;
	}
	flags_ |= HAVE_NUP;
    }
    return nup_;
}

unsigned int
callgraph_diagram_t::node_t::ndown()
{
    if (!(flags_ & HAVE_NDOWN))
    {
    	GList *iter;

	for (iter = callnode_->out_arcs ; iter != 0 ; iter = iter->next)
	{
	    cov_callarc_t *a = (cov_callarc_t *)iter->data;
	    node_t *to = node_t::from_callnode(a->to);

	    if (to != 0 && to->rank_ > rank_)
		ndown_++;
	}
	flags_ |= HAVE_NDOWN;
    }
    return ndown_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
gboolean
callgraph_diagram_t::node_t::any_self()
{
    GList *iter;

    for (iter = callnode_->out_arcs ; iter != 0 ; iter = iter->next)
    {
	cov_callarc_t *a = (cov_callarc_t *)iter->data;
	node_t *to = node_t::from_callnode(a->to);

	if (to != 0 && to->rank_ == rank_)
	    return TRUE;
    }
    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraph_diagram_t::node_t::push_spread_rootwards(double deltaspread)
{
    GList *iter;
    
    dprintf3(D_DCALLGRAPH|D_VERBOSE, "push_spread_rootwards: node %s spread %g deltaspread %g\n",
	    	callnode_->name.data(), spread_, deltaspread);

    spread_ += deltaspread;
    deltaspread /= nup();

    for (iter = callnode_->in_arcs ; iter != 0 ; iter = iter->next)
    {
	cov_callarc_t *a = (cov_callarc_t *)iter->data;
	node_t *from = node_t::from_callnode(a->from);

	if (from != 0 && from->rank_ < rank_)
	    from->push_spread_rootwards(deltaspread);
    }
}

void
callgraph_diagram_t::node_t::push_spread_leafwards(double deltaspread)
{
    GList *iter;

    dprintf3(D_DCALLGRAPH|D_VERBOSE, "push_spread_leafwards: node %s spread %g deltaspread %g\n",
	    	callnode_->name.data(), spread_, deltaspread);

    spread_ += deltaspread;
    deltaspread /= ndown();

    for (iter = callnode_->out_arcs ; iter != 0 ; iter = iter->next)
    {
	cov_callarc_t *a = (cov_callarc_t *)iter->data;
	node_t *to = node_t::from_callnode(a->to);

	if (to != 0 && to->rank_ > rank_)
    	    to->push_spread_leafwards(deltaspread);
    }
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
callgraph_diagram_t::find_roots_1(cov_callnode_t *cn, void *closure)
{
    callgraph_diagram_t *self = (callgraph_diagram_t *)closure;
    enum { OTHER, DISCONNECTED, ROOT } type = OTHER;
    
    if (!strcmp(cn->name, "main"))
    	type = ROOT;
    else if (cn->in_arcs == 0)
    	type = (cn->out_arcs == 0 ? DISCONNECTED : ROOT);

    switch (type)
    {
    case ROOT:
	dprintf1(D_DCALLGRAPH, "root node \"%s\"\n", cn->name.data());
	self->callnode_roots_.append(cn);
    	break;
    case DISCONNECTED:
	dprintf1(D_DCALLGRAPH, "disconnected node \"%s\"\n", cn->name.data());
	self->disconnected_.append(cn);
	break;
    case OTHER:
    	break;
    }
}

int
callgraph_diagram_t::compare_root_nodes(const cov_callnode_t *a,
    	    	    	    	    	const cov_callnode_t *b)
{
    int r = 0;

    /* a node named "main" is presented first */
    if (!strcmp(a->name, "main"))
    	r = -1;
    else if (!strcmp(b->name, "main"))
    	r = 1;

    /* root nodes with more descendants are presented earlier */	
    if (r == 0)
    	r = g_list_length(b->out_arcs) - g_list_length(a->out_arcs);

    /* as a final resort, root nodes are presented in alphabetical order */	
    if (r == 0)
    	r = strcmp(a->name, b->name);
    
    return r;
}

void
callgraph_diagram_t::check_reached_1(cov_callnode_t *cn, void *userdata)
{
    unsigned int *nunreachedp = (unsigned int *)userdata;
    node_t *n = node_t::from_callnode(cn);

    if (n == 0)
    {
	duprintf1("find_roots: \"%s\" not reached\n",
		  cn->name.data());
	(*nunreachedp)++;
    }
}

void
callgraph_diagram_t::find_roots()
{
    dprintf0(D_DCALLGRAPH, "finding root and disconnected nodes:\n");
    cov_callnode_t::foreach(find_roots_1, this);
    callnode_roots_.sort(compare_root_nodes);
    
    list_iterator_t<cov_callnode_t> iter;
    for (iter = callnode_roots_.first() ; iter != (cov_callnode_t *)0 ; ++iter)
    {
    	cov_callnode_t *cn = (*iter);
	dprintf1(D_DCALLGRAPH, "building nodes for \"%s\"\n", cn->name.data());
    	node_t *n = build_node(cn, 0);
	if (!strcmp(cn->name, "main"))
	    n->flags_ |= node_t::FIXED_RANK;
    	roots_.append(n);
    }

    if (debug_enabled(D_DCALLGRAPH))
    {
	/* check for unreached nodes and whine about them */    
	unsigned int nunreached = 0;

	cov_callnode_t::foreach(check_reached_1, &nunreached);
	if (nunreached)
	    duprintf1("find_roots: %u nodes not reached\n", nunreached);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraph_diagram_t::adjust_rank(callgraph_diagram_t::node_t *n, int delta)
{
    GList *iter;

    dprintf3(D_DCALLGRAPH|D_VERBOSE, "adjust_rank: node %s rank %d delta %d\n",
	    	n->callnode_->name.data(), n->rank_, delta);
    assert(delta > 0);

    if (n->generation_ == generation_)
    {
    	dprintf1(D_DCALLGRAPH, "adjust_rank: avoiding loop at \"%s\"\n",
	    	    n->callnode_->name.data());
    	return;
    }

    n->generation_ = generation_;
    n->rank_ += delta;
    if (n->rank_ > max_rank_)
	max_rank_ = n->rank_;
    
    int minrank = n->rank_+1;
    for (iter = n->callnode_->out_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *a = (cov_callarc_t *)iter->data;
	node_t *nto = node_t::from_callnode(a->to);

    	if (nto != 0 &&
	    nto != n &&
	    nto->rank_ < minrank)
    	    adjust_rank(nto, (minrank - nto->rank_));
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Calculate maximum depth and maximum rank for all nodes.
 */
void
callgraph_diagram_t::prepare_ranks(callgraph_diagram_t::node_t *n, int depth)
{
    GList *iter;
    int ndown = 0;

    for (iter = n->callnode_->out_arcs ; iter != 0 ; iter = iter->next)
    {
	cov_callarc_t *a = (cov_callarc_t *)iter->data;
	node_t *to = node_t::from_callnode(a->to);

	if (to != 0 && to->rank_ > n->rank_)
	{
	    prepare_ranks(to, depth+1);
	    if (!ndown || to->max_depth_ > n->max_depth_)
	    	n->max_depth_ = to->max_depth_;
	    if (!ndown || to->max_rank_ < n->max_rank_)
	    	n->max_rank_ = to->max_rank_;
	    if ((to->flags_ & node_t::FIXED_RANK))
		n->flags_ |= node_t::FIXED_RANK;
	    ndown++;
	}
    }

    if (!ndown)
    {
    	n->max_depth_ = depth;
	n->max_rank_ = max_rank_;
	if (depth == (max_rank_+1))
	    n->flags_ |= node_t::FIXED_RANK;
    }
    else
    {
    	n->max_rank_--;
    }
    
    dprintf4(D_DCALLGRAPH, "prepare_ranks: node %s rank %d max_rank %d max_depth %d\n",
	    n->callnode_->name.data(), n->rank_, n->max_rank_, n->max_depth_);
}

/*
 * Push nodes, especially false roots, downrank as far as they will go
 */
#if 0
void
callgraph_diagram_t::maximise_ranks(callgraph_diagram_t::node_t *n)
{
    GList *iter;

    for (iter = n->callnode_->out_arcs ; iter != 0 ; iter = iter->next)
    {
	cov_callarc_t *a = (cov_callarc_t *)iter->data;
	node_t *to = node_t::from_callnode(a->to);

	if (to != 0 && to->rank_ > n->rank_)
	    maximise_ranks(to);
    }

    int delta = n->max_rank_ - n->rank_;
    if (delta > 0 && n->ndown() >= n->nup())
    {
	fprintf(stderr, "maximise_ranks: pushing node %s\n",
	    	n->callnode_->name.data());
	generation_++;
	adjust_rank(n, delta);
    }
}
#endif

/* Push roots down as far as they can go, to eliminate false roots */
void
callgraph_diagram_t::push_false_root(callgraph_diagram_t::node_t *n)
{
    int delta = n->max_rank_ - n->rank_;
    if (delta > 0)
    {
	fprintf(stderr, "push_false_root: pushing %s\n",
	    	n->callnode_->name.data());
	generation_++;
	adjust_rank(n, delta);
	n->flags_ |= node_t::FIXED_RANK;
    }
}

void
callgraph_diagram_t::balance_ranks(callgraph_diagram_t::node_t *n)
{
#if 0
    list_iterator_t<node_t> iter;
    int nfixed = 0;
    
    for (iter = n->callnode_->out_arcs ; iter != 0 ; iter = iter->next)
    {
	cov_callarc_t *a = (cov_callarc_t *)iter->data;
	node_t *to = node_t::from_callnode(a->to);

	if (to != 0 && to->rank_ > n->rank_ && (to->flags_ & node_t::FIXED_RANK))
	    nfixed++;
    }

    while (nfixed < n->ndown())
    {
    	
    }
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static const char *
rank_plusses(int rank)
{
    static estring buf;

    buf.truncate();
    buf.append_string("    ");
    for ( ; rank ; rank--)
	buf.append_char('+');
    return buf.data();
}

/* 
 * Recursive descent from the roots, building node_t's
 * and calculating of node rank (which can be O(N^2) as we may
 * have to adjust the ranks of subtrees up to the entire tree).
 */
callgraph_diagram_t::node_t *
callgraph_diagram_t::build_node(cov_callnode_t *cn, int rank)
{
    node_t *n;
    GList *iter;

    dprintf2(D_DCALLGRAPH, "%s \"%s\"\n",
	    rank_plusses(rank), cn->name.data());

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
	int delta = rank - (int)n->rank_;
	if (delta > 0)
	    adjust_rank(n, delta);
    }
    else if (cn->function == 0)
    {
    	fprintf(stderr, "build_node: skipping library function %s\n",
	    cn->name.data());
    	return 0;
    }
    else
    {
    	n = new node_t(cn);
	n->rank_ = rank;
	if (rank > max_rank_)
	    max_rank_ = rank;
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
callgraph_diagram_t::build_ranks(callgraph_diagram_t::node_t *n)
{
    GList *iter;
    rank_t *r;

    if (n->file_)
    	return;     /* already been here on another branch in the graph */

    dprintf1(D_DCALLGRAPH, "callgraph_diagram_t::build_ranks(\"%s\")\n",
    	    	n->callnode_->name.data());

    if (n->rank_ >= (int)ranks_->length() || (r = ranks_->nth(n->rank_)) == 0)
    {
	dprintf1(D_DCALLGRAPH, "callgraph_diagram_t::build_ranks: expanding ranks to %d\n",
	    	    n->rank_);
	r = new rank_t();
	ranks_->set(n->rank_, r);
    }
    r->nodes_.append(n);
    n->file_ = r->nodes_.length();
    if (n->file_ > max_file_)
    	max_file_ = n->file_;
    
    for (iter = n->callnode_->out_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *a = (cov_callarc_t *)iter->data;
	node_t *child = node_t::from_callnode(a->to);

    	if (child != 0)
    	    build_ranks(child);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraph_diagram_t::calc_spread(int pass, int rank)
{
    dprintf2(D_DCALLGRAPH, "calc_spread: pass=%d rank=%d\n", pass, rank);

    rank_t *r = ranks_->nth(rank);
    if (r == 0)
    	return;

    list_iterator_t<node_t> iter;
    for (iter = r->nodes_.first() ; iter != (node_t *)0 ; ++iter)
    {
	node_t *n = (*iter);
	double d = 1.0 - n->spread_;
	if (d > EPSILON)
	{
	    dprintf2(D_DCALLGRAPH|D_VERBOSE, "calc_spread: n=%s d=%g\n",
		     n->callnode_->name.data(), d);
	    if (pass == 1)
		n->push_spread_rootwards(d);
	    else
		n->push_spread_leafwards(d);
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
callgraph_diagram_t::any_self_arcs(rank_t *r)
{
    list_iterator_t<node_t> iter;
    for (iter = r->nodes_.first() ; iter != (node_t *)0 ; ++iter)
    {
	node_t *n = (*iter);
	if (n->any_self())
	    return TRUE;
    }
    return FALSE;
}

void
callgraph_diagram_t::assign_geometry()
{
    double height;
    unsigned int i;

    dprintf0(D_DCALLGRAPH, "assign_geometry:\n");

    bounds_.initialise();

    /*
     * Calculate:
     * - total spread for each rank
     * - maximum of rank total spreads
     * - global minimum node spread
     */
    double minspread = HUGE;
    double maxtotspread = 0.0;
    for (i = 0 ; i < ranks_->length() ; i++)
    {
	rank_t *r = ranks_->nth(i);
	list_iterator_t<node_t> iter;

    	if (r == 0)
	    continue;

	r->total_spread_ = 0.0;
	for (iter = r->nodes_.first() ; iter != (node_t *)0 ; ++iter)
	{
	    node_t *n = (*iter);
	    r->total_spread_ += n->spread_;
	    if (n->spread_ < minspread)
		minspread = n->spread_;
	}
	if (r->total_spread_ > maxtotspread)
	    maxtotspread = r->total_spread_;
	dprintf3(D_DCALLGRAPH, "assign_geometry: rank[%u] total_spread=%g minspread=%g\n",
		    i, r->total_spread_, minspread);
    }

    height = (BOX_HEIGHT + FILE_GAP) * (maxtotspread / minspread) - FILE_GAP;
    dprintf3(D_DCALLGRAPH, "assign_geometry: maxtotspread=%g minspread=%g height=%g\n",
	     maxtotspread, minspread, height);

    for (i = 0 ; i < ranks_->length() ; i++)
    {
	rank_t *r = ranks_->nth(i);
	list_iterator_t<node_t> iter;

    	if (r == 0)
	    continue;

	double yperspread = height / r->total_spread_;
	double sy = 0.0;
	for (iter = r->nodes_.first() ; iter != (node_t *)0 ; ++iter)
	{
	    node_t *n = (*iter);
	    cov_callnode_t *cn = n->callnode_;
	    double ey;

	    n->h_ = n->spread_ * yperspread;
	    ey = sy + n->h_;
	    n->x_ = n->rank_ * (BOX_WIDTH + RANK_GAP);
	    n->y_ = (sy + ey)/2.0;
	    sy = ey;

	    dprintf4(D_DCALLGRAPH, "assign_geometry: [%u]%s:%s spread_=%g\n",
		i, 
		(cn->function == 0 ? "library" : cn->function->file()->minimal_name()),
		cn->name.data(),
		n->spread_);

	    bounds_.adjust(n->x_, n->y_ - BOX_HEIGHT/2.0,
			   n->x_ + BOX_WIDTH, n->y_ + BOX_HEIGHT/2.0);
	    dprintf4(D_DCALLGRAPH|D_VERBOSE,
		"assign_geometry: {x1=%g y1=%g x2=%g y2=%g}\n",
		 bounds_.x1, bounds_.y1, bounds_.x2, bounds_.y2);
	}
    }

    /* allow for any arcs within the last rank */
    if (any_self_arcs(ranks_->nth(ranks_->length()-1)))
	bounds_.x2 += RANK_GAP/2.0;

    bounds_.expand(MARGIN, MARGIN);
    dprintf4(D_DCALLGRAPH, "bounds={x1=%g y1=%g x2=%g y2=%g}\n",
    	    	bounds_.x1, bounds_.y1, bounds_.x2, bounds_.y2);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraph_diagram_t::dump_ranks()
{
    unsigned int i;

    duprintf0("dump_ranks:\n");
    for (i = 0 ; i < ranks_->length() ; i++)
    {
	rank_t *r = ranks_->nth(i);
	list_iterator_t<node_t> iter;

	if (r == 0)
	    continue;
	duprintf1("    [%u]:\n", i);

	for (iter = r->nodes_.first() ; iter != (node_t *)0 ; ++iter)
	{
	    node_t *n = (*iter);
	    duprintf2("        %g \"%s\"\n",
		     n->spread_, n->callnode_->name.data());
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
 
void
callgraph_diagram_t::dump_graph_1(cov_callnode_t *cn, void *closure)
{
//    callgraph_diagram_t *self = (callgraph_diagram_t *)closure;
    node_t *n = node_t::from_callnode(cn);
    GList *iter;

    duprintf1("    %s\n", cn->name.data());
    if (n != 0)
    	duprintf3("        rank %d file %d spread %g\n",
	    	  n->rank_, n->file_, n->spread_);

    for (iter = cn->in_arcs ; iter != 0 ; iter = iter->next)
    {
	cov_callarc_t *a = (cov_callarc_t *)iter->data;
	node_t *from = node_t::from_callnode(a->from);
	
	duprintf1("        in %s", a->from->name.data());
	if (from != 0)
	    duprintf1(" rank %d", from->rank_);
	duprintf0("\n");
    }
    for (iter = cn->out_arcs ; iter != 0 ; iter = iter->next)
    {
	cov_callarc_t *a = (cov_callarc_t *)iter->data;
	node_t *to = node_t::from_callnode(a->to);
	
	duprintf1("        out %s", a->to->name.data());
	if (to != 0)
	    duprintf1(" rank %d", to->rank_);
	duprintf0("\n");
    }
}

void
callgraph_diagram_t::dump_graph()
{
    duprintf0("dump_graph:\n");
    cov_callnode_t::foreach(dump_graph_1, this);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraph_diagram_t::prepare()
{
    list_iterator_t<node_t> iter;
    int i;

    dprintf0(D_DCALLGRAPH, "callgraph_diagram_t::prepare\n");

    find_roots();

    for (iter = roots_.first() ; iter != (node_t *)0 ; ++iter)
        prepare_ranks(*iter, 1);
    for (iter = roots_.first() ; iter != (node_t *)0 ; ++iter)
        push_false_root(*iter);
    for (iter = roots_.first() ; iter != (node_t *)0 ; ++iter)
        balance_ranks(*iter);

    ranks_ = new ptrarray_t<rank_t>;
    max_file_ = 0;

    bounds_.initialise();

    for (iter = roots_.first() ; iter != (node_t *)0 ; ++iter)
	build_ranks((*iter));

    for (i = ranks_->length() - 1 ; i >= 0 ; --i)
    	calc_spread(1, i);
    for (i = 0 ; i < (int)ranks_->length() ; i++)
    	calc_spread(2, i);

    if (debug_enabled(D_DCALLGRAPH))
    {
	dump_ranks();
	dump_graph();
    }

    assign_geometry();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraph_diagram_t::show_node(node_t *n, scenegen_t *sg)
{
    cov_callnode_t *cn = n->callnode_;
    GList *iter;
    string_var label;
    unsigned int rgb;

    if ((n->flags_ & node_t::SHOWN))
    	return;     /* already been here */
    n->flags_ |= node_t::SHOWN;

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

    double y = n->y_-BOX_HEIGHT/2.0;
    sg->function(cn->function);
    sg->fill(rgb);
    sg->border(RGB(0,0,0));
    sg->box(n->x_, y, BOX_WIDTH, BOX_HEIGHT);
    sg->fill(RGB(0,0,0));
    sg->textbox(n->x_, y, BOX_WIDTH, BOX_HEIGHT, label);

    for (iter = cn->out_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *ca = (cov_callarc_t *)iter->data;
	node_t *child = node_t::from_callnode(ca->to);

    	if (child == 0)
	    continue;

    	show_node(child, sg);

	sg->arrow_size(ARROW_SIZE);
	sg->fill(fg_rgb_by_status_[ca->count ? cov::COVERED : cov::UNCOVERED]);
	sg->polyline_begin(FALSE);
	if (child->rank_ > n->rank_)
	{
	    /* downrank arc */
	    double px = n->x_+ BOX_WIDTH;
	    double py = n->y_;

	    double cx = child->x_;
	    double cy = child->y_;

	    double ix = (px + cx)/2.0;
	    double iy = 0.1 * py + 0.9 * cy;

	    sg->polyline_point(px, py);
	    sg->polyline_point(ix, iy);
	    sg->polyline_point(cx, cy);
	}
	else if (child->rank_ < n->rank_)
	{
	    /* uprank arc */
	    sg->polyline_point(n->x_, n->y_ - BOX_HEIGHT/4.0);
	    sg->polyline_point(child->x_ + BOX_WIDTH, child->y_ - BOX_HEIGHT/4.0);
	}
	else
	{
	    /* arc within same rank, including to self node */
	    double px = n->x_ + BOX_WIDTH;
	    double py = n->y_;

	    double cx = px;
	    double cy = child->y_ - BOX_HEIGHT/4.0;

	    double ix = px + RANK_GAP/2.0;

	    sg->polyline_point(px, py);
	    sg->polyline_point(ix, py);
	    sg->polyline_point(ix, cy);
	    sg->polyline_point(cx, cy);
	}
	sg->polyline_end(TRUE);
    }

    if (debug_enabled(D_DCALLGRAPH))
    {
	/* draw a red border showing the space allocated to this node */
	sg->nofill();
	sg->border(RGB(0xff,0,0));
	sg->box(n->x_, n->y_ - n->h_/2.0, BOX_WIDTH, n->h_);
    }
}

void
callgraph_diagram_t::render(scenegen_t *sg)
{
    list_iterator_t<node_t> iter;

    if (debug_enabled(D_DCALLGRAPH))
    {
	/* draw a light blue background behind the whole diagram */
	sg->fill(RGB(0xc0,0xc0,0xff));
	sg->noborder();
	sg->box(bounds_.x1+MARGIN,
		bounds_.y1+MARGIN,
	        bounds_.x2-bounds_.x1-2*MARGIN,
	        bounds_.y2-bounds_.y1-2*MARGIN);
    }

    for (iter = roots_.first() ; iter != (node_t *)0 ; ++iter)
    {
	node_t *n = *iter;
	show_node(n, sg);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
callgraph_diagram_t::get_bounds(dbounds_t *db)
{
    *db = bounds_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
