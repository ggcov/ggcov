/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2005 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _ggcov_cov_arc_H_
#define _ggcov_cov_arc_H_ 1

#include "string_var.H"

class cov_file_t;
class cov_function_t;
class cov_block_t;

class cov_arc_t
{
public:
    const char *
    name() const
    {
	return name_;
    }

    count_t
    count() const
    {
	return count_;
    }

    unsigned int
    aindex() const
    {
	return idx_;
    }
    boolean is_call() const
    {
	return call_;
    }
    boolean
    is_fall_through() const
    {
	return fall_through_;
    }
    boolean
    is_return() const
    {
	cov_block_t *epilogue = to_->function()->blocks().tail();
	return to_ == epilogue;
    }
    const char *call_name() const
    {
	return name_;
    }
    const cov_location_t *call_location() const
    {
	/*
	 * Experiment indicates that gcc always allocates basic blocks
	 * which contain a call such that the call is the last part
	 * of the block.  Thus we can avoid storing the call location
	 * explicitly, even though we do get it from either the object
	 * file scan or the 12bp file.
	 *
	 * Actually this isn't true of calls to functions marked
	 * with attribute((pure)), which don't cause a bb break but
	 * are instead subject to optimisation within a bb.  However
	 * gcc doesn't instrument those calls, so there's point trying
	 * to handle them.
	 */
	return from_->locations().tail();
    }

    cov_block_t *
    from() const
    {
	return from_;
    }
    cov_block_t *
    to() const
    {
	return to_;
    }

    const cov_location_t *
    get_from_location() const
    {
	return from_->locations().head();
    }

    boolean is_suppressed() const
    {
	return (suppression_ != 0);
    }
    const cov_suppression_t *suppression() const
    {
	return suppression_;
    }

    cov::status_t status();

private:
    cov_arc_t();
    void attach(cov_block_t *from, cov_block_t *to);
    ~cov_arc_t();

    void suppress(const cov_suppression_t *s);

    void set_count(count_t count);
    static count_t total(const list_t<cov_arc_t> &list);
    static cov_arc_t *find_invalid(const list_t<cov_arc_t> &list,
				   gboolean may_be_call);
    void take_name(char *name); /* takes over string */

    cov_block_t *from_, *to_;
    unsigned int idx_;      /* serial number in from->out_arcs */
    count_t count_;
    string_var name_;               /* name of function called (if known) or NULL */
    boolean on_tree_:1;
    boolean call_:1;
    boolean fall_through_:1;
    boolean count_valid_:1;
    const cov_suppression_t *suppression_;

    friend class cov_file_t;
    friend class cov_function_t;
    friend class cov_block_t;
    friend void dump_arc(FILE *, cov_arc_t *a);
};


#endif /* _ggcov_cov_arc_H_ */
