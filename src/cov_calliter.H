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

#ifndef _ggcov_cov_calliter_H_
#define _ggcov_cov_calliter_H_ 1

#include "common.h"
#include "cov_block.H"
#include "cov_arc.H"
#include "list.H"

class cov_function_t;

class cov_call_iterator_t
{
protected:
    cov_call_iterator_t();
public:
    virtual ~cov_call_iterator_t();
    
    virtual gboolean next() = 0;

    cov_block_t *block() const
    {
    	return block_;
    }
    
    const char *name() const
    {
	if (pure_ != 0)
    	    return pure_->name_;
	else if (arc_ != 0)
    	    return arc_->call_name();
	else
    	    return 0;
    }

    const cov_location_t *location() const
    {
	if (pure_ != 0)
    	    return &pure_->location_;
	else if (arc_ != 0)
    	    return arc_->call_location();
	else
    	    return 0;
    }

    count_t count() const
    {
	if (pure_ != 0)
    	    return block_->count();
	else if (arc_ != 0)
    	    return arc_->count();
	else
    	    return 0;
    }
    
    gboolean is_pure() const
    {
	return (block_ != 0 && pure_ != 0);
    }

protected:
    void block_start(cov_block_t *);
    gboolean block_next();

private:
    int state_;
    cov_block_t *block_;
    list_iterator_t<cov_block_t::call_t> pure_iter_;
    cov_block_t::call_t *pure_;
    cov_arc_t *arc_;
};


class cov_function_call_iterator_t : public cov_call_iterator_t
{
public:
    cov_function_call_iterator_t(cov_function_t *);
    ~cov_function_call_iterator_t();
    gboolean next();

private:
    cov_function_t *function_;
    unsigned int bindex_;
};


class cov_range_call_iterator_t : public cov_call_iterator_t
{
public:
    cov_range_call_iterator_t(const cov_location_t *first,
    	    	    	      const cov_location_t *last);
    ~cov_range_call_iterator_t();
    gboolean next();

private:
    const cov_location_t *first_, *last_;
    cov_location_t location_;
    list_iterator_t<cov_block_t> biter_;
};

#endif /* _ggcov_cov_calliter_H_ */
