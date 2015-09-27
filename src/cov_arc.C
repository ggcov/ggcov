/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2005 Greg Banks <gnb@users.sourceforge.net>
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
#include "cov_suppression.H"
#include "estring.H"
#include "filename.h"

CVSID("$Id: cov_arc.C,v 1.12 2010-05-09 05:37:15 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_arc_t::cov_arc_t()
{
}

void
cov_arc_t::attach(cov_block_t *from, cov_block_t *to)
{
    idx_ = from->out_arcs_.length();

    from_ = from;
    from_->out_arcs_.append(this);
    if (!call_)
	from_->out_ninvalid_++;
    else
	from_->out_ncalls_++;

    to_ = to;
    to_->in_arcs_.append(this);
    if (!call_)
	to_->in_ninvalid_++;

    /* inherit any suppression from the origin block */
    suppress(from->suppression_);
}

cov_arc_t::~cov_arc_t()
{
    to_->in_arcs_.remove(this);
    from_->out_arcs_.remove(this);
    /* TODO: ninvalid counts?!?! */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov::status_t
cov_arc_t::status()
{
    if (suppression_)
	return cov::SUPPRESSED;
    else if (count_)
	return cov::COVERED;
    else
	return cov::UNCOVERED;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_arc_t::set_count(count_t count)
{
    assert(!count_valid_);
    count_valid_ = true;
    count_ = count;
    if (!call_)
    {
	from_->out_ninvalid_--;
	to_->in_ninvalid_--;
    }
}

count_t
cov_arc_t::total(const list_t<cov_arc_t> &list)
{
    count_t total = 0;
    list_iterator_t<cov_arc_t> iter;

    for (iter = list.first() ; iter != (cov_arc_t *)0 ; ++iter)
    {
	/* Some of the counts will be invalid, but they are zero,
	   so adding it in also doesn't hurt.  */
	cov_arc_t *a = (*iter);
	if (!a->call_)
	    total += a->count_;
    }
    return total;
}

cov_arc_t *
cov_arc_t::find_invalid(const list_t<cov_arc_t> &list, gboolean may_be_call)
{
    list_iterator_t<cov_arc_t> iter;

    for (iter = list.first() ; iter != (cov_arc_t *)0 ; ++iter)
    {
	cov_arc_t *a = *iter;

	if (!a->count_valid_ && (may_be_call || !a->call_))
	    return a;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_arc_t::suppress(const cov_suppression_t *s)
{
    if (s && !suppression_)
    {
	if (debug_enabled(D_SUPPRESS))
	{
	    string_var fdesc = from_->describe();
	    string_var tdesc = to_->describe();
	    duprintf3("suppressing arc from %s to %s: %s\n",
		      fdesc.data(), tdesc.data(), s->describe());
	}
	suppression_ = s;
    }
}

void
cov_arc_t::take_name(char *name)
{
    assert(name);
    name_ = name;

    from_->suppress(cov_suppression_t::find(name_, cov_suppression_t::BLOCK_CALLS));
    suppress(cov_suppression_t::find(name_, cov_suppression_t::ARC_CALLS));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
