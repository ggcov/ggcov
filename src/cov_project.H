/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2010 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _ggcov_cov_project_H_
#define _ggcov_cov_project_H_ 1

#include "common.h"
#include "cov_file.H"
#include "cov_callgraph.H"
#include "list.H"
#include "hashtable.H"
#include "ptrarray.H"
#include "string_var.H"

class cov_bfd_t;
class covio_t;

/*
 * Note that the project is an MVC model which can be listened
 * to for notification of changes to its set of files.
 */

struct cov_project_t
{
public:
    cov_project_t(const char *name, const char *basedir);
    ~cov_project_t();

    const char *name() const { return name_; }
    time_t mtime() const;
    const char *description() const { return description_; }

    static cov_project_t *current();
    void make_current();
    static cov_project_t *get(const char *name);
    static list_iterator_t<cov_project_t>
    first()
    {
	return all_.first();
    }

    /* Call this before reading any files */
    void pre_read(void);
    /* Call this after reading all files; mostly just calculates callgraph */
    void post_read(void);

    /*
     * The project maintains a notion of the common prefix of
     * the names of all files, and defines a minimal name
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
    const char *minimal_name(const char *);
    /* returns a new string */
    char *minimise_name(const char *name);
    /* returns the common initial part of all minimal names */
    const char *common_path();

    /* find a file by name, full or minimal */
    cov_file_t *find_file(const char *name);
    /* Return an iterator over all the files in minimal filename order */
    list_iterator_t<cov_file_t> first_file() const
    {
	return files_list_.first();
    }
    void attach_file(cov_file_t *f);
    void detach_file(cov_file_t *f);

    /* Add a directory to the search path for .bb, .bbg, .da, and .o files */
    void add_search_directory(const char *dir);
    covio_t *find_file(const char *name, const char *ext) const;
    void file_missing(const char *name, const char *ext, const char *ext2) const;

    /* Read coverage data for the given source file */
    gboolean read_source_file(const char *filename);
    /* Read all the coveraged source files for the given object file or executable */
    gboolean read_object_file(const char *exefilename);
    /* Read all the coveraged source files in the given directory */
    unsigned int read_directory(const char *dirname, gboolean recursive);
    /* Read whatever coverage data can be gleaned from the
     * filename, which will be automatically detected as a
     * source file, object file, or directory.  */
    gboolean read_file(const char *filename, gboolean recursive);
    /* Read a list of files, detecting types */
    gboolean read_files(GList *files, gboolean recursive);
    /* Read all files under the basedir, detecting types, recursively */
    gboolean read_all_files();

    char *get_pathname(const char *filename) const;

    /*
     * Returns a new list of all unsuppressed functions
     * sorted by the compare function.
     */
    /* TODO: this function should return an iterator, NOT a new list */
    list_t<cov_function_t> *list_all_functions() const;

    const cov_callgraph_t &callgraph() const
    {
	return callgraph_;
    }

private:
    void read_description();

    static void post_read_1(const char *, cov_file_t *, gpointer);
    void add_name(const char *name);
    void dirty_common_path();
    static void add_name_tramp(const char *, cov_file_t *, gpointer);
    void check_common_path();
    /* returns a new string */
    char *unminimise_name(const char *name);

    gboolean read_source_file_2(const char *fname, gboolean quiet);
    gboolean read_shlibs(cov_bfd_t *b, int depth);
    gboolean read_one_object_file(const char *exefilename, int depth);
    unsigned int read_directory_2(const char *dirname,
				  gboolean recursive, gboolean quiet);

    static list_t<cov_project_t> all_;
    static cov_project_t *current_;
    string_var name_;
    string_var basedir_;
    string_var description_;

    /* set of files */
    hashtable_t<const char, cov_file_t> *files_;
    list_t<cov_file_t> files_list_;
    list_t<char> search_path_;
    char *common_path_;
    int common_len_;

    cov_callgraph_t callgraph_;
};


#endif /* _ggcov_cov_project_H_ */
