/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2003 Greg Banks <gnb@alphalink.com.au>
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

CVSID("$Id: cov_block.C,v 1.7 2003-06-06 15:21:30 gnb Exp $");

hashtable_t<const char *, GList> *cov_block_t::by_location_; 	/* GList of blocks keyed on "file:line" */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_block_t::cov_block_t()
{
}

cov_block_t::~cov_block_t()
{
    cov_location_t *loc;
    string_var key;
    cov_arc_t *a;
    
    while ((loc = locations_.remove_head()) != 0)
    {
    	key = loc->make_key();
	by_location_->remove(key);
	delete loc;
    }
    
    while ((a = in_arcs_.head()) != 0)
    	delete a;
    while ((a = out_arcs_.head()) != 0)
    	delete a;
}

void
cov_block_t::init()
{
    by_location_ = new hashtable_t<const char *, GList>;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
cov_block_t::describe() const
{
    return g_strdup_printf("%s:%d", function_->name(), idx_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_block_t::add_location(const char *filename, unsigned lineno)
{
    cov_location_t *loc;
    string_var key;
    GList *list;

#if DEBUG > 1
    fprintf(stderr, "Block %s:%d adding location %s:%d\n",
    	    	function_->name(), idx_,
		filename, lineno);
#endif
    
    loc = new(cov_location_t);
    loc->filename = g_strdup(filename); /* TODO: hashtable to reduce storage */
    loc->lineno = lineno;
    
    locations_.append(loc);

    key = loc->make_key();
    list = by_location_->lookup(key);
    
    if (list == 0)
    	by_location_->insert(key.take(), g_list_append(0, this));
    else
    {
    	g_list_append(list, this);
#if DEBUG > 1
    	fprintf(stderr, "%s:%ld: this line belongs to %d blocks\n",
	    	    	    loc->filename, loc->lineno, g_list_length(list));
#endif
    }
    
    function_->file()->add_location(loc);
}

const GList *
cov_block_t::find_by_location(const cov_location_t *loc)
{
    string_var key = loc->make_key();
    return by_location_->lookup(key);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_block_t::set_count(count_t count)
{
    assert(!count_valid_);
    count_valid_ = TRUE;
    count_ = count;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_block_t::needs_call() const
{
    /*
     * TODO: can avoid calling ncalls() and just test the number
     * of incoming arcs to the last block in the function, which
     * should be the same.
     */
    return (idx_ > 0 &&
    	    idx_ < function_->num_blocks()-1 &&
	    call_ == 0 &&
    	    cov_arc_t::ncalls(out_arcs_) > 0);
}

void
cov_block_t::add_call(const char *callname)
{
    assert(call_ == 0);
    call_ = callname;
}

char *
cov_block_t::pop_call()
{
    assert(call_ != 0);
    return call_.take();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Calculate stats on a block.  I don't really understand
 * the meaning of the arc bits, but copied the implications
 * from gcov.c.
 */

void
cov_block_t::calc_stats(cov_stats_t *stats) const
{
     list_iterator_t<cov_arc_t> aiter;
     list_iterator_t<cov_location_t> liter;

    /*
     * Calculate call and branches coverage.
     */
    for (aiter = out_arcs_.first() ; aiter != (cov_arc_t *)0 ; ++aiter)
    {
	cov_arc_t *a = *aiter;

    	if (a->is_fall_through())
	    continue;	/* control flow does not branch */

	if (a->is_call())
	{
	    stats->calls++;
	    if (count_)
		stats->calls_executed++;
	}
	else
	{
	    stats->branches++;
	    if (count_)
		stats->branches_executed++;
	    if (a->count_)
		stats->branches_taken++;
	}
    }

    /*
     * Calculate line coverage.
     */
    for (liter = locations_.first() ; liter != (cov_location_t *)0 ; ++liter)
    {
	cov_location_t *loc = *liter;
	const GList *blocks = find_by_location(loc);

	/*
	 * Compensate for multiple blocks on a line by
	 * only counting when we hit the first block.
	 * This will lead to anomalies when there are
	 * multiple functions on the same line, but code
	 * like that *deserves* anomalies.
	 */
	if (blocks->data != this)
	    continue;

	stats->lines++;
	if (total(blocks))
	    /* any blocks on this line were executed */
	    stats->lines_executed++;
    }
}

count_t
cov_block_t::total(const GList *list)
{
    count_t total = 0;

    for ( ; list != 0 ; list = list->next)
    {
    	cov_block_t *b = (cov_block_t *)list->data;
	
	total += b->count_;
    }
    return total;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
