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

#include "common.h"
#include "demangle.h"
#include "estring.H"
#include "string_var.H"

#ifdef HAVE_LIBBFD
extern "C" {
#include "libiberty/demangle.h"
}
#endif

CVSID("$Id: demangle.c,v 1.2 2003-06-28 10:31:43 gnb Exp $");

#define LEFT_BRACKET	'('
#define RIGHT_BRACKET	')'

static inline int
issym(char c)
{
    return (isalnum(c) || c == ':' || c == '_');
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#ifdef HAVE_LIBBFD
static char *
normalise_whitespace(const char *p)
{
    estring r;
    char lastc;

    /*
     * Run through the symbol forwards, building a new string
     * `r' which is the same but with probably less whitespace.
     * First, preallocate slightly more than enough room in `r'.
     */
    r.append_string(p);
    r.truncate();

    lastc = '\0';
    for (;;)
    {
    	/* skip any whitespace */
	while (*p && isspace(*p))
	    p++;
	if (!*p)
	    break;

    	/*
	 * Preserve a single space only between consecutive
	 * alphanumerics or after commas.
	 */
	if ((issym(*p) && issym(lastc)))
	    r.append_char(' ');

    	/* append sequence of non-whitespace */
	while (*p && !isspace(*p))
	    r.append_char(*p++);
	if (!*p)
	    break;
	lastc = p[-1];
    }
        
    return r.take();
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
demangle(const char *sym)
{
#ifdef HAVE_LIBBFD
    string_var dem = cplus_demangle(sym, DMGL_ANSI|DMGL_PARAMS);
    return (dem == 0 ? g_strdup(sym) : normalise_whitespace(dem));
#else
    return g_strdup(sym);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
normalise_mangled(const char *sym)
{
#ifdef HAVE_LIBBFD
    string_var buf = sym;
    int func_count = 0;
    char *p;
    
    p = (char *)buf.data() + strlen(buf.data()) - 1;
    
    if (strchr(buf.data(), LEFT_BRACKET) == 0)
    	return buf.take();  /* not mangled */

    /*
     * Use a simple state machine to strip off the return type.
     * It's simple (rather than trivial) to handle return types
     * which are pointers to functions.
     */
    do
    {
	while (p > buf.data() && isspace(*p))
    	    p--;

    	if (*p == RIGHT_BRACKET)
	{
	    int bracket_count = 0;
	    do
	    {
    		switch (*p--)
		{
		case LEFT_BRACKET:
		    bracket_count--;
		    break;
		case RIGHT_BRACKET:
		    bracket_count++;
		    break;
		}
	    }
	    while (p > buf.data() && bracket_count > 0);
    	}

	while (p > buf.data() && isspace(*p))
    	    p--;

	if (*p == RIGHT_BRACKET)
	{
	    *p-- = '\0';
    	    func_count++;
	}
	else if (*p == LEFT_BRACKET)
	{
	    *p = ' ';
	    --func_count;
	}
	else
	{
	    while (p > buf.data() && issym(*p))
    		p--;
	    if (p > buf.data() && (*p == '*' || *p == '&'))
	    	*p-- = ' ';
	}
    }
    while (func_count);

    /*
     * At this point `p' points to (some whitespace before)
     * the symbol with the return type stripped.  Now normalise
     * the whitespace.
     */
    buf = normalise_whitespace(p);
    
    /*
     * The C++ main routine is mangled as just "main".
     */
    if (!strcmp(buf, "main(int,char**)") ||
    	!strcmp(buf, "main(int,char**,char**)"))
    	buf = "main";

    return buf.take();
#else
    return g_strdup(sym);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
