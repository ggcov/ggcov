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

#include "covio.h"
#include "estring.h"

CVSID("$Id: covio.c,v 1.1 2001-11-23 03:47:48 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

    /* saved as 32 bit little-endian */
gboolean
covio_read_u32(FILE *fp, covio_u32_t *wp)
{
    covio_u32_t w;
    
    w = (unsigned int)fgetc(fp) & 0xff;
    w |= ((unsigned int)fgetc(fp) & 0xff) << 8;
    w |= ((unsigned int)fgetc(fp) & 0xff) << 16;
    w |= ((unsigned int)fgetc(fp) & 0xff) << 24;
    
    *wp = w;

    return (!feof(fp));
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

    /* saved as 64 bit little-endian */
gboolean
covio_read_u64(FILE *fp, covio_u64_t *wp)
{
    covio_u64_t w;
    
    w = (covio_u64_t)fgetc(fp) & 0xff;
    w |= ((covio_u64_t)fgetc(fp) & 0xff) << 8;
    w |= ((covio_u64_t)fgetc(fp) & 0xff) << 16;
    w |= ((covio_u64_t)fgetc(fp) & 0xff) << 24;
    
    w |= ((covio_u64_t)fgetc(fp) & 0xff) << 32;
    w |= ((covio_u64_t)fgetc(fp) & 0xff) << 40;
    w |= ((covio_u64_t)fgetc(fp) & 0xff) << 48;
    w |= ((covio_u64_t)fgetc(fp) & 0xff) << 56;

    *wp = w;

    return (!feof(fp));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
covio_read_bbstring(FILE *fp, covio_u32_t endtag)
{
    covio_u32_t t;
    estring buf;
    
    estring_init(&buf);
    
    while (covio_read_u32(fp, &t))
    {
    	if (t == endtag)
	    break;
	    
	/* pick apart tag as chars and add them to the buf */
	estring_append_char(&buf, (t & 0xff));
	estring_append_char(&buf, ((t>>8) & 0xff));
	estring_append_char(&buf, ((t>>16) & 0xff));
	estring_append_char(&buf, ((t>>24) & 0xff));
    }
    
    return buf.data;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
