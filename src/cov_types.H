/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001 Greg Banks <gnb@alphalink.com.au>
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

#ifndef _ggcov_cov_types_H_
#define _ggcov_cov_types_H_ 1

#include "common.h"
#include "list.H"

typedef unsigned long long  	count_t;
class cov_file_t;
class cov_function_t;
class cov_block_t;
class cov_arc_t;
typedef struct cov_callnode_s	cov_callnode_t;
typedef struct cov_callarc_s	cov_callarc_t;
struct cov_read_state_t;

/*
 * TODO: have a static member hashtable which uniquifies the storage
 * of all the filenames in cov_location_t's.  Add a ctor to use it.
 */
struct cov_location_t
{
    char *filename;
    unsigned long lineno;
    
private:
    char *make_key() const;
    friend class cov_block_t;
};


struct cov_stats_t
{
    unsigned long lines;
    unsigned long lines_executed;
    unsigned long calls;
    unsigned long calls_executed;
    unsigned long branches;
    unsigned long branches_executed;
    unsigned long branches_taken;
    
    /*
     * c'tor zeroes out counters, rather than relying on
     * global operator new, because cov_stats_t is mostly
     * used as an auto or member variable, not allocated.
     */
    cov_stats_t()
    {
    	memset(this, 0, sizeof(*this));
    }
};

/*
 * Node in the callgraph, representing a function
 * which may be one of the covered functions or
 * may be an external library function.
 */
struct cov_callnode_s
{
    char *name;
    cov_function_t *function;	/* may be NULL */
    count_t count;
    GList *in_arcs, *out_arcs;
    void *userdata;
};

/*
 * Arcs between nodes in the callgraph.
 */
struct cov_callarc_s
{
    cov_callnode_t *from, *to;
    count_t count;
};

#endif /* _ggcov_cov_types_H_ */
