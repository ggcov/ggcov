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

#include "cov.H"
#include "cov_suppression.H"

hashtable_t<const char, const cov_suppression_t> *
    cov_suppression_t::by_word_[cov_suppression_t::NUM_TYPES];
list_t<const cov_suppression_t> cov_suppression_t::prefixed_[NUM_TYPES];
list_t<const cov_suppression_t> cov_suppression_t::all_[NUM_TYPES];

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_suppression_t::cov_suppression_t(const char *w,
				     type_t t,
				     const char *o)
 :  word_(w),
    type_(t),
    origin_(o)
{
    if (w)
    {
	const char *star = strchr(word_, '*');
	if (star)
	{
	    /* only a trailing * is supported */
	    assert(star[1] == '\0');
	    prefixed_[t].append(this);
	}
	else
	{
	    if (!by_word_[t])
		by_word_[t] = new hashtable_t<const char, const cov_suppression_t>;
	    by_word_[t]->insert(word_, this);
	}
    }
    all_[t].append(this);
}

cov_suppression_t::~cov_suppression_t()
{
    all_[type_].remove(this);
    prefixed_[type_].remove(this);
    by_word_[type_]->remove(word_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const cov_suppression_t *
cov_suppression_t::find(const char *w, type_t t)
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
	const char *star = strchr(s->word_, '*');
	if (!strncmp(w, s->word_, (star - s->word_)))
	    return s;
    }
    return 0;
}

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

void
cov_suppression_t::init_builtins(void)
{
    static const init_t inits[] =
    {
	/* system include files */
	{ FILENAME, "/usr/include/*" },
	{ FILENAME, "/usr/lib/*" },
	/* global constructors */
	{ FUNCTION, "_GLOBAL_*" },
	/* inlines in glibc's </sys/sysmacros.h> */
	{ FUNCTION, "gnu_dev_major" },
	{ FUNCTION, "gnu_dev_minor" },
	{ FUNCTION, "gnu_dev_makedev" },
	/* inlines in glibc's <sys/stat.h> */
	{ FUNCTION, "stat" },
	{ FUNCTION, "lstat" },
	{ FUNCTION, "fstat" },
	{ FUNCTION, "mknod" },
	/* gcc 3.4 exception handling */
	{ ARC_CALLS,	"__cxa_allocate_exception" },
	{ ARC_CALLS,	"__cxa_begin_catch" },
	{ ARC_CALLS,	"__cxa_call_unexpected" },
	{ ARC_CALLS,	"__cxa_end_catch" },
	{ ARC_CALLS,	"__cxa_throw" },
	{ ARC_CALLS,	"_Unwind_Resume" },
	/* externs in glibc's <assert.h> */
	{ BLOCK_CALLS, "__assert_fail" },
	{ BLOCK_CALLS, "__assert_perror_fail" },
	{ BLOCK_CALLS, "__assert" },
	{ BLOCK_CALLS, "abort" },
	/* millicode & other relocs to ignore while
	 * scanning object code for calls */
	{ RELOC, "__bb_init_func" },	    /* code inserted by gcc to instrument blocks */
	{ RELOC, "__gcov_init" },	    /* a more modern version of the same */
	{ RELOC, "_Unwind_Resume" },	    /* gcc 3.4 exception handling */
	{ RELOC, "__cxa_call_unexpected" }, /* gcc 3.4 exception handling */
	{ RELOC, "__cxa_end_catch" },	    /* gcc 3.4 exception handling */
	{ RELOC, "__i686.get_pc_thunk.bx" },/* gcc 4.x -fPIC */
	/* Note: -fstack-protector is on by default on Ubuntu */
	{ RELOC, "__stack_chk_fail" },	    /* gcc 4.x -fstack-protector */
	{ RELOC, "__stack_chk_fail_local" },/* gcc 4.x -fstack-protector */
	{ (type_t)0, 0 }
    };
    const init_t *si;

    for (si = inits ; si->word ; si++)
	new cov_suppression_t(si->word, si->type, "builtin defaults");

    new cov_suppression_t(0, MERGED, "builtin defaults");

    new cov_suppression_t(0, UNSOLVABLE, "builtin defaults");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
