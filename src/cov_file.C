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

#include "cov.H"
#include "covio.h"
#include "estring.H"
#include "filename.h"

#include <bfd.h>
#include <elf.h>


CVSID("$Id: cov_file.C,v 1.1 2002-12-29 13:14:16 gnb Exp $");


GHashTable *cov_file_t::files_;
char *cov_file_t::common_path_;
int cov_file_t::common_len_;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


cov_file_t::cov_file_t(const char *name)
{
    strassign(name_, name);
    functions_ = g_ptr_array_new();
    functions_by_name_ = g_hash_table_new(g_str_hash, g_str_equal);
    
    g_hash_table_insert(files_, name_, this);
    add_name(name_);
}

cov_file_t::~cov_file_t()
{
#if 0
    g_hash_table_remove(files_, name_);

    strdelete(name_);
    g_ptr_array_free(functions_, /*free_seg*/TRUE);
    g_ptr_array_free(blocks_, /*free_seg*/TRUE);
    g_ptr_array_free(arcs_, /*free_seg*/TRUE);
#else
    assert(0);
#endif
}

void
cov_file_t::init()
{
    files_ = g_hash_table_new(g_str_hash, g_str_equal);
    common_path_ = 0;
    common_len_ = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

	/* TODO: handle names relative to $PWD ? */

cov_file_t *
cov_file_t::find(const char *name)
{
    assert(files_ != 0);
    estring fullname = unminimise_name(name);
    return (cov_file_t *)g_hash_table_lookup(files_, fullname.data());
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* TODO:  Desperately need a hash table for which it is possible
 * to implement a reasonable Iterator.
 */

typedef struct
{
    void (*func)(cov_file_t *, void *);
    void *userdata;
} cov_file_foreach_rec_t;

static void
cov_file_foreach_tramp(gpointer key, gpointer value, gpointer userdata)
{
    cov_file_t *f = (cov_file_t *)value;
    cov_file_foreach_rec_t *rec = (cov_file_foreach_rec_t *)userdata;
    
    (*rec->func)(f, rec->userdata);
}

void
cov_file_t::foreach(
    void (*func)(cov_file_t*, void *userdata),
    void *userdata)
{
    cov_file_foreach_rec_t rec;
    
    rec.func = func;
    rec.userdata = userdata;
    g_hash_table_foreach(files_, cov_file_foreach_tramp, &rec);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_file_t::add_name(const char *name)
{
    assert(name[0] == '/');
    if (common_path_ == 0)
    {
    	/* first filename: initialise the common path to the directory */
	char *p;
    	common_path_ = g_strdup(name);
	if ((p = strrchr(common_path_, '/')) != 0)
	    p[1] = '\0';
    }
    else
    {
    	/* subsequent filenames: shrink common path as necessary */
	char *cs, *ce, *ns, *ne;
	cs = common_path_+1;
	ns = (char *)name+1;
	for (;;)
	{
	    if ((ne = strchr(ns, '/')) == 0)
	    	break;
	    if ((ce = strchr(cs, '/')) == 0)
	    	break;
	    if ((ce - cs) != (ne - ns))
	    	break;
	    if (memcmp(cs, ns, (ne - ns)))
	    	break;
	    cs = ce+1;
	    ns = ne+1;
	}
	*cs = '\0';
    }
    common_len_ = strlen(common_path_);
#if DEBUG
    fprintf(stderr, "cov_file_t::add_name: name=\"%s\" => common=\"%s\"\n",
    	    	name, common_path_);
#endif
}

const char *
cov_file_t::minimal_name() const
{
    return name_ + common_len_;
}

char *
cov_file_t::minimise_name(const char *name)
{
    if (!strncmp(name, common_path_, common_len_))
    {
    	return g_strdup(name + common_len_);
    }
    else
    {
	assert(name[0] == '/');
	return g_strdup(name);
    }
}

char *
cov_file_t::unminimise_name(const char *name)
{
    if (name[0] == '/')
    {
    	/* absolute name */
    	return g_strdup(name);
    }
    else
    {
    	/* partial, presumably minimal, name */
	return g_strconcat(common_path_, name, 0);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * TODO: there is a bug here, the last function in the list is not
 *       necessarily the last in the source file.  We need to keep
 *       a member variable to track to largest linenumber seen in
 *       a function at function add time, and just report that here.
 */
const cov_location_t *
cov_file_t::get_last_location() const
{
    int fnidx;
    const cov_location_t *loc;
    
    for (fnidx = num_functions()-1 ; fnidx >= 0 ; fnidx--)
    {
    	if ((loc = nth_function(fnidx)->get_last_location()) != 0)
	    return loc;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_function_t *
cov_file_t::add_function()
{
    cov_function_t *fn;
    
    fn = new cov_function_t();
    
    fn->idx_ = functions_->len;
    g_ptr_array_add(functions_, fn);    
    fn->file_ = this;
    
    return fn;
}

cov_function_t *
cov_file_t::find_function(const char *fnname) const
{
    return (cov_function_t *)g_hash_table_lookup(functions_by_name_, fnname);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_file_t::calc_stats(cov_stats_t *stats) const
{
    unsigned int fnidx;
    
    for (fnidx = 0 ; fnidx < num_functions() ; fnidx++)
    {
	nth_function(fnidx)->calc_stats(stats);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_file_t::solve()
{
    unsigned int fnidx;
    
    for (fnidx = 0 ; fnidx < num_functions() ; fnidx++)
    {
    	if (!nth_function(fnidx)->solve())
	{
	    fprintf(stderr, "ERROR: could not solve flow graph\n");
	    return FALSE;
	}
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define BB_FILENAME 	0x80000001
#define BB_FUNCTION 	0x80000002
#define BB_ENDOFLIST	0x00000000

gboolean
cov_file_t::read_bb_file(const char *bbfilename)
{
    FILE *fp;
    covio_u32_t tag;
    char *funcname = 0;
    char *filename = 0;
    cov_function_t *fn = 0;
    int funcidx;
    int bidx;
    int line;
    int nlines;
    
#if DEBUG    
    fprintf(stderr, "Reading .bb file \"%s\"\n", bbfilename);
#endif

    if ((fp = fopen(bbfilename, "r")) == 0)
    {
    	perror(bbfilename);
	return FALSE;
    }


    funcidx = 0;
    line = 0;
    nlines = 0;
    while (covio_read_u32(fp, &tag))
    {
    	switch (tag)
	{
	case BB_FILENAME:
	    if (filename != 0)
	    	g_free(filename);
	    filename = covio_read_bbstring(fp, tag);
	    if (strchr(filename, '/') == 0 &&
	    	!strcmp(filename, file_basename_c(name_)))
	    {
	    	g_free(filename);
		filename = g_strdup(name_);
	    }
#if DEBUG    
	    fprintf(stderr, "BB filename = \"%s\"\n", filename);
#endif
	    break;
	    
	case BB_FUNCTION:
	    funcname = covio_read_bbstring(fp, tag);
#if DEBUG    
	    fprintf(stderr, "BB function = \"%s\"\n", funcname);
#endif
	    fn = nth_function(funcidx);
	    funcidx++;
	    bidx = 0;
	    line = 0;
	    nlines = 0;
	    fn->set_name(funcname);
	    g_free(funcname);
	    break;
	
	case BB_ENDOFLIST:
	    if (line != 0 && nlines == 0)
	    {
		assert(fn != 0);
		assert(bidx != 0);
		assert(filename != 0);
		fn->nth_block(bidx)->add_location(filename, line);
	    }
	    bidx++;
	    nlines = 0;
	    break;
	    
	default:
#if DEBUG    
	    fprintf(stderr, "BB line = %d\n", (int)tag);
#endif
	    assert(fn != 0);

    	    line = tag;
	    fn->nth_block(bidx)->add_location(filename, line);
	    nlines++;
	    break;
	}
    }
    
    strdelete(filename);
    fclose(fp);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define BBG_SEPERATOR	0x80000001

gboolean
cov_file_t::read_bbg_function(FILE *fp)
{
    covio_u32_t nblocks, totnarcs, narcs;
    covio_u32_t bidx, aidx;
    covio_u32_t dest, flags;
    covio_u32_t sep;
    cov_block_t *b;
    cov_arc_t *a;
    cov_function_t *fn;
    
#if DEBUG    
    fprintf(stderr, "BBG reading function\n");
#endif
    
    if (!covio_read_u32(fp, &nblocks))
    	return FALSE;
    covio_read_u32(fp, &totnarcs);
    
    fn = add_function();
    for (bidx = 0 ; bidx < nblocks ; bidx++)
    	fn->add_block();
	
    for (bidx = 0 ; bidx < nblocks ; bidx++)
    {
#if DEBUG    
    	fprintf(stderr, "BBG   block %ld\n", bidx);
#endif
	b = fn->nth_block(bidx);
	covio_read_u32(fp, &narcs);

	for (aidx = 0 ; aidx < narcs ; aidx++)
	{
	    covio_read_u32(fp, &dest);
	    covio_read_u32(fp, &flags);

#if DEBUG    
    	    fprintf(stderr, "BBG     arc %ld: %ld->%ld flags %s,%s,%s\n",
	    	    	    aidx,
			    bidx, dest,
			    (flags & 0x1 ? "on_tree" : ""),
			    (flags & 0x2 ? "fake" : ""),
			    (flags & 0x4 ? "fall_through" : ""));
#endif
			    
	    a = new cov_arc_t(fn->nth_block(bidx), fn->nth_block(dest));
	    a->on_tree_ = (flags & 0x1);
	    a->fake_ = !!(flags & 0x2);
	    a->fall_through_ = !!(flags & 0x4);
	}
    }

    covio_read_u32(fp, &sep);
    if (sep != BBG_SEPERATOR)
    	return FALSE;
	
    return TRUE;
}


gboolean
cov_file_t::read_bbg_file(const char *bbgfilename)
{
    FILE *fp;
    
#if DEBUG    
    fprintf(stderr, "Reading .bbg file \"%s\"\n", bbgfilename);
#endif
    
    if ((fp = fopen(bbgfilename, "r")) == 0)
    {
    	perror(bbgfilename);
	return FALSE;
    }
    
    while (!feof(fp))
    	read_bbg_function(fp);

    fclose(fp);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_file_t::read_da_file(const char *dafilename)
{
    FILE *fp;
    covio_u64_t nents;
    covio_u64_t ent;
    unsigned int fnidx;
    unsigned int bidx;
    list_iterator_t<cov_arc_t> aiter;
    
#if DEBUG    
    fprintf(stderr, "Reading .da file \"%s\"\n", dafilename);
#endif
    
    if ((fp = fopen(dafilename, "r")) == 0)
    {
    	perror(dafilename);
	return FALSE;
    }

    covio_read_u64(fp, &nents);
    
    for (fnidx = 0 ; fnidx < num_functions() ; fnidx++)
    {
    	cov_function_t *fn = nth_function(fnidx);
	
	for (bidx = 0 ; bidx < fn->num_blocks() ; bidx++)
	{
    	    cov_block_t *b = fn->nth_block(bidx);
	
	    for (aiter = b->out_arc_iterator() ; aiter != (cov_arc_t *)0 ; ++aiter)
	    {
	    	cov_arc_t *a = *aiter;
		
		if (a->on_tree_)
		    continue;

    	    	/* TODO: check that nents is correct */
    		if (!covio_read_u64(fp, &ent))
		{
		    fprintf(stderr, "%s: short file\n", dafilename);
		    return FALSE;
		}

#if DEBUG
    	    	estring fromdesc = a->from()->describe();
    	    	estring todesc = a->to()->describe();
    	    	fprintf(stderr, "DA arc {from=%s to=%s} count=%llu\n",
		    	    fromdesc.data(),
		    	    todesc.data(),
			    ent);
#endif

    	    	a->set_count(ent);
	    }
	}
    }    
    
    fclose(fp);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

struct cov_read_state_t
{
    bfd *abfd;
    asection *section;
    asymbol **symbols;
    long nsymbols;
};

void
cov_o_file_add_call(
    cov_read_state_t *rs,
    unsigned long address,
    const char *callname)
{
    const char *filename = 0;
    const char *function = 0;
    unsigned int lineno = 0;
    cov_location_t loc;
    const GList *blocks;

    if (!bfd_find_nearest_line(rs->abfd, rs->section, rs->symbols, address,
		    	       &filename, &function, &lineno))
	return;
#if DEBUG
    fprintf(stderr, "%s:%d: %s calls %s\n",
		file_basename_c(filename), lineno, function, callname);
#endif

    loc.filename = (char *)filename;
    loc.lineno = lineno;
    blocks = cov_block_t::find_by_location(&loc);
    
    if (blocks == 0)
    {
	fprintf(stderr, "No blocks for call to %s at %s:%ld\n",
		    callname, loc.filename, loc.lineno);
    }
    else
    {
    	const GList *iter;
    	for (iter = blocks ; iter != 0 ; iter = iter->next)
	{
	    cov_block_t *b = (cov_block_t *)iter->data;
#if DEBUG
    	    estring desc = b->describe();
#endif

    	    /*
	     * Multiple blocks on the same line, the first doesn't
	     * do the call: skip until we find the one that does.
	     * Also, multiple blocks with calls in the same statement
	     * (line numbers are only recorded at the start of the
	     * statement); both the blocks and relocs are ordered,
	     * so assume it's 1:1 and skip blocks which already have
	     * a call name recorded.  This breaks if the statement
	     * mixes calls to static and extern functions.
	     */
    	    if (b->needs_call())
	    {
#if DEBUG
    	        fprintf(stderr, "    block %s\n", desc.data());
#endif
		b->add_call(callname);
		return;
	    }
#if DEBUG
	    fprintf(stderr, "    skipping block %s\n", desc.data());
#endif

	}
	assert(0);
    }
}

#ifdef __i386__

#define read_lu32(p)	\
    ( (p)[0] |      	\
     ((p)[1] <<  8) |	\
     ((p)[2] << 16) |	\
     ((p)[3] << 24))

static void
cov_o_file_scan_static_calls(
    cov_read_state_t *rs,
    unsigned long startaddr,
    unsigned long endaddr)
{
    unsigned char *buf, *p;
    unsigned long len = endaddr - startaddr;
    unsigned long callfrom, callto;
    int i;
    
    if (len < 1)
    	return;
    buf = g_new(unsigned char, len);
    
    if (!bfd_get_section_contents(rs->abfd, rs->section, buf, startaddr, len))
    {
    	g_free(buf);
    	return;
    }
    
    /*
     * TODO: presumably it is more efficient to scan through the relocs
     * looking for PCREL32 to static functions and double-check that the
     * preceding byte is the CALL instruction.
     */

    /* CALL instruction is 5 bytes long so don't bother scanning last 5 bytes */
    for (p = buf ; p < buf+len-4 ; p++)
    {
    	if (*p != 0xe8)
	    continue;	    /* not a CALL instruction */
	callfrom = startaddr + (p - buf);
	callto = callfrom + read_lu32(p+1) + 5;
	
	/*
	 * Scan symbols to see if this is a PCREL32
	 * reference to a static function entry point
	 */
	for (i = 0 ; rs->symbols[i] != 0 ; i++)
	{
	    if (rs->symbols[i]->section == rs->section &&
	    	rs->symbols[i]->value == callto &&
	    	(rs->symbols[i]->flags & (BSF_LOCAL|BSF_GLOBAL|BSF_FUNCTION)) == 
		    	    	         (BSF_LOCAL|           BSF_FUNCTION))
	    {
#if DEBUG
    	    	fprintf(stderr, "Scanned static call\n");
#endif
		cov_o_file_add_call(rs, callfrom, rs->symbols[i]->name);
		p += 4;
    	    	break;
	    }
	}
    }
    
    g_free(buf);
}
#else

static void
cov_o_file_scan_static_calls(
    cov_read_state_t *rs,
    unsigned long startaddr,
    unsigned long endaddr)
{
}

#endif

/*
 * Use the BFD library to scan relocation records in the .o file.
 */
gboolean
cov_file_t::read_o_file_relocs(const char *ofilename)
{
    cov_read_state_t rs;
    asymbol *sym;
    asection *sec;
    long i;
    unsigned codesectype = (SEC_ALLOC|SEC_HAS_CONTENTS|SEC_RELOC|SEC_CODE|SEC_READONLY);
    
#if DEBUG
    fprintf(stderr, "Reading .o file \"%s\"\n", ofilename);
#endif
    
    if ((rs.abfd = bfd_openr(ofilename, 0)) == 0)
    {
    	/* TODO */
    	bfd_perror(ofilename);
	return FALSE;
    }
    if (!bfd_check_format(rs.abfd, bfd_object))
    {
    	/* TODO */
    	bfd_perror(ofilename);
	bfd_close(rs.abfd);
	return FALSE;
    }

#if DEBUG
    fprintf(stderr, "%s: reading symbols...\n", ofilename);
#endif
    rs.nsymbols = bfd_get_symtab_upper_bound(rs.abfd);
    rs.symbols = g_new(asymbol*, rs.nsymbols);
    rs.nsymbols = bfd_canonicalize_symtab(rs.abfd, rs.symbols);
    if (rs.nsymbols < 0)
    {
	bfd_perror(ofilename);
	bfd_close(rs.abfd);
	g_free(rs.symbols);
	return FALSE;
    }

#if DEBUG > 5
    for (i = 0 ; i < nsymbols ; i++)
    {
    	sym = rs.symbols[i];
	
	fprintf(stderr, "%s\n", sym->name);
    }
#endif

    for (sec = rs.abfd->sections ; sec != 0 ; sec = sec->next)
    {
	unsigned long lastaddr = 0UL;
	arelent **relocs, *rel;
	long nrelocs;

#if DEBUG
	fprintf(stderr, "%s[%d %s]: ", ofilename, sec->index, sec->name);
#endif

	if ((sec->flags & codesectype) != codesectype)
	{
#if DEBUG
	    fprintf(stderr, "skipping\n");
#endif
    	    continue;
	}

#if DEBUG
	fprintf(stderr, "reading relocs...\n");
#endif

    	nrelocs = bfd_get_reloc_upper_bound(rs.abfd, sec);
	relocs = g_new(arelent*, nrelocs);
    	nrelocs = bfd_canonicalize_reloc(rs.abfd, sec, relocs, rs.symbols);
	if (nrelocs < 0)
	{
	    bfd_perror(ofilename);
	    g_free(relocs);
	    continue;
	}
    
    	for (i = 0 ; i < nrelocs ; i++)
	{
	    
	    rel = relocs[i];
	    sym = *rel->sym_ptr_ptr;
	    
#if DEBUG
    	    {
		char *type;

		if ((sym->flags & BSF_FUNCTION))
	    	    type = "FUN";
		else if ((sym->flags & BSF_OBJECT))
	    	    type = "DAT";
		else if (sym->flags & BSF_SECTION_SYM)
	    	    type = "SEC";
		else if ((sym->flags & (BSF_LOCAL|BSF_GLOBAL)) == 0)
	    	    type = "UNK";
		else
	    	    type = "---";
		fprintf(stderr, "%5ld %08lx %08lx %08lx %d(%s) %s %s\n",
	    		i, rel->address, rel->addend,
			(unsigned long)sym->flags,
			rel->howto->type,
			rel->howto->name,
			type, sym->name);
    	    }
#endif

#ifdef __i386__
    	    /*
	     * Experiment shows that functions calls result in an R_386_PC32
	     * reloc and external data references in an R_386_32 reloc.
	     * Haven't yet seen any others -- so give a warning if we do.
	     */
	    if (rel->howto->type == R_386_32)
	    	continue;
	    else if (rel->howto->type != R_386_PC32)
	    {
	    	fprintf(stderr, "%s: Warning unexpected 386 reloc howto type %d\n",
		    	    	    ofilename, rel->howto->type);
	    	continue;
	    }
#endif

    	    /* __bb_init_func is code inserted by gcc to instrument blocks */
    	    if (!strcmp(sym->name, "__bb_init_func"))
	    	continue;
    	    if ((sym->flags & BSF_FUNCTION) ||
		(sym->flags & (BSF_LOCAL|BSF_GLOBAL|BSF_SECTION_SYM|BSF_OBJECT)) == 0)
	    {

    	    	/*
		 * Scan the instructions between the previous reloc and
		 * this instruction for calls to static functions.  Very
		 * platform specific!
		 */
		rs.section = sec;
		cov_o_file_scan_static_calls(&rs, lastaddr, rel->address);
		lastaddr = rel->address + bfd_get_reloc_size(rel->howto);
		
		cov_o_file_add_call(&rs, rel->address, sym->name);
	    }

	}
    	g_free(relocs);

    	if (lastaddr < sec->_raw_size)
	{
	    rs.section = sec;
	    cov_o_file_scan_static_calls(&rs, lastaddr, sec->_raw_size);
	}
    }

    g_free(rs.symbols);
    
    bfd_close(rs.abfd);
    return TRUE;
}


gboolean
cov_file_t::read_o_file(const char *ofilename)
{
    unsigned int fnidx;

    if (!read_o_file_relocs(ofilename))
    {
    	/* TODO */
    	fprintf(stderr, "%s: Warning: could not read object file, less information may be displayed\n",
	    ofilename);
	return TRUE;	    /* this info is optional */
    }
    
    for (fnidx = 0 ; fnidx < num_functions() ; fnidx++)
    	nth_function(fnidx)->reconcile_calls();
    
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
