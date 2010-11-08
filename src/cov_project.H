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
#include "list.H"
#include "string_var.H"

class cov_bfd_t;

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


    list_iterator_t<cov_file_t> get_files() const
    {
	return cov_file_t::first();
    }


private:
    gboolean read_source_file_2(const char *fname, gboolean quiet);
    gboolean read_shlibs(cov_bfd_t *b, int depth);
    gboolean read_one_object_file(const char *exefilename, int depth);
    unsigned int read_directory_2(const char *dirname,
				  gboolean recursive, gboolean quiet);

    static list_t<cov_project_t> all_;
    static cov_project_t *current_;
    string_var name_;
    string_var basedir_;
};


#endif /* _ggcov_cov_project_H_ */
