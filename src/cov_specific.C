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

CVSID("$Id: cov_specific.C,v 1.1 2003-06-14 13:57:30 gnb Exp $");

 
cov_factory_item_t *cov_factory_item_t::all_;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_filename_scanner_t::cov_filename_scanner_t()
{
#ifdef HAVE_LIBBFD
    abfd_ = 0;
#endif
}

cov_filename_scanner_t::~cov_filename_scanner_t()
{
#ifdef HAVE_LIBBFD
    if (abfd_ != 0)
	bfd_close(abfd_);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_filename_scanner_t::open(const char *filename)
{
#ifdef HAVE_LIBBFD
    if ((abfd_ = bfd_openr(filename, 0)) == 0)
    {
    	/* TODO */
    	bfd_perror(filename);
	return FALSE;
    }

    if (!bfd_check_format(abfd_, bfd_object))
    {
    	/* TODO */
    	bfd_perror(filename);
	bfd_close(abfd_);
	abfd_ = 0;
	return FALSE;
    }
    return TRUE;
#else
    return FALSE;
#endif
}

#ifdef HAVE_LIBBFD
void *
cov_filename_scanner_t::get_section(const char *secname, bfd_size_type *lenp)
{
    asection *sec;
    bfd_size_type size;
    void *contents;

    if ((sec = bfd_get_section_by_name(abfd_, secname)) == 0)
    {
    	/* TODO */
    	fprintf(stderr, "%s: no %s section\n", abfd_->filename, secname);
	return 0;
    }

    size = bfd_section_size(abfd_, sec);
    contents = gnb_xmalloc(size);
    if (!bfd_get_section_contents(abfd_, sec, contents, 0, size))
    {
    	/* TODO */
    	bfd_perror(abfd_->filename);
	free(contents);
	return 0;
    }
    
    if (lenp != 0)
    	*lenp = size;
    return contents;
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: allocate these dynamically */
int
cov_filename_scanner_t::factory_category()
{
    return 1;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
