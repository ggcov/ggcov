/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>
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
#include <unistd.h>
#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <sys/wait.h>
#include <errno.h>
#include "testfw.H"

testfn_t *testfn_t::head_, **testfn_t::tailp_ = &head_;
static int status_pipe = -1;

#define STATUS_LEN  2048

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
testrunner_t::_dmsg(const char *fn, const char *fmt, ...)
{
    if (verbose())
    {
	fprintf(stderr, "%s %d ", fn, getpid());
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputc('\n', stderr);
    }
}

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

static int
retry_write(int fd, const char *buf, size_t len)
{
    while (len > 0)
    {
	int n = write(fd, buf, len);
	if (n < 0)
	{
	    perror("write");
	    return -1;
	}
	buf += n;
	len -= n;
    }
    return 0;
}

/* Read an fd until it returns EOF
 * or we run out of buffer space.  The
 * buffer is always a NUL-terminated
 * string afterwards. */
static int
read_status(char *buf, size_t len, int fd)
{
    len--;  /* allow for trailing nul */
    buf[0] = '\0';
    int nread = 0;
    for (;;)
    {
	int n = read(fd, buf, len);
	if (n < 0)
	{
	    perror("read");
	    return -1;
	}
	if (n == 0)
	    break;
	nread += n;
	buf += n;
	len -= n;
	buf[0] = '\0';
    }
    return nread;
}

/* Wait for the given child pid to finish and do some
 * diagnosis on the status.  Returns 0 if the process
 * exited normally, > 0 if the process exited abnormally,
 * or < 0 on error.  Prints to stderr on error. */
static int
wait_for_child(pid_t child)
{
    for (;;)
    {
	int status;
	pid_t found = waitpid(child, &status, 0);
	if (found < 0)
	{
	    int e = errno;
	    if (errno != ESRCH)
		perror("waitpid");
	    return -e;
	}
	if (found != child)
	{
	    dmsg("waitpid returned unexpected pid %s, ignoring", found);
	    continue;
	}
	if (WIFEXITED(status))
	{
	    dmsg("Child process exited with code %d", WEXITSTATUS(status));
	    return WEXITSTATUS(status);
	}
	if (WIFSIGNALED(status))
	{
	    dmsg("Child process terminated by signal %d", WTERMSIG(status));
	    return 128+WTERMSIG(status);
	}
	dmsg("Spurious return from waitpid, ignoring");
    }
    return -1;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
testfn_t::run()
{
    if (before_)
	before_->run();
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
    if (after_)
	after_->run();
}

const char *
testfn_t::suite()
{
    if (suite_)
	return suite_;  /* cached */

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
    forking_(true),
    scheduled_(0),
    nscheduled_(0),
    nrun_(0),
    npass_(0)
{
}

testrunner_t::~testrunner_t()
{
    free(scheduled_);
}

void
testrunner_t::schedule(testfn_t *fn)
{
    /* not very efficient, but who cares, it's more
     * important to be correct in the test framework */

    for (testfn_t *ff = testfn_t::head_ ; ff ; ff = ff->next_)
    {
	if (strcmp(ff->suite(), fn->suite()))
	    continue;
	if (ff->role_ == testfn_t::RSETUP)
	    fn->before_ = ff;
	else if (ff->role_ == testfn_t::RTEARDOWN)
	    fn->after_ = ff;
    }

    scheduled_ = (testfn_t **)_testfw_realloc(scheduled_,
					      sizeof(testfn_t *) * (nscheduled_+1));
    scheduled_[nscheduled_++] = fn;
}

void
testrunner_t::set_verbose(int v)
{
    verbose_ = v;
}

void
testrunner_t::set_forking(bool v)
{
    forking_ = v;
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
testrunner_t::record_pass(testfn_t *fn)
{
    fprintf(stderr, "PASS %s:%s\n", fn->suite(), fn->name());
    npass_++;
}

void
testrunner_t::record_fail(testfn_t *fn, const char *details)
{
    fprintf(stderr, "Test %s:%s failed:\n", fn->suite(), fn->name());
    fputs(details, stderr);
}

#ifndef PIPE_READ
#define PIPE_READ 0
#endif
#ifndef PIPE_WRITE
#define PIPE_WRITE 1
#endif

void
testrunner_t::run_test_in_child(testfn_t *fn)
{
    dmsg("Making pipe");
    int pipefd[2];
    if (pipe(pipefd) < 0)
    {
	perror("pipe");
	return;
    }

    dmsg("Forking");
    pid_t pid = fork();
    if (pid < 0)
    {
	perror("fork");
	close(pipefd[PIPE_READ]);
	close(pipefd[PIPE_WRITE]);
	return;
    }
    if (pid == 0)
    {
	/* child process - return and run the test code, dtor will exit. */
	dmsg("In child process");
	close(pipefd[PIPE_READ]);
	/* record the write end of the status pipe for _check() */
	status_pipe = pipefd[PIPE_WRITE];
	/* run the fixtures and test function */
	fn->run();
	/* send a success message */
	char buf[STATUS_LEN];
	snprintf(buf, sizeof(buf), "+PASS %s.%s", fn->suite(), fn->name());
	retry_write(status_pipe, buf, strlen(buf));
	dmsg("Child process exiting normally");
	exit(0);
    }
    else
    {
	/* parent process - read status and wait for the child to finish */
	dmsg("In parent process");
	close(pipefd[PIPE_WRITE]);

	/* we expect at most one status line, from either a
	 * _check() failure or a successful test.  */
	char buf[STATUS_LEN];
	if (read_status(buf, sizeof(buf), pipefd[PIPE_READ]) < 0)
	    return;
	close(pipefd[PIPE_READ]);

	/* wait the for child process to exit */
	int r = wait_for_child(pid);

	/* diagnose test failure */
	if (r < 0 && r != -ESRCH)
	    return;
	if (r == 0 && buf[0] == '+')
            record_pass(fn);
	else if (buf[0] != '\0')
            record_fail(fn, buf+1);
        // we ran the test and have a clear pass/fail
    }
}

void
testrunner_t::run_test_directly(testfn_t *fn)
{
    /* run the fixtures and test function */
    fn->run();
    /* test succeeded if we got here */
    record_pass(fn);
}

void
testrunner_t::run_test(testfn_t *fn)
{
    running_ = fn;
    nrun_++;
    if (forking_)
        run_test_in_child(fn);
    else
        run_test_directly(fn);
    running_ = 0;
}

int
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
    {
	run_test(scheduled_[i]);
    }
    current_ = 0;

    fprintf(stderr, "%u/%u tests passed\n", npass_, nrun_);
    return (npass_ == nrun_);
}

testfn_t *testrunner_t::running()
{
    return current_ ? current_->running_ : 0;
}

void
testrunner_t::_check(int pass, const char *file, int line, const char *fmt, ...)
{
    va_list args;
    char buf[STATUS_LEN];

    /* build a status line.  1st char is a code which
     * is interpreted in run_test() above. */

    snprintf(buf, sizeof(buf)-2, "%c%s:%d: %s: ",
	    (pass ? '+' : '-'), file, line, (pass ? "passed" : "FAILED"));

    va_start(args, fmt);
    vsnprintf(buf+strlen(buf), sizeof(buf)-2-strlen(buf), fmt, args);
    va_end(args);

    strcat(buf, "\n");

    if (current_->verbose_)
	fputs(buf+1, stderr);

    if (!pass)
    {
	fflush(stderr);
	if (status_pipe >= 0)
	    retry_write(status_pipe, buf, strlen(buf));
	abort();
	/*NOTREACHED*/
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
