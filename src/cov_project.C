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

#include "cov.H"
#include "filename.h"

list_t<cov_project_t> cov_project_t::all_;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_project_t::cov_project_t(const char *name, const char *basedir)
 :  name_(name),
    basedir_(basedir)
{
    all_.append(this);
}

cov_project_t::~cov_project_t()
{
    all_.remove(this);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
cov_project_t::get_pathname(const char *filename) const
{
    // TODO: ensure @filename is relative and does not escape from basedir
    return file_make_absolute_to_dir(filename, basedir_);
}

void
cov_project_t::read_file(const char *filename)
{
    string_var path = get_pathname(filename);
    GList *files = g_list_append(NULL, (gpointer)path.data());
    cov_read_files(files);
    listclear(files);
}

void
cov_project_t::read_all_files()
{
    /* Awful hack to set the global `recursive' flag */
    for (int i = 0 ; cov_popt_options[i].shortName ; i++)
    {
	if (cov_popt_options[i].shortName == 'r')
	{
	    *(int *)cov_popt_options[i].arg = 1;
	    break;
	}
    }

    GList *files = g_list_append(NULL, (gpointer)basedir_.data());
    cov_read_files(files);
    listclear(files);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_project_t *
cov_project_t::get(const char *name)
{
    if (!name || !*name)
	return 0;

    list_iterator_t<cov_project_t> itr;
    for (itr = all_.first() ; itr != (cov_project_t *)0 ; ++itr)
    {
	if (!strcmp((*itr)->name_, name))
	    return *itr;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
