/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>
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
#include "list.H"

class cov_function_t;
struct cov_callarc_t;
struct cov_callnode_t;
struct cov_callspace_t;


typedef hashtable_iter_t<const char, cov_callspace_t> cov_callspace_iter_t;
typedef hashtable_iter_t<const char, cov_callnode_t> cov_callnode_iter_t;

/*
 * Wrapper class for the entire callgraph.
 */
class cov_callgraph_t
{
public:
    cov_callgraph_t();
    ~cov_callgraph_t();

    void add_nodes(cov_file_t*);
    void add_arcs(cov_file_t*);

    cov_callspace_iter_t first() const;

    cov_callnode_t *default_node() const { return default_node_; }

private:
    cov_callspace_t *get_file_space(cov_file_t *f);
    static gboolean delete_one(const char *name, cov_callspace_t *cs, gpointer userdata);

    cov_callspace_t *global_;
    hashtable_t<const char, cov_callspace_t> *files_;
    cov_callnode_t *default_node_;
};

/*
 * Name space for nodes in the call graph; needed to disambiguate
 * multiple functions with the same name but different link scopes, e.g.
 * static functions in each of two different compile units.
 */
class cov_callspace_t
{
public:
    cov_callspace_t(const char *name);
    ~cov_callspace_t();

    const char *name() const { return name_; }

    cov_callnode_t *add(cov_callnode_t *cn);
    cov_callnode_t *remove(cov_callnode_t *cn);
    cov_callnode_t *find(const char *name) const;
    cov_callnode_iter_t first() const;

private:
    static gboolean delete_one(const char *name, cov_callnode_t *cn, gpointer userdata);
    void delete_all(void);

    string_var name_;
    hashtable_t<const char, cov_callnode_t> *nodes_;
};

/*
 * Node in the callgraph, representing a function
 * which may be one of the covered functions or
 * may be an external library function.
 */
struct cov_callnode_t
{
    cov_callnode_t(const char *name);
    ~cov_callnode_t();

    string_var name;
    cov_function_t *function;   /* may be NULL */
    count_t count;
    list_t<cov_callarc_t> in_arcs, out_arcs;
    void *userdata;

    cov_callarc_t *find_arc_to(cov_callnode_t *to) const;

    static int compare_by_name(const cov_callnode_t **a, const cov_callnode_t **b);
    static int compare_by_name_and_file(const cov_callnode_t *cna, const cov_callnode_t *cnb);

    const char *unambiguous_name() const
    {
	return function ? function->unambiguous_name() : name.data();
    }
};

/*
 * Arcs between nodes in the callgraph.
 */
struct cov_callarc_t
{
    cov_callarc_t(cov_callnode_t *from, cov_callnode_t *to);
    ~cov_callarc_t();

    cov_callnode_t *from, *to;
    count_t count;

    void add_count(count_t);

    static int compare_by_count_and_from(const cov_callarc_t *ca1, const cov_callarc_t *ca2);
    static int compare_by_count_and_to(const cov_callarc_t *ca1, const cov_callarc_t *ca2);
};


#endif /* _ggcov_cov_callgraph_H_ */
