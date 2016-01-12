/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2005 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _common_h_
#define _common_h_ 1

/* Include autoconf defines */
#include <config.h>

#if HAVE_STDINT_H
/*
 * Glibc <stdint.h> says:
 * The ISO C 9X standard specifies that in C++ implementations these
 * macros [UINT64_MAX et al] should only be defined if explicitly requested.
 */
#define __STDC_LIMIT_MACROS 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#if HAVE_STDINT_H
#include <stdint.h>
#endif
#if HAVE_STDBOOL_H
#include <stdbool.h>
#endif

#include <glib.h>

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if !(defined(__GXX_EXPERIMENTAL_CXX0X__) || (__cplusplus >= 201103L))
#define noexcept
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define listdelete(v,type,dtor) \
    do { \
	while ((v) != 0) \
	{ \
	    dtor((type *)(v)->data); \
	    (v) = g_list_remove_link((v), (v)); \
	} \
    } while(0)

#define listclear(v) \
    do { \
	while ((v) != 0) \
	{ \
	    (v) = g_list_remove_link((v), (v)); \
	} \
    } while(0)

#define boolassign(bv, s) \
    (bv) = strbool((s), (bv))

#define boolstr(b)      ((b) ? "true" : "false")

#if !HAVE_STDBOOL_H
typedef unsigned char _bool;
#define bool _bool
#define false (0U)
#define true (1U)
#endif

#define safestr(s)  ((s) == 0 ? "" : (s))
static inline int safe_strcmp(const char *a, const char *b)
{
    return strcmp(safestr(a), safestr(b));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern void fatal(const char *fmt, ...)
#ifdef __GNUC__
__attribute__ (( noreturn ))
__attribute__ (( format(printf,1,2) ))
#endif
;

extern void *gnb_xmalloc(size_t sz);
#ifdef __cplusplus
/*
 * Operators new and delete operators are matched to avoid
 * complaints from valgrind and Purify about non-portable
 * mixing of malloc and operator delete.  Other than that,
 * the overloaded global operator delete provides no value.
 */
void *operator new(size_t sz);
void operator delete(void *) noexcept;
#else
#define new(ty)         ((ty *)gnb_xmalloc(sizeof(ty)))
#define delete(p)       free((p))
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Unsigned 32-bit and 64-bit integral types are used in various
 * places throughout ggcov, in particular the files store execution
 * counts as 64-bit numbers.  We use <stdint.h> where its available
 * otherwise fall back to other less modern & portable techniques.
  */

#if !HAVE_STDINT_H
/* works on 32 bit machines with gcc */
typedef unsigned long           uint32_t;
typedef unsigned long long      uint64_t;
#endif

/* These work for 32 bit machines */
/* TODO: make these portable by calculating 0,1,or 2 'l's in configure */
#define GNB_U32_DFMT            "%u"
#define GNB_U32_XFMT            "%08x"

#define GNB_U64_DFMT            "%llu"
#define GNB_U64_XFMT            "%016llx"

/* Comparison functions, for sorting */
extern int u32cmp(uint32_t, uint32_t);
extern int u64cmp(uint64_t, uint64_t);

extern void timing_impl(const char *func, unsigned int line,
			const char *detail);
#define timing() timing_impl(__FUNCTION__, __LINE__, 0)
#define timingx(detail) timing_impl(__FUNCTION__, __LINE__, (detail))

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* These functions localize and neutralize the annoying warning
 * "ISO C++ forbids casting between pointer-to-function and
 * pointer-to-object". */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

inline void (*object_to_function(void *p))()
{
    return (void (*)())p;
}

inline void *function_to_object(void (*fn)())
{
    return (void *)fn;
}

#pragma GCC diagnostic pop
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#ifdef _
#undef _
#endif
#define _(s)    s

#ifdef N_
#undef N_
#endif
#define N_(s)   s

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


#endif /* _common_h_ */
