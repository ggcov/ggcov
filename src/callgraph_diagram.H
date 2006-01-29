/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2005 Greg Banks <gnb@alphalink.com.au>
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

#ifndef _ggcov_callgraph_diagram_H_
#define _ggcov_callgraph_diagram_H_ 1

#include "common.h"
#include "cov.H"
#include "diagram.H"
#include "ptrarray.H"
#include "list.H"

class callgraph_diagram_t : public diagram_t
{
public:
    callgraph_diagram_t();
    ~callgraph_diagram_t();

    const char *name();
    const char *title();
    void prepare();
    void render(scenegen_t *);
    void get_bounds(dbounds_t *db);

private:
    struct node_t
    {
    	cov_callnode_t *callnode_;
	cov_scope_t *scope_;
	unsigned int rank_;
	unsigned int file_;
	unsigned long generation_;
	unsigned int spread_;
	double x_, y_;
	gboolean on_path_;
	gboolean have_geom_;
	gboolean shown_;

    	static node_t *from_callnode(cov_callnode_t *cn)
	{
	    return (node_t *)cn->userdata;
	}
    	node_t(cov_callnode_t *);
	~node_t();
    };

    struct rank_t
    {
	list_t<node_t> nodes_;
    };

    static void find_roots_1(cov_callnode_t *cn, void *userdata);
    static int compare_root_nodes(const cov_callnode_t *, const cov_callnode_t *);
    void find_roots();
    void adjust_rank(node_t *n, int delta);
    node_t *build_node(cov_callnode_t*, int rank);
    void add_spread(node_t *n);
    void build_ranks(node_t *);
    void assign_geometry(node_t *n, double ystart, double yend);
    void dump_ranks();
    static void dump_graph_1(cov_callnode_t *cn, void *userdata);
    void dump_graph();
    void show_node(node_t *, scenegen_t *sg);

    list_t<cov_callnode_t> callnode_roots_;
    list_t<node_t> roots_;
    list_t<cov_callnode_t> disconnected_;
    ptrarray_t<rank_t> *ranks_;
    dbounds_t bounds_;
    unsigned long generation_;
    unsigned int max_file_;
};

#endif /* _ggcov_callgraph_diagram_H_ */
