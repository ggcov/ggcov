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
#include "string_var.H"

CVSID("$Id: cov_function.C,v 1.26 2010-05-09 05:37:15 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_function_t::cov_function_t()
{
    blocks_ = new ptrarray_t<cov_block_t>();
}

cov_function_t::~cov_function_t()
{
    unsigned int i;

    for (i = 0 ; i < blocks_->length() ; i++)
	delete blocks_->nth(i);
    delete blocks_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_function_t::set_name(const char *name)
{
    assert(name_ == 0);
    name_ = name;
    file_->functions_by_name_->insert(name_, this);

    suppress(cov_suppression_t::find(name_, cov_suppression_t::FUNCTION));
}

void
cov_function_t::set_id(uint64_t id)
{
    assert(id_ == 0);
    id_ = id;
    file_->functions_by_id_->insert(&id_, this);
}

cov_block_t *
cov_function_t::add_block()
{
    cov_block_t *b;

    b = new cov_block_t;

    b->idx_ = blocks_->append(b);
    b->function_ = this;
    b->suppress(suppression_);

    return b;
}

void
cov_function_t::suppress(const cov_suppression_t *s)
{
    /*
     * If it weren't for the case of compiler-generated functions
     * which have no corresponding source lines, we could just rely
     * on suppressions percolating down to cov_line_t's and back up
     * up again to files.  Instead we have to remember when we're
     * suppressed externally or by self.
     */
    if (s && !suppression_)
    {
	dprintf2(D_SUPPRESS, "suppressing function %s: %s\n", name(), s->describe());
	suppression_ = s;
	for (ptrarray_iterator_t<cov_block_t> bitr = blocks_->first() ; *bitr ; ++bitr)
	    (*bitr)->suppress(s);
    }
}

void
cov_function_t::finalise()
{
    for (ptrarray_iterator_t<cov_block_t> bitr = blocks_->first() ; *bitr ; ++bitr)
	(*bitr)->finalise();

    if (!suppression_)
    {
	/* suppress the function if all it's
	 * blocks are suppressed */
	cov_suppression_combiner_t c;
	for (ptrarray_iterator_t<cov_block_t> bitr = blocks_->first() ; *bitr ; ++bitr)
	    c.add((*bitr)->suppression_);
	suppress(c.result());
    }
}

/*
 * Finding the first and last block in a function is actually
 * quite difficult, and has become more difficult as gcc gets
 * better at inlining functions.
 *
 * We try to deal with the case of the first few or last few
 * locations recorded in blocks being from inlined functions
 * in header files, by skipping locations in different files.
 *
 * However we don't handle the case where the function begins
 * or ends with a call to a static function later in the same
 * file which the compiler (assuming a fairly modern gcc)
 * helpfully decides to inline.  To do that properly we would
 * need better location data from gcc.
 */

const cov_location_t *
cov_function_t::get_first_location() const
{
    for (ptrarray_iterator_t<cov_block_t> bitr = blocks_->first() ; *bitr ; ++bitr)
    {
	for (list_iterator_t<cov_location_t> liter = (*bitr)->locations().first() ; *liter ; ++liter)
	{
	    const cov_location_t *loc = *liter;

	    /*
	     * We can get away with a pointer comparison here,
	     * because we know that cov_file::add_location()
	     * uses file->name() as the location's  ->filename
	     * without saving a new copy.
	     */
	    if (loc->filename != file_->name())
		continue;
	    return loc;
	}
    }
    return 0;
}

const cov_location_t *
cov_function_t::get_last_location() const
{
    for (ptrarray_iterator_t<cov_block_t> bitr = blocks_->last() ; *bitr ; --bitr)
    {
	for (list_iterator_t<cov_location_t> liter = (*bitr)->locations().last(); *liter ; --liter)
	{
	    const cov_location_t *loc = *liter;

	    /*
	     * We can get away with a pointer comparison here,
	     * because we know that cov_file::add_location()
	     * uses file->name() as the location's  ->filename
	     * without saving a new copy.
	     */
	    if (loc->filename != file_->name())
		continue;
	    return loc;
	}
    }
    return 0;
}

int
cov_function_t::compare(const cov_function_t *fa, const cov_function_t *fb)
{
    int ret;

    ret = strcmp(fa->name_, fb->name_);
    if (ret == 0)
	ret = strcmp(fa->file_->name(), fb->file_->name());
    return ret;
}

unsigned int
cov_function_t::entry_block() const
{
    return 0;
}

unsigned int
cov_function_t::exit_block() const
{
    return (file_->exit_block_is_1() ? 1 : blocks_->length()-1);
}

unsigned int
cov_function_t::first_real_block() const
{
    return (file_->exit_block_is_1() ? 2 : 1);
}

unsigned int
cov_function_t::last_real_block() const
{
    unsigned int len = blocks_->length();
    return (file_->exit_block_is_1() ? len-1 : len-2);
}

cov::status_t
cov_function_t::calc_stats(cov_stats_t *stats) const
{
    unsigned int bidx;
    cov_stats_t mine;
    cov::status_t st;

    assert(file_->finalised_);

    if (suppression_)
	st = cov::SUPPRESSED;
    else
    {
	/* skip the psuedo-blocks which don't correspond to code */
	assert(num_blocks() >= 2);
	for (bidx = first_real_block() ; bidx <= last_real_block() ; bidx++)
	    nth_block(bidx)->calc_stats(&mine);
	stats->accumulate(&mine);
	st = mine.status_by_lines();
    }

    stats->add_function(st);

    return st;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_function_t::reconcile_calls()
{
    unsigned int bidx;
    gboolean ret = TRUE;

    if (suppression_)
	return TRUE;    /* ignored */

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

    for (bidx = first_real_block() ; bidx <= last_real_block() ; bidx++)
    {
	cov_block_t *b = nth_block(bidx);

	if (b->out_ncalls_ != (b->call_ == 0 ? 0U : 1U))
	{
	    /* TODO */
	    if (b->locations().head() != 0)
	    {
		/*
		 * Don't complain about not being to reconcile weird
		 * calls inserted by g++, like _Unwind_Resume() or
		 * __cxa_throw(), or any other call with no direct
		 * relationship to the source code.
		 */
		dprintf1(D_CGRAPH, "Failed to reconcile calls for block %s\n",
			    b->describe());
		dprintf2(D_CGRAPH, "    %d call arcs, %d recorded calls\n",
			    b->out_ncalls_,
			    (b->call_ == 0 ? 0 : 1));
	    }
	    b->call_ = (const char *)0;   /* free and null out */
	    ret = FALSE;
	    continue;
	}

	for (list_iterator_t<cov_arc_t> aiter = b->first_arc() ; *aiter ; ++aiter)
	{
	    cov_arc_t *a = *aiter;
	    char *name;

	    if (a->is_call() && (name = b->pop_call()))
	    {
		dprintf2(D_CGRAPH|D_VERBOSE, "    block %s calls %s\n",
			  b->describe(), name);
		a->take_name(name);
	    }
	}
	dprintf2(D_CGRAPH, "Reconciled %d calls for block %s\n",
		  b->out_ncalls_, b->describe());
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

gboolean
cov_function_t::solve()
{
    int passes, changes;
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

    dprintf1(D_SOLVE, " ---> %s\n", name_.data());

    /*
     * In the new gcc 3.3 file format we cannot expect to get arcs into
     * the entry block and out of the exit block.  So if we don't get any,
     * tweak the {in,out}_ninvalid_ count up to effective infinity to
     * ensure the solve algorithm doesn't accidentally use the lack of
     * counts.
     */
    assert(num_blocks() >= 2);
    if ((b = blocks_->nth(entry_block()))->in_arcs_.head() == 0)
    {
	assert(file_->format_version_ > 0);
	b->in_ninvalid_ = ~0U;
	dprintf0(D_SOLVE, "entry block tweaked\n");
    }
    if ((b = blocks_->nth(exit_block()))->out_arcs_.head() == 0)
    {
	assert(file_->format_version_ > 0);
	b->out_ninvalid_ = ~0U;
	dprintf0(D_SOLVE, "exit block tweaked\n");
    }

    changes = 1;
    passes = 0;
    while (changes)
    {
	passes++;
	changes = 0;

	dprintf1(D_SOLVE, "pass %d\n", passes);

	for (ptrarray_iterator_t<cov_block_t> bitr = blocks_->last() ; *bitr ; --bitr)
	{
	    b = *bitr;
	    dprintf1(D_SOLVE, "[%d]\n", b->bindex());

	    if (!b->count_valid_)
	    {
		dprintf3(D_SOLVE, "[%d] out_ninvalid_=%u in_ninvalid_=%u\n",
			    b->bindex(), b->out_ninvalid_, b->in_ninvalid_);

		/*
		 * For blocks with calls we have to ignore the outbound total
		 * when calculating the block count, because of the possibility
		 * of calls to fork(), exit() or other functions which can
		 * return more or less frequently than they're called.  Given
		 * the existance of longjmp() all calls are potentially like
		 * that.
		 */
		if (b->out_ncalls_ == 0 && b->out_ninvalid_ == 0)
		{
		    b->set_count(cov_arc_t::total(b->out_arcs_));
		    changes++;
		    dprintf2(D_SOLVE, "[%d] count=%llu\n", b->bindex(), (unsigned long long)b->count());
		}
		else if (b->in_ninvalid_ == 0)
		{
		    b->set_count(cov_arc_t::total(b->in_arcs_));
		    changes++;
		    dprintf2(D_SOLVE, "[%d] count=%llu\n", b->bindex(), (unsigned long long)b->count());
		}
	    }

	    if (b->count_valid_)
	    {
		if (b->out_ninvalid_ == 1)
		{
		    /* Search for the invalid arc, and set its count.  */
		    if ((a = cov_arc_t::find_invalid(b->out_arcs_, FALSE)) == 0)
			return FALSE;   /* ERROR */
		    /* Calculate count for remaining arc by conservation.  */
		    /* One of the counts will be invalid, but it is zero,
		       so adding it in also doesn't hurt.  */
		    count_t out_total = cov_arc_t::total(b->out_arcs_);
		    if (b->count_ < out_total)
		    {
			fprintf(stderr, "Function %s cannot be solved because "
					"the arc counts are inconsistent, suppressing\n",
					name_.data());
			suppress(cov_suppression_t::find(0, cov_suppression_t::UNSOLVABLE));
			return TRUE;
		    }
		    assert(b->count_ >= out_total);
		    a->set_count(b->count_ - out_total);
		    changes++;
		    dprintf3(D_SOLVE, "[%d->%d] count=%llu\n",
			    a->from()->bindex(), a->to()->bindex(),
			    (unsigned long long)a->count());
		}
		if (b->in_ninvalid_ == 1)
		{
		    /* Search for the invalid arc, and set its count.  */
		    if ((a = cov_arc_t::find_invalid(b->in_arcs_, FALSE)) == 0)
			return FALSE;   /* ERROR */
		    /* Calculate count for remaining arc by conservation.  */
		    /* One of the counts will be invalid, but it is zero,
		       so adding it in also doesn't hurt.  */
		    count_t in_total = cov_arc_t::total(b->in_arcs_);
		    if (b->count_ < in_total)
		    {
			fprintf(stderr, "Function %s cannot be solved because "
					"the arc counts are inconsistent, suppressing\n",
					name_.data());
			suppress(cov_suppression_t::find(0, cov_suppression_t::UNSOLVABLE));
			return TRUE;
		    }
		    assert(b->count_ >= in_total);
		    a->set_count(b->count_ - in_total);
		    changes++;
		    dprintf3(D_SOLVE, "[%d->%d] count=%llu\n",
			    a->from()->bindex(), a->to()->bindex(),
			    (unsigned long long)a->count());
		}
	    }
	}
    }

    /*
     * If the graph has been correctly solved, every block will
     * have a valid count and all its inbound and outbounds arcs
     * will also have valid counts.
     */
    for (ptrarray_iterator_t<cov_block_t> bitr = blocks_->first() ; *bitr ; ++bitr)
    {
	b = *bitr;
	if (!b->count_valid_)
	    return FALSE;
	if (b->out_ninvalid_ > 0 && b->out_ninvalid_ < ~0U)
	    return FALSE;
	if (b->in_ninvalid_ > 0 && b->in_ninvalid_ < ~0U)
	    return FALSE;
	if (cov_arc_t::find_invalid(b->in_arcs_, TRUE) != 0)
	    return FALSE;
	if (cov_arc_t::find_invalid(b->out_arcs_, TRUE) != 0)
	    return FALSE;
    }

    dprintf2(D_SOLVE, "Solved flow graph for %s in %d passes\n",
			name(), passes);

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

list_t<cov_function_t> *
cov_function_t::list_all()
{
    list_t<cov_function_t> *list = new list_t<cov_function_t>;

    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
	for (ptrarray_iterator_t<cov_function_t> fnitr = (*iter)->functions().first() ; *fnitr ; ++fnitr)
	    list->prepend(*fnitr);
    list->sort(cov_function_t::compare);
    return list;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_function_t::zero_arc_counts()
{
    for (ptrarray_iterator_t<cov_block_t> bitr = blocks_->first() ; *bitr ; ++bitr)
    {
	for (list_iterator_t<cov_arc_t> aiter = (*bitr)->first_arc() ; *aiter ; ++aiter)
	{
	    cov_arc_t *a = *aiter;

	    if (!a->on_tree_)
		a->set_count(0UL);
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
