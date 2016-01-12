/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2003-2004 Greg Banks <gnb@users.sourceforge.net>
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

#include "cov_bfd.H"
#include "string_var.H"
#include "demangle.h"
#include "logging.H"

static logging::logger_t &_log = logging::find_logger("cgraph");

#ifdef HAVE_LIBBFD
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
compare_symbols(const void *v1, const void *v2)
{
    const asymbol *a = *(const asymbol **)v1;
    const asymbol *b = *(const asymbol **)v2;

    int r = 0;

    if (a->section->id > b->section->id)
	r = 1;
    else if (a->section->id < b->section->id)
	r = -1;

    if (!r)
    {
	if (a->value > b->value)
	    r = 1;
	else if (a->value < b->value)
	    r = -1;
    }

    return r;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_bfd_t::cov_bfd_t()
{
}

cov_bfd_t::~cov_bfd_t()
{
    if (abfd_ != 0)
	bfd_close(abfd_);
    if (symbols_ != 0)
	g_free(symbols_);
    if (sorted_symbols_ != 0)
	g_free(sorted_symbols_);
    if (code_sections_ != 0)
	g_free(code_sections_);
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

bool
cov_bfd_t::post_open()
{
    if (!bfd_check_format(abfd_, bfd_object))
    {
	/* TODO */
	bfd_perror(bfd_get_filename(abfd_));
	bfd_close(abfd_);
	abfd_ = 0;
	return false;
    }

#if __BYTE_ORDER == __LITTLE_ENDIAN
    assert(bfd_little_endian(abfd_));
#else
    assert(bfd_big_endian(abfd_));
#endif

    abfd_->usrdata = this;

    return true;
}

bool
cov_bfd_t::open(const char *filename)
{
    if ((abfd_ = bfd_openr(filename, /*target*/0)) == 0)
    {
	/* TODO */
	bfd_perror(filename);
	return false;
    }
    return post_open();
}

bool
cov_bfd_t::open(const char *filename, FILE *fp)
{
    if ((abfd_ = bfd_openstreamr(filename, /*target*/0, fp)) == 0)
    {
	/* TODO */
	bfd_perror(filename);
	return false;
    }
    return post_open();
}

const char *
cov_bfd_t::filename() const
{
    return (abfd_ == 0 ? 0 : abfd_->filename);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_bfd_section_t *
cov_bfd_t::find_section(const char *secname)
{
    asection *sec;

    if (abfd_ == 0)
	return 0;
    if ((sec = bfd_get_section_by_name(abfd_, secname)) == 0)
	return 0;
    return (cov_bfd_section_t *)sec;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

bool
cov_bfd_t::get_symbols()
{
    assert(!have_symbols_);
    have_symbols_ = true;

    _log.debug("Reading symbols from %s\n", filename());

    num_symbols_ = bfd_get_symtab_upper_bound(abfd_);
    symbols_ = g_new(asymbol*, num_symbols_);
    num_symbols_ = bfd_canonicalize_symtab(abfd_, symbols_);

    if (num_symbols_ < 0)
    {
	bfd_perror(filename());
	g_free(symbols_);
	symbols_ = 0;
	num_symbols_ = 0;
	return false;
    }

    sorted_symbols_ = g_new(asymbol*, num_symbols_);
    memcpy(sorted_symbols_, symbols_, num_symbols_*sizeof(*symbols_));
    qsort((void *)sorted_symbols_, num_symbols_,
	  sizeof(asymbol*), compare_symbols);

    if (_log.is_enabled(logging::DEBUG2))
    {
	unsigned int i;
	for (i = 0 ; i < num_symbols_ ; i++)
	    cov_bfd_t::dump_symbol(i, symbols_[i]);
    }

    return true;
}

unsigned int
cov_bfd_t::num_symbols()
{
    if (abfd_ == 0)
	return 0;
    if (!have_symbols_ && !get_symbols())
	return 0;
    return num_symbols_;
}

const asymbol *
cov_bfd_t::nth_symbol(unsigned int i)
{
    if (abfd_ == 0)
	return 0;
    if (!have_symbols_ && !get_symbols())
	return 0;
    return (symbols_ == 0 || i >= num_symbols_ ? 0 : symbols_[i]);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define codesectype \
    (SEC_ALLOC|SEC_HAS_CONTENTS|SEC_RELOC|SEC_CODE|SEC_READONLY)

bool
cov_bfd_t::get_code_sections()
{
    asection *sec;

    assert(!have_code_sections_);
    have_code_sections_ = true;

    if (abfd_ == 0)
	return false;

    _log.debug("Gathering code sections from %s\n", filename());

    code_sections_ = g_new(asection*, abfd_->section_count);

    for (sec = abfd_->sections ; sec != 0 ; sec = sec->next)
    {
	_log.debug2("    [%d]%s: ", sec->index, sec->name);

	if ((sec->flags & codesectype) != codesectype)
	{
	    _log.debug2("skipping\n");
	    continue;
	}
	_log.debug2("is code section\n");
	code_sections_[num_code_sections_++] = sec;
    }

    return true;
}

unsigned int
cov_bfd_t::num_code_sections()
{
    if (abfd_ == 0)
	return 0;
    if (!have_code_sections_ && !get_code_sections())
	return 0;
    return num_code_sections_;
}

cov_bfd_section_t *
cov_bfd_t::nth_code_section(unsigned int i)
{
    if (abfd_ == 0)
	return 0;
    if (!have_code_sections_ && !get_code_sections())
	return 0;
    return (code_sections_ == 0 || i >= num_code_sections_ ? 0 :
	    (cov_bfd_section_t *)code_sections_[i]);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char *
symbol_type_as_string(const asymbol *sym)
{
    if (sym->flags & BSF_FILE)
	return "file";
    else if (sym->flags & BSF_SECTION_SYM)
	return "sect";
    else if (sym->flags & BSF_WEAK)
	return (sym->flags & BSF_GLOBAL ? "WEAK" : "weak");
    else if (sym->flags & BSF_FUNCTION)
	return (sym->flags & BSF_GLOBAL ? "FUNC" : "func");
    else if (sym->flags & BSF_OBJECT)
	return (sym->flags & BSF_GLOBAL ? "DATA" : "data");
    else if ((sym->flags & (BSF_LOCAL|BSF_GLOBAL)) == 0)
	return "und";
    return "";
}

void
cov_bfd_t::dump_symbol(unsigned int idx, asymbol *sym)
{
    if (!idx)
	_log.debug2("index|value   |flags|type|section   |name\n");
    if (sym == 0)
	return;

    string_var dem = demangle(sym->name);
    string_var str2 = "";
    if (strcmp(dem, sym->name))
	str2 = g_strdup_printf(" (%s)", dem.data());

    _log.debug2("%5d|%08lx|%5x|%-4s|%-10s|%s%s\n",
	      idx,
	      (long unsigned)sym->value,
	      (unsigned)sym->flags,
	      symbol_type_as_string(sym),
	      sym->section->name,
	      sym->name,
	      str2.data());
}

void
cov_bfd_t::dump_reloc(unsigned int idx, arelent *rel)
{
    if (!idx)
    {
	_log.debug2("relocation                              |symbol\n");
	_log.debug2("index|address |addend  |type            |flags|type|name\n");
    }
    if (rel == 0)
	return;

    asymbol *sym = *rel->sym_ptr_ptr;
    string_var name_dem = demangle(sym->name);
    char reltype[32];

    snprintf(reltype, sizeof(reltype), "%d(%s)",
	    rel->howto->type,
	    rel->howto->name);
    _log.debug2("%5d|%08lx|%08lx|%-16s|%5x|%-4s|%s\n",
	    idx,
	    (long unsigned)rel->address,
	    (long unsigned)rel->addend,
	    reltype,
	    (unsigned)sym->flags,
	    symbol_type_as_string(sym),
	    name_dem.data());
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

unsigned char *
cov_bfd_section_t::get_contents(bfd_size_type *lenp)
{
    asection *sec = (asection *)this;
    bfd_size_type size;
    unsigned char *contents;

    size = bfd_section_size(sec->owner, sec);
    contents = get_contents(0, size);
    if (lenp != 0)
	*lenp = size;
    return contents;
}

unsigned char *
cov_bfd_section_t::get_contents(unsigned long startaddr, unsigned long length)
{
    asection *sec = (asection *)this;
    unsigned char *contents;

    contents = (unsigned char *)gnb_xmalloc(length);
    if (!bfd_get_section_contents(sec->owner, sec, contents, startaddr, length))
    {
	/* TODO */
	bfd_perror(owner()->filename());
	free(contents);
	return 0;
    }

    return contents;
}

static int
compare_arelentp(const void *va, const void *vb)
{
    const arelent *a = *(const arelent **)va;
    const arelent *b = *(const arelent **)vb;

#ifdef BFD64
    return u64cmp(a->address, b->address);
#else
    return u32cmp(a->address, b->address);
#endif
}

arelent **
cov_bfd_section_t::get_relocs(unsigned int *lenp)
{
    asection *sec = (asection *)this;
    cov_bfd_t *b = owner();
    arelent **relocs;
    unsigned int nrelocs;

    if (!b->have_symbols_ && !b->get_symbols())
	return false;

    _log.debug("Reading relocs from %s\n", b->filename());

    nrelocs = bfd_get_reloc_upper_bound(sec->owner, sec);
    relocs = g_new(arelent*, nrelocs);
    nrelocs = bfd_canonicalize_reloc(sec->owner, sec, relocs, b->symbols_);
    if (nrelocs < 0)
    {
	bfd_perror(b->filename());
	g_free(relocs);
	return 0;
    }

    /*
     * Call scanners expect the relocs to be sorted in
     * increasing address order, so ensure this is true.
     */
    qsort(relocs, nrelocs, sizeof(arelent*), compare_arelentp);

    if (lenp != 0)
	*lenp = nrelocs;
    return relocs;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

bool
cov_bfd_section_t::find_nearest_line(
    unsigned long address,
    cov_location_t *locp,
    const char **functionp)
{
    asection *sec = (asection *)this;
    cov_bfd_t *b = owner();
    const char *filename = 0;
    const char *function = 0;
    unsigned int lineno = 0;

    if (b == 0 || b->abfd_ == 0)
	return 0;
    if (!b->have_symbols_ && !b->get_symbols())
	return false;
    if (!bfd_find_nearest_line(sec->owner, sec, b->symbols_, address,
			       &filename, &function, &lineno))
	return false;

    if (locp != 0)
    {
	locp->filename = (char *)filename;
	locp->lineno = (unsigned long)lineno;
    }
    if (functionp != 0)
	*functionp = function;

    return true;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const asymbol *
cov_bfd_section_t::find_symbol_by_value(
    unsigned long val,
    unsigned int flagsmask,
    unsigned int flagsmatch)
{
    asection *sec = (asection *)this;
    cov_bfd_t *b = owner();

    asymbol key, *keyp = &key;
    memset(&key, 0, sizeof(key));
    key.value = val;
    key.section = sec;

    const asymbol **found = (const asymbol **)
	bsearch((void *)&keyp, (void *)b->sorted_symbols_,
		b->num_symbols_, sizeof(asymbol*), compare_symbols);
    if (!found)
	return 0;
    assert(found >= b->sorted_symbols_);
    assert(found < b->sorted_symbols_+b->num_symbols_);
    assert((*found)->section == sec);
    assert((*found)->value == val);
    if (((*found)->flags & flagsmask) != flagsmatch)
	return 0;
    return (*found);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#else

bool cov_bfd_t::open(const char *filename)
{
    _log.debug("ggcov was compiled without the BFD library, cannot open %s\n", filename);
    return false;
}

#endif /*HAVE_LIBBFD*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
