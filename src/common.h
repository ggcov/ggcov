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

/*
 * Define as non-zero to turn on debugging prints.
 */
#ifndef DEBUG 
#define DEBUG 0
#endif

/*
 * The fake flag used to be present in the .bbg file format, but
 * disappeared sometime before gcc 2.96.  It was always redundant
 * information, but convenient.  On the bright side, we don't need
 * to cross-check it anymore.
 */
#undef HAVE_BBG_FAKE_FLAG

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

#define safestr(s)  ((s) == 0 ? "" : (s))

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
