/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2005 Greg Banks <gnb@users.sourceforge.net>
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
#include "ptrarray.H"
#include "hashtable.H"
#include "filename.h"
#include "tok.H"
#include "logging.H"

#ifdef HAVE_LIBBFD

/*
 * Machine-specific code to read 32-bit or 64-bit entries from an
 * ELF executable's .dynamic section and parse them for required
 * dynamic libraries.
 */

static logging::logger_t &_log = logging::find_logger("elf");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if BFD_ARCH_SIZE == 32
#define elf_word_t      int32_t
#define elf_uword_t     uint32_t
#define elf_addr_t      uint32_t
#define ELF_ADDR_FMT    "0x%08x"
#elif BFD_ARCH_SIZE == 64
#define elf_word_t      int64_t
#define elf_uword_t     uint64_t
#define elf_addr_t      uint64_t
#define ELF_ADDR_FMT    "0x%016lx"
#else
#error BFD architecture size not supported
#endif

/*
 * This struct describes the in-file format of a .dynamic entry
 */
typedef struct
{
    elf_word_t tag;
    elf_addr_t value;
} cov_dyn_t;

/* The specific tags we need: copied from glibc's <elf.h> */
#ifndef DT_NEEDED
#define DT_NEEDED       1               /* Name of needed library */
#define DT_RPATH        15              /* Library search path (deprecated) */
#define DT_RUNPATH      29              /* Library search path */
#endif

class cov_elf_shlib_scanner_t : public cov_shlib_scanner_t
{
public:
    ~cov_elf_shlib_scanner_t();
    gboolean attach(cov_bfd_t *b);
    char *next();

private:
    cov_dyn_t *find_by_tag(elf_word_t tag) const;
    const char *strvalue(const cov_dyn_t *d) const;
    const char *tag_as_string(const cov_dyn_t *d) const;
    const char *value_as_string(const cov_dyn_t *d) const;
    void add_to_rpath(const char *path);
    char *find_shlib(const char *name);

    /* contents of .dynamic section */
    cov_dyn_t *dyns_;
    bfd_size_type num_dyns_;
    /* contents of .dynstr section */
    char *strings_;
    bfd_size_type string_size_;
    /* variables for searching */
    ptrarray_t<char> *rpath_;
    /* iteration variables */
    unsigned int dyni_;
};

COV_FACTORY_STATIC_REGISTER(cov_shlib_scanner_t,
			    cov_elf_shlib_scanner_t);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_elf_shlib_scanner_t::~cov_elf_shlib_scanner_t()
{
    unsigned int i;

    if (dyns_ != 0)
	g_free(dyns_);
    if (strings_ != 0)
	g_free(strings_);
    if (rpath_ != 0)
    {
	for (i = 0 ; i < rpath_->length() ; i++)
	    g_free(rpath_->nth(i));
	delete rpath_;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
cov_elf_shlib_scanner_t::strvalue(const cov_dyn_t *d) const
{
    switch (d->tag)
    {
    case DT_NEEDED:
    case DT_RPATH:
    case DT_RUNPATH:
	if (d->value < string_size_)
	    return strings_ + d->value;
	break;
    }

    return 0;
}

const char *
cov_elf_shlib_scanner_t::tag_as_string(const cov_dyn_t *d) const
{
    static char buf[32];

    switch (d->tag)
    {
    case DT_NEEDED: return "NEEDED";
    case DT_RPATH: return "RPATH";
    case DT_RUNPATH: return "RUNPATH";
    }

    snprintf(buf, sizeof(buf), "0x%x", (unsigned int)d->tag);
    return buf;
}

const char *
cov_elf_shlib_scanner_t::value_as_string(const cov_dyn_t *d) const
{
    const char *v;
    static char buf[32];

    if ((v = strvalue(d)) == 0)
    {
	/* could be string, could be something else...who knows? Play it safe */
	snprintf(buf, sizeof(buf), ELF_ADDR_FMT, (long unsigned)d->value);
	v = buf;
    }
    return v;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_dyn_t *
cov_elf_shlib_scanner_t::find_by_tag(elf_word_t tag) const
{
    unsigned int i;

    for (i = 0 ; i < num_dyns_ ; i++)
    {
	if (dyns_[i].tag == tag)
	    return &dyns_[i];
    }

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_elf_shlib_scanner_t::add_to_rpath(const char *path)
{
    if (path != 0)
    {
	tok_t tok(path, ":");
	const char *dir;

	while ((dir = tok.next()) != 0)
	{
	    if (rpath_ == 0)
		rpath_ = new ptrarray_t<char>;
	    rpath_->append(g_strdup(dir));
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_elf_shlib_scanner_t::attach(cov_bfd_t *b)
{
    cov_bfd_section_t *sec;

    if (b->flavour() != bfd_target_elf_flavour)
	return FALSE;

    if (!cov_shlib_scanner_t::attach(b))
	return FALSE;

    if ((sec = cbfd_->find_section(".dynamic")) == 0 ||
	(dyns_ = (cov_dyn_t *)sec->get_contents(&num_dyns_)) == 0)
	return FALSE;
    num_dyns_ /= sizeof(cov_dyn_t);

    if ((sec = cbfd_->find_section(".dynstr")) == 0 ||
	(strings_ = (char *)sec->get_contents(&string_size_)) == 0)
    if (strings_ == 0)
	return FALSE;

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
cov_elf_shlib_scanner_t::find_shlib(const char *name)
{
    unsigned int i;
    string_var file;

    if (rpath_ == 0)
    {
	cov_dyn_t *d;

	add_to_rpath(getenv("LD_LIBRARY_PATH"));

	/* DT_RUNPATH obsoletes DT_RPATH */
	if ((d = find_by_tag(DT_RUNPATH)) != 0)
	    add_to_rpath(strvalue(d));
	else if ((d = find_by_tag(DT_RPATH)) != 0)
	    add_to_rpath(strvalue(d));

	add_to_rpath("/lib:/usr/lib");

	if (_log.is_enabled(logging::DEBUG2))
	{
	    _log.debug2("Shared library search path is:\n");
	    for (i = 0 ; i < rpath_->length() ; i++)
		_log.debug2("    \"%s\"\n", rpath_->nth(i));
	}
    }

    /* TODO: run "ldconfig -p" and scan it for mappings */

    for (i = 0 ; i < rpath_->length()  ; i++)
    {
	file = g_strconcat(rpath_->nth(i), "/", name, (char *)0);
	_log.debug2("find_shlib: trying \"%s\"\n", file.data());
	if (file_exists(file) == 0)
	{
	    _log.debug2("    found\n");
	    return file.take();
	}
    }

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
cov_elf_shlib_scanner_t::next()
{
    cov_dyn_t *d;
    const char *name;
    char *file;

    if (!dyni_)
	_log.debug2("index|tag       |value\n");

    for ( ; dyni_ < num_dyns_ ; dyni_++)
    {
	d = &dyns_[dyni_];

	_log.debug2("%5u|%10s|%s\n",
		    dyni_, tag_as_string(d), value_as_string(d));

	if (d->tag != DT_NEEDED ||
	    d->value >= string_size_)
	    continue;

	name = strings_ + d->value;
	if (*name == '/')
	{
	    /* absolute filename, how convenient */
	    dyni_++;
	    return g_strdup(name);
	}
	else if (!strncmp(name, "lib", 3))
	{
	    if ((file = find_shlib(name)) != 0)
	    {
		/* found one */
		dyni_++;
		return file;
	    }
	}
    }

    return 0;   /* end of iteration */
}

#endif /*HAVE_LIBBFD*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
