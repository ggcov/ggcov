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

private:
    cov_arc_t(cov_block_t *from, cov_block_t *to);
    ~cov_arc_t();

    void set_count(count_t count);
    static count_t total(const list_t<cov_arc_t> &list);
#ifdef HAVE_BBG_FAKE_FLAG
    static unsigned int nfake(const list_t<cov_arc_t> &list);
#endif
    static unsigned int ncalls(const list_t<cov_arc_t> &list);
    static cov_arc_t *find_invalid(const list_t<cov_arc_t> &list);

    cov_block_t *from_, *to_;
    unsigned int idx_; 	    /* serial number in from->out_arcs */
    count_t count_;
    string_var name_;     	    /* name of function called (if known) or NULL */
    gboolean on_tree_:1;
#ifdef HAVE_BBG_FAKE_FLAG
    gboolean fake_:1;
#endif
    gboolean fall_through_:1;
    gboolean count_valid_:1;
    
    friend class cov_file_t;
    friend class cov_function_t;
    friend class cov_block_t;
    friend void dump_arc(cov_arc_t *a);
#ifdef HAVE_BBG_FAKE_FLAG
    friend void cov_check_fakeness(cov_file_t *f);
#endif
};


#endif /* _ggcov_cov_arc_H_ */
