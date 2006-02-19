/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2003-2005 Greg Banks <gnb@alphalink.com.au>
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

#ifndef _ggcov_bfd_H_
#define _ggcov_bfd_H_ 1

#include "common.h"
#include "cov_types.H"

#ifdef HAVE_LIBBFD
#include <bfd.h>

#ifdef HAVE_ELF_H
#include <elf.h>
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * cov_bfd_t is an abstraction of an object file or executable
 * accessed using the GNU BFD (Binary File Descriptor) library.
 * As ggcov never writes objects, cov_bfd_t is hardcoded for
 * BFD read mode.
 */

class cov_bfd_section_t
{
public:
    arelent **get_relocs(unsigned int *lenp);
    
    unsigned char *get_contents(bfd_size_type *lenp);
    unsigned char *get_contents(unsigned long startaddr, unsigned long length);

    gboolean find_nearest_line(unsigned long address,
    	    	    	       cov_location_t *locp/*return*/,
			       const char **functionp/*return*/);
			       
    unsigned long raw_size() const
    {
#ifdef HAVE_BFD_ASECTION_RAWSIZE
    	return ((asection *)this)->rawsize;
#else
    	return ((asection *)this)->_raw_size;
#endif
    }
};

class cov_bfd_t
{
public:
    cov_bfd_t();
    ~cov_bfd_t();
    
    gboolean open(const char *filename);
    gboolean open(const char *filename, FILE *);
    const char *filename() const;
    
    unsigned int num_symbols();
    const asymbol *nth_symbol(unsigned int);
    
    unsigned int num_code_sections();
    cov_bfd_section_t *nth_code_section(unsigned int);

    cov_bfd_section_t *find_section(const char *secname);

    /* Dump various structures, for debugging;  idx==0 dumps header */
    static void dump_symbol(unsigned int idx, asymbol *sym);
    static void dump_reloc(unsigned int idx, arelent *rel);

    
    enum bfd_flavour flavour() const
    {
    	return bfd_get_flavour(abfd_);
    }

private:
    /* utility functions */
    gboolean get_symbols();
    gboolean get_code_sections();
    gboolean post_open();


    bfd *abfd_;

    gboolean have_symbols_:1;
    gboolean have_code_sections_:1;

    unsigned int num_symbols_;
    asymbol **symbols_;
    
    unsigned int num_code_sections_;
    asection **code_sections_;
    
    friend class cov_bfd_section_t;
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#else /* !HAVE_LIBBFD */

/* just enough to get other code compiling but not running */
typedef struct _asymbol asymbol;
typedef struct _arelent arelent;
typedef uint32_t bfd_size_type;
enum bfd_flavour { bfd_target_unknown_flavour };

class cov_bfd_section_t
{
public:
    arelent **get_relocs(unsigned int *lenp) { return 0; }
    
    unsigned char *get_contents(bfd_size_type *lenp) { return 0; }
    unsigned char *get_contents(unsigned long startaddr, unsigned long length) { return 0; }

    gboolean find_nearest_line(unsigned long address,
    	    	    	       cov_location_t *locp/*return*/,
			       const char **functionp/*return*/)
    { return FALSE; }
			       
    unsigned long raw_size() const { return 0; }
};

class cov_bfd_t
{
public:
    cov_bfd_t() {}
    
    ~cov_bfd_t() {}
    
    gboolean open(const char *filename) { return FALSE; }
    const char *filename() const { return 0; }
    
    unsigned int num_symbols() { return 0; }
    
    const asymbol *nth_symbol(unsigned int) { return 0; }
    
    unsigned int num_code_sections() { return 0; }
    cov_bfd_section_t *nth_code_section(unsigned int) { return 0; }

    cov_bfd_section_t *find_section(const char *secname) { return 0; }

    /* Dump various structures, for debugging;  idx==0 dumps header */
    static void dump_symbol(unsigned int idx, asymbol *sym) {}
    static void dump_reloc(unsigned int idx, arelent *rel) {}

    enum bfd_flavour flavour() const { return (enum bfd_flavour)0; }

private:
};

#endif /* !HAVE_LIBBFD */
#endif /* _ggcov_bfd_H_ */
