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

#include <glib.h>

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

typedef enum
{
    /* bit0 is the "verbose" bit -- the __debug_enabled macro relies on this */
    /* cov library related features */
    D_FILES=1,
    D_CPP,
    D_BB,
    D_BBG,
    D_DA,
    D_IO,
    D_CGRAPH,
    D_SOLVE,
    D_STABS,
    D_DWARF,
    D_ELF,
    D_DUMP,
    D_REPORT,
    D_DCALLGRAPH,
    D_DLEGO,
    /* gui related features */
    D_UICORE,
    D_SUMMARYWIN,
    D_SOURCEWIN,
    D_PREFSWIN,
    D_CALLSWIN,
    D_FUNCSWIN,
    D_FILESWIN,
    D_GRAPHWIN,
    D_DIAGWIN,
    D_REPORTWIN,
    /* web related features */
    D_WEB,
    /* bitwise OR in D_VERBOSE to enable printing only when verbose */
#define _D_VERBOSE_SHIFT    7
    D_VERBOSE=(1<<_D_VERBOSE_SHIFT)
} debug_features_t;
extern unsigned long debug;
void debug_set(const char *str);
char *debug_enabled_tokens(void);

/*
 * The beauty of this arrangement is that `debug' is ANDed with
 * a compile-time constant so the generated code is efficient.
 */
#define __debug_mask(val) \
    ((1UL << ((val) & ~D_VERBOSE)) | (((val) & D_VERBOSE) >> _D_VERBOSE_SHIFT))
#define __debug_enabled(val) \
    ((debug & __debug_mask(val)) == __debug_mask(val))

#ifdef __GNUC__
/*
 * Give the compiler a frequency hint to hopefully move
 * the debug code out of line where that helps.
 */
#define debug_enabled(val)  	__builtin_expect(__debug_enabled(val), 0)
#else
#define debug_enabled(val)	__debug_enabled(val)
#endif

/* I wish I could rely on variadic macros */
/* Conditional debugging prints */
#define dprintf0(val, fmt) \
    if (debug_enabled(val)) fprintf(stderr, (fmt))
#define dprintf1(val, fmt, a1) \
    if (debug_enabled(val)) fprintf(stderr, (fmt), (a1))
#define dprintf2(val, fmt, a1, a2) \
    if (debug_enabled(val)) fprintf(stderr, (fmt), (a1), (a2))
#define dprintf3(val, fmt, a1, a2, a3) \
    if (debug_enabled(val)) fprintf(stderr, (fmt), (a1), (a2), (a3))
#define dprintf4(val, fmt, a1, a2, a3, a4) \
    if (debug_enabled(val)) fprintf(stderr, (fmt), (a1), (a2), (a3), (a4))
#define dprintf5(val, fmt, a1, a2, a3, a4, a5) \
    if (debug_enabled(val)) fprintf(stderr, (fmt), (a1), (a2), (a3), (a4), (a5))
#define dprintf6(val, fmt, a1, a2, a3, a4, a5, a6) \
    if (debug_enabled(val)) fprintf(stderr, (fmt), (a1), (a2), (a3), (a4), (a5), (a6))
#define dprintf7(val, fmt, a1, a2, a3, a4, a5, a6, a7) \
    if (debug_enabled(val)) fprintf(stderr, (fmt), (a1), (a2), (a3), (a4), (a5), (a6), (a7))
#define dprintf8(val, fmt, a1, a2, a3, a4, a5, a6, a7, a8) \
    if (debug_enabled(val)) fprintf(stderr, (fmt), (a1), (a2), (a3), (a4), (a5), (a6), (a7), (a8))
/* Unconditional debugging prints, use inside if(debug_enabled()) */
#define duprintf0(fmt) \
    fprintf(stderr, (fmt))
#define duprintf1(fmt, a1) \
    fprintf(stderr, (fmt), (a1))
#define duprintf2(fmt, a1, a2) \
    fprintf(stderr, (fmt), (a1), (a2))
#define duprintf3(fmt, a1, a2, a3) \
    fprintf(stderr, (fmt), (a1), (a2), (a3))
#define duprintf4(fmt, a1, a2, a3, a4) \
    fprintf(stderr, (fmt), (a1), (a2), (a3), (a4))
#define duprintf5(fmt, a1, a2, a3, a4, a5) \
    fprintf(stderr, (fmt), (a1), (a2), (a3), (a4), (a5))
#define duprintf6(fmt, a1, a2, a3, a4, a5, a6) \
    fprintf(stderr, (fmt), (a1), (a2), (a3), (a4), (a5), (a6))
#define duprintf7(fmt, a1, a2, a3, a4, a5, a6, a7) \
    fprintf(stderr, (fmt), (a1), (a2), (a3), (a4), (a5), (a6), (a7))
#define duprintf8(fmt, a1, a2, a3, a4, a5, a6, a7, a8) \
    fprintf(stderr, (fmt), (a1), (a2), (a3), (a4), (a5), (a6), (a7), (a8))

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

#define boolstr(b)  	((b) ? "true" : "false")

/* boolean, true, and false need to be defined always, and
 * unsigned so that struct fields of type boolean:1 do not
 * generate whiny gcc warnings */
#ifdef boolean
#undef boolean
#endif
#define boolean	    unsigned int

#ifdef false
#undef false
#endif
#define false	    (0U)

#ifdef false
#undef false
#endif
#define false	    (0U)

#define safestr(s)  ((s) == 0 ? "" : (s))
static inline int safe_strcmp(const char *a, const char *b)
{
    return strcmp(safestr(a), safestr(b));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if defined(__ELF__) && defined(__GNUC__)
#define _CVSID(u,s) __asm__ (".ident \"" s "\"" )
#else
#define _CVSID(u,s) static const char * const __cvsid##u[2] = {(s), (const char*)__cvsid}
#endif
#define CVSID(s) _CVSID(,s)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern void fatal(const char *fmt, ...)
#ifdef __GNUC__
__attribute__ (( noreturn ))
__attribute__ (( format(printf,1,2) ))
#endif
;

extern void *gnb_xmalloc(size_t sz);
extern void *gnb_xcalloc(size_t n, size_t sz);
extern void *gnb_xrealloc(void *p, size_t sz);
#ifdef __cplusplus
/*
 * Operators new and delete operators are matched to avoid
 * complaints from valgrind and Purify about non-portable
 * mixing of malloc and operator delete.  Other than that,
 * the overloaded global operator delete provides no value.
 */
void *operator new(size_t sz);
void operator delete(void *);
#else
#define new(ty)     	((ty *)gnb_xmalloc(sizeof(ty)))
#define delete(p)     	free((p))
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Unsigned 32-bit and 64-bit integral types are used in various
 * places throughout ggcov, in particular the files store execution
 * counts as 64-bit numbers.  We use <stdint.h> where its available
 * otherwise fall back to other less modern & portable techniques.
  */

#if HAVE_STDINT_H
typedef uint32_t    	    	gnb_u32_t;
typedef uint64_t    	    	gnb_u64_t;
#else
/* works on 32 bit machines with gcc */
typedef unsigned long	    	gnb_u32_t;
typedef unsigned long long	gnb_u64_t;
#endif

/* These work for 32 bit machines */
/* TODO: make these portable by calculating 0,1,or 2 'l's in configure */
#define GNB_U32_DFMT	    	"%u"
#define GNB_U32_XFMT	    	"%08x"

#define GNB_U64_DFMT	    	"%llu"
#define GNB_U64_XFMT	    	"%016llx"

/* Comparison functions, for sorting */
extern int u32cmp(gnb_u32_t, gnb_u32_t);
extern int u64cmp(gnb_u64_t, gnb_u64_t);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#ifdef _
#undef _
#endif
#define _(s)	s

#ifdef N_
#undef N_
#endif
#define N_(s)	s

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


#endif /* _common_h_ */
