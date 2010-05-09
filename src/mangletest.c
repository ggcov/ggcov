/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2003 Greg Banks <gnb@users.sourceforge.net>
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
#include <stdio.h>
#include "demangle.h"

const char *argv0;

int
main(int argc, char **argv)
{
    enum { DEMANGLE, NORMALISE } mode = DEMANGLE;
    int i;

    argv0 = argv[0];
    
    for (i = 1 ; i < argc ; i++)
    {
    	if (!strcmp(argv[i], "--demangle"))
	{
	    mode = DEMANGLE;
	}
	else if (!strcmp(argv[i], "--normalise"))
	{
	    mode = NORMALISE;
	}
	else if (argv[i][0] == '-')
	{
	    fprintf(stderr, "Usage: mangletest [--demangle|--normalise] sym...\n");
	    exit(1);
	}
	else
	{
	    char *res;

	    if (mode == DEMANGLE)
	    {
	    	res = demangle(argv[i]);
		printf("demangle(\"%s\") =\n\"%s\"\n",
		    argv[i],
		    (res == 0 ? "(null)" : res));
	    }
	    else
	    {
	    	res = normalise_mangled(argv[i]);
		printf("normalise_mangled(\"%s\") =\n\"%s\"\n",
		    argv[i],
		    (res == 0 ? "(null)" : res));
	    }
	    if (res)
	    	g_free(res);
	}
    }
    
    return 0;
}

