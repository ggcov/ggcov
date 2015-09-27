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

#ifndef _ggcov_lego_diagram_H_
#define _ggcov_lego_diagram_H_ 1

#include "common.h"
#include "cov.H"
#include "diagram.H"

class lego_diagram_t : public diagram_t
{
public:
    lego_diagram_t();
    ~lego_diagram_t();

    const char *name();
    const char *title();
    void prepare();
    void render(scenegen_t *);
    void get_bounds(dbounds_t *db);

private:
    struct node_t
    {
	cov_file_t *file_;          /* NULL for directories */
	string_var name_;
	/* TODO: really, really need a "directory" scope object */
	cov_stats_t stats_;
	list_t<node_t> children_;
	unsigned int depth_;        /* 1 = immediate children of minimaldir */
	double x_, y_, w_, h_;      /* in [0,1] space */
	double h_cov_, h_uncov_;

	node_t() : file_(0) {}
	~node_t() {}
    };

    void show_node(node_t *, scenegen_t *);
    void dump_node(node_t *node, FILE *fp);
    void assign_geometry(node_t *, double x, double y, double w, double h);
    char *root_name() const;

    node_t *root_;
    unsigned int maxdepth_;
};

#endif /* _ggcov_lego_diagram_H_ */
