/*
 * CANT - A C implementation of the Apache/Tomcat ANT build system
 * Copyright (c) 2001-2003 Greg Banks <gnb@alphalink.com.au>
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

/*
 * There are twice as many functions as necessary because the
 * lowest level formatting changed at around gcc 3.3.  Integral
 * data are now saved bigendian, and string data grew a preceeding
 * length field.  The format now looks a *lot* like XDR!
 */

/* These functions return TRUE unless EOF */
gboolean covio_read_lu32(FILE *fp, gnb_u32_t*); /* old format */
gboolean covio_read_lu64(FILE *fp, gnb_u64_t*); /* old format */
gboolean covio_read_bu32(FILE *fp, gnb_u32_t*); /* new format */
gboolean covio_read_bu64(FILE *fp, gnb_u64_t*); /* new format */
/* Returns a new string */
char *covio_read_bbstring(FILE *fp, gnb_u32_t endtag);	/* old format */
char *covio_read_string(FILE *fp);	    	    	/* new format */
char *covio_read_lstring(FILE *fp);			/* like new format but little-endian */

#endif /* _ggcov_covio_h_ */
