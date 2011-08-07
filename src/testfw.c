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
testfn_t::run()
{
    if (role_ == RTEST)
    {
	if (testrunner_t::current_->verbose_)
	    fprintf(stderr, "Test %s:%s\n", suite(), name_);
	function_();
    }
    else
    {
	int (*fixture)(void) = (int(*)(void))function_;
	if (testrunner_t::current_->verbose_)
	    fprintf(stderr, "Fixture %s:%s\n", suite(), name_);
	if (fixture())
	{
	    fprintf(stderr, "%s:%s: fixture failed\n", suite(), name_);
	    exit(1);
	}
    }
}

const char *
testfn_t::suite()
{
    if (suite_)
	return suite_;	/* cached */

    /* convert the .suite_ string from a filenames
     * to a suite name */

    /* find the pathname tail */
    const char *b = strrchr(filename_, '/');
    if (b)
	b++;
    else
	b = filename_;

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
    return suite_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

testrunner_t *testrunner_t::current_ = 0;

testrunner_t::testrunner_t()
 :  verbose_(0),
    scheduled_(0),
    nscheduled_(0)
{
}

testrunner_t::~testrunner_t()
{
    free(scheduled_);
}

void
testrunner_t::schedule(testfn_t *fn)
{
    testfn_t *setup = 0, *teardown = 0;
    int n;

    /* not very efficient, but who cares, it's more
     * important to be correct in the test framework */

    for (testfn_t *ff = testfn_t::head_ ; ff ; ff = ff->next_)
    {
	if (strcmp(ff->suite(), fn->suite()))
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

    scheduled_ = (testfn_t **)_testfw_realloc(scheduled_,
					      sizeof(testfn_t *) * (nscheduled_+n));

    if (setup)
	scheduled_[nscheduled_++] = setup;
    scheduled_[nscheduled_++] = fn;
    if (teardown)
	scheduled_[nscheduled_++] = teardown;
}

void
testrunner_t::set_verbose(int v)
{
    verbose_ = v;
}

void
testrunner_t::list()
{
    for (testfn_t *fn = testfn_t::head_ ; fn ; fn = fn->next_)
    {
	if (fn->role_ == testfn_t::RTEST)
	    printf("%s:%s\n", fn->suite(), fn->name_);
    }
}

int
testrunner_t::schedule_matching(const char *suite, const char *name)
{
    int nmatches = 0;

    for (testfn_t *fn = testfn_t::head_ ; fn ; fn = fn->next_)
    {
	if (fn->role_ != testfn_t::RTEST)
	    continue;
	if (suite && strcmp(suite, fn->suite()))
	    continue;
	if (name && strcmp(name, fn->name_))
	    continue;
	schedule(fn);
	nmatches++;
    }
    return nmatches;
}

int
testrunner_t::schedule(const char *arg)
{
    char *name = 0;
    char *suite = _testfw_strdup(arg);
    char *p = strchr(suite, ':');
    if (p)
    {
	*p++ = '\0';
	name = p;
    }
    int r = schedule_matching(suite, name);
    free(suite);
    return r;
}

void
testrunner_t::run()
{
    if (!nscheduled_)
	schedule_matching(0, 0);
    if (!nscheduled_)
    {
	fprintf(stderr, "testfw: no tests found.  This can't be right.\n");
	exit(1);
    }

    current_ = this;
    for (unsigned int i = 0 ; i < nscheduled_ ; i++)
	scheduled_[i]->run();
    current_ = 0;
}

void
testrunner_t::_check(int pass, const char *file, int line, const char *fmt, ...)
{
    va_list args;
    char buf[2048];

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (!pass || current_->verbose_)
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
/*END*/
