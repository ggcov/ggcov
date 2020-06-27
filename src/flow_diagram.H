/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2006-2020 Greg Banks <gnb@fastmail.fm>
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

#ifndef _ggcov_flow_diagram_H_
#define _ggcov_flow_diagram_H_ 1

#include "common.h"
#include "cov.H"
#include "diagram.H"
#include "ptrarray.H"

class flow_diagram_t : public diagram_t
{
public:
    flow_diagram_t(cov_function_t *);
    ~flow_diagram_t();

    const char *name();
    const char *title();
    void prepare();
    void render(scenegen_t *);
    void get_bounds(dbounds_t *db);

private:
    enum arc_case_t
    {
	AC_DOWN1,   /* arc down from one line to the next */
	AC_UP1,     /* arc up from one line to the previous */
	AC_ACROSS1, /* arc from one rank to the next */
	AC_DOWN,    /* more general arc downwards */
	AC_UP,      /* more general arc upwards */
	AC_ACROSS   /* arc between vertically overlapping nodes **/
    };

    struct node_t
    {
	cov_block_t *block_;
	node_t *first_;             /* first node_t for this block, holds rank_ */
	node_t *next_, *prev_;      /* circular list of node_t for this block */
	int first_idx_, last_idx_;
	int rank_;
	double x_, y_, w_, h_;      /* in [0,1] space */

	gboolean
	have_geometry() const
	{
	    return (first_->rank_ >= 0 && first_idx_ >= 0);
	}

	double topy() const { return y_; }
	double midy() const { return y_ + h_ / 2.0; }
	double bottomy() const { return y_ + h_; }

	double leftx() const { return x_; }
	double midx() const { return x_ + w_ / 2.0; }
	double rightx() const { return x_ + w_; }

	double slotx(int slot) const;

	node_t(cov_block_t *b) : block_(b), first_idx_(-1), rank_(-1) {}
	~node_t() {}
    };
    struct arc_t
    {
	cov_arc_t *arc_;
	node_t *from_, *to_;
	int first_, last_;
	arc_case_t case_;
	// slot index, -ve for left slots +ve for right slots,
	// zero means no slot allocated
	int slot_;

	/* length of arc slot needed, 0=doesn't need slot */
	int
	slot_needed() const
	{
	    return last_ - first_;
	}
	static int compare(const arc_t **, const arc_t **);

	double overlap_x() const;
	double direct_fromx() const;
	double direct_tox() const;

	double overlap_y() const;
	double direct_fromy() const;
	double direct_toy() const;
    };

    unsigned int generate_nodes();

    int max_rank_of_line(unsigned int idx);
    void assign_ranks();

    gboolean no_intervening_nodes(node_t *, node_t *) const;
    arc_case_t classify_arc(node_t *from, node_t *to) const;
    void add_arc(cov_arc_t *a, node_t *from, node_t *to);
    void generate_arcs();

    void slot_distance(node_t *, int, int *) const;
    int choose_side(arc_t *) const;
    void allocate_arc_slots();

    void assign_geometry();

    void show_debug_grid(scenegen_t *sg);
    double slotx(int slot) const;
    void show_arc(arc_t *, scenegen_t *);
    void show_node(node_t *, scenegen_t *);

    list_t<node_t> nodes_;
    list_t<node_t> *nodes_by_line_;
    unsigned int num_lines_;
    ptrarray_t<node_t> *nodes_by_bindex_;   /* first node for block, by block index */
    int max_rank_;
    ptrarray_t<arc_t> *arcs_;
    unsigned int num_slots_[2]; /* [0] left [1] right */
    /*
     * [0] left edge of whole diagram
     * [1] left edge of left arc slots area
     * [2] right edge of left arc slots area
     * [3] left edge of nodes area
     * [4] right edge of nodes area
     * [5] left edge of right arc slots area
     * [6] right edge of right arc slots area
     * [7] right edge of whole diagram
     */
    double xpos_[8];
    /*
     * [0] top edge of whole diagram
     * [1] top edge of nodes area, left & right arc slots areas
     * [2] bottom edge of nodes area, left & right arc slots areas
     * [3] bottom edge of whole diagram
     */
    double ypos_[4];
    cov_function_t *function_;
    string_var title_;
};

#endif /* _ggcov_flow_diagram_H_ */
