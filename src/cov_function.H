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

#ifndef _ggcov_cov_function_H_
#define _ggcov_cov_function_H_ 1

#include "common.h"
#include "string_var.H"

class cov_file_t;

class cov_function_t
{
public:

    const char *
    name() const
    {
    	return name_;
    }
    cov::status_t status() const
    {
    	cov_stats_t stats;
	return calc_stats(&stats);
    }
    gboolean is_suppressed() const
    {
    	return (status() == cov::SUPPRESSED);
    }
    
    cov_file_t *
    file() const
    {
    	return file_;
    }
    unsigned int
    findex() const
    {
    	return idx_;
    }

    unsigned int
    num_blocks() const
    {
	return blocks_->length();
    }
    cov_block_t *
    nth_block(unsigned int n) const
    {
    	return blocks_->nth(n);
    }

    const cov_location_t *get_first_location() const;
    const cov_location_t *get_last_location() const;

    static int compare(gconstpointer, gconstpointer);
    
    /*
     * Returns a new list of all unsuppressed functions
     * sorted by the compare function.
     */
    static GList *list_all();

private:
    cov_function_t();
    ~cov_function_t();
    
    void set_name(const char *name);
    cov_block_t *add_block();
    gboolean is_self_suppressed() const;
    void suppress();
    void finalise();
    gboolean reconcile_calls();
    gboolean solve();
    cov::status_t calc_stats(cov_stats_t *) const;

    string_var name_;
    gboolean suppressed_;   /* suppressed from above or by self */
    unsigned int idx_; 	    /* serial number in file */
    cov_file_t *file_;
    ptrarray_t<cov_block_t> *blocks_;
    
    friend class cov_file_t;
    friend class cov_function_scope_t;
};

#endif /* _ggcov_cov_function_H_ */
