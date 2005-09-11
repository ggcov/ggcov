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

#include "cov_specific.H"
#include "string_var.H"

#if defined(HAVE_LIBBFD) && defined(COV_I386)

CVSID("$Id: cov_i386.C,v 1.7 2005-09-11 09:20:28 gnb Exp $");

/*
 * Machine-specific code to scan i386 object code for function calls.
 */
 
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


class cov_i386_call_scanner_t : public cov_call_scanner_t
{
public:
    cov_i386_call_scanner_t();
    ~cov_i386_call_scanner_t();
    int next(cov_call_scanner_t::calldata_t *);

    const asymbol *find_function_by_value(cov_bfd_section_t *, unsigned long);
    int scan_statics(cov_call_scanner_t::calldata_t *calld);

private:
    unsigned int section_;

    unsigned int reloc_;
    arelent **relocs_;
    unsigned int nrelocs_;

    unsigned long startaddr_;   /* section address of start of contents_ buffer */
    unsigned long endaddr_; 	/* section address of end of contents_ buffer */
    unsigned long offset_;  	/* current offset into contents_[] */
    unsigned char *contents_;	
    
    unsigned char *buf_;
};

COV_FACTORY_STATIC_REGISTER(cov_call_scanner_t,
    	    	    	    cov_i386_call_scanner_t);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_i386_call_scanner_t::cov_i386_call_scanner_t()
{
}

cov_i386_call_scanner_t::~cov_i386_call_scanner_t()
{
    if (relocs_)
    	g_free(relocs_);
    if (buf_)
    	g_free(buf_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const asymbol *
cov_i386_call_scanner_t::find_function_by_value(
    cov_bfd_section_t *sec,
    unsigned long value)
{
    unsigned int i;

    for (i = 0 ; i < cbfd_->num_symbols() ; i++)
    {
	const asymbol *sym = cbfd_->nth_symbol(i);
	if (sym->section == (asection *)sec &&
	    sym->value == value &&
	    (sym->flags & (BSF_LOCAL|BSF_GLOBAL|BSF_FUNCTION)) == 
		    	  (BSF_LOCAL|           BSF_FUNCTION))
	    return sym;
    }
    return 0;
}

#define read_lu32(p)	\
    ( (p)[0] |      	\
     ((p)[1] <<  8) |	\
     ((p)[2] << 16) |	\
     ((p)[3] << 24))

int
cov_i386_call_scanner_t::scan_statics(cov_call_scanner_t::calldata_t *calld)
{
    cov_bfd_section_t *sec = cbfd_->nth_code_section(section_);
    unsigned long len = endaddr_ - startaddr_;
    unsigned char *p, *end;
    unsigned long callfrom, callto;
    const asymbol *sym;
    
    dprintf3(D_CGRAPH|D_VERBOSE, "scan_statics: scanning %s %lx to %lx\n",
    	    cbfd_->filename(), startaddr_, endaddr_);
    if (len < 1)
    	return 0;
    if (contents_ == 0)
    {
    	if ((contents_ = sec->get_contents(startaddr_, len)) == 0)
    	    return 0;	/* end of scan */
    	offset_ = 0;
    }
    end = contents_ + len - 4;
    
    /*
     * It would presumably be more efficient to scan through the relocs
     * looking for PCREL32 to static functions and double-check that the
     * preceding byte is the CALL instruction.  Except that we don't
     * actually get any such relocs, at least for some versions of gcc.
     */

    /* CALL instruction is 5 bytes long so don't bother scanning last 5 bytes */
    for (p = contents_+offset_ ; p < end ; p++)
    {
    	if (*p != 0xe8)
	    continue;	    /* not a CALL instruction */
	callfrom = startaddr_ + (p - contents_);
	p++;
	callto = callfrom + read_lu32(p) + 5;
	p += 4;
	dprintf2(D_CGRAPH|D_VERBOSE, "scan_statics: possible call from %lx to %lx\n",
	    	callfrom, callto);
	
	/*
	 * Scan symbols to see if this is a PCREL32
	 * reference to a static function entry point
	 */
	if ((sym = find_function_by_value(sec, callto)) != 0)
	{
	    offset_ = (p - contents_);
    	    dprintf0(D_CGRAPH, "scan_statics: scanned static call\n");
	    return (setup_calldata(sec, callfrom, sym->name, calld) ? 
	    	    1/* have calldata */ : -1/* something is wrong */);
	}
    }
    
    g_free(contents_);
    contents_ = 0;
    return 0;	/* end of scan */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
cov_i386_call_scanner_t::next(cov_call_scanner_t::calldata_t *calld)
{
    int r;

    for ( ; section_ < cbfd_->num_code_sections() ; section_++)
    {
    	cov_bfd_section_t *sec = cbfd_->nth_code_section(section_);

    	if (relocs_ == 0)
	{
	    if ((relocs_ = sec->get_relocs(&nrelocs_)) == 0)
		continue;
	    reloc_ = 0;
	    startaddr_ = endaddr_ = 0UL;
	}
    
	for ( ; reloc_ < nrelocs_ ; reloc_++)
	{
	    arelent *rel = relocs_[reloc_];
	    const asymbol *sym = *rel->sym_ptr_ptr;
	    
    	    if (debug_enabled(D_CGRAPH|D_VERBOSE))
    		cov_bfd_t::dump_reloc(reloc_, rel);

	    switch (rel->howto->type)
	    {
	    case R_386_32:  	/* external data reference from static code */
    	    case R_386_GOTPC:	/* external data reference from PIC code */
	    case R_386_GOTOFF:	/* external data reference from PIC code */
	    case R_386_GOT32:	/* external data reference from ??? code */
	    	continue;
	    case R_386_PC32:	/* function call from static code */
	    case R_386_PLT32:	/* function call from PIC code */
	    	break;
	    default:
	    	fprintf(stderr, "%s: Warning unexpected 386 reloc howto type %d\n",
		    	    	    cbfd_->filename(), rel->howto->type);
	    	continue;
	    }

    	    if (symbol_is_ignored(sym->name))
	    	continue;
    	    if ((sym->flags & BSF_FUNCTION) ||
		(sym->flags & (BSF_LOCAL|BSF_GLOBAL|BSF_SECTION_SYM|BSF_OBJECT)) == 0)
	    {

    	    	/*
		 * Scan the instructions between the previous reloc and
		 * this instruction for calls to static functions.  Very
		 * platform specific!
		 */
		endaddr_ = rel->address;
		if ((r = scan_statics(calld)))
		    return r;	/* -1 or 1 */
		startaddr_ = endaddr_ + bfd_get_reloc_size(rel->howto);
		reloc_++;
		
		return (setup_calldata(sec, rel->address, sym->name, calld) ? 
	    		1/* have calldata */ : -1/* something is wrong */);
	    }

	}

    	g_free(relocs_);
	relocs_ = 0;

    	if (endaddr_ < sec->raw_size())
	{
	    endaddr_ = sec->raw_size();
	    if ((r = scan_statics(calld)))
		return r;	/* -1 or 1 */
	}
    }

    return 0;	/* end of scan */
}

#endif /*HAVE_LIBBFD && COV_I386 */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
