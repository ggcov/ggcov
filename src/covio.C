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

#include "covio.H"
#include "estring.H"

CVSID("$Id: covio.C,v 1.1 2004-11-20 09:13:38 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

covio_t::~covio_t()
{
    if (ownfp_ && fp_ != 0)
    {
    	fclose(fp_);
	fp_ = 0;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
covio_t::open_read()
{
    if (fp_ != 0)
    	return TRUE;
    ownfp_ = TRUE;
    return (fp_ = fopen(fn_, "r")) != NULL;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

    /* old & gcc34l formats: 32 bit little-endian */
gboolean
covio_t::read_lu32(gnb_u32_t *wp)
{
    gnb_u32_t w;
    
    w = (unsigned int)fgetc(fp_) & 0xff;
    w |= ((unsigned int)fgetc(fp_) & 0xff) << 8;
    w |= ((unsigned int)fgetc(fp_) & 0xff) << 16;
    w |= ((unsigned int)fgetc(fp_) & 0xff) << 24;
    
    *wp = w;

    return (!feof(fp_));
}

    /* gcc33 & gcc34b formats: saved as 32 bit big-endian */
gboolean
covio_t::read_bu32(gnb_u32_t *wp)
{
    gnb_u32_t w;
    
    w  = ((unsigned int)fgetc(fp_) & 0xff) << 24;
    w |= ((unsigned int)fgetc(fp_) & 0xff) << 16;
    w |= ((unsigned int)fgetc(fp_) & 0xff) << 8;
    w |= (unsigned int)fgetc(fp_) & 0xff;
    
    *wp = w;

    return (!feof(fp_));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

    /* old format: saved as 64 bit little-endian */
gboolean
covio_t::read_lu64(gnb_u64_t *wp)
{
    gnb_u64_t w;
    
    w = (gnb_u64_t)fgetc(fp_) & 0xff;
    w |= ((gnb_u64_t)fgetc(fp_) & 0xff) << 8;
    w |= ((gnb_u64_t)fgetc(fp_) & 0xff) << 16;
    w |= ((gnb_u64_t)fgetc(fp_) & 0xff) << 24;
    
    w |= ((gnb_u64_t)fgetc(fp_) & 0xff) << 32;
    w |= ((gnb_u64_t)fgetc(fp_) & 0xff) << 40;
    w |= ((gnb_u64_t)fgetc(fp_) & 0xff) << 48;
    w |= ((gnb_u64_t)fgetc(fp_) & 0xff) << 56;

    *wp = w;

    return (!feof(fp_));
}

    /* gcc33 format: saved as 64 bit big-endian */
gboolean
covio_t::read_bu64(gnb_u64_t *wp)
{
    gnb_u64_t w;
    
    w  = ((gnb_u64_t)fgetc(fp_) & 0xff) << 56;
    w |= ((gnb_u64_t)fgetc(fp_) & 0xff) << 48;
    w |= ((gnb_u64_t)fgetc(fp_) & 0xff) << 40;
    w |= ((gnb_u64_t)fgetc(fp_) & 0xff) << 32;

    w |= ((gnb_u64_t)fgetc(fp_) & 0xff) << 24;
    w |= ((gnb_u64_t)fgetc(fp_) & 0xff) << 16;
    w |= ((gnb_u64_t)fgetc(fp_) & 0xff) << 8;
    w |= (gnb_u64_t)fgetc(fp_) & 0xff;

    *wp = w;

    return (!feof(fp_));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* gcc33, gcc34l, gcc34b formats: read padded NUL terminated string */
char *
covio_t::read_string_len(gnb_u32_t len)
{
    char *buf;
    
    if (len == 0)
    	return g_strdup("");	/* valid empty string */

    if ((buf = (char *)malloc(len)) == 0)
    	return 0;   	    	/* memory allocation failure */
    if (fread(buf, 1, len, fp_) != len || buf[len-1] != '\0')
    {
    	g_free(buf);
	return 0;   	    	/* short file */
    }
    return buf;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
covio_t::skip(unsigned int length)
{
    for ( ; length ; length--)
    {
	if (fgetc(fp_) == EOF)
	    return FALSE;
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
covio_old_t::read_u32(gnb_u32_t *wp)
{
    return read_lu32(wp);
}

gboolean
covio_old_t::read_u64(gnb_u64_t *wp)
{
    return read_lu64(wp);
}

/* oldplus format: little-endian int32 length, then string bytes, terminating nul, 0-3 pad bytes */
char *
covio_old_t::read_string()
{
    gnb_u32_t len;

    if (!read_lu32(&len))
    	return 0;   	    	/* short file */
    return read_string_len(len ? (len + 4) & ~0x3 : 0);
}

/* old .bb format: string bytes, 0-3 pad bytes, following by some magic tag */
char *
covio_old_t::read_bbstring(gnb_u32_t endtag)
{
    gnb_u32_t t;
    estring buf;
    
    while (read_lu32(&t))
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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
covio_gcc33_t::read_u32(gnb_u32_t *wp)
{
    return read_bu32(wp);
}

gboolean
covio_gcc33_t::read_u64(gnb_u64_t *wp)
{
    return read_bu64(wp);
}

/*
 * gcc33 format:
 * big-endian length = 0, OR
 * big-endian length, string bytes, NUL, 0-3 pads 
 * note: length in bytes excludes NUL and pads
 */
char *
covio_gcc33_t::read_string()
{
    gnb_u32_t len;

    if (!read_bu32(&len))
    	return 0;   	    	/* short file */
    return read_string_len(len ? (len + 4) & ~0x3 : 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
covio_gcc34l_t::read_u32(gnb_u32_t *wp)
{
    return read_lu32(wp);
}

/* gcc34l format: lu32 lo, lu32 hi => lu64 */
gboolean
covio_gcc34l_t::read_u64(gnb_u64_t *wp)
{
    return read_lu64(wp);
}

/*
 * gcc34l format:
 * little-endian length = 0, OR
 * little-endian length, string bytes, NUL, 0-3 pads
 * note: length in 4-byte units includes NUL and pads
 */
char *
covio_gcc34l_t::read_string()
{
    gnb_u32_t len;

    if (!read_lu32(&len))
    	return 0;   	    	/* short file */
    return read_string_len(len << 2);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
covio_gcc34b_t::read_u32(gnb_u32_t *wp)
{
    return read_bu32(wp);
}

/* gcc34b format: bu32 lo, bu32 hi */
gboolean
covio_gcc34b_t::read_u64(gnb_u64_t *wp)
{
    gnb_u32_t lo, hi;
    
    if (!read_bu32(&lo) || !read_bu32(&hi))
    	return FALSE;
    *wp = (gnb_u64_t)lo | ((gnb_u64_t)hi)<<32;
    return TRUE;
}

/*
 * gcc34b format:
 * big-endian length = 0, OR
 * big-endian length, string bytes, NUL, 0-3 pads
 * note: length in 4-byte units includes NUL and pads
 */
char *
covio_gcc34b_t::read_string()
{
    gnb_u32_t len;

    if (!read_bu32(&len))
    	return 0;   	    	/* short file */
    return read_string_len(len << 2);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
