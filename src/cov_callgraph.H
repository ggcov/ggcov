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

#ifndef _ggcov_cov_callgraph_H_
#define _ggcov_cov_callgraph_H_ 1

#include "string_var.H"

class cov_function_t;

/*
 * Node in the callgraph, representing a function
 * which may be one of the covered functions or
 * may be an external library function.
 */
struct cov_callnode_t
{
    string_var name;
    cov_function_t *function;	/* may be NULL */
    count_t count;
    GList *in_arcs, *out_arcs;
    void *userdata;
    
    static cov_callnode_t *find(const char *name);
    static void foreach(void (*func)(cov_callnode_t*, void *userdata),
    	    	        void *userdata);

private:
    cov_callnode_t(const char *name);
    ~cov_callnode_t();
    
    static void init(void);

    cov_callarc_t *find_arc_to(cov_callnode_t *to) const;

    static GHashTable *all_;
    
    friend void cov_init(void);
    friend void cov_add_callnodes(cov_file_t*);
    friend void cov_add_callarcs(cov_file_t*);
};

/*
 * Arcs between nodes in the callgraph.
 */
struct cov_callarc_t
{
    cov_callnode_t *from, *to;
    count_t count;
    
private:
    cov_callarc_t(cov_callnode_t *from, cov_callnode_t *to);
    ~cov_callarc_t();

    void add_count(count_t);

    friend void cov_add_callarcs(cov_file_t*);
};


#endif /* _ggcov_cov_callgraph_H_ */
