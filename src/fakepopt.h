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

#ifndef _fakepopt_h_
#define _fakepopt_h_ 1

#include "common.h"

#ifdef HAVE_LIBPOPT
#include <popt.h>
#else

#ifdef __cplusplus
extern "C" {
#endif

struct poptOption
{
    const char *longName;
    char shortName;
    unsigned int argInfo;
    void *arg;
    int val;
    const char *descrip;
    const char *argDescrip;
};

#define POPT_ARG_NONE		0U
#define POPT_ARG_STRING		1U
#define POPT_ARG_INCLUDE_TABLE	4U
#define POPT_ARG_CALLBACK	5U

#define POPT_BADOPTION_NOALIAS  0

#define POPT_AUTOHELP
#define POPT_TABLEEND { NULL, '\0', 0, 0, 0, NULL, NULL }

enum poptCallbackReason {
    POPT_CALLBACK_REASON_OPTION=2
};

typedef struct _poptContext *poptContext;

extern poptContext poptGetContext(const char *, int, const char **,
				  const struct poptOption *, int);
extern void poptSetOtherOptionHelp(poptContext, const char *);
extern int poptGetNextOpt(poptContext);
extern const char *poptGetArg(poptContext);
extern const char *poptStrerror(int);
extern const char *poptBadOption(poptContext, int);
extern void poptFreeContext(poptContext);

#ifdef __cplusplus
}
#endif

#endif /* !HAVE_LIBPOPT */

#endif /* _fakepopt_h_ */
