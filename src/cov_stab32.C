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

#ifdef HAVE_LIBBFD

CVSID("$Id: cov_stab32.C,v 1.4 2004-04-03 03:23:12 gnb Exp $");

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
    gboolean attach(cov_bfd_t *b);
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
cov_stab32_filename_scanner_t::attach(cov_bfd_t *b)
{
    cov_bfd_section_t *sec;

    if (!cov_filename_scanner_t::attach(b))
    	return FALSE;

    if ((sec = cbfd_->find_section(".stab")) == 0 ||
    	(stabs_ = (cov_stab32_t *)sec->get_contents(&num_stabs_)) == 0)
    	return FALSE;
    num_stabs_ /= sizeof(cov_stab32_t);
    
    if ((sec = cbfd_->find_section(".stabstr")) == 0 ||
    	(strings_ = (char *)sec->get_contents(&string_size_)) == 0)
    if (strings_ == 0)
    	return FALSE;
    
    assert(sizeof(cov_stab32_t) == 12);

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
cov_stab32_filename_scanner_t::next()
{
    if (!stabi_)
	dprintf0(D_STABS|D_VERBOSE, "index|type|other|desc|value   |string\n");

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
		    cbfd_->filename(),
		    stabi_, st->strx, st->type);
		continue;
	    }
	    
    	    const char *s = strings_ + stroff_ + st->strx;
	    
	    dprintf6(D_STABS|D_VERBOSE,
	    	     "%5u|%4x|%5x|%04x|%08x|%s\n",
		     stabi_, st->type, st->other, st->desc, st->value, s);

	    if (s[0] == '\0')
	    	continue;

	    if (s[strlen(s)-1] == '/')
	    	dir_ = s;
	    else
	    {
	    	stabi_++;
		char *res = (s[0] == '/' ? g_strdup(s) : g_strconcat(dir_.data(), s, 0));
		dprintf1(D_STABS, "Scanned source file \"%s\"\n", res);
		return res;
	    }
	}
    }

    return 0;	/* end of iteration */
}

#endif /*HAVE_LIBBFD*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
