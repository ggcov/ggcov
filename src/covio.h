/*
 * CANT - A C implementation of the Apache/Tomcat ANT build system
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

#ifndef _ggcov_covio_h_
#define _ggcov_covio_h_ 1

#include "common.h"

/* TODO: use uint32 where available */
typedef unsigned long	    	covio_u32_t;
#define COVIO_U32_DFMT	    	"%ld"
#define COVIO_U32_XFMT	    	"%08lx"
typedef unsigned long long  	covio_u64_t;
#define COVIO_U64_DFMT	    	"%lld"
#define COVIO_U64_XFMT	    	"%016llx"

/* These functions return TRUE unless EOF */
gboolean covio_read_u32(FILE *fp, covio_u32_t*);
gboolean covio_read_u64(FILE *fp, covio_u64_t*);
/* Returns a new string */
char *covio_read_bbstring(FILE *fp, covio_u32_t endtag);

#endif /* _ggcov_covio_h_ */
