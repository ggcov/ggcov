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

typedef void (*poptCallbackFn)(poptContext, enum poptCallbackReason,
				const struct poptOption*, const char *, void *);

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

#define is_end(opt) \
    (opt->longName == NULL && opt->shortName == '\0' && opt->argInfo == 0)

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
find_long_option(const struct poptOption *opt, const char *name, const char **valp,
		 poptCallbackFn *callbackp, void **callback_datap)
{
    const struct poptOption *o;
    int len;

    *callbackp = NULL;
    *callback_datap = NULL;
    for ( ; !is_end(opt) ; opt++)
    {
	switch (opt->argInfo)
	{
	case POPT_ARG_CALLBACK:
	    *callbackp = (poptCallbackFn)opt->arg;
	    *callback_datap = (void *)opt->descrip;
	    break;
	case POPT_ARG_INCLUDE_TABLE:
	    o = find_long_option((const struct poptOption *)opt->arg, name, valp,
				 callbackp, callback_datap);
	    if (o != 0)
		return o;
	    break;
	default:
	    len = strlen(opt->longName);
	    if (!strncmp(name, opt->longName, len) && name[len] == '=')
	    {
		*valp = name+len+1;
		return opt;
	    }
	    /* fall through */
	case POPT_ARG_NONE:
	    if (!strcmp(name, opt->longName))
		return opt;
	    break;
	}
    }
    return 0;
}

static const struct poptOption *
find_short_option(const struct poptOption *opt, char name,
		  poptCallbackFn *callbackp, void **callback_datap)
{
    const struct poptOption *o;

    *callbackp = NULL;
    *callback_datap = NULL;
    for ( ; !is_end(opt) ; opt++)
    {
	switch (opt->argInfo)
	{
	case POPT_ARG_CALLBACK:
	    *callbackp = (poptCallbackFn)opt->arg;
	    *callback_datap = (void *)opt->descrip;
	    break;
	case POPT_ARG_INCLUDE_TABLE:
	    o = find_short_option((const struct poptOption *)opt->arg, name,
				  callbackp, callback_datap);
	    if (o != 0)
		return o;
	    break;
	default:
	    if (name == opt->shortName)
		return opt;
	    break;
	}
    }
    return 0;
}

static int
handle_option(poptContext con, const struct poptOption *opt, const char *val,
	      poptCallbackFn callback, void *callback_data)
{
    if (opt == 0)
	return -2;  /* unknown option */
    if (callback)
    {
	callback(con, POPT_CALLBACK_REASON_OPTION, opt, val, callback_data);
	return 0;
    }
    assert(opt->arg != 0);
    assert(opt->val == 0);
    switch (opt->argInfo)
    {
    case POPT_ARG_NONE:
	assert(val == 0);
	*(int *)opt->arg = TRUE;
	break;
    case POPT_ARG_STRING:
	if (val == 0)
	{
	    val = con->argv[++con->argi];
	    if (val == 0)
		return -3; /* no value */
	}
	*(const char **)opt->arg = val;
	break;
    }
    return opt->val;
}

int
poptGetNextOpt(poptContext con)
{
    const char *arg;
    int rc;
    const char *val;
    const struct poptOption *opt;
    poptCallbackFn callback = NULL;
    void *callback_data = NULL;

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
	    opt = find_long_option(con->options, arg+2, &val,
				   &callback, &callback_data);
	    if ((rc = handle_option(con, opt, val, callback, callback_data)))
		return rc;
	}
	else
	{
	    for (arg++ ; *arg ; arg++)
	    {
		opt = find_short_option(con->options, *arg, &callback, &callback_data);
		if ((rc = handle_option(con, opt, 0, callback, callback_data)))
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
