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

#include "cov_specific.H"
#include "string_var.H"

CVSID("$Id: cov_stab32.C,v 1.1 2003-06-14 13:57:30 gnb Exp $");

/*
 * Machine-specific code to read 32-bit .stabs entries from an
 * object file or executable and parse them for source filenames.
 * Large chunks of code ripped from binutils/rddbg.c and stabs.c
 */
 
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * This struct describes the in-file format of a 32-bit .stabs entry
 */
typedef struct
{
    uint32_t strx;
#define N_SO 0x64
    uint8_t type;
    uint8_t other;
    uint16_t desc;
    uint32_t value;
} cov_stab32_t;

class cov_stab32_filename_scanner_t : public cov_filename_scanner_t
{
public:
    ~cov_stab32_filename_scanner_t();
    gboolean open(const char *name);
    char *next();

private:
    /* contents of .stab section */
    cov_stab32_t *stabs_;
    bfd_size_type num_stabs_;
    /* contents of .stabstr section */
    char *strings_;
    bfd_size_type string_size_;
    /* iteration variables */
    unsigned int stabi_;
    bfd_size_type stroff_;
    bfd_size_type next_stroff_;
    string_var dir_;
};

COV_FACTORY_STATIC_REGISTER(cov_filename_scanner_t,
    	    	    	    cov_stab32_filename_scanner_t);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_stab32_filename_scanner_t::~cov_stab32_filename_scanner_t()
{
    if (stabs_ != 0)
    	g_free(stabs_);
    if (strings_ != 0)
    	g_free(strings_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_stab32_filename_scanner_t::open(const char *filename)
{
    if (!cov_filename_scanner_t::open(filename))
    	return FALSE;

    stabs_ = (cov_stab32_t *)get_section(".stab", &num_stabs_);
    if (stabs_  == 0)
    	return FALSE;
    num_stabs_ /= sizeof(cov_stab32_t);
    
    strings_ = (char *)get_section(".stabstr", &string_size_);
    if (strings_ == 0)
    	return FALSE;
    
    assert(sizeof(cov_stab32_t) == 12);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    assert(bfd_little_endian(abfd_));
#else
    assert(bfd_big_endian(abfd_));
#endif

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
cov_stab32_filename_scanner_t::next()
{
    for ( ; stabi_ < num_stabs_ ; stabi_++)
    {
    	cov_stab32_t *st = &stabs_[stabi_];

    	if (st->type == 0)
	{
	    /*
	     * Special type 0 stabs indicate the offset to the
	     * next string table.
	     */
	    stroff_ = next_stroff_;
	    next_stroff_ += st->value;
	}
	else if (st->type == N_SO)
	{
	    if (stroff_ + st->strx > string_size_)
	    {
	    	/* TODO */
		fprintf(stderr, "%s: stab entry %u is corrupt, strx = 0x%x, type = %d\n",
		    abfd_->filename,
		    stabi_, st->strx, st->type);
		continue;
	    }
	    
    	    const char *s = strings_ + stroff_ + st->strx;
	    
#if DEBUG > 5
	    fprintf(stderr, "stab32:%5u|%02x|%02x|%04x|%08x|%s|\n",
	    	stabi_, st->type, st->other, st->desc, st->value, s);
#endif
	    if (s[0] == '\0')
	    	continue;

	    if (s[strlen(s)-1] == '/')
	    	dir_ = s;
	    else
	    {
	    	stabi_++;
	    	return g_strconcat(dir_.data(), s, 0);
	    }
	}
    }

    return 0;	/* end of iteration */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
