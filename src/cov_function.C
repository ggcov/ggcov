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
#include "string_var.H"

CVSID("$Id: cov_function.C,v 1.10 2003-06-30 13:52:56 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_function_t::cov_function_t()
{
    blocks_ = g_ptr_array_new();
}

cov_function_t::~cov_function_t()
{
    unsigned int i;
    
    for (i = 0 ; i < num_blocks() ; i++)
    	delete nth_block(i);
    g_ptr_array_free(blocks_, /*delete_seg*/TRUE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_function_t::set_name(const char *name)
{
    assert(name_ == 0);
    name_ = name;
    file_->functions_by_name_->insert(name_, this);
}

cov_block_t *
cov_function_t::add_block()
{
    cov_block_t *b;
    
    b = new cov_block_t;

    b->idx_ = blocks_->len;
    g_ptr_array_add(blocks_, b);
    b->function_ = this;
    
    return b;
}

gboolean
cov_function_t::is_suppressed() const
{
    if (!strncmp(name_, "_GLOBAL_", 8))
    	return TRUE;
    return FALSE;
}

const cov_location_t *
cov_function_t::get_first_location() const
{
    unsigned int bidx;
    const cov_location_t *loc;
    
    for (bidx = 0 ; bidx < num_blocks() ; bidx++)
    {
	if ((loc = nth_block(bidx)->get_first_location()) != 0)
	    return loc;
    }
    return 0;
}

const cov_location_t *
cov_function_t::get_last_location() const
{
    int bidx;
    const cov_location_t *loc;
    
    for (bidx = num_blocks()-1 ; bidx >= 0 ; bidx--)
    {
	if ((loc = nth_block(bidx)->get_last_location()) != 0)
	    return loc;
    }
    return 0;
}

int
cov_function_t::compare(gconstpointer pa, gconstpointer pb)
{
    const cov_function_t *fa = (cov_function_t *)pa;
    const cov_function_t *fb = (cov_function_t *)pb;
    int ret;
    
    ret = strcmp(fa->name_, fb->name_);
    if (ret == 0)
    	ret = strcmp(fa->file_->name(), fb->file_->name());
    return ret;
}

void
cov_function_t::calc_stats(cov_stats_t *stats) const
{
    unsigned int bidx;
    
    for (bidx = 0 ; bidx < num_blocks() ; bidx++)
	nth_block(bidx)->calc_stats(stats);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_function_t::reconcile_calls()
{
    unsigned int bidx;
    list_iterator_t<cov_arc_t> aiter;
    gboolean ret = TRUE;

    if (is_suppressed())
	return TRUE;	/* ignored */

    /*
     * Last two blocks in a function appear to be the function
     * epilogue with one fake arc out representing returns *from*
     * the function, and a block inserted as the target of all
     * function call arcs, with one fake arc back to block 0
     * representing returns *to* the function.  So don't attempt
     * to reconcile those.
     */
    if (num_blocks() <= 2)
	return TRUE;

    for (bidx = 0 ; bidx < num_blocks()-2 ; bidx++)
    {
    	cov_block_t *b = nth_block(bidx);
	string_var desc = b->describe();

	if (cov_arc_t::ncalls(b->out_arcs_) != (b->call_ == 0 ? 0U : 1U))
	{
	    /* TODO */
	    fprintf(stderr, "Failed to reconcile calls for %s\n",
		    	desc.data());
	    fprintf(stderr, "    %d call arcs, %d recorded calls\n",
		    	    cov_arc_t::ncalls(b->out_arcs_),
			    (b->call_ == 0 ? 0 : 1));
	    b->call_ = (const char *)0;   /* free and null out */
	    ret = FALSE;
	    continue;
	}

	for (aiter = b->out_arc_iterator() ; aiter != (cov_arc_t *)0 ; ++aiter)
	{
	    cov_arc_t *a = *aiter;

    	    if (a->is_call())
	    	a->name_ = b->pop_call();
    	}
#if DEBUG > 1
	fprintf(stderr, "Reconciled %d calls for %s\n",
		    	    cov_arc_t::ncalls(b->out_arcs_), desc.data());
#endif
    }
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Solves the program flow graph for a function -- calculates
 * all the bblock and arc execution counts from the minimal
 * set encoded in the .da file.  This code is lifted almost
 * directly from gcov.c, including comments, with only the data
 * structures changed to protect the innocent.  And formatting
 * of course ;-)
 */

#if DEBUG > 5
#define trace0(fmt) \
    fprintf(stderr, "%s:%d: " fmt "\n", __FUNCTION__, __LINE__)
#define trace1(fmt, a1) \
    fprintf(stderr, "%s:%d: " fmt "\n", __FUNCTION__, __LINE__, a1)
#define trace2(fmt, a1, a2) \
    fprintf(stderr, "%s:%d: " fmt "\n", __FUNCTION__, __LINE__, a1, a2)
#define trace3(fmt, a1, a2, a3) \
    fprintf(stderr, "%s:%d: " fmt "\n", __FUNCTION__, __LINE__, a1, a2, a3)
#else
#define trace0(fmt)
#define trace1(fmt, a1)
#define trace2(fmt, a1, a2)
#define trace3(fmt, a1, a2, a3)
#endif    
 
gboolean
cov_function_t::solve()
{
    int passes, changes;
    int i;
    cov_arc_t *a;
    cov_block_t *b;

    /* For every block in the file,
       - if every exit/entrance arc has a known count, then set the block count
       - if the block count is known, and every exit/entrance arc but one has
	 a known execution count, then set the count of the remaining arc

       As arc counts are set, decrement the succ/pred count, but don't delete
       the arc, that way we can easily tell when all arcs are known, or only
       one arc is unknown.  */

    /* The order that the basic blocks are iterated through is important.
       Since the code that finds spanning trees starts with block 0, low numbered
       arcs are put on the spanning tree in preference to high numbered arcs.
       Hence, most instrumented arcs are at the end.  Graph solving works much
       faster if we propagate numbers from the end to the start.

       This takes an average of slightly more than 3 passes.  */

    trace1(" ---> %s", name_.data());
    
    /*
     * In the new gcc 3.3 file format we cannot expect to get arcs into
     * the entry block and out of the exit block.  So if we don't get any,
     * tweak the {in,out}_ninvalid_ count up to effective infinity to
     * ensure the solve algorithm doesn't accidentally use the lack of
     * counts.
     */
    assert(num_blocks() >= 2);
    if ((b = nth_block(0))->in_arcs_.head() == 0)
    {
    	assert(file_->format_version_ > 0);
	b->in_ninvalid_ = ~0U;
	trace0("entry block tweaked");
    }
    if ((b = nth_block(num_blocks()-1))->out_arcs_.head() == 0)
    {
    	assert(file_->format_version_ > 0);
	b->out_ninvalid_ = ~0U;
	trace0("exit block tweaked");
    }

    changes = 1;
    passes = 0;
    while (changes)
    {
	passes++;
	changes = 0;

    	trace1("pass %d", passes);

	for (i = num_blocks() - 1; i >= 0; i--)
	{
	    b = nth_block(i);
    	    trace1("[%d]", b->bindex());
	    
	    if (!b->count_valid_)
	    {
    		trace3("[%d] out_ninvalid_=%u in_ninvalid_=%u",
		    	    b->bindex(), b->out_ninvalid_, b->in_ninvalid_);
		if (b->out_ninvalid_ == 0)
		{
		    b->set_count(cov_arc_t::total(b->out_arcs_));
		    changes++;
    	    	    trace2("[%d] count=%llu", b->bindex(), b->count());
		}
		else if (b->in_ninvalid_ == 0)
		{
		    b->set_count(cov_arc_t::total(b->in_arcs_));
		    changes++;
    	    	    trace2("[%d] count=%llu", b->bindex(), b->count());
		}
	    }
	    
	    if (b->count_valid_)
	    {
		if (b->out_ninvalid_ == 1)
		{
		    /* Search for the invalid arc, and set its count.  */
		    if ((a = cov_arc_t::find_invalid(b->out_arcs_)) == 0)
			return FALSE;	/* ERROR */
		    /* Calculate count for remaining arc by conservation.  */
		    /* One of the counts will be invalid, but it is zero,
		       so adding it in also doesn't hurt.  */
		    assert(b->count_ >= cov_arc_t::total(b->out_arcs_));
		    a->set_count(b->count_ - cov_arc_t::total(b->out_arcs_));
		    changes++;
    	    	    trace3("[%d->%d] count=%llu",
		    	    a->from()->bindex(), a->to()->bindex(), a->count());
		}
		if (b->in_ninvalid_ == 1)
		{
		    /* Search for the invalid arc, and set its count.  */
		    if ((a = cov_arc_t::find_invalid(b->in_arcs_)) == 0)
			return FALSE;	/* ERROR */
		    /* Calculate count for remaining arc by conservation.  */
		    /* One of the counts will be invalid, but it is zero,
		       so adding it in also doesn't hurt.  */
		    assert(b->count_ >= cov_arc_t::total(b->in_arcs_));
		    a->set_count(b->count_ - cov_arc_t::total(b->in_arcs_));
		    changes++;
    	    	    trace3("[%d->%d] count=%llu",
		    	    a->from()->bindex(), a->to()->bindex(), a->count());
		}
	    }
	}
    }

    /* If the graph has been correctly solved, every block will have a
       succ and pred count of zero.  */
    for (i = 0 ; i < (int)num_blocks() ; i++)
    {
    	b = nth_block(i);
	if (!b->count_valid_)
	    return FALSE;
	if (b->out_ninvalid_ > 0 && b->out_ninvalid_ < ~0U)
	    return FALSE;
	if (b->in_ninvalid_ > 0 && b->in_ninvalid_ < ~0U)
	    return FALSE;
    }

#if DEBUG > 1
    fprintf(stderr, "Solved flow graph for %s in %d passes\n",
    	    	    	name(), passes);
#endif
    	
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GList *
cov_function_t::list_all()
{
    GList *list = 0;
    list_iterator_t<cov_file_t> iter;
    unsigned int fnidx;
    
    for (iter = cov_file_t::first() ; iter != (cov_file_t *)0 ; ++iter)
    {
    	cov_file_t *f = *iter;

	for (fnidx = 0 ; fnidx < f->num_functions() ; fnidx++)
	{
    	    cov_function_t *fn = f->nth_function(fnidx);

	    if (!fn->is_suppressed())
		list = g_list_prepend(list, fn);
	}
    }
    return g_list_sort(list, cov_function_t::compare);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
