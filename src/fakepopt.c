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

#include "fakepopt.h"

#ifndef HAVE_LIBPOPT

/*
 * Simulate enough of the popt interface so that I only have
 * to write and maintain a single argument parsing routine for
 * all builds, e.g. gtk1.2, gtk2+popt, gtk2-popt.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct _poptContext
{
    int argc;
    const char **argv;
    int argi;

    int nfiles;
    const char **files;
    int filei;
    gboolean file_mode;

    const struct poptOption *options;
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

poptContext
poptGetContext(
    const char *ignored,
    int argc,
    const char **argv,
    const struct poptOption *options,
    int flags)
{
    poptContext con;
    
    con = (poptContext)gnb_xmalloc(sizeof(*con));

    con->argc = argc;
    con->argv = argv;
    con->argi = 0;  /* points at last argument visited */
    
    con->nfiles = 0;
    con->files = (const char **)gnb_xmalloc(sizeof(const char *) * con->argc);
    con->filei = 0;
    con->file_mode = FALSE;

    con->options = options;

    return con;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const struct poptOption *
find_long_option(poptContext con, const char *name)
{
    const struct poptOption *opt;

    for (opt = con->options ; opt->long_name || opt->short_name ; opt++)
    {
	if (!strcmp(name, opt->long_name))
	    return opt;
    }
    return 0;
}

static const struct poptOption *
find_short_option(poptContext con, char name)
{
    const struct poptOption *opt;

    for (opt = con->options ; opt->long_name || opt->short_name ; opt++)
    {
	if (name == opt->short_name)
	    return opt;
    }
    return 0;
}

static int
handle_option(poptContext con, const struct poptOption *opt)
{
    if (opt == 0)
    	return -2;  /* unknown option */
    assert(opt->type == POPT_ARG_NONE);
    assert(opt->value_ptr != 0);
    assert(opt->value == 0);
    *(int *)opt->value_ptr = TRUE;
    return opt->value;
}

int
poptGetNextOpt(poptContext con)
{
    const char *arg;
    int rc;
    
    for (;;)
    {
	arg = con->argv[++con->argi];
	if (arg == 0)
    	    return -1;  /* end of arguments */

    	if (con->file_mode || arg[0] != '-')
	{
    	    con->files[con->nfiles++] = arg;
	}
	else if (arg[0] == '-' && arg[1] == '-')
	{
	    if (arg[2] == '\0')
	    {
	    	/* handle '--' */
	    	con->file_mode = TRUE;
	    	continue;
	    }
	    if ((rc = handle_option(con, find_long_option(con, arg+2))))
	    	return rc;
	}
	else
	{
	    for (arg++ ; *arg ; arg++)
	    {
		if ((rc = handle_option(con, find_short_option(con, *arg))))
	    	    return rc;
	    }
	}
    }
    /*NOTREACHED*/
    return 0;
}

const char *
poptGetArg(poptContext con)
{
    if (con->filei >= con->nfiles)
    	return 0;
    return con->files[con->filei++];
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
poptStrerror(int rc)
{
    switch (rc)
    {
    case -2: return "bad option";
    default: return "no error";
    }
}

const char *
poptBadOption(poptContext con, int ignored)
{
    return con->argv[con->argi];
}

void
poptFreeContext(poptContext con)
{
    free(con->files);
    free(con);
}

#ifdef __cplusplus
}
#endif

#endif /* !HAVE_LIBPOPT */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
