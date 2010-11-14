/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2004 Greg Banks <gnb@users.sourceforge.net>
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

#include "cov.H"
#include "cov_calliter.H"

CVSID("$Id: cov_callgraph.C,v 1.11 2010-05-09 05:37:15 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callgraph_t::node_t::node_t(const char *nname)
 :  name(nname)
{
}

cov_callgraph_t::node_t::~node_t()
{
#if 0
    all_->remove(name);
    listdelete(out_arcs, arc_t, delete);
    listclear(in_arcs);
#else
    assert(0);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callgraph_t::cov_callgraph_t()
{
    nodes_ = new hashtable_t<const char, node_t>;
}

cov_callgraph_t::~cov_callgraph_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callgraph_t::node_t *
cov_callgraph_t::find_node(const char *name) const
{
    return nodes_->lookup(name);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef struct
{
    void (*func)(cov_callgraph_t::node_t *, void *);
    void *userdata;
} cov_callnode_foreach_rec_t;

static void
cov_callnode_foreach_tramp(
    const char *name,
    cov_callgraph_t::node_t *cn,
    gpointer userdata)
{
    cov_callnode_foreach_rec_t *rec = (cov_callnode_foreach_rec_t *)userdata;

    (*rec->func)(cn, rec->userdata);
}

void
cov_callgraph_t::foreach_node(
    void (*func)(cov_callgraph_t::node_t*, void *userdata),
    void *userdata) const
{
    cov_callnode_foreach_rec_t rec;

    rec.func = func;
    rec.userdata = userdata;
    nodes_->foreach(cov_callnode_foreach_tramp, &rec);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if 0
static gboolean
cov_callgraph_t::delete_one(
    const char *name,
    cov_callgraph_t::node_t *cn,
    gpointer userdata)
{
    delete cn;
    return TRUE;    /* please remove me */
}

static void
cov_callgraph::delete_all(void)
{
    nodes_->foreach_remove(delete_one, 0);
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callgraph_t::arc_t *
cov_callgraph_t::node_t::find_arc_to(cov_callgraph_t::node_t *to) const
{
    GList *iter;

    for (iter = out_arcs ; iter != 0 ; iter = iter->next)
    {
    	arc_t *ca = (arc_t *)iter->data;

	if (ca->to == to)
	    return ca;
    }

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callgraph_t::arc_t::arc_t(
    cov_callgraph_t::node_t *ffrom,
    cov_callgraph_t::node_t *tto)
{
    from = ffrom;
    from->out_arcs = g_list_append(from->out_arcs, this);

    to = tto;
    to->in_arcs = g_list_append(to->in_arcs, this);
}

cov_callgraph_t::arc_t::~arc_t()
{
#if 0
#else
    assert(0);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_callgraph_t::arc_t::add_count(count_t ccount)
{
    count += ccount;
    to->count += ccount;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callgraph_t::node_t *
cov_callgraph_t::add_node(const char *name)
{
    node_t *cn = new node_t(name);
    nodes_->insert(cn->name, cn);
    return cn;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_callgraph_t::add_nodes(cov_file_t *f)
{
    unsigned int fnidx;
    node_t *cn;

    for (fnidx = 0 ; fnidx < f->num_functions() ; fnidx++)
    {
    	cov_function_t *fn = f->nth_function(fnidx);

    	if (fn->is_suppressed())
	    continue;

	if ((cn = find_node(fn->name())) == 0)
	    cn = add_node(fn->name());

	if (cn->function != 0 && cn->function != fn)
	    fprintf(stderr, "Callgraph name collision: %s:%s and %s:%s\n",
	    	fn->file()->name(), fn->name(),
	    	cn->function->file()->name(), cn->function->name());
	if (cn->function == 0)
	    cn->function = fn;
    }
}

void
cov_callgraph_t::add_arcs(cov_file_t *f)
{
    unsigned int fnidx;
    node_t *from;
    node_t *to;
    arc_t *ca;

    for (fnidx = 0 ; fnidx < f->num_functions() ; fnidx++)
    {
    	cov_function_t *fn = f->nth_function(fnidx);

	if (fn->is_suppressed())
	    continue;

	from = find_node(fn->name());
	assert(from != 0);

	cov_call_iterator_t *itr =
	    new cov_function_call_iterator_t(fn);
	while (itr->next())
	{
	    if (itr->name() == 0)
		continue;

	    if ((to = find_node(itr->name())) == 0)
		to = add_node(itr->name());

	    if ((ca = from->find_arc_to(to)) == 0)
		ca = new arc_t(from, to);

	    /* TODO: when new files are opened, old counts are double-counted */
	    ca->add_count(itr->count());
	}
	delete itr;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
