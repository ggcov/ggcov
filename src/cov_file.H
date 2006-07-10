/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2005 Greg Banks <gnb@alphalink.com.au>
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

#ifndef _ggcov_cov_file_H_
#define _ggcov_cov_file_H_ 1

#include "common.h"
#include "list.H"
#include "hashtable.H"
#include "ptrarray.H"
#include "string_var.H"

class cov_function_t;
class cov_line_t;
class covio_t;

class cov_file_t
{
public:

    const char *
    name() const
    {
    	return name_;
    }

    /*
     * cov_file_t maintain a notion of the common prefix of
     * the names of all instances, and defines a minimal name
     * as the part after the common prefix.  The common part
     * is private, but various operations which use it are
     * defined here.
     *
     * To minimise GUI clutter usually only the minimal name
     * is displayed to the user.
     *
     * For a project containing source files in only one
     * directory, the minimal name is the basename.  Minimal
     * names are calculated on a path component basis, not
     * a character basis, so the common path for "/home/foobar"
     * and "/home/foobaz" is "/home/".
     */
    const char *minimal_name() const;
    /* returns a new string */
    static char *minimise_name(const char *name);
    /* returns a new string */
    static char *unminimise_name(const char *name);
    /* returns the common initial part of all minimal names */
    static const char *common_path();

    cov::status_t status() const
    {
    	cov_stats_t stats;
	return calc_stats(&stats);
    }
    gboolean is_suppressed() const
    {
    	return (status() == cov::SUPPRESSED);
    }
    
    /* find a file by name, full or minimal */
    static cov_file_t *find(const char *name);
    /* Return an iterator over all the files in minimal filename order */
    static list_iterator_t<cov_file_t> first();
    /*
     * Returns an MVC model which can be listened to for
     * notification of changes to the global set of files.
     */
    static void *files_model();

    /*
     * For iterating over a file's functions.  Functions are
     * numbered 0..num_functions-1
     */
    unsigned int
    num_functions() const
    {
    	return functions_->length();
    }
    cov_function_t *
    nth_function(unsigned int n) const
    {
    	return functions_->nth(n);
    }
    cov_function_t *find_function(const char *name) const;

    /*
     * Unlike everything else, lines are numbered 1..num_lines
     */
    unsigned int
    num_lines() const
    {
    	return lines_->length();
    }
    /* 
     * Never returns a NULL pointer.  If n is outside the line number
     * range or the line is not instrumented, returns a default line
     * object which has count=0 and status=UNINSTRUMENTED.
     */
    cov_line_t *nth_line(unsigned int n) const;

private:
    cov_file_t(const char *name);
    ~cov_file_t();
    
    struct format_rec_t
    {
    	const char *magic_;
	unsigned int magic_len_;
	gboolean (cov_file_t::*read_func_)(covio_t *);
	const char *description_;
    };
    static format_rec_t formats[];
    
    static void init();
    static void post_read_1(const char *, cov_file_t *, gpointer);
    static void post_read();
    static void add_name(const char *name);
    static void dirty_common_path();
    static void add_name_tramp(const char *, cov_file_t *, gpointer);
    static void check_common_path();
    cov_function_t *add_function();
    void add_location(cov_block_t *, const char *, unsigned long);
    cov_line_t *get_nth_line(unsigned int lineno);

    covio_t *try_file(const char *dir, const char *ext) const;
    covio_t *find_file(const char *ext, gboolean quiet) const;
    void file_missing(const char *ext, const char *ext2) const;

    gboolean
    gcc296_braindeath() const
    {
    	return (num_missing_fake_ > 0 && num_expected_fake_ > 0);
    }

    static void search_path_append(const char *dir);
    gboolean read(gboolean quiet);

    int skip_oldplus_func_header(covio_t *io, const char *prefix);
    gboolean read_bb_file(covio_t *);
    gboolean read_old_bbg_function(covio_t *);
    gboolean read_old_bbg_file_common(covio_t *);
    gboolean read_old_bbg_file(covio_t *);
    gboolean read_oldplus_bbg_file(covio_t *);
    gboolean read_gcc3_bbg_file_common(covio_t *io, gnb_u32_t expect_version);
    gboolean read_gcc33_bbg_file(covio_t *);
    gboolean read_gcc34l_bbg_file(covio_t *);
    gboolean read_gcc34b_bbg_file(covio_t *);
    gboolean read_bbg_file(covio_t *);
    gboolean read_old_da_file(covio_t *);
    gboolean read_oldplus_da_file(covio_t *);
    gboolean read_gcc3_da_file(covio_t *io, gnb_u32_t expect_magic);
    gboolean read_da_file(covio_t *);
    gboolean read_12bp_file(covio_t *);
#ifdef HAVE_LIBBFD
    static gboolean o_file_add_call(const cov_location_t *, const char *);
    gboolean scan_o_file_calls(covio_t *);
    gboolean read_o_file(covio_t *);
#endif
    gboolean read_src_file();
    gboolean solve();
    void finalise();

    cov::status_t calc_stats(cov_stats_t *stats) const;

    static hashtable_t<const char, cov_file_t> *files_;
    static list_t<cov_file_t> files_list_;
    static list_t<char> search_path_;
    static char *common_path_;
    static int common_len_;
    static void *files_model_;

    string_var name_;
    gboolean common_;	    /* whether this file uses common_path_ */
    gboolean finalised_;    /* data read & post-read connections made */
    gnb_u32_t format_version_;	/* file format version */
    gboolean little_endian_;	/* for gcc34 format */
    ptrarray_t<cov_function_t> *functions_;
    hashtable_t<const char, cov_function_t> *functions_by_name_;
    /* extra hashtable needed for RH-hacked gcc3.4 formats */
    hashtable_t<gnb_u64_t, cov_function_t> *functions_by_id_;
    ptrarray_t<cov_line_t> *lines_;
    cov_line_t *null_line_; /* returned for all uninstrumented lines */

    /* Fields used to detect gcc 2.96 braindeath */
    unsigned num_missing_fake_;
    unsigned num_expected_fake_;

    friend void cov_add_search_directory(const char *fname);
    friend gboolean cov_read_source_file_2(const char *fname, gboolean quiet);
    friend void cov_init(void);
    friend void cov_post_read(void);
    friend class cov_function_t;
    friend class cov_block_t;
    friend class cov_overall_scope_t;
    friend class cov_file_scope_t;
    friend class cov_file_src_parser_t;
};


#endif /* _ggcov_cov_file_H_ */
