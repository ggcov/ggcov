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
typedef struct cov_stats_s	cov_stats_t;
typedef struct cov_callnode_s	cov_callnode_t;
typedef struct cov_callarc_s	cov_callarc_t;


struct cov_file_s
{
    char *name;
    GPtrArray *functions;
    GHashTable *functions_by_name;
};

struct cov_function_s
{
    char *name;
    unsigned idx; 	    /* serial number in file */
    cov_file_t *file;
    GPtrArray *blocks;
};

struct cov_arc_s
{
    cov_block_t *from, *to;
    unsigned idx; 	    /* serial number in from->out_arcs */
    count_t count;
    char *name;     	    /* name of function called (if known) or NULL */
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
    
    /* used while reading .o files to get arc names */
    GList *calls;
};

struct cov_location_s
{
    char *filename;
    unsigned long lineno;
};


struct cov_stats_s
{
    unsigned long lines;
    unsigned long lines_executed;
    unsigned long calls;
    unsigned long calls_executed;
    unsigned long branches;
    unsigned long branches_executed;
    unsigned long branches_taken;
};

/*
 * Node in the callgraph, representing a function
 * which may be one of the covered functions or
 * may be an external library function.
 */
struct cov_callnode_s
{
    char *name;
    cov_function_t *function;	/* may be NULL */
    count_t count;
    GList *in_arcs, *out_arcs;
};

/*
 * Arcs between nodes in the callgraph.
 */
struct cov_callarc_s
{
    cov_callnode_t *from, *to;
    count_t count;
};

#define cov_file_nth_function(f, n) \
    	    ((cov_function_t *)(f)->functions->pdata[(n)])
#define cov_function_nth_block(fn, n) \
    	    ((cov_block_t *)(fn)->blocks->pdata[(n)])

void cov_init(void);
gboolean cov_handle_c_file(const char *cfilename);
/* mostly just calculates callgraph */
void cov_post_read(void);
cov_file_t *cov_file_find(const char *name);
void cov_get_count_by_location(const cov_location_t *loc,
			       count_t *countp, gboolean *existsp);
const GList *cov_blocks_find_by_location(const cov_location_t *loc);
void cov_file_foreach(void (*func)(cov_file_t*, void *userdata), void *userdata);

const cov_location_t *cov_function_get_first_location(const cov_function_t *fn);
const cov_location_t *cov_function_get_last_location(const cov_function_t *fn);
const cov_location_t *cov_file_get_last_location(const cov_file_t *f);

#define cov_stats_init(sp)  memset((sp), 0, sizeof(cov_stats_t))
void cov_function_calc_stats(const cov_function_t *, cov_stats_t *);
void cov_file_calc_stats(const cov_file_t *f, cov_stats_t *stats);
void cov_overall_calc_stats(cov_stats_t *stats);
void cov_range_calc_stats(const cov_location_t *start,
    	    	    	  const cov_location_t *end, cov_stats_t *stats);

cov_callnode_t *cov_callnode_find(const char *name);
void cov_callnode_foreach(void (*func)(cov_callnode_t*, void *userdata), void *userdata);

#endif /* _ggcov_cov_h_ */
