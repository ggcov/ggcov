/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>
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
#include "logging.H"

static logging::logger_t &_log = logging::find_logger("cgraph");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callspace_t::cov_callspace_t(const char *name)
 :  name_(name)
{
    nodes_ = new hashtable_t<const char, cov_callnode_t>;
}

cov_callspace_t::~cov_callspace_t()
{
    delete_all();
    delete nodes_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callnode_t *cov_callspace_t::add(cov_callnode_t *cn)
{
    nodes_->insert(cn->name, cn);
    return cn;
}

cov_callnode_t *cov_callspace_t::remove(cov_callnode_t *cn)
{
    nodes_->remove(cn->name);
    return cn;
}

cov_callnode_t *cov_callspace_t::find(const char *name) const
{
    return nodes_->lookup(name);
}

cov_callnode_iter_t cov_callspace_t::first() const
{
    return nodes_->first();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_callspace_t::delete_one(const char *name, cov_callnode_t *cn, gpointer userdata)
{
    delete cn;
    return TRUE;    /* please remove me */
}

void
cov_callspace_t::delete_all(void)
{
    nodes_->foreach_remove(delete_one, 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


cov_callnode_t::cov_callnode_t(const char *nname)
 :  name(nname)
{
}

cov_callnode_t::~cov_callnode_t()
{
    out_arcs.delete_all();
    in_arcs.remove_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callarc_t *
cov_callnode_t::find_arc_to(cov_callnode_t *to) const
{
    for (list_iterator_t<cov_callarc_t> itr = out_arcs.first() ; *itr ; ++itr)
    {
	cov_callarc_t *ca = *itr;

	if (ca->to == to)
	    return ca;
    }

    return 0;
}

int
cov_callnode_t::compare_by_name(const cov_callnode_t **a, const cov_callnode_t **b)
{
    return strcmp((*a)->name, (*b)->name);
}

int
cov_callnode_t::compare_by_name_and_file(const cov_callnode_t *cna, const cov_callnode_t *cnb)
{
    int ret;

    ret = strcmp(cna->name, cnb->name);
    if (ret == 0 && cna->function != 0 && cnb->function != 0)
	ret = strcmp(cna->function->file()->name(), cnb->function->file()->name());
    return ret;
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callarc_t::cov_callarc_t(cov_callnode_t *ffrom, cov_callnode_t *tto)
{
    from = ffrom;
    from->out_arcs.prepend(this);

    to = tto;
    to->in_arcs.append(this);
}

cov_callarc_t::~cov_callarc_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_callarc_t::add_count(count_t ccount)
{
    count += ccount;
    to->count += ccount;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
cov_callarc_t::compare_by_count_and_from(const cov_callarc_t *ca1, const cov_callarc_t *ca2)
{
    int r;

    r = -u64cmp(ca1->count, ca2->count);
    if (!r)
	r = strcmp(ca1->from->name, ca2->from->name);
    return r;
}

int
cov_callarc_t::compare_by_count_and_to(const cov_callarc_t *ca1, const cov_callarc_t *ca2)
{
    int r;

    r = -u64cmp(ca1->count, ca2->count);
    if (!r)
	r = strcmp(ca1->to->name, ca2->to->name);
    return r;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_callgraph_t::cov_callgraph_t()
{
    global_ = new cov_callspace_t("-global-");
    files_ = new hashtable_t<const char, cov_callspace_t>;
    files_->insert(global_->name(), global_);
}

gboolean
cov_callgraph_t::delete_one(const char *name, cov_callspace_t *space, gpointer userdata)
{
    delete space;
    return TRUE;    /* please remove me */
}

cov_callgraph_t::~cov_callgraph_t()
{
    files_->foreach_remove(delete_one, 0);
    delete files_;
}

cov_callspace_iter_t cov_callgraph_t::first() const
{
    return files_->first();
}

cov_callspace_t *
cov_callgraph_t::get_file_space(cov_file_t *f)
{
    cov_callspace_t *space = files_->lookup(f->name());
    if (!space)
    {
	space = new cov_callspace_t(f->name());
	files_->insert(f->name(), space);
    }
    return space;
}

void
cov_callgraph_t::add_nodes(cov_file_t *f)
{
    cov_callspace_t *filespace = get_file_space(f);
    cov_callnode_t *cn;

    for (ptrarray_iterator_t<cov_function_t> fnitr = f->functions().first() ; *fnitr ; ++fnitr)
    {
	cov_function_t *fn = *fnitr;

	if (fn->is_suppressed())
	    continue;

	cov_callspace_t *cs = (fn->linkage() == cov_function_t::LOCAL ?  filespace : global_);

	if ((cn = cs->find(fn->name())) == 0)
	    cn = cs->add(new cov_callnode_t(fn->name()));

	if (cn->function != 0 && cn->function != fn)
	    _log.error("Callgraph name collision: %s:%s and %s:%s\n",
		       fn->file()->name(), fn->name(),
		       cn->function->file()->name(), cn->function->name());
	if (cn->function == 0)
	    cn->function = fn;

	if (!default_node_ || !strcmp(cn->name, "main"))
	    default_node_ = cn;
    }
}

void
cov_callgraph_t::add_arcs(cov_file_t *f)
{
    cov_callspace_t *filespace = get_file_space(f);
    cov_callnode_t *from;
    cov_callnode_t *to;
    cov_callarc_t *ca;

    for (ptrarray_iterator_t<cov_function_t> fnitr = f->functions().first() ; *fnitr ; ++fnitr)
    {
	cov_function_t *fn = *fnitr;

	if (fn->is_suppressed())
	    continue;

	cov_callspace_t *cs = (fn->linkage() == cov_function_t::LOCAL ?  filespace : global_);
	from = cs->find(fn->name());
	assert(from != 0);

	cov_call_iterator_t *itr =
	    new cov_function_call_iterator_t(fn);
	while (itr->next())
	{
	    if (itr->name() == 0)
		continue;

	    to = filespace->find(itr->name());
	    if (!to)
		to = global_->find(itr->name());
	    if (!to)
		to = global_->add(new cov_callnode_t(itr->name()));

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
