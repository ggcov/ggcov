/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2002-2004 Greg Banks <gnb@users.sourceforge.net>
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
#include "estring.H"
#include "list.H"

#if HAVE_STDINT_H
typedef uint64_t                count_t;
#define COV_COUNT_MAX           UINT64_MAX
#else
typedef unsigned long long int  count_t;
#define COV_COUNT_MAX           18446744073709551615ULL
#endif

class cov_file_t;
class cov_function_t;
class cov_block_t;
class cov_arc_t;
struct cov_callnode_t;
struct cov_callarc_t;
struct cov_read_state_t;

struct cov_location_t
{
    char *filename;
    unsigned long lineno;

    int operator==(const cov_location_t &o) const
    {
	return ((o.lineno == lineno) && !safe_strcmp(o.filename, filename));
    }
    int operator!=(const cov_location_t &o) const
    {
	return ((o.lineno != lineno) || safe_strcmp(o.filename, filename));
    }
    cov_location_t &operator++()
    {
	++lineno;
	return *this;
    }
    const char *describe() const;
};

struct cov_location_var
{
    cov_location_t loc_;

    // default c'tor
    cov_location_var()
    {
	loc_.filename = 0;
	loc_.lineno = 0;
    }
    // copy c'tor
    cov_location_var(const cov_location_var &o)
    {
	loc_.filename = (o.loc_.filename != 0 ? g_strdup(o.loc_.filename) : 0);
	loc_.lineno = o.loc_.lineno;
    }
    // c'tor from cov_location
    cov_location_var(const cov_location_t &o)
    {
	loc_.filename = (o.filename != 0 ? g_strdup(o.filename) : 0);
	loc_.lineno = o.lineno;
    }
    ~cov_location_var()
    {
	if (loc_.filename != 0)
	    g_free(loc_.filename);
    }

    void set(const char *filename, unsigned long lineno)
    {
	if (loc_.filename != 0)
	    g_free(loc_.filename);
	loc_.filename = (filename != 0 ? g_strdup(filename) : 0);
	loc_.lineno = lineno;
    }
    void set(char *filename, unsigned long lineno)
    {
	if (loc_.filename != 0)
	    g_free(loc_.filename);
	loc_.filename = filename;
	loc_.lineno = lineno;
    }
    void invalidate()
    {
	if (loc_.filename != 0)
	    g_free(loc_.filename);
	loc_.filename = 0;
	loc_.lineno = 0;
    }

    // cast to cov_location_t
    operator const cov_location_t() const
    {
	return loc_;
    }

    int operator==(const cov_location_t &o) const
    {
	return (loc_ == o);
    }
    int operator!=(const cov_location_t &o) const
    {
	return (loc_ != o);
    }

    const char *describe() const
    {
	return loc_.describe();
    }
};

struct cov
{
    enum status_t
    {
	COVERED,
	PARTCOVERED,
	UNCOVERED,
	UNINSTRUMENTED,
	SUPPRESSED
    };
    enum _constants
    {
	NUM_STATUS=5
    };

    static status_t merge(status_t a, status_t b)
    {
	if (a == b)
	    return a;
	status_t smax = MAX(a, b);
	status_t smin = MIN(a, b);
	switch (smax)
	{
	case COVERED:
	    return COVERED; /* NOTREACHED */
	case PARTCOVERED:
	case UNCOVERED:
	    return PARTCOVERED;
	case SUPPRESSED:
	    /* This would be a lot cleaner if SUPPRESSED and
	     * UNINSTRUMENTED were the other way around, but
	     * I don't have the courage to fix that right now */
	    return (smin == UNINSTRUMENTED ? SUPPRESSED : smin);
	case UNINSTRUMENTED:
	    return smin;
	}
	return a;   /* NOTREACHED */
    }
    static const char *short_name(status_t);
    static const char *long_name(status_t);

private:
    cov() { }    /* prevent instantiation */
};

class cov_stats_t
{
public:
    /*
     * c'tor zeroes out counters, rather than relying on
     * global operator new, because cov_stats_t is mostly
     * used as an auto or member variable, not allocated.
     */
    cov_stats_t()
    {
	memset(this, 0, sizeof(*this));
    }

    /* default bitwise copy c'tor is enough */

    void
    clear()
    {
	memset(this, 0, sizeof(*this));
    }

private:
    void
    accumulate_rows(const cov_stats_t *st, unsigned int firstrow, unsigned int nrows)
    {
	unsigned int i;
	unsigned int ncounts = nrows * cov::NUM_STATUS;
	unsigned long *uthis = ((unsigned long *)this) + firstrow * cov::NUM_STATUS;
	unsigned long *ust = ((unsigned long *)st) + firstrow * cov::NUM_STATUS;

	for (i = 0 ; i < ncounts ; i++)
	    *uthis++ += *ust++;
    }

public:
    void
    accumulate(const cov_stats_t *st)
    {
	accumulate_rows(st, 0, NUM_ROWS);
    }
    void
    accumulate_blocks(const cov_stats_t *st)
    {
	accumulate_rows(st, BLOCKS_ROW, 1);
    }
    void
    accumulate_lines(const cov_stats_t *st)
    {
	accumulate_rows(st, LINES_ROW, 1);
    }
    void
    accumulate_functions(const cov_stats_t *st)
    {
	accumulate_rows(st, FUNCTIONS_ROW, 1);
    }
    void
    accumulate_calls(const cov_stats_t *st)
    {
	accumulate_rows(st, CALLS_ROW, 1);
    }
    void
    accumulate_branches(const cov_stats_t *st)
    {
	accumulate_rows(st, BRANCHES_ROW, 1);
    }


    /* there are no partial blocks; blocks are the unit of coverage */
    unsigned long blocks_executed() const
    {
	return blocks_[cov::COVERED];
    }
    unsigned long blocks_total() const
    {
	return blocks_[cov::COVERED] +
	       blocks_[cov::UNCOVERED];
    }
    unsigned long blocks_suppressed() const
    {
	return blocks_[cov::SUPPRESSED];
    }
    const unsigned long *blocks_by_status() const
    {
	return blocks_;
    }
    double blocks_fraction() const
    {
	unsigned long n = blocks_executed();
	unsigned long d = blocks_total();
	return (d == 0 ? 0.0 : (double)n / (double)d);
    }
    /* for sorting objects: -1 forces uninstrumented last */
    double blocks_sort_fraction() const
    {
	unsigned long n = blocks_executed();
	unsigned long d = blocks_total();
	return (d == 0 ? -1.0 : (double)n / (double)d);
    }

    unsigned long lines_executed() const
    {
	return lines_[cov::COVERED] +
	       lines_[cov::PARTCOVERED];
    }
    unsigned long lines_full() const
    {
	return lines_[cov::COVERED];
    }
    unsigned long lines_partial() const
    {
	return lines_[cov::PARTCOVERED];
    }
    unsigned long lines_total() const
    {
	return lines_[cov::COVERED] +
	       lines_[cov::PARTCOVERED] +
	       lines_[cov::UNCOVERED];
    }
    unsigned long lines_suppressed() const
    {
	return lines_[cov::SUPPRESSED];
    }
    const unsigned long *lines_by_status() const
    {
	return lines_;
    }
    double lines_fraction() const
    {
	unsigned long n = lines_executed();
	unsigned long d = lines_total();
	return (d == 0 ? 0.0 : (double)n / (double)d);
    }
    /* for sorting objects: -1 forces uninstrumented last */
    double lines_sort_fraction() const
    {
	unsigned long n = lines_executed();
	unsigned long d = lines_total();
	return (d == 0 ? -1.0 : (double)n / (double)d);
    }

    unsigned long functions_executed() const
    {
	return functions_[cov::COVERED] +
	       functions_[cov::PARTCOVERED];
    }
    unsigned long functions_full() const
    {
	return functions_[cov::COVERED];
    }
    unsigned long functions_partial() const
    {
	return functions_[cov::PARTCOVERED];
    }
    unsigned long functions_total() const
    {
	return functions_[cov::COVERED] +
	       functions_[cov::PARTCOVERED] +
	       functions_[cov::UNCOVERED];
    }
    unsigned long functions_suppressed() const
    {
	return functions_[cov::SUPPRESSED];
    }
    const unsigned long *functions_by_status() const
    {
	return functions_;
    }
    double functions_fraction() const
    {
	unsigned long n = functions_executed();
	unsigned long d = functions_total();
	return (d == 0 ? 0.0 : (double)n / (double)d);
    }
    /* for sorting objects: -1 forces uninstrumented last */
    double functions_sort_fraction() const
    {
	unsigned long n = functions_executed();
	unsigned long d = functions_total();
	return (d == 0 ? -1.0 : (double)n / (double)d);
    }

    /* calls happen on block boundaries hence cannot be partial */
    unsigned long calls_executed() const
    {
	return calls_[cov::COVERED];
    }
    unsigned long calls_total() const
    {
	return calls_[cov::COVERED] +
	       calls_[cov::UNCOVERED];
    }
    unsigned long calls_suppressed() const
    {
	return calls_[cov::SUPPRESSED];
    }
    const unsigned long *calls_by_status() const
    {
	return calls_;
    }
    double calls_fraction() const
    {
	unsigned long n = calls_executed();
	unsigned long d = calls_total();
	return (d == 0 ? 0.0 : (double)n / (double)d);
    }
    /* for sorting objects: -1 forces uninstrumented last */
    double calls_sort_fraction() const
    {
	unsigned long n = calls_executed();
	unsigned long d = calls_total();
	return (d == 0 ? -1.0 : (double)n / (double)d);
    }


    /*
     * Branches happen on block boundaries hence cannot be partial,
     * but for storage purposes we use [PARTCOVERED]=test executed
     * but branch not taken and [COVERED]=branch taken.
     */
    unsigned long branches_executed() const
    {
	return branches_[cov::COVERED] +
	       branches_[cov::PARTCOVERED];
    }
    unsigned long branches_taken() const
    {
	return branches_[cov::COVERED];
    }
    unsigned long branches_total() const
    {
	return branches_[cov::COVERED] +
	       branches_[cov::PARTCOVERED] +
	       branches_[cov::UNCOVERED];
    }
    unsigned long branches_suppressed() const
    {
	return branches_[cov::SUPPRESSED];
    }
    const unsigned long *branches_by_status() const
    {
	return branches_;
    }
    double branches_fraction() const
    {
	unsigned long n = branches_executed();
	unsigned long d = branches_total();
	return (d == 0 ? 0.0 : (double)n / (double)d);
    }
    /* for sorting objects: -1 forces uninstrumented last */
    double branches_sort_fraction() const
    {
	unsigned long n = branches_executed();
	unsigned long d = branches_total();
	return (d == 0 ? -1.0 : (double)n / (double)d);
    }

    /* Used in calculating status for aggregated objects like functions */
    cov::status_t status_by_blocks() const
    {
	unsigned long ntot = blocks_total();

	if (!ntot)
	    return (blocks_[cov::SUPPRESSED] ? cov::SUPPRESSED : cov::UNINSTRUMENTED);
	else if (blocks_[cov::COVERED] == ntot)
	    return cov::COVERED;
	else if (blocks_[cov::UNCOVERED] == ntot)
	    return cov::UNCOVERED;
	else
	    return cov::PARTCOVERED;
    }
    cov::status_t status_by_lines() const
    {
	unsigned long ntot = lines_total();

	if (!ntot)
	    return (lines_[cov::SUPPRESSED] ? cov::SUPPRESSED : cov::UNINSTRUMENTED);
	else if (lines_[cov::COVERED] == ntot)
	    return cov::COVERED;
	else if (lines_[cov::UNCOVERED] == ntot)
	    return cov::UNCOVERED;
	else
	    return cov::PARTCOVERED;
    }

    void add_block(cov::status_t st) { blocks_[st]++; }
    void add_line(cov::status_t st) { lines_[st]++; }
    void add_function(cov::status_t st) { functions_[st]++; }
    void add_call(cov::status_t st) { calls_[st]++; }
    void add_branch(cov::status_t st) { branches_[st]++; }

    /* for testing */
    int operator==(const cov_stats_t &o) const
    {
	/* Avoid comparing uninstrumented and suppressed counts; they're
	 * deliberately not reported anywhere and can differ spuriously
	 * when cov_stats_t are created by different methods, e.g. by
	 * line vs by block. */
	return !memcmp(blocks_, o.blocks_,
		      sizeof(blocks_[0]) * (int)cov::UNINSTRUMENTED) &&
	       !memcmp(lines_, o.lines_,
		      sizeof(lines_[0]) * (int)cov::UNINSTRUMENTED) &&
	       !memcmp(functions_, o.functions_,
		      sizeof(functions_[0]) * (int)cov::UNINSTRUMENTED) &&
	       !memcmp(calls_, o.calls_,
		      sizeof(calls_[0]) * (int)cov::UNINSTRUMENTED) &&
	       !memcmp(branches_, o.branches_,
		      sizeof(branches_[0]) * (int)cov::UNINSTRUMENTED);
    }

    static void format_row_labels(const unsigned long *cc, estring &absbuf, estring &pcbuf);

    /* for testing */
    void dump(FILE *fp) const;

private:
    enum {
	BLOCKS_ROW,
	LINES_ROW,
	FUNCTIONS_ROW,
	CALLS_ROW,
	BRANCHES_ROW,
	NUM_ROWS
    };
    unsigned long blocks_[cov::NUM_STATUS];
    unsigned long lines_[cov::NUM_STATUS];
    unsigned long functions_[cov::NUM_STATUS];
    unsigned long calls_[cov::NUM_STATUS];
    unsigned long branches_[cov::NUM_STATUS];
};


#endif /* _ggcov_cov_types_H_ */
