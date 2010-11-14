/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2004 Greg Banks <gnb@users.sourceforge.net>
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
#include "hashtable.H"

class cov_function_t;

class cov_callgraph_t
{
public:
    cov_callgraph_t();
    ~cov_callgraph_t();

    struct arc_t;

    /*
     * Node in the callgraph, representing a function
     * which may be one of the covered functions or
     * may be an external library function.
     */
    struct node_t
    {
	string_var name;
	cov_function_t *function;	/* may be NULL */
	count_t count;
	GList *in_arcs, *out_arcs;
	void *userdata;

    private:
	node_t(const char *name);
	~node_t();

	arc_t *find_arc_to(node_t *to) const;

	friend class cov_callgraph_t;
    };

    /*
     * Arcs between nodes in the callgraph.
     */
    struct arc_t
    {
	node_t *from, *to;
	count_t count;

    private:
	arc_t(node_t *from, node_t *to);
	~arc_t();

	void add_count(count_t);

	friend class cov_callgraph_t;
    };

    node_t *add_node(const char *);
    node_t *find_node(const char *name) const;
    void foreach_node(void (*func)(node_t*, void *userdata), void *userdata) const;

    void add_nodes(cov_file_t*);
    void add_arcs(cov_file_t*);

private:
    hashtable_t<const char, node_t> *nodes_;
};

#endif /* _ggcov_cov_callgraph_H_ */
