/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2005 Greg Banks <gnb@alphalink.com.au>
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

#ifndef _ggcov_cov_arc_H_
#define _ggcov_cov_arc_H_ 1

#include "string_var.H"

class cov_file_t;
class cov_function_t;
class cov_block_t;

class cov_arc_t
{
public:
    const char *
    name() const
    {
    	return name_;
    }

    count_t
    count() const
    {
    	return count_;
    }
    
    unsigned int
    aindex() const
    {
    	return idx_;
    }
    gboolean is_call() const;
    gboolean
    is_fall_through() const
    {
    	return fall_through_;
    }
    
    cov_block_t *
    from() const
    {
    	return from_;
    }
    cov_block_t *
    to() const
    {
    	return to_;
    }

    const cov_location_t *
    get_from_location() const
    {
	return from_->get_first_location();
    }

    gboolean is_suppressed() const;
    
    /* TODO: cache the status and move the calculation out of line */
    cov::status_t status()
    {
#if 0
    /* TODO: verify whether these 2 code fragments are the same */
    /* I'm assuming that for calls count_ will be the same as from_->count_
      */
	if (suppressed_)
	    return cov::SUPPRESSED;
	else if (count_)
	    return cov::COVERED;
	else if (from_->count_)
	    return cov::PARTCOVERED;
	else
	    return cov::UNCOVERED;
#else
    	if (is_fall_through() || is_call())
	{
	    if (is_suppressed())
		return cov::SUPPRESSED;
	    else if (from_->count_)
		return cov::COVERED;
	    else
		return cov::UNCOVERED;
	}
	else
	{
	    if (is_suppressed())
		return cov::SUPPRESSED;
	    else if (count_)
		return cov::COVERED;
	    else if (from_->count_)
		return cov::PARTCOVERED;
	    else
		return cov::UNCOVERED;
	}
#endif
    }
    
    void finalise();

private:
    cov_arc_t(cov_block_t *from, cov_block_t *to);
    ~cov_arc_t();

    void suppress()
    {
    	suppressed_ = TRUE;
    }

    void set_count(count_t count);
    static count_t total(const list_t<cov_arc_t> &list);
    static unsigned int ncalls(const list_t<cov_arc_t> &list);
    static cov_arc_t *find_invalid(const list_t<cov_arc_t> &list);

    cov_block_t *from_, *to_;
    unsigned int idx_; 	    /* serial number in from->out_arcs */
    count_t count_;
    string_var name_;     	    /* name of function called (if known) or NULL */
    gboolean on_tree_:1;
    gboolean fall_through_:1;
    gboolean count_valid_:1;
    gboolean suppressed_:1;
    
    friend class cov_file_t;
    friend class cov_function_t;
    friend class cov_block_t;
    friend void dump_arc(FILE *, cov_arc_t *a);
};


#endif /* _ggcov_cov_arc_H_ */
