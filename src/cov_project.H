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

struct cov_project_t
{
public:
    cov_project_t(const char *name, const char *basedir);
    ~cov_project_t();

    const char *name() const { return name_; }

    char *get_pathname(const char *filename) const;
    void read_file(const char *filename);
    void read_all_files();
    static cov_project_t *get(const char *name);

    static list_iterator_t<cov_project_t>
    first()
    {
	return all_.first();
    }

    list_iterator_t<cov_file_t> get_files() const
    {
	return cov_file_t::first();
    }

private:
    static list_t<cov_project_t> all_;
    string_var name_;
    string_var basedir_;
};


#endif /* _ggcov_cov_project_H_ */
