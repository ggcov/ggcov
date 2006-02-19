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

#ifndef _ggcov_cov_block_H_
#define _ggcov_cov_block_H_ 1

#include "common.h"
#include "list.H"
#include "hashtable.H"

class cov_file_t;
class cov_function_t;
class cov_arc_t;
class cov_call_iterator_t;

class cov_block_t
{
public:
    cov::status_t status() const
    {
	cov_stats_t stats;
	return calc_stats(&stats);
    }
    gboolean is_suppressed() const
    {
    	return (status() == cov::SUPPRESSED);
    }

    cov_function_t *
    function() const
    {
    	return function_;
    }
    unsigned int
    bindex() const
    {
    	return idx_;
    }
    
    /* returns a new string */
    char *describe() const;

    /* For iterating over outgoing and incoming arcs */
    list_iterator_t<cov_arc_t>
    in_arc_iterator() const
    {
    	return in_arcs_.first();
    }
    list_iterator_t<cov_arc_t>
    out_arc_iterator() const
    {
    	return out_arcs_.first();
    }

    const cov_location_t *
    get_first_location() const
    {
    	return locations_.head();
    }
    const cov_location_t *
    get_last_location() const
    {
    	return locations_.tail();
    }
    list_iterator_t<cov_location_t>
    location_iterator() const
    {
    	return locations_.first();
    }

    count_t
    count() const
    {
    	return count_;
    }
    
    gboolean is_epilogue() const;
    
    void finalise();

private:
    struct call_t
    {
	call_t(const char *name, const cov_location_t *loc)
	 :  name_(name)
	{
	    location_.filename = g_strdup(loc->filename);
	    location_.lineno = loc->lineno;
	}
	~call_t()
	{
	    g_free(location_.filename);
	}

	string_var name_;
	cov_location_t location_;
    };

    cov_block_t();
    ~cov_block_t();
    
    static count_t total(const GList *list);
    
    void set_count(count_t);
    void add_location(const char *filename, unsigned lineno);
    gboolean is_call_site() const;
    gboolean needs_call() const;
    void add_call(const char *name, const cov_location_t *loc);
    char *pop_call();
    void suppress();
    cov::status_t calc_stats(cov_stats_t *) const;

    gboolean suppressed_;   /* suppressed from above or by self */

    cov_function_t *function_;
    unsigned int idx_; 	    /* serial number in function */
    
    count_t count_;
    gboolean count_valid_:1;
    
    list_t<cov_arc_t> in_arcs_;
    unsigned in_ninvalid_;   /* number of inbound non-call arcs with invalid counts */
    
    list_t<cov_arc_t> out_arcs_;
    unsigned out_ninvalid_;  /* number of outbound non-call arcs with invalid counts */
    unsigned int out_ncalls_;/* number of outbound call arcs */
    
    list_t<cov_location_t> locations_;
    list_t<call_t> pure_calls_;
    
    /* used while reading .o files to get arc names */
    string_var call_;
    
    friend gboolean cov_o_file_add_call(cov_read_state_t *rs,
    	    	    	    		unsigned long address,
					const char *callname);
    friend class cov_file_t;
    friend class cov_function_t;
    friend class cov_arc_t;
    friend class cov_range_scope_t;
    friend class cov_call_iterator_t;
    friend void dump_block(FILE *fp, cov_block_t *b);
};

#endif /* _ggcov_cov_block_H_ */
