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
#include "covio.h"
#include "estring.H"
#include "string_var.H"
#include "filename.h"

CVSID("$Id: cov_scope.C,v 1.2 2003-05-31 14:35:41 gnb Exp $");

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
    	if (!calc_stats(&stats_))
	    return 0;
	dirty_ = FALSE;
    }
    return &stats_;
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

gboolean
cov_overall_scope_t::calc_stats(cov_stats_t *stats)
{
    list_iterator_t<cov_file_t> iter;
    
    for (iter = cov_file_t::first() ; iter != (cov_file_t *)0 ; ++iter)
    	(*iter)->calc_stats(stats);

    return TRUE;
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

gboolean
cov_file_scope_t::calc_stats(cov_stats_t *stats)
{
    if (!file_)
    	return FALSE;
    file_->calc_stats(stats);
    return TRUE;
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

gboolean
cov_function_scope_t::calc_stats(cov_stats_t *stats)
{
    if (!function_)
    	return FALSE;
    function_->calc_stats(stats);
    return TRUE;
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

gboolean
cov_range_scope_t::calc_stats(cov_stats_t *stats)
{
    cov_location_t start, end;
    cov_block_t *b;
    unsigned fnidx, bidx;
    unsigned long lastline;
    const GList *startblocks, *endblocks;

    if (!file_)
    	return FALSE;

    start.lineno = start_;
    end.lineno = end_;
    end.filename = start.filename = (char *)file_->name();

    /*
     * Check inputs
     */
    if (start.lineno > end.lineno)
    	return FALSE;     	/* invalid range */
    if (start.lineno == 0 || end.lineno == 0)
    	return FALSE;     	/* invalid range */
    lastline = file_->get_last_location()->lineno;
    if (start.lineno > lastline)
    	return FALSE;     	/* range is outside file */
    if (end.lineno > lastline)
    	end.lineno = lastline;  /* clamp range to file */
    
    /*
     * Get blocklists for start and end.
     */
    do
    {
    	startblocks = cov_block_t::find_by_location(&start);
    } while (startblocks == 0 && ++start.lineno <= end.lineno);
    
    if (startblocks == 0)
    	return TRUE;     	/* no executable lines in the given range */

    do
    {
    	endblocks = cov_block_t::find_by_location(&end);
    } while (endblocks == 0 && --end.lineno > start.lineno-1);
    
    assert(endblocks != 0);
    assert(start.lineno <= end.lineno);
    

    /*
     * Iterate over the blocks between start and end,
     * gathering stats as we go.  Note that this can
     * span functions.
     */    
    b = (cov_block_t *)startblocks->data;
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
    } while (b != (cov_block_t *)endblocks->data);

    return TRUE;
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

gboolean
cov_compound_scope_t::calc_stats(cov_stats_t *stats)
{
    list_iterator_t<cov_scope_t> iter;
    const cov_stats_t *cstats;
    
    for (iter = children_.first() ; iter != (cov_scope_t *)0 ; ++iter)
    {
    	// accumulate stats with caching
    	if ((cstats = (*iter)->get_stats()) != 0)
	    stats->accumulate(cstats);
    }

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
