/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2003-2004 Greg Banks <gnb@users.sourceforge.net>
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
#include "string_var.H"
#include "filename.h"

CVSID("$Id: cov_scope.C,v 1.9 2010-05-09 05:37:15 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_scope_t::cov_scope_t()
{
    dirty_ = TRUE;
}

cov_scope_t::~cov_scope_t()
{
}

const cov_stats_t *
cov_scope_t::get_stats()
{
    if (dirty_)
    {
	stats_.clear();
	status_ = calc_stats(&stats_);
	dirty_ = FALSE;
    }
    return &stats_;
}

cov::status_t
cov_scope_t::status()
{
    get_stats();
    return status_;
}

void
cov_scope_t::dirty()
{
    dirty_ = TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_overall_scope_t::cov_overall_scope_t()
{
}

cov_overall_scope_t::~cov_overall_scope_t()
{
}

const char *
cov_overall_scope_t::describe() const
{
    return _("Overall");
}

cov::status_t
cov_overall_scope_t::calc_stats(cov_stats_t *stats)
{
    for (list_iterator_t<cov_file_t> iter = cov_file_t::first() ; *iter ; ++iter)
	(*iter)->calc_stats(stats);

    return stats->status_by_blocks();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_file_scope_t::cov_file_scope_t(const char *filename)
 :  file_(cov_file_t::find(filename))
{
}

cov_file_scope_t::cov_file_scope_t(const cov_file_t *file)
 :  file_(file)
{
}

cov_file_scope_t::~cov_file_scope_t()
{
}

const char *
cov_file_scope_t::describe() const
{
    return (file_ == 0 ? 0 : file_->minimal_name());
}

cov::status_t
cov_file_scope_t::calc_stats(cov_stats_t *stats)
{
    assert(file_ != 0);
    return file_->calc_stats(stats);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_function_scope_t::cov_function_scope_t(const cov_function_t *fn)
 :  function_(fn)
{
}

cov_function_scope_t::~cov_function_scope_t()
{
}

const char *
cov_function_scope_t::describe() const
{
    return (function_ == 0 ? 0 : function_->name());
}

cov::status_t
cov_function_scope_t::calc_stats(cov_stats_t *stats)
{
    assert(function_ != 0);
    return function_->calc_stats(stats);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_range_scope_t::cov_range_scope_t(
    const char *filename,
    unsigned long start,
    unsigned long end)
 :  file_(cov_file_t::find(filename)),
    start_(start),
    end_(end)
{
    set_description();
}

cov_range_scope_t::cov_range_scope_t(
    const cov_file_t *file,
    unsigned long start,
    unsigned long end)
 :  file_(file),
    start_(start),
    end_(end)
{
    set_description();
}

cov_range_scope_t::~cov_range_scope_t()
{
    if (description_)
	g_free(description_);
}

void
cov_range_scope_t::set_description()
{
    if (file_)
	description_ = g_strdup_printf("%s:%lu-%lu",
					file_->minimal_name(),
					start_, end_);
}

const char *
cov_range_scope_t::describe() const
{
    return description_;
}


/*
 * TODO: the method used here is completely wrong and doesn't
 * handle inline functions or functions in #included source
 * properly.  Now that we have per-line records stored in an
 * array on the cov_file_t we should use that instead.
 */
cov::status_t
cov_range_scope_t::calc_stats(cov_stats_t *stats)
{
    cov_location_t start, end;
    cov_block_t *b;
    unsigned fnidx, bidx;
    unsigned long lastline;
    cov_line_t *startln, *endln;

    assert(file_ != 0);

    start.lineno = start_;
    end.lineno = end_;
    end.filename = start.filename = (char *)file_->name();

    /*
     * Check inputs
     */
    if (start.lineno > end.lineno)
	return cov::SUPPRESSED;         /* invalid range */
    if (start.lineno == 0 || end.lineno == 0)
	return cov::SUPPRESSED;         /* invalid range */
    lastline = file_->num_lines();
    if (start.lineno > lastline)
	return cov::SUPPRESSED;         /* range is outside file */
    if (end.lineno > lastline)
	end.lineno = lastline;  /* clamp range to file */

    /*
     * Get blocklists for start and end.
     */
    do
    {
	startln = cov_line_t::find(&start);
    } while ((startln == 0 || !startln->blocks().head()) &&
	     ++start.lineno <= end.lineno);

    if (startln == 0 || !startln->blocks().head())
	return cov::SUPPRESSED;         /* no executable lines in the given range */
    assert(startln != 0);
    assert(startln->blocks().head());

    do
    {
	endln = cov_line_t::find(&end);
    } while ((endln == 0 || !endln->blocks().head()) &&
	     --end.lineno > start.lineno-1);

    assert(endln != 0);
    assert(endln->blocks().head());
    assert(start.lineno <= end.lineno);


    /*
     * Iterate over the blocks between start and end,
     * gathering stats as we go.  Note that this can
     * span functions.
     */
    b = startln->blocks().head();
    bidx = b->bindex();
    fnidx = b->function()->findex();

    do
    {
	b = file_->nth_function(fnidx)->nth_block(bidx);
	b->calc_stats(stats);
	if (++bidx == file_->nth_function(fnidx)->num_blocks())
	{
	    bidx = 0;
	    ++fnidx;
	}
    } while (b != endln->blocks().head() &&
	     fnidx < file_->num_functions());

    return stats->status_by_blocks();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_compound_scope_t::cov_compound_scope_t()
{
}

cov_compound_scope_t::cov_compound_scope_t(list_t<cov_scope_t> *children)
{
    children_.take(children);
}

cov_compound_scope_t::~cov_compound_scope_t()
{
    children_.remove_all();
}

void
cov_compound_scope_t::add_child(cov_scope_t *sc)
{
    /* caller's responsibility to avoid adding twice if so desired */
    children_.prepend(sc);
    dirty();
}

void
cov_compound_scope_t::remove_child(cov_scope_t *sc)
{
    children_.remove(sc);
    dirty();
}

const char *
cov_compound_scope_t::describe() const
{
    return "compound";
}

cov::status_t
cov_compound_scope_t::calc_stats(cov_stats_t *stats)
{
    const cov_stats_t *cstats;

    for (list_iterator_t<cov_scope_t> iter = children_.first() ; *iter ; ++iter)
    {
	// accumulate stats with caching
	if ((cstats = (*iter)->get_stats()) != 0)
	    stats->accumulate(cstats);
    }

    return stats->status_by_blocks();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
