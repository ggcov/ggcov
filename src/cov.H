/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001 Greg Banks <gnb@alphalink.com.au>
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

/* Call this to initialise the cov library */
void cov_init(void);

gboolean cov_is_source_filename(const char *filename);

/* Call this before reading any object or source files */
void cov_pre_read(void);
/* Read coverage data for the given source file */
gboolean cov_read_source_file(const char *cfilename);
/* Read all the coveraged source files for the given object file or executable */
gboolean cov_read_object_file(const char *exefilename);
/* Read all the coveraged source files in the given directory */
gboolean cov_read_directory(const char *exefilename);
/* Call this after reading all files; mostly just calculates callgraph */
void cov_post_read(void);

cov_status_t cov_get_count_by_location(const cov_location_t *loc,
			       count_t *countp);

void cov_overall_calc_stats(cov_stats_t *stats);
void cov_range_calc_stats(const cov_location_t *start,
    	    	    	  const cov_location_t *end, cov_stats_t *stats);

#endif /* _ggcov_cov_H_ */
