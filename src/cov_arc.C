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

#include "cov.H"
#include "estring.H"
#include "filename.h"

CVSID("$Id: cov_arc.C,v 1.1 2002-12-29 13:14:16 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_arc_t::cov_arc_t(cov_block_t *from, cov_block_t *to)
{
    idx_ = from->out_arcs_.length();
    
    from_ = from;
    from_->out_arcs_.append(this);
    from_->out_ninvalid_++;

    to_ = to;
    to_->in_arcs_.append(this);
    to_->in_ninvalid_++;
}

cov_arc_t::~cov_arc_t()
{
#if 0
#else
    assert(0);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_arc_t::set_count(count_t count)
{
    assert(!count_valid_);
    count_valid_ = TRUE;
    count_ = count;
    from_->out_ninvalid_--;
    to_->in_ninvalid_--;
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
	total += (*iter)->count_;
    }
    return total;
}

unsigned int
cov_arc_t::nfake(const list_t<cov_arc_t> &list)
{
    unsigned int nfake = 0;
    list_iterator_t<cov_arc_t> iter;

    for (iter = list.first() ; iter != (cov_arc_t *)0 ; ++iter)
    {
	if ((*iter)->fake_)
	    nfake++;
    }
    return nfake;
}	    

cov_arc_t *
cov_arc_t::find_invalid(const list_t<cov_arc_t> &list)
{
    list_iterator_t<cov_arc_t> iter;

    for (iter = list.first() ; iter != (cov_arc_t *)0 ; ++iter)
    {
    	cov_arc_t *a = *iter;
	
	if (!a->count_valid_)
	    return a;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
