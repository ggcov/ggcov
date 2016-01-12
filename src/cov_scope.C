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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_scope_t::cov_scope_t()
{
    dirty_ = true;
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
	dirty_ = false;
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
    dirty_ = true;
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


cov::status_t
cov_range_scope_t::calc_stats(cov_stats_t *stats)
{
    /*
     * Check inputs
     */
    if (start_ > end_)
	return cov::SUPPRESSED;         /* invalid range */
    if (start_ == 0 || end_ == 0)
	return cov::SUPPRESSED;         /* invalid range */
    unsigned int lastline = file_->num_lines();
    if (start_ > lastline)
	return cov::SUPPRESSED;         /* range is outside file */
    if (end_ > lastline)
	end_ = lastline;		/* clamp range to file */

    /* we treat this as a set */
    hashtable_t<void, void> *blocks_seen = new hashtable_t<void, void>;
    /* we treat this as a set */
    hashtable_t<void, void> *functions_seen = new hashtable_t<void, void>;
    cov_stats_t block_stats;
    cov_stats_t function_stats;
    cov_stats_t line_stats;
    cov_file_t::line_iterator_t start_itr(file_, start_);
    cov_file_t::line_iterator_t end_itr(file_, end_+1);
    for (cov_file_t::line_iterator_t itr = start_itr ; itr != end_itr ; ++itr)
    {
	cov_line_t *line = itr.line();
	line_stats.add_line(line->status());
	for (list_iterator_t<cov_block_t> bitr = line->blocks().first() ; *bitr ; ++bitr)
	{
	    cov_block_t *b = *bitr;
	    if (!blocks_seen->lookup(b))
	    {
		b->calc_stats(&block_stats);
		blocks_seen->insert(b, b);
		cov_function_t *f = b->function();
		if (!functions_seen->lookup(f))
		{
		    f->calc_stats(&function_stats);
		    functions_seen->insert(f, f);
		}
	    }
	}
    }

    stats->accumulate_blocks(&block_stats);
    stats->accumulate_lines(&line_stats);
    stats->accumulate_functions(&function_stats);
    stats->accumulate_calls(&block_stats);
    stats->accumulate_branches(&block_stats);

    delete blocks_seen;
    delete functions_seen;
    return stats->status_by_lines();
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
