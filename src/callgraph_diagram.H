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

#ifndef _ggcov_callgraph_diagram_H_
#define _ggcov_callgraph_diagram_H_ 1

#include "common.h"
#include "cov.H"
#include "cov_callgraph.H"
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
    	cov_callgraph_t::node_t *callnode_;
	cov_scope_t *scope_;
	int rank_;
	unsigned int file_;
	unsigned int nup_;	/* number of incoming arcs from upper ranks */
	unsigned int ndown_;	/* number of outgoing arcs to lower ranks */
	int max_rank_;	    	/* lowest rank this node could be pushed to */
	int max_depth_;     	/* longest path to leaf through this node */
	unsigned long generation_;
	double spread_;
	double x_, y_, h_;
	gboolean on_path_;
	unsigned int shown_;
	enum flags
	{
	    HAVE_NUP = (1<<0),
	    HAVE_NDOWN = (1<<1),
	    FIXED_RANK = (1<<3)
	};
	int flags_;

    	unsigned int nup();
    	unsigned int ndown();
    	gboolean any_self();
        void push_spread_rootwards(double deltaspread);
    	void push_spread_leafwards(double deltaspread);
    	static node_t *from_callnode(cov_callgraph_t::node_t *cn)
	{
	    return (node_t *)cn->userdata;
	}
    	node_t(cov_callgraph_t::node_t *);
	~node_t();
    };

    struct rank_t
    {
	list_t<node_t> nodes_;
	double total_spread_;
    };

    static void find_roots_1(cov_callgraph_t::node_t *cn, void *userdata);
    static int compare_root_nodes(const cov_callgraph_t::node_t *, const cov_callgraph_t::node_t *);
    static void check_reached_1(cov_callgraph_t::node_t *cn, void *userdata);
    void find_roots();
    void adjust_rank(node_t *n, int delta);
    void prepare_ranks(node_t *n, int);
    void push_false_root(node_t *n);
    void balance_ranks(node_t *n);
    node_t *build_node(cov_callgraph_t::node_t*, int rank);
    void build_ranks(node_t *);
    void calc_spread(int pass, int rank);
    gboolean any_self_arcs(rank_t *r);
    void assign_geometry();
    void dump_ranks();
    static void dump_graph_1(cov_callgraph_t::node_t *cn, void *userdata);
    void dump_graph();
    void show_node(node_t *, scenegen_t *sg);

    const cov_callgraph_t &callgraph_;
    unsigned int shown_;
    list_t<cov_callgraph_t::node_t> callnode_roots_;
    list_t<node_t> roots_;
    list_t<cov_callgraph_t::node_t> disconnected_;
    ptrarray_t<rank_t> *ranks_;
    dbounds_t bounds_;
    unsigned long generation_;
    int max_rank_;
    unsigned int max_file_;
};

#endif /* _ggcov_callgraph_diagram_H_ */
