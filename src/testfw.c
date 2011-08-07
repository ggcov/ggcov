/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2011 Greg Banks <gnb@users.sourceforge.net>
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include "testfw.h"

static int verbose = 0;

testfn_t *testfn_t::head_, **testfn_t::tailp_ = &head_;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/* memory allocation wrappers */

static void _testfw_oom(void) __attribute__((noreturn));

static void
_testfw_oom(void)
{
    fputs("testfw: out of memory\n", stderr);
    fflush(stderr);
    exit(1);
}

static void *
_testfw_realloc(void *x, size_t sz)
{
    void *r;

    r = (!x ? malloc(sz) : realloc(x, sz));
    if (!r)
	_testfw_oom();
    return r;
}

static char *
_testfw_strndup(const char *s, size_t sz)
{
    char *r = (char *)malloc(sz+1);
    if (!r)
	_testfw_oom();
    memcpy(r, s, sz);
    r[sz] = '\0';
    return r;
}

static char *
_testfw_strdup(const char *s)
{
    char *r = strdup(s);
    if (!r)
	_testfw_oom();
    return r;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
__testfw_check(int pass, const char *file, int line, const char *fmt, ...)
{
    va_list args;
    char buf[2048];

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (!pass || verbose)
	fprintf(stderr, "%s:%d: %s: %s\n",
	        file, line, (pass ? "passed" : "FAILED"), buf);

    if (!pass)
    {
	fflush(stderr);
	abort();
	/*NOTREACHED*/
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
testfn_t::run(void)
{
    if (role_ == RTEST)
    {
	if (verbose)
	    fprintf(stderr, "Test %s:%s\n", suite_, name_);
	function_();
    }
    else
    {
	int (*fixture)(void) = (int(*)(void))function_;
	if (verbose)
	    fprintf(stderr, "Fixture %s:%s\n", suite_, name_);
	if (fixture())
	{
	    fprintf(stderr, "%s:%s: fixture failed\n", suite_, name_);
	    exit(1);
	}
    }
}

void
testfn_t::init(void)
{
    /* convert the .suite_ string from a filenames
     * to a suite name */

    /* find the pathname tail */
    const char *b = strrchr(suite_, '/');
    if (b)
	b++;
    else
	b = suite_;

    /* find filename suffix - e points to the char
     * after the last one we want to keep */
    const char *e = strrchr(b, '.');
    if (!e)
	e = b + strlen(b);

    /* drop a "test" prefix or suffix if they exist */
    if (!strncasecmp(b, "test", 4))
	b += 4;
    if (!strncasecmp(e-4, "test", 4))
	e -= 4;

    /* skip likely word separators at the start and end */
    for ( ; *b && (*b == '_' || *b == '.' || *b == '-') ; b++)
	;
    for ( ; e > b && (e[-1] == '_' || e[-1] == '.' || e[-1] == '-') ; --e)
	;

    /* make a copy */
    suite_ = _testfw_strndup(b, e-b);
}

void
testfn_t::uninit()
{
    /* free the .suite_ string */
    free((char *)suite_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static testfn_t **scheduled;
static unsigned int nscheduled;

static void
schedule(testfn_t *fn)
{
    testfn_t *setup = 0, *teardown = 0;
    int n;

    /* not very efficient, but who cares, it's more
     * important to be correct in the test framework */

    for (testfn_t *ff = testfn_t::head_ ; ff ; ff = ff->next_)
    {
	if (strcmp(ff->suite_, fn->suite_))
	    continue;
	if (ff->role_ == testfn_t::RSETUP)
	    setup = ff;
	else if (ff->role_ == testfn_t::RTEARDOWN)
	    teardown = ff;
    }

    n = 1;
    if (setup)
	n++;
    if (teardown)
	n++;

    scheduled = (testfn_t **)_testfw_realloc(scheduled, sizeof(testfn_t *) * (nscheduled+n));

    if (setup)
	scheduled[nscheduled++] = setup;
    scheduled[nscheduled++] = fn;
    if (teardown)
	scheduled[nscheduled++] = teardown;
}

void
testfw_init(void)
{
    for (testfn_t *fn = testfn_t::head_ ; fn ; fn = fn->next_)
	fn->init();
}

void
testfw_set_verbose(int v)
{
    verbose = 1;
}

void
testfw_list(void)
{
    for (testfn_t *fn = testfn_t::head_ ; fn ; fn = fn->next_)
    {
	if (fn->role_ == testfn_t::RTEST)
	    printf("%s:%s\n", fn->suite_, fn->name_);
    }
}

static int
_testfw_schedule_matching(const char *suite, const char *name)
{
    int nmatches = 0;

    for (testfn_t *fn = testfn_t::head_ ; fn ; fn = fn->next_)
    {
	if (fn->role_ != testfn_t::RTEST)
	    continue;
	if (suite && strcmp(suite, fn->suite_))
	    continue;
	if (name && strcmp(name, fn->name_))
	    continue;
	schedule(fn);
	nmatches++;
    }
    return nmatches;
}

int
testfw_schedule(const char *arg)
{
    char *name = 0;
    char *suite = _testfw_strdup(arg);
    char *p = strchr(suite, ':');
    if (p)
    {
	*p++ = '\0';
	name = p;
    }
    int r = _testfw_schedule_matching(suite, name);
    free(suite);
    return r;
}

void
testfw_run(void)
{
    if (!nscheduled)
	_testfw_schedule_matching(0, 0);

    for (unsigned int i = 0 ; i < nscheduled ; i++)
	scheduled[i]->run();

    for (testfn_t *fn = testfn_t::head_ ; fn ; fn = fn->next_)
	fn->uninit();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
