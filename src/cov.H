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

#ifndef _ggcov_cov_H_
#define _ggcov_cov_H_ 1

#include "cov_types.H"
#include "cov_file.H"
#include "cov_function.H"
#include "cov_block.H"
#include "cov_arc.H"
#include "cov_callgraph.H"
#include "cov_scope.H"
#include "cov_line.H"
#include "cov_calliter.H"
#include "cov_project_params.H"
#include "fakepopt.h"

/* Call this to initialise the cov library */
void cov_init(void);

gboolean cov_is_source_filename(const char *filename);

/* Call this before reading any object or source files */
void cov_pre_read(void);
/* Add a directory to the search path for .bb, .bbg, .da, and .o files */
void cov_add_search_directory(const char *dir);
/* Read coverage data for the given source file */
gboolean cov_read_source_file(const char *cfilename);
/* Read all the coveraged source files for the given object file or executable */
gboolean cov_read_object_file(const char *exefilename);
/* Read all the coveraged source files in the given directory */
unsigned int cov_read_directory(const char *dirname, gboolean recursive);
/* Call this after reading all files; mostly just calculates callgraph */
void cov_post_read(void);

/*
 * Dump human-readable debugging description
 * of contents of cov data structures to the log.
 */
void cov_dump();

extern int cov_read_files(const cov_project_params_t &);

/*
 * Returns a new list of all unsuppressed functions
 * sorted by the function compare function.
 */
extern list_t<cov_function_t> *cov_list_all_functions();


#endif /* _ggcov_cov_H_ */
