/*
 * CANT - A C implementation of the Apache/Tomcat ANT build system
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

#include "common.h"

CVSID("$Id: common.c,v 1.6 2003-02-18 14:47:04 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern char *argv0;

void
fatal(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    fprintf(stderr, "%s: ", argv0);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    exit(1);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern char *argv0;

void *
gnb_xmalloc(size_t sz)
{
    void *x;
    
    if ((x = g_malloc0(sz)) == 0)
    {
    	static const char message[] = ": no memory, exiting\n";
	
	write(2, argv0, strlen(argv0));
	write(2, message, sizeof(message)-1);
	exit(1);
    }
    
    return x;
}

void *
operator new(size_t sz)
{
    return gnb_xmalloc(sz);
}

void
operator delete(void *p)
{
    if (p != 0)     /* delete 0 is explicitly allowed */
	free(p);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
u32cmp(gnb_u32_t ul1, gnb_u32_t ul2)
{
    if (ul1 > ul2)
    	return 1;
    if (ul1 < ul2)
    	return -1;
    return 0;
}

int
u64cmp(gnb_u64_t ull1, gnb_u64_t ull2)
{
    if (ull1 > ull2)
    	return 1;
    if (ull1 < ull2)
    	return -1;
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
