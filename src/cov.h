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

#ifndef _ggcov_cov_h_
#define _ggcov_cov_h_ 1

#include "common.h"

typedef unsigned long long  	count_t;
typedef struct cov_file_s   	cov_file_t;
typedef struct cov_function_s	cov_function_t;
typedef struct cov_arc_s	cov_arc_t;
typedef struct cov_block_s	cov_block_t;
typedef struct cov_location_s	cov_location_t;


struct cov_file_s
{
    char *name;
    GPtrArray *functions;
    GHashTable *functions_by_name;
};

struct cov_function_s
{
    char *name;
    cov_file_t *file;
    GPtrArray *blocks;
};

struct cov_arc_s
{
    cov_block_t *from, *to;
    count_t count;
    gboolean on_tree:1;
    gboolean fake:1;
    gboolean fall_through:1;
    gboolean count_valid:1;
};

struct cov_block_s
{
    cov_function_t *function;
    unsigned idx; 	    /* serial number in function */
    
    count_t count;
    gboolean count_valid:1;
    
    GList *in_arcs;
    unsigned in_ninvalid;   /* number of inbound arcs with invalid counts */
    
    GList *out_arcs;
    unsigned out_ninvalid;  /* number of outbound arcs with invalid counts */
    
    GList *locations;	    /* list of cov_location_t */
};

struct cov_location_s
{
    char *filename;
    unsigned lineno;
};


gboolean cov_handle_c_file(const char *cfilename);
cov_file_t *cov_file_find(const char *name);
const GList *cov_blocks_find_by_location(const char *filename, unsigned lineno);
void cov_file_foreach(void (*func)(cov_file_t*, void *userdata), void *userdata);

const cov_location_t *cov_function_get_first_location(const cov_function_t *fn);
const cov_location_t *cov_function_get_last_location(const cov_function_t *fn);

#endif /* _ggcov_cov_h_ */
