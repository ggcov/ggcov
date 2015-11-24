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

#include "cov_priv.H"
#include "estring.H"
#include "filename.h"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_block_t::cov_block_t()
{
}

cov_block_t::~cov_block_t()
{
    cov_arc_t *a;

    cov_location_t *loc;
    while ((loc = locations_.remove_head()) != 0)
    {
	cov_line_t::remove(loc, this);
	delete loc;
    }

    while ((a = in_arcs_.head()) != 0)
	delete a;
    while ((a = out_arcs_.head()) != 0)
	delete a;

    call_t *cc;
    while ((cc = pure_calls_.remove_head()) != 0)
	delete cc;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
cov_block_t::describe() const
{
    static estring buf;
    buf.truncate();
    buf.append_printf("%s:%d", function_->name(), idx_);
    if (idx_ == function_->entry_block())
	buf.append_string("[entry]");
    else if (idx_ == function_->exit_block())
	buf.append_string("[exit]");
    return buf.data();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_block_t::is_epilogue() const
{
    unsigned int nblocks = function_->num_blocks();

    return (nblocks >= 4 &&
	    idx_ == (nblocks - 2) &&
	    out_arcs_.length() == 1 &&
	    (*(out_arcs_.first()))->to()->bindex() == (nblocks-1));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_block_t::add_location(const char *filename, unsigned lineno)
{
    cov_location_t *loc;

    loc = new(cov_location_t);
    loc->filename = (char *)filename;   /* stored externally */
    loc->lineno = lineno;
    locations_.append(loc);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_block_t::set_count(count_t count)
{
    assert(!count_valid_);
    count_valid_ = true;
    count_ = count;

    assert(out_ncalls_ == 0 || out_ncalls_ == 1);
    if (out_ncalls_)
    {
	for (list_iterator_t<cov_arc_t> aiter = out_arcs_.first() ; *aiter ; ++aiter)
	{
	    cov_arc_t *a = (*aiter);
	    if (a->call_)
	    {
		a->set_count(count);
		break;
	    }
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_block_t::is_call_site() const
{
    return (idx_ > 0 &&
	    idx_ < function_->num_blocks()-1 &&
	    out_ncalls_ > 0);
}

gboolean
cov_block_t::needs_call() const
{
    return (is_call_site() && call_ == 0);
}

void
cov_block_t::add_call(const char *name, const cov_location_t *loc)
{
    static const char fn[] = "cov_block_t::add_call";
    const cov_location_t *last;

    if (name == 0)
	name = "*pointer";

    if (is_call_site() &&
	((last = locations_.tail()) == 0 || *loc == *last))
    {
	dprintf5(D_CGRAPH, "%s: call from %s:%u to %s at %s\n",
		fn, function_->name(), idx_, name, loc->describe());
	if (call_ != 0)
	{
	    /* multiple calls: assume the earlier one is actually pure */
	    dprintf5(D_CGRAPH, "%s: assuming earlier call from %s:%u to %s at %s was pure\n",
		fn, function_->name(), idx_, call_.data(), locations_.tail()->describe());
	    pure_calls_.append(new call_t(call_, locations_.tail()));
	}
	call_ = name;
    }
    else
    {
	dprintf5(D_CGRAPH, "%s: pure call from %s:%u to %s at %s\n",
		fn, function_->name(), idx_, name, loc->describe());
	pure_calls_.append(new call_t(name, loc));
    }
}

char *
cov_block_t::pop_call()
{
    assert(call_ != 0);
    return call_.take();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_block_t::suppress(const cov_suppression_t *s)
{
    if (s && !suppression_)
    {
	dprintf2(D_SUPPRESS, "suppressing block %s: %s\n", describe(), s->describe());
	suppression_ = s;

	/* suppress all outbound arcs */
	for (list_iterator_t<cov_arc_t> aiter = out_arcs_.first() ; *aiter ; ++aiter)
	    (*aiter)->suppress(s);
    }
}

void
cov_block_t::finalise()
{
    if (!suppression_)
    {
	/* suppress the block if all it's
	 * lines are suppressed */
	cov_suppression_combiner_t c(cov_suppressions);
	for (list_iterator_t<cov_location_t> liter = locations_.first() ; *liter ; ++liter)
	    c.add(cov_line_t::find(*liter)->suppression_);
	suppress(c.result());
    }

    if (!suppression_)
    {
	/* suppress the block if all it's
	 * outgoing arcs are suppressed */
	cov_suppression_combiner_t c(cov_suppressions);
	for (list_iterator_t<cov_arc_t> aiter = out_arcs_.first() ; *aiter ; ++aiter)
	    c.add((*aiter)->suppression_);
	suppress(c.result());
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Calculate stats on a block.  I don't really understand
 * the meaning of the arc bits, but copied the implications
 * from gcov.c.
 */

cov::status_t
cov_block_t::calc_stats(cov_stats_t *stats) const
{
    unsigned int bits = 0;
    cov_stats_t mine;
    cov::status_t st;

    assert(function_->file()->finalised_);

    if (suppression_)
	st = cov::SUPPRESSED;
    else
    {
	/*
	 * Calculate call and branches coverage.
	 */
	for (list_iterator_t<cov_arc_t> aiter = out_arcs_.first() ; *aiter ; ++aiter)
	{
	    cov_arc_t *a = *aiter;

	    if (a->is_fall_through())
		continue;       /* control flow does not branch */

	    if (a->is_call())
		mine.add_call(a->status());
	    else
		mine.add_branch(a->status());
	}

	/*
	 * Calculate line coverage.
	 */
	for (list_iterator_t<cov_location_t> liter = locations_.first() ; *liter ; ++liter)
	{
	    cov_line_t *ln = cov_line_t::find(*liter);

	    st = ln->status();

	    /*
	     * Compensate for multiple blocks on a line by
	     * only counting when we hit the first block.
	     * This will lead to anomalies when there are
	     * multiple functions on the same line, but code
	     * like that *deserves* anomalies.
	     */
	    if (ln->blocks().head() != this)
	    {
		if (st == cov::SUPPRESSED)
		    bits |= (1<<st);    /* handle middle block on suppressed line */
		continue;
	    }

	    mine.add_line(st);
	    bits |= (1<<st);
	}

	/*
	 * Return the block's status.  Note this can't be achieved
	 * by simply calling cov_stats_t::status() as that works on
	 * lines and may be inaccurate when the first or last line is
	 * shared with other blocks.  Instead we use the block's count_.
	 *
	 * Note: we cannot have the case where all the lines
	 * are suppressed, that should have set suppression_
	 */
	assert(bits != (1<<cov::SUPPRESSED));
	st = (count_ ? cov::COVERED : cov::UNCOVERED);
	stats->accumulate(&mine);
    }

    /*
     * Calculate block coverage
     */
    stats->add_block(st);

    return st;
}

count_t
cov_block_t::total(const list_t<cov_block_t> &list)
{
    count_t total = 0;

    for (list_iterator_t<cov_block_t> itr = list.first() ; *itr ; ++itr)
	total += (*itr)->count_;
    return total;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
