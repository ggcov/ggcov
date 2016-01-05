/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2005 Greg Banks <gnb@users.sourceforge.net>
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

#include <sys/types.h>
#include <sys/time.h>
#include "common.h"
#include "logging.H"

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

static void oom(void) __attribute__((noreturn));

/* Do write() but handle short writes */
static int
do_write(int fd, const char *buf, int len)
{
    int remain = len;

    while (remain > 0)
    {
	int n = write(fd, buf, remain);
	if (n < 0)
	    return -1;
	buf += n;
	remain -= n;
    }
    return len;
}

static void
oom(void)
{
    static const char message[] = ": no memory, exiting\n";

    if (do_write(2, argv0, strlen(argv0)) < 0)
	exit(1);
    if (do_write(2, message, sizeof(message)-1) < 0)
	exit(1);
    exit(1);
}

void *
gnb_xmalloc(size_t sz)
{
    void *x;

    if ((x = g_malloc0(sz)) == 0)
	oom();

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
u32cmp(uint32_t ul1, uint32_t ul2)
{
    if (ul1 > ul2)
	return 1;
    if (ul1 < ul2)
	return -1;
    return 0;
}

int
u64cmp(uint64_t ull1, uint64_t ull2)
{
    if (ull1 > ull2)
	return 1;
    if (ull1 < ull2)
	return -1;
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void timing_impl(const char *func, unsigned int line, const char *detail)
{
    static FILE *fp;
    static struct timeval prev;
    struct timeval now;
    static const char filename[] = "timing.dat";

    gettimeofday(&now, NULL);
    if (prev.tv_sec)
    {
	struct timeval d;
	timersub(&now, &prev, &d);
	if (!fp)
	{
	    fp = fopen(filename, "w");
	    if (!fp)
	    {
		perror(filename);
		exit(1);
	    }
	}
	fprintf(fp, "%s:%u %lu.%06lu %s\n",
		func, line,
		(unsigned long)d.tv_sec, (unsigned long)d.tv_usec,
		(detail ? detail : ""));
	fflush(fp);
    }
    prev = now;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
