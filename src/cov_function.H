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

#ifndef _ggcov_cov_function_H_
#define _ggcov_cov_function_H_ 1

#include "common.h"
#include "cached_string.H"
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
    /* Returns a pointer to a static buffer which will
     * be overwritten on the next call. */
    const char *unambiguous_name() const;

    cov::status_t status() const
    {
	cov_stats_t stats;
	return calc_stats(&stats);
    }
    gboolean is_suppressed() const
    {
	return (suppression_ != 0);
    }
    const cov_suppression_t *suppression() const
    {
        return suppression_;
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

    enum linkage_t
    {
	UNKNOWN, LOCAL, GLOBAL
    };
    linkage_t linkage() const
    {
	return linkage_;
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
    ptrarray_t<cov_block_t> &
    blocks() const
    {
	return *blocks_;
    }

    const cov_location_t *get_first_location() const;
    const cov_location_t *get_last_location() const;

    unsigned long get_first_line() const
    {
        const cov_location_t *loc = get_first_location();
        return loc ? loc->lineno : 0;
    }
    unsigned long get_last_line() const
    {
        const cov_location_t *loc = get_last_location();
        return loc ? loc->lineno : 0;
    }

    static int compare(const cov_function_t*, const cov_function_t*);

    void zero_arc_counts();

    /*
     * Each function has two special blocks, one of which is there so
     * that arcs from it can represent calls to the function and one of
     * returns from the function.  Since the dawn of time, the entry
     * block was block 0 (the very first block) and the exit block was
     * block N-1 (the very last block).  Since gcc 4.8 the exit block is
     * now block 1 (the second block).  These functions hide that.
     */
    unsigned int entry_block() const;
    unsigned int exit_block() const;
    unsigned int first_real_block() const;
    unsigned int last_real_block() const;

    cov::status_t calc_stats(cov_stats_t *) const;

    void set_dup_count(unsigned int v) { dup_count_ = v; }
    unsigned int dup_count() const { return dup_count_; }

private:
    cov_function_t();
    ~cov_function_t();

    void set_name(const char *name);
    void set_id(uint64_t); /* needed for gcc 4.0 and hacked-up RH gcc 3.4 file formats */
    cov_block_t *add_block();
    void suppress(const cov_suppression_t *);
    void finalise();
    gboolean reconcile_calls();
    gboolean solve();
    void set_linkage(linkage_t ll)
    {
	linkage_ = ll;
    }

    cached_string name_;
    uint64_t id_;
    unsigned int idx_;      /* serial number in file */
    cov_file_t *file_;
    linkage_t linkage_;
    const cov_suppression_t *suppression_;
    ptrarray_t<cov_block_t> *blocks_;
    unsigned int dup_count_;

    friend class cov_file_t;
    friend class cov_function_scope_t;
};

#endif /* _ggcov_cov_function_H_ */
