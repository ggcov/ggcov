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

#include "fakepopt.h"

#ifndef HAVE_LIBPOPT

CVSID("$Id: fakepopt.c,v 1.5 2010-05-09 05:37:15 gnb Exp $");

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
    const char *current;

    int nfiles;
    const char **files;
    int filei;
    gboolean file_mode;

    char *other_option_help;

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

    con = g_new0(struct _poptContext, 1);

    con->argc = argc;
    con->argv = argv;
    con->argi = 0;  /* points at last argument visited */

    con->nfiles = 0;
    con->files = g_new0(const char *, con->argc);
    con->filei = 0;
    con->file_mode = FALSE;

    con->options = options;

    return con;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
poptSetOtherOptionHelp(poptContext con, const char *help)
{
    con->other_option_help = g_strdup(help);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const struct poptOption *
find_long_option(const struct poptOption *opt, const char *name, const char **valp)
{
    const struct poptOption *o;
    int len;

    for ( ; opt->long_name || opt->short_name ; opt++)
    {
	switch (opt->type)
	{
	case POPT_ARG_INCLUDE_TABLE:
	    o = find_long_option((const struct poptOption *)opt->value_ptr, name, valp);
	    if (o != 0)
		return o;
	    break;
	default:
	    len = strlen(opt->long_name);
	    if (!strncmp(name, opt->long_name, len) && name[len] == '=')
	    {
		*valp = name+len+1;
		return opt;
	    }
	    /* fall through */
	case POPT_ARG_NONE:
	    if (!strcmp(name, opt->long_name))
		return opt;
	    break;
	}
    }
    return 0;
}

static const struct poptOption *
find_short_option(const struct poptOption *opt, char name)
{
    const struct poptOption *o;

    for ( ; opt->long_name || opt->short_name ; opt++)
    {
	switch (opt->type)
	{
	case POPT_ARG_INCLUDE_TABLE:
	    o = find_short_option((const struct poptOption *)opt->value_ptr, name);
	    if (o != 0)
		return o;
	    break;
	default:
	    if (name == opt->short_name)
		return opt;
	    break;
	}
    }
    return 0;
}

static int
handle_option(poptContext con, const struct poptOption *opt, const char *val)
{
    if (opt == 0)
	return -2;  /* unknown option */
    assert(opt->value_ptr != 0);
    assert(opt->value == 0);
    switch (opt->type)
    {
    case POPT_ARG_NONE:
	assert(val == 0);
	*(int *)opt->value_ptr = TRUE;
	break;
    case POPT_ARG_STRING:
	if (val == 0)
	{
	    val = con->argv[++con->argi];
	    if (val == 0)
		return -3; /* no value */
	}
	*(const char **)opt->value_ptr = val;
	break;
    }
    return opt->value;
}

int
poptGetNextOpt(poptContext con)
{
    const char *arg;
    int rc;
    const char *val;
    const struct poptOption *opt;

    for (;;)
    {
	arg = con->argv[++con->argi];
	if (arg == 0)
	    return -1;  /* end of arguments */
	con->current = arg;

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
	    val = 0;
	    opt = find_long_option(con->options, arg+2, &val);
	    if ((rc = handle_option(con, opt, val)))
		return rc;
	}
	else
	{
	    for (arg++ ; *arg ; arg++)
	    {
		if ((rc = handle_option(con, find_short_option(con->options, *arg), 0)))
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
    case -3: return "no value";
    default: return "no error";
    }
}

const char *
poptBadOption(poptContext con, int ignored)
{
    return con->current;
}

void
poptFreeContext(poptContext con)
{
    if (con->other_option_help != 0)
	free(con->other_option_help);
    free(con->files);
    free(con);
}

#ifdef __cplusplus
}
#endif

#endif /* !HAVE_LIBPOPT */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
