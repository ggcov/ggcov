/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2005 Greg Banks <gnb@users.sourceforge.net>
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

CVSID("$Id: cov_calliter.C,v 1.4 2010-05-09 05:37:15 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_call_iterator_t::cov_call_iterator_t()
{
    block_ = 0;
    pure_ = 0;
}

cov_call_iterator_t::~cov_call_iterator_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_call_iterator_t::block_start(cov_block_t *b)
{
    state_ = 0;
    block_ = b;
    pure_ = 0;
    arc_ = 0;
    pure_iter_ = b->pure_calls_.first();
}

gboolean
cov_call_iterator_t::block_next()
{
    if (block_ == 0)
    	return FALSE;
    if (state_ == 0)
    {
	if (pure_iter_ != (cov_block_t::call_t *)0)
	{
	    pure_ = *pure_iter_;
	    ++pure_iter_;
	    return TRUE;
	}
	state_++;
    }
    if (state_ == 1)
    {
    	pure_ = 0;
	for (list_iterator_t<cov_arc_t> oi = block_->first_arc() ; *oi ; ++oi)
	{
	    arc_ = *oi;
	    if (!arc_->is_call())
	    	continue;
	    if (arc_->is_suppressed())
	    	continue;
	    state_++;
	    return TRUE;
	}
    }
    block_ = 0;
    arc_ = 0;
    pure_ = 0;
    // state remains =2 so we don't try to do more if called again
    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_function_call_iterator_t::cov_function_call_iterator_t(cov_function_t *fn)
 :  function_(fn),
    bindex_(1)
{
    block_start(fn->nth_block(bindex_));
}

cov_function_call_iterator_t::~cov_function_call_iterator_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_function_call_iterator_t::next()
{
    for (;;)
    {
	if (block_next())
    	    return TRUE;
	cov_block_t *b;
	if (++bindex_ >= function_->num_blocks() ||
	    (b = function_->nth_block(bindex_)) == 0)
	    break;
	block_start(b);
    }

    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_range_call_iterator_t::cov_range_call_iterator_t(
    const cov_location_t *first,
    const cov_location_t *last)
 :  first_(first),
    last_(last)
{
    if (first_)
    {
	location_ = *first;
	cov_line_t *ln = cov_line_t::find(&location_);
	if (ln != 0)
	    biter_ = ln->blocks().first();
    }
}

cov_range_call_iterator_t::~cov_range_call_iterator_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_range_call_iterator_t::next()
{
    for (;;)
    {
	if (block_next())
	{
	    if (*location() == location_)
		return TRUE;
	    continue;
	}
	if (*biter_)
	{
	    block_start(*biter_);
	    ++biter_;
	    continue;
	}
	for (;;)
	{
	    if (last_ == 0 || location_ == *last_)
		return FALSE;
	    ++location_;
	    cov_line_t *ln = cov_line_t::find(&location_);
	    if (ln != 0 && (biter_ = ln->blocks().first()) != 0)
		break;
	}
    }

    return FALSE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
