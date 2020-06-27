/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2015-2020 Greg Banks <gnb@fastmail.fm>
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
#include "cov_suppression.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char * const type_descs[] =
{
    "line depends on #ifdef",
    "line contains comment",
    "line between comments",
    "arc calls",
    "block calls",
    "function matches",
    "filename matches",
    "all components are suppressed",
    "function cannot be solved",
    "reloc record matches",
    0
};

const char *
cov_suppression_t::describe() const
{
    static estring buf;
    buf.truncate();
    buf.append_printf("%s %s ",
		      type_descs[type_],
		      word_.data());
    if (word2_.data())
	buf.append_printf("and %s ", word2_.data());
    buf.append_printf("(%s)", origin_.data());
    return buf;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
    
cov_suppression_set_t::cov_suppression_set_t()
{
}

cov_suppression_set_t::~cov_suppression_set_t()
{
    // We really should be deleting or unref'ing all
    // the suppression objects here.  But, this only
    // happens at program exit in the current design.
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_suppression_set_t::add(cov_suppression_t *sup)
{
    if (sup->word())
    {
	const char *star = strchr(sup->word(), '*');
	if (star)
	{
	    /* only a trailing * is supported */
	    assert(star[1] == '\0');
	    prefixed_[sup->type()].append(sup);
	}
	else
	{
	    if (!by_word_[sup->type()])
		by_word_[sup->type()] = new hashtable_t<const char, const cov_suppression_t>;
	    by_word_[sup->type()]->insert(sup->word(), sup);
	}
    }
    all_[sup->type()].append(sup);
}

void
cov_suppression_set_t::remove(const cov_suppression_t *sup)
{
    all_[sup->type()].remove(sup);
    prefixed_[sup->type()].remove(sup);
    by_word_[sup->type()]->remove(sup->word());
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const cov_suppression_t *
cov_suppression_set_t::find(const char *w, cov_suppression_t::type_t t) const
{
    const cov_suppression_t *s;

    if (!w)
	return all_[t].head();

    /* try an exact match */
    if (by_word_[t] && (s = by_word_[t]->lookup(w)))
	return s;

    /* try a prefix match */
    for (list_iterator_t<const cov_suppression_t> itr = prefixed_[t].first() ; *itr ; ++itr)
    {
	s = *itr;
	const char *star = strchr(s->word(), '*');
	if (!strncmp(w, s->word(), (star - s->word())))
	    return s;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_suppression_set_t::init_builtins(void)
{
    static const init_t inits[] =
    {
	/* system include files */
	{ cov_suppression_t::FILENAME, "/usr/include/*" },
	{ cov_suppression_t::FILENAME, "/usr/lib/*" },
	/* global constructors */
	{ cov_suppression_t::FUNCTION, "_GLOBAL_*" },
	/* inlines in glibc's </sys/sysmacros.h> */
	{ cov_suppression_t::FUNCTION, "gnu_dev_major" },
	{ cov_suppression_t::FUNCTION, "gnu_dev_minor" },
	{ cov_suppression_t::FUNCTION, "gnu_dev_makedev" },
	/* inlines in glibc's <sys/stat.h> */
	{ cov_suppression_t::FUNCTION, "stat" },
	{ cov_suppression_t::FUNCTION, "lstat" },
	{ cov_suppression_t::FUNCTION, "fstat" },
	{ cov_suppression_t::FUNCTION, "mknod" },
	/* gcc 3.4 exception handling */
	{ cov_suppression_t::ARC_CALLS,    "__cxa_allocate_exception" },
	{ cov_suppression_t::ARC_CALLS,    "__cxa_begin_catch" },
	{ cov_suppression_t::ARC_CALLS,    "__cxa_call_unexpected" },
	{ cov_suppression_t::ARC_CALLS,    "__cxa_end_catch" },
	{ cov_suppression_t::ARC_CALLS,    "__cxa_throw" },
	{ cov_suppression_t::ARC_CALLS,    "_Unwind_Resume" },
	/* externs in glibc's <assert.h> */
	{ cov_suppression_t::BLOCK_CALLS, "__assert_fail" },
	{ cov_suppression_t::BLOCK_CALLS, "__assert_perror_fail" },
	{ cov_suppression_t::BLOCK_CALLS, "__assert" },
	{ cov_suppression_t::BLOCK_CALLS, "abort" },
	/* millicode & other relocs to ignore while
	 * scanning object code for calls */
	{ cov_suppression_t::RELOC, "__bb_init_func" },        /* code inserted by gcc to instrument blocks */
	{ cov_suppression_t::RELOC, "__gcov_init" },           /* a more modern version of the same */
	{ cov_suppression_t::RELOC, "_Unwind_Resume" },        /* gcc 3.4 exception handling */
	{ cov_suppression_t::RELOC, "__cxa_call_unexpected" }, /* gcc 3.4 exception handling */
	{ cov_suppression_t::RELOC, "__cxa_end_catch" },       /* gcc 3.4 exception handling */
	{ cov_suppression_t::RELOC, "__i686.get_pc_thunk.bx" },/* gcc 4.x -fPIC */
	/* Note: -fstack-protector is on by default on Ubuntu */
	{ cov_suppression_t::RELOC, "__stack_chk_fail" },      /* gcc 4.x -fstack-protector */
	{ cov_suppression_t::RELOC, "__stack_chk_fail_local" },/* gcc 4.x -fstack-protector */
	{ (cov_suppression_t::type_t)0, 0 }
    };
    const init_t *si;

    for (si = inits ; si->word ; si++)
	add(new cov_suppression_t(si->word, si->type, "builtin defaults"));

    add(new cov_suppression_t(0, cov_suppression_t::MERGED, "builtin defaults"));

    add(new cov_suppression_t(0, cov_suppression_t::UNSOLVABLE, "builtin defaults"));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
