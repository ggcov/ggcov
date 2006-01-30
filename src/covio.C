/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2005 Greg Banks <gnb@alphalink.com.au>
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

CVSID("$Id: covio.C,v 1.7 2006-01-29 23:39:01 gnb Exp $");

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
covio_t::read_lu32(gnb_u32_t &wr)
{
    gnb_u32_t w;
    
    w = (unsigned int)fgetc(fp_) & 0xff;
    w |= ((unsigned int)fgetc(fp_) & 0xff) << 8;
    w |= ((unsigned int)fgetc(fp_) & 0xff) << 16;
    w |= ((unsigned int)fgetc(fp_) & 0xff) << 24;
    
    wr = w;

    dprintf1(D_IO|D_VERBOSE, "covio_t::read_lu32() = 0x%08lx\n",
	     (unsigned long)w);

    return (!feof(fp_));
}

    /* gcc33 & gcc34b formats: saved as 32 bit big-endian */
gboolean
covio_t::read_bu32(gnb_u32_t &wr)
{
    gnb_u32_t w;
    
    w  = ((unsigned int)fgetc(fp_) & 0xff) << 24;
    w |= ((unsigned int)fgetc(fp_) & 0xff) << 16;
    w |= ((unsigned int)fgetc(fp_) & 0xff) << 8;
    w |= (unsigned int)fgetc(fp_) & 0xff;
    
    wr = w;

    dprintf1(D_IO|D_VERBOSE, "covio_t::read_bu32() = 0x%08lx\n",
	     (unsigned long)w);

    return (!feof(fp_));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

    /* old format: saved as 64 bit little-endian */
gboolean
covio_t::read_lu64(gnb_u64_t &wr)
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

    wr = w;

    dprintf1(D_IO|D_VERBOSE, "covio_t::read_lu64() = 0x%016llx\n", w);

    return (!feof(fp_));
}

    /* gcc33 format: saved as 64 bit big-endian */
gboolean
covio_t::read_bu64(gnb_u64_t &wr)
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

    wr = w;

    dprintf1(D_IO|D_VERBOSE, "covio_t::read_bu64() = 0x%016llx\n", w);

    return (!feof(fp_));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* gcc33, gcc34l, gcc34b formats: read padded NUL terminated string */
gboolean
covio_t::read_string_len(estring &e, gnb_u32_t len)
{
    e.truncate_to(0);
    if (len == 0)
    	return TRUE;	/* valid empty string */

    e.truncate_to(len+1);
    char *buf = (char *)e.data();
    if (fread(buf, 1, len, fp_) != len)
	return FALSE;   	    	/* short file */
    buf[len] = '\0';	/* JIC */
    dprintf2(D_IO|D_VERBOSE, "covio_t::read_string_len(%d) = \"%s\"\n", len, buf);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
covio_t::skip(unsigned int length)
{
    dprintf1(D_IO|D_VERBOSE, "covio_t::skip(%d)\n", length);
    for ( ; length ; length--)
    {
	if (fgetc(fp_) == EOF)
	    return FALSE;
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
covio_t::read(char *buf, unsigned int len)
{
    return fread(buf, 1, len, fp_);
}

int
covio_t::read(estring &e, unsigned int len)
{
    int r;

    e.truncate_to(len);
    r = fread((char *)e.data(), 1, len, fp_);
    e.truncate_to(r < 0 ? 0 : r);
    return r;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
covio_old_t::read_u32(gnb_u32_t &wr)
{
    return read_lu32(wr);
}

gboolean
covio_old_t::read_u64(gnb_u64_t &wr)
{
    return read_lu64(wr);
}

/* oldplus format: little-endian int32 length, then string bytes, terminating nul, 0-3 pad bytes */
gboolean
covio_old_t::read_string(estring &e)
{
    gnb_u32_t len;

    if (!read_lu32(len))
    	return FALSE;   	    	/* short file */
    return read_string_len(e, len ? (len + 4) & ~0x3 : 0);
}

/* old .bb format: string bytes, 0-3 pad bytes, following by some magic tag */
gboolean
covio_old_t::read_bbstring(estring &buf, gnb_u32_t endtag)
{
    gnb_u32_t t;
    buf.truncate_to(0);

    while (read_lu32(t))
    {
    	if (t == endtag)
	{
	    buf.trim_nuls();
	    return TRUE;
	}
	    
	/* pick apart tag as chars and add them to the buf */
	buf.append_char(t & 0xff);
	buf.append_char((t>>8) & 0xff);
	buf.append_char((t>>16) & 0xff);
	buf.append_char((t>>24) & 0xff);
    }
    
    return FALSE;   	/* short file */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
covio_gcc33_t::read_u32(gnb_u32_t &wr)
{
    return read_bu32(wr);
}

gboolean
covio_gcc33_t::read_u64(gnb_u64_t &wr)
{
    return read_bu64(wr);
}

/*
 * gcc33 format:
 * big-endian length = 0, OR
 * big-endian length, string bytes, NUL, 0-3 pads 
 * note: length in bytes excludes NUL and pads
 */
gboolean
covio_gcc33_t::read_string(estring &e)
{
    gnb_u32_t len;

    if (!read_bu32(len))
    	return FALSE;   	    	/* short file */
    return read_string_len(e, len ? (len + 4) & ~0x3 : 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
covio_gcc34l_t::read_u32(gnb_u32_t &wr)
{
    return read_lu32(wr);
}

/* gcc34l format: lu32 lo, lu32 hi => lu64 */
gboolean
covio_gcc34l_t::read_u64(gnb_u64_t &wr)
{
    return read_lu64(wr);
}

/*
 * gcc34l format:
 * little-endian length = 0, OR
 * little-endian length, string bytes, NUL, 0-3 pads
 * note: length in 4-byte units includes NUL and pads
 */
gboolean
covio_gcc34l_t::read_string(estring &e)
{
    gnb_u32_t len;

    if (!read_lu32(len))
    	return FALSE;   	    	/* short file */
    return read_string_len(e, len << 2);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
covio_gcc34b_t::read_u32(gnb_u32_t &wr)
{
    return read_bu32(wr);
}

/* gcc34b format: bu32 lo, bu32 hi */
gboolean
covio_gcc34b_t::read_u64(gnb_u64_t &wr)
{
    gnb_u32_t lo, hi;
    
    if (!read_bu32(lo) || !read_bu32(hi))
    	return FALSE;
    wr = (gnb_u64_t)lo | ((gnb_u64_t)hi)<<32;
    return TRUE;
}

/*
 * gcc34b format:
 * big-endian length = 0, OR
 * big-endian length, string bytes, NUL, 0-3 pads
 * note: length in 4-byte units includes NUL and pads
 */
gboolean
covio_gcc34b_t::read_string(estring &e)
{
    gnb_u32_t len;

    if (!read_bu32(len))
    	return FALSE;   	    	/* short file */
    return read_string_len(e, len << 2);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
