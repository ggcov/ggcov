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

unsigned long debug = 0;


static struct
{
    const char *name;
    unsigned int mask;
} const debug_tokens[] = {
#define F(feature)              (1UL<<(feature))
#define U(feature)              (F((feature)+1)-1)
#define R(feat1, feat2)         (U(feat2) & ~U(feat1))
{"verbose",     1},

{"files",       F(D_FILES)},
{"cpp",         F(D_CPP)},
{"bb",          F(D_BB)},
{"bbg",         F(D_BBG)},
{"da",          F(D_DA)},
{"io",          F(D_IO)},
{"cgraph",      F(D_CGRAPH)},
{"suppress",    F(D_SUPPRESS)},
{"solve",       F(D_SOLVE)},
{"stabs",       F(D_STABS)},
{"dwarf",       F(D_DWARF)},
{"elf",         F(D_ELF)},
{"dump",        F(D_DUMP)},
{"scene",       F(D_SCENE)},
{"report",      F(D_REPORT)},
{"dcallgraph",  F(D_DCALLGRAPH)},
{"dlego",       F(D_DLEGO)},

{"uicore",      F(D_UICORE)},
{"summarywin",  F(D_SUMMARYWIN)},
{"sourcewin",   F(D_SOURCEWIN)},
{"prefswin",    F(D_PREFSWIN)},
{"callswin",    F(D_CALLSWIN)},
{"funcswin",    F(D_FUNCSWIN)},
{"fileswin",    F(D_FILESWIN)},
{"graphwin",    F(D_GRAPHWIN)},
{"diagwin",     F(D_DIAGWIN)},
{"reportwin",   F(D_REPORTWIN)},

{"web",         F(D_WEB)},

#define SPECIALS_START     (const char *)1
{SPECIALS_START, 0},

{"cov",         R(D_FILES,D_DUMP)},
{"ui",          R(D_UICORE,D_DIAGWIN)},
{"none",        0},
{"all",         ~1U /*"all" doesn't get you "verbose"*/},
{0, 0}
#undef R
#undef U
#undef F
};

void
debug_set(const char *str)
{
    char *buf, *buf2, *x;
    int i;
    int dirn;

    buf = buf2 = g_strdup(str);
    debug = 0;

    while ((x = strtok(buf2, " \t\n\r,")) != 0)
    {
	buf2 = 0;

	if (*x == '+')
	{
	    dirn = 1;
	    x++;
	}
	else if (*x == '-')
	{
	    dirn = -1;
	    x++;
	}
	else
	    dirn = 1;

	for (i = 0 ; debug_tokens[i].name ; i++)
	{
	    if (debug_tokens[i].name == SPECIALS_START)
		continue;
	    if (!strcmp(debug_tokens[i].name, x))
	    {
		if (dirn == 1)
		    debug |= debug_tokens[i].mask;
		else
		    debug &= ~debug_tokens[i].mask;
		break;
	    }
	}
	if (!debug_tokens[i].name)
	    fprintf(stderr, "Unknown debug token \"%s\", ignoring", x);
    }

    g_free(buf);
}

char *
debug_enabled_tokens(void)
{
    int i;
    int len = 0;
    char *str, *p;

    for (i = 0 ; debug_tokens[i].name != SPECIALS_START ; i++)
    {
	if (!(debug & debug_tokens[i].mask))
	    continue;
	len += strlen(debug_tokens[i].name)+1;
    }

    p = str = (char *)gnb_xmalloc(len);

    for (i = 0 ; debug_tokens[i].name != SPECIALS_START ; i++)
    {
	if (!(debug & debug_tokens[i].mask))
	    continue;
	if (p != str)
	    *p++ = ',';
	strcpy(p, debug_tokens[i].name);
	p += strlen(p);
    }

    return str;
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
