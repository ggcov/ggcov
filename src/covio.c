/*
 * ggcov - A GTK frontend for exploring gcov coverage data
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

#include "covio.h"
#include "estring.H"

CVSID("$Id: covio.c,v 1.5 2003-06-28 10:33:42 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

    /* old format: saved as 32 bit little-endian */
gboolean
covio_read_lu32(FILE *fp, gnb_u32_t *wp)
{
    gnb_u32_t w;
    
    w = (unsigned int)fgetc(fp) & 0xff;
    w |= ((unsigned int)fgetc(fp) & 0xff) << 8;
    w |= ((unsigned int)fgetc(fp) & 0xff) << 16;
    w |= ((unsigned int)fgetc(fp) & 0xff) << 24;
    
    *wp = w;

    return (!feof(fp));
}

    /* new format: saved as 32 bit big-endian */
gboolean
covio_read_bu32(FILE *fp, gnb_u32_t *wp)
{
    gnb_u32_t w;
    
    w  = ((unsigned int)fgetc(fp) & 0xff) << 24;
    w |= ((unsigned int)fgetc(fp) & 0xff) << 16;
    w |= ((unsigned int)fgetc(fp) & 0xff) << 8;
    w |= (unsigned int)fgetc(fp) & 0xff;
    
    *wp = w;

    return (!feof(fp));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

    /* old format: saved as 64 bit little-endian */
gboolean
covio_read_lu64(FILE *fp, gnb_u64_t *wp)
{
    gnb_u64_t w;
    
    w = (gnb_u64_t)fgetc(fp) & 0xff;
    w |= ((gnb_u64_t)fgetc(fp) & 0xff) << 8;
    w |= ((gnb_u64_t)fgetc(fp) & 0xff) << 16;
    w |= ((gnb_u64_t)fgetc(fp) & 0xff) << 24;
    
    w |= ((gnb_u64_t)fgetc(fp) & 0xff) << 32;
    w |= ((gnb_u64_t)fgetc(fp) & 0xff) << 40;
    w |= ((gnb_u64_t)fgetc(fp) & 0xff) << 48;
    w |= ((gnb_u64_t)fgetc(fp) & 0xff) << 56;

    *wp = w;

    return (!feof(fp));
}

    /* new format: saved as 64 bit big-endian */
gboolean
covio_read_bu64(FILE *fp, gnb_u64_t *wp)
{
    gnb_u64_t w;
    
    w  = ((gnb_u64_t)fgetc(fp) & 0xff) << 56;
    w |= ((gnb_u64_t)fgetc(fp) & 0xff) << 48;
    w |= ((gnb_u64_t)fgetc(fp) & 0xff) << 40;
    w |= ((gnb_u64_t)fgetc(fp) & 0xff) << 32;

    w |= ((gnb_u64_t)fgetc(fp) & 0xff) << 24;
    w |= ((gnb_u64_t)fgetc(fp) & 0xff) << 16;
    w |= ((gnb_u64_t)fgetc(fp) & 0xff) << 8;
    w |= (gnb_u64_t)fgetc(fp) & 0xff;

    *wp = w;

    return (!feof(fp));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* old format: string bytes, 0-3 pad bytes, following by some magic tag */
char *
covio_read_bbstring(FILE *fp, gnb_u32_t endtag)
{
    gnb_u32_t t;
    estring buf;
    
    while (covio_read_lu32(fp, &t))
    {
    	if (t == endtag)
	    break;
	    
	/* pick apart tag as chars and add them to the buf */
	buf.append_char(t & 0xff);
	buf.append_char((t>>8) & 0xff);
	buf.append_char((t>>16) & 0xff);
	buf.append_char((t>>24) & 0xff);
    }
    
    return buf.take();
}

/* new format: int32 length, then string bytes, terminating nul, 0-3 pad bytes */
char *
covio_read_string(FILE *fp)
{
    gnb_u32_t len;
    char *buf;
    
    if (!covio_read_bu32(fp, &len))
    	return 0;   	    	/* short file */
	
    if (len == 0)
    	return g_strdup("");	/* valid empty string */

    if ((buf = (char *)malloc(len+1)) == 0)
    	return 0;   	    	/* memory allocation failure */
    if (fread(buf, 1, len+1, fp) != len+1)
    {
    	g_free(buf);
	return 0;   	    	/* short file */
    }    

    /* skip padding bytes */
    for (len++ ; len&0x3 ; len++)
    {
    	if (fgetc(fp) == EOF)
	{
	    g_free(buf);
	    return 0;	    	/* short file */
	}
    }
    
    return buf;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
