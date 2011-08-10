/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2003-2005 Greg Banks <gnb@users.sourceforge.net>
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

CVSID("$Id: cov_line.C,v 1.9 2010-05-09 05:37:15 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_line_t::cov_line_t()
{
}

cov_line_t::~cov_line_t()
{
    blocks_.remove_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_function_t *
cov_line_t::function() const
{
    if (!blocks_.head())
	return 0;
    return blocks_.head()->function();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_line_t::calculate_count()
{
    count_t minc = COV_COUNT_MAX, maxc = 0;
    unsigned int nsupp = 0;
    unsigned int len = 0;

    assert(!count_valid_);
    count_valid_ = true;

    /*
     * TODO: implement the new smarter algorithm from gcov 3.3 here
     */
    for (list_iterator_t<cov_block_t> itr = blocks_.first() ; *itr ; ++itr)
    {
	cov_block_t *b = *itr;

	len++;
	if (b->is_self_suppressed())
	{
	    nsupp++;
	    continue;
	}
	if (b->count() > maxc)
	    maxc = b->count();
	if (b->count() < minc)
	    minc = b->count();
    }

    count_ = maxc;

    if (len == 0)
	status_ = cov::UNINSTRUMENTED;
    else if (nsupp == len)
	status_ = cov::SUPPRESSED;
    else if (maxc == 0)
	status_ = cov::UNCOVERED;
    else
	status_ = (minc == 0 ? cov::PARTCOVERED : cov::COVERED);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_line_t *
cov_line_t::find(const cov_location_t *loc)
{
    cov_file_t *f;

    return ((f = cov_file_t::find(loc->filename)) == 0 ||
    	    loc->lineno < 1 ||
	    loc->lineno > f->num_lines()
    	    ? 0
	    : f->nth_line(loc->lineno));
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_line_t::remove(const cov_location_t *loc, cov_block_t *b)
{
    cov_line_t *ln = find(loc);

    if (ln)
	ln->blocks_.remove(b);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

unsigned int
cov_line_t::format_blocks(char *buf, unsigned int maxlen)
{
    unsigned int len;
    unsigned int start = 0, end = 0;
    char *base = buf;

    for (list_iterator_t<cov_block_t> itr = blocks_.first() ;
	 *itr && maxlen > 0 ;
	 ++itr)
    {
	cov_block_t *b = *itr;

	assert(b->bindex() != 0);
	if (start > 0 && b->bindex() == end+1)
	{
	    end++;
	    continue;
	}
	if (start == 0)
	{
	    start = end = b->bindex();
	    continue;
	}

	if (start == end)
    	    snprintf(buf, maxlen, "%u,", start);
	else
    	    snprintf(buf, maxlen, "%u%c%u,",
	    	    start, (end == start+1 ? ',' : '-'), end);
	len = strlen(buf);
	buf += len;
	maxlen -= len;
	start = end = b->bindex();
    }
    
    if (maxlen > 0 && start > 0)
    {
	if (start == end)
    	    snprintf(buf, maxlen, "%u", start);
	else
    	    snprintf(buf, maxlen, "%u%c%u",
	    		start, (end == start+1 ? ',' : '-'), end);
	len = strlen(buf);
	buf += len;
	maxlen -= len;
    }

    *buf = '\0';

    return (buf - base);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
