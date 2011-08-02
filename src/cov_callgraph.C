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

hashtable_t<const char, cov_callnode_t> *cov_callnode_t::all_;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callnode_t::cov_callnode_t(const char *nname)
{
    name = nname;

    all_->insert(name, this);
}

cov_callnode_t::~cov_callnode_t()
{
#if 0
    all_->remove(name);
    listdelete(out_arcs, cov_callarc_t, delete);
    listclear(in_arcs);
#else
    assert(0);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_callnode_t::init()
{
    all_ = new hashtable_t<const char, cov_callnode_t>;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callnode_t *
cov_callnode_t::find(const char *nname)
{
    return all_->lookup(nname);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if 0
static gboolean
cov_callnode_t::delete_one(const char *name, cov_callnode_t *cn, gpointer userdata)
{
    delete cn;
    return TRUE;    /* please remove me */
}

static void
cov_callnode_t::delete_all(void)
{
    all_->foreach_remove(delete_one, 0);
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callarc_t *
cov_callnode_t::find_arc_to(cov_callnode_t *to) const
{
    GList *iter;

    for (iter = out_arcs ; iter != 0 ; iter = iter->next)
    {
    	cov_callarc_t *ca = (cov_callarc_t *)iter->data;

	if (ca->to == to)
	    return ca;
    }

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callarc_t::cov_callarc_t(cov_callnode_t *ffrom, cov_callnode_t *tto)
{
    from = ffrom;
    from->out_arcs = g_list_append(from->out_arcs, this);

    to = tto;
    to->in_arcs = g_list_append(to->in_arcs, this);
}

cov_callarc_t::~cov_callarc_t()
{
#if 0
#else
    assert(0);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_callarc_t::add_count(count_t ccount)
{
    count += ccount;
    to->count += ccount;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_add_callnodes(cov_file_t *f)
{
    cov_callnode_t *cn;

    for (ptrarray_iterator_t<cov_function_t> fnitr = f->functions().first() ; *fnitr ; ++fnitr)
    {
	cov_function_t *fn = *fnitr;

    	if (fn->is_suppressed())
	    continue;

	if ((cn = cov_callnode_t::find(fn->name())) == 0)
	    cn = new cov_callnode_t(fn->name());

	if (cn->function != 0 && cn->function != fn)
	    fprintf(stderr, "Callgraph name collision: %s:%s and %s:%s\n",
	    	fn->file()->name(), fn->name(),
	    	cn->function->file()->name(), cn->function->name());
	if (cn->function == 0)
	    cn->function = fn;
    }
}

void
cov_add_callarcs(cov_file_t *f)
{
    cov_callnode_t *from;
    cov_callnode_t *to;
    cov_callarc_t *ca;

    for (ptrarray_iterator_t<cov_function_t> fnitr = f->functions().first() ; *fnitr ; ++fnitr)
    {
	cov_function_t *fn = *fnitr;

	if (fn->is_suppressed())
	    continue;

	from = cov_callnode_t::find(fn->name());
	assert(from != 0);

	cov_call_iterator_t *itr =
	    new cov_function_call_iterator_t(fn);
	while (itr->next())
	{
	    if (itr->name() == 0)
		continue;

	    if ((to = cov_callnode_t::find(itr->name())) == 0)
		to = new cov_callnode_t(itr->name());

	    if ((ca = from->find_arc_to(to)) == 0)
		ca = new cov_callarc_t(from, to);

	    /* TODO: when new files are opened, old counts are double-counted */
	    ca->add_count(itr->count());
	}
	delete itr;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
