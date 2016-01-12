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

#ifndef _ggcov_cov_file_H_
#define _ggcov_cov_file_H_ 1

#include "common.h"
#include "list.H"
#include "hashtable.H"
#include "ptrarray.H"
#include "string_var.H"
#include "cov_line.H"
#include "covio.H"

class cov_function_t;
class cov_suppression_t;
class cov_bfd_t;
class cov_project_params_t;

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
    /* returns a pointer to a static buffer or to it's argument */
    static const char *unminimise_name(const char *name);
    /* returns the common initial part of all minimal names */
    static const char *common_path();

    cov::status_t status() const
    {
	cov_stats_t stats;
	return calc_stats(&stats);
    }
    bool is_suppressed() const
    {
	return (suppression_ != 0);
    }

    /* find a file by name, full or minimal */
    static cov_file_t *find(const char *name);
    /* Return an iterator over all the files in minimal filename order */
    static list_iterator_t<cov_file_t> first();
    static unsigned int length();
    /*
     * Returns an MVC model which can be listened to for
     * notification of changes to the global set of files.
     */
    static void *files_model();

    static cov_line_t *find_line(const cov_location_t *loc);

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
    const ptrarray_t<cov_function_t> &
    functions() const
    {
	return *functions_;
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

    class line_iterator_t
    {
    public:
	line_iterator_t(const cov_file_t *, unsigned int);
	line_iterator_t(const line_iterator_t &o)
	 :  file_(o.file_), loc_(o.loc_) {}
	~line_iterator_t();
	line_iterator_t &operator++()
	{
	    loc_.lineno++;
	    return *this;
	}
	int operator==(const line_iterator_t &o) const
	{
	    return (file_ == o.file_ &&
		    loc_.lineno == o.loc_.lineno);
	}
	int operator!=(const line_iterator_t &o) const
	{
	    return !operator==(o);
	}
	const cov_location_t &location() const
	{
	    return loc_;
	}
	cov_line_t *line()
	{
	    return file_->nth_line(loc_.lineno);
	}

    private:
	const cov_file_t *file_;
	cov_location_t loc_;
    };

    line_iterator_t lines_begin() const;
    line_iterator_t lines_end() const;

private:
    cov_file_t(const char *name, const char *relpath);
    ~cov_file_t();

    struct format_rec_t
    {
	const char *magic_;
	unsigned int magic_len_;
	bool (cov_file_t::*read_bbg_)(covio_t *);
	bool (cov_file_t::*read_da_)(covio_t *);
	const char *description_;
    };
    static const format_rec_t formats[];

    enum feature_flags
    {
	/* From 3.4 onwards: timestamps in .gcno and .gcda files, and
	 * lengths are 4 bytes long. */
	FF_TIMESTAMP    =(1<<0),
	/* From gcc 4.0 onwards, and also in the pre-4.0 CVS snapshot
	 * that RedHat used for the FC3 compiler.  Functions are
	 * identified by 64b functionids rather than names. */
	FF_FUNCIDS      =(1<<1),
	/* From gcc 4.0 onwards.  A zero tag in the .gcda file is valid
	 * and a terminator. */
	FF_DA0TAG       =(1<<2),
	/* In the gcc 3.3 shipped in some versions of SLES.  The
	 * function name in the .gcda file is mangled, which matches
	 * the function name in the .gcno file, unlike stock gcc 3.3.  */
	FF_DAMANGLED    =(1<<3),
	/* Before gcc 3.3.  A separate .bb file contains some of the
	 * compile-time coverage information. */
	FF_BBFILE       =(1<<4),
	/* In the Fedora Core 1 compiler.  Additional information is
	 * encoded in a per-function header which we ignore, and other
	 * format changes. */
	FF_OLDPLUS      =(1<<5),
	/* From 4.7, two checksums are stored in function headers.
	 * We ignore them both. */
	FF_FNCHECKSUM2  =(1<<6),
	/* From 4.8, an extra histogram of counters is stored in the
	 * PROGRAM_SUMMARY which we ignore.  More importantly, the
	 * special exit block in a function is now block 1 rather
	 * than block N-1 */
	FF_EXITBLOCK1   =(1<<7)
    };

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
    covio_t *find_file(const char *ext, bool quiet,
		       const char *prefix) const;
    void file_missing(const char *ext, const char *ext2) const;

    bool exit_block_is_1() const { return !!(features_ & FF_EXITBLOCK1); }

    bool
    gcc296_braindeath() const
    {
	return (num_missing_fake_ > 0 && num_expected_fake_ > 0);
    }

    static void search_path_append(const char *dir);
    static void set_gcda_prefix(const char *dir);
    bool read(bool quiet);

    int skip_oldplus_func_header(covio_t *io, const char *prefix);
    bool read_bb_file(covio_t *);
    bool read_old_bbg_function(covio_t *);
    bool read_old_bbg_file_common(covio_t *);
    bool read_old_bbg_file(covio_t *);
    bool read_oldplus_bbg_file(covio_t *);
    void infer_compilation_directory(const char *abspath);
    const char *make_absolute(const char *filename) const;
    bool read_gcc3_bbg_file(covio_t *io, uint32_t expect_version,
				covio_t::format_t);
    bool read_gcc33_bbg_file(covio_t *);
    bool read_gcc34l_bbg_file(covio_t *);
    bool read_gcc34b_bbg_file(covio_t *);
    bool read_bbg_file(covio_t *);
    bool read_old_da_file(covio_t *);
    bool read_oldplus_da_file(covio_t *);
    bool read_gcc3_da_file(covio_t *io, uint32_t expect_magic,
			       covio_t::format_t);
    bool read_gcc33_da_file(covio_t *);
    bool read_gcc34l_da_file(covio_t *);
    bool read_gcc34b_da_file(covio_t *);
    bool read_da_file(covio_t *);
    bool read_rtl_file(covio_t *);
    bool discover_format(covio_t *io);
#ifdef HAVE_LIBBFD
    bool o_file_add_call(cov_location_t, const char *);
    bool scan_o_file_calls(cov_bfd_t *);
    void scan_o_file_linkage(cov_bfd_t *);
    bool read_o_file(covio_t *);
#endif
    bool read_src_file();
    void zero_arc_counts();
    bool solve();
    void suppress(const cov_suppression_t *);
    void finalise();

    cov::status_t calc_stats(cov_stats_t *stats) const;

    static hashtable_t<const char, cov_file_t> *files_;
    static list_t<cov_file_t> files_list_;
    static list_t<char> search_path_;
    static string_var gcda_prefix_;
    static char *common_path_;
    static int common_len_;
    static void *files_model_;

    string_var name_;       /* full absolute pathname of this file */
    string_var relpath_;    /* relative path with which this file was found */
    string_var compiledir_; /* directory in which this source was compiled;
			     * base for relative pathnames */
    const cov_suppression_t *suppression_;
    bool finalised_;    /* data read & post-read connections made */
    bool has_locations_;/* any locations added */
    const format_rec_t *format_;
    uint32_t format_version_;  /* file format version */
    uint32_t features_;        /* FF_* flags */
    ptrarray_t<cov_function_t> *functions_;
    hashtable_t<const char, cov_function_t> *functions_by_name_;
    /* extra hashtable needed for RH-hacked gcc3.4 formats */
    hashtable_t<uint64_t, cov_function_t> *functions_by_id_;
    ptrarray_t<cov_line_t> *lines_;
    cov_line_t *null_line_; /* returned for all uninstrumented lines */

    /* Fields used to detect gcc 2.96 braindeath */
    unsigned num_missing_fake_;
    unsigned num_expected_fake_;

    friend void cov_add_search_directory(const char *fname);
    friend int cov_read_files(const cov_project_params_t &params);
    friend bool cov_read_source_file_2(const char *fname, bool quiet);
    friend void cov_init(void);
    friend void cov_post_read(void);
    friend class cov_function_t;
    friend class cov_block_t;
    friend class cov_overall_scope_t;
    friend class cov_file_scope_t;
    friend class cov_file_src_parser_t;
};

class cov_file_annotator_t
{
public:
    cov_file_annotator_t(cov_file_t *);
    ~cov_file_annotator_t();

    // TODO: really should throw an exception in the ctor
    bool is_valid() const { return (fp_ != 0); }
    bool next();

    unsigned long lineno() const { return lineno_; }
    cov::status_t status() const { return ln_->status(); }
    count_t count() const { return ln_->count(); }
    const char *count_as_string() const;
    void format_blocks(char *buf, size_t maxlen) const { ln_->format_blocks(buf, maxlen); }
    const char *blocks_as_string() const;
    const char *text() const { return buf_; }
    cov_function_t *function() const { return ln_->function(); }

private:
    cov_file_t *file_;
    FILE *fp_;
    unsigned long lineno_;
    cov_line_t *ln_;
    char buf_[1024];
};

#endif /* _ggcov_cov_file_H_ */
