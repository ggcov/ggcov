/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2015 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _ggcov_filerec_H_
#define _ggcov_filerec_H_ 1

#include "common.h"
#include "cov.H"
#include "list.H"
#include "logging.H"

/*
 * file_rec_t is used to build a tree of coverage stats
 * including aggregated stats for directories, for use
 * in the GUI files window and when generating static HTML.
 */

class file_rec_t
{
public:
    file_rec_t();
    file_rec_t(const char *nm, cov_file_t *f);
    ~file_rec_t();

    bool is_file() const { return !!file_; }
    bool is_directory() const { return !is_file(); }

    file_rec_t *find_child(const char *name) const;

    void add_child(file_rec_t *child);
    file_rec_t *add_descendent(cov_file_t *f);
    void add_descendents(list_iterator_t<cov_file_t> iter);

    void set_client_data(void *cd) { client_data_ = cd; }
    void *get_client_data() const { return client_data_; }

    const char *get_name() const { return name_; }
    cov_file_t *get_file() const { return file_; }
    cov_scope_t *get_scope() const { return scope_; }
    list_iterator_t<file_rec_t> first_child() const { return children_.first(); }

    void dump(int indent, logging::logger_t &);

private:
    string_var name_;            /* partial name */
    cov_file_t *file_;
    cov_scope_t *scope_;
    void *client_data_;
    list_t<file_rec_t> children_;
    /* directory file_rec_t's have children and no file */
};

#endif /* _ggcov_filerec_H_ */
