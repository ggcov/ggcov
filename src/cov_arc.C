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
#include "estring.H"
#include "filename.h"

CVSID("$Id: cov_arc.C,v 1.12 2010-05-09 05:37:15 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_arc_t::cov_arc_t()
{
    counter_ = cov_project_t::current()->next_counter();
}

void
cov_arc_t::attach(cov_block_t *from, cov_block_t *to)
{
    idx_ = from->out_arcs_.length();
    
    from_ = from;
    from_->out_arcs_.append(this);
    if (call_)
    	from_->out_ncalls_++;

    to_ = to;
    to_->in_arcs_.append(this);
}

cov_arc_t::~cov_arc_t()
{
    to_->in_arcs_.remove(this);
    from_->out_arcs_.remove(this);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

boolean
cov_arc_t::is_suppressed() const
{
    const cov_location_t *loc;
    cov_line_t *ln;

    /* externally suppressed, e.g. function suppressed by name */
    if (suppressed_)
    	return TRUE;
    
    /* originating line suppressed, e.g. by ifdef */
    return ((loc = get_from_location()) != 0 &&
	    (ln = cov_line_t::find(loc)) != 0 &&
	    ln->is_suppressed());
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_arc_t::set_count(count_t count)
{
    assert(cov_project_t::current()->get_counter(counter_) == COV_COUNT_INVALID);
    cov_project_t::current()->set_counter(counter_, count);
}

count_t
cov_arc_t::total(const list_t<cov_arc_t> &list)
{
    count_t total = 0, c;
    list_iterator_t<cov_arc_t> iter;

    for (iter = list.first() ; iter != (cov_arc_t *)0 ; ++iter)
    {
	cov_arc_t *a = (*iter);
    	if (!a->call_ && (c = a->count()) != COV_COUNT_INVALID)
	    total += c;
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
	
	if (a->count() == COV_COUNT_INVALID && (may_be_call || !a->call_))
	    return a;
    }
    return 0;
}

unsigned int
cov_arc_t::count_invalid(const list_t<cov_arc_t> &list)
{
    list_iterator_t<cov_arc_t> iter;
    unsigned int total = 0;

    for (iter = list.first() ; iter != (cov_arc_t *)0 ; ++iter)
    {
    	cov_arc_t *a = *iter;

	if (a->count() == COV_COUNT_INVALID && !a->call_)
	    total++;
    }

    return total;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_arc_t::finalise()
{
    static const char * const ignored[] =
    {
	"__cxa_allocate_exception", /* gcc 3.4 exception handling */
	"__cxa_begin_catch",   	    /* gcc 3.4 exception handling */
	"__cxa_call_unexpected",    /* gcc 3.4 exception handling */
	"__cxa_end_catch",   	    /* gcc 3.4 exception handling */
	"__cxa_throw",   	    /* gcc 3.4 exception handling */
	"_Unwind_Resume",   	    /* gcc 3.4 exception handling */
    	0
    };
    const char * const *p;
    
    if (name_ == 0)
    	return;
    
    /*
     * Suppress arcs which are calls to any of the internal
     * language functions we don't care about, such as g++
     * exception handling.
     * TODO: also stuff like integer arithmetic millicode.
     */
    for (p = ignored ; *p != 0 ; p++)
    {
	if (!strcmp(name_, *p))
	{
	    suppress();
	    return;
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
