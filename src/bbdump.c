/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2002-2004 Greg Banks <gnb@users.sourceforge.net>
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
#include "covio.H"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>

const char *argv0;

static void
hexdump(covio_t *io, off_t lastoff)
{
    off_t here;
    gnb_u32_t d;
    
    here = io->tell();
    assert((here - lastoff) % 4 == 0);
    io->seek(lastoff);
    for ( ; lastoff < here ; lastoff += 4)
    {
    	io->read_u32(d);
	printf(GNB_U32_XFMT" ", d);
    }
    assert(here == lastoff);
    assert(lastoff == io->tell());
}


#define BB_FILENAME 	0x80000001
#define BB_FUNCTION 	0x80000002
#define BB_ENDOFLIST	0x00000000

static void
do_tags(covio_t *io)
{
    gnb_u32_t tag;
    off_t lastoff = 0;
    estring s;
    
    while (io->read_u32(tag))
    {
    	printf("%08lx: ", lastoff);
    	switch (tag)
	{
	case BB_FILENAME:
	    io->read_bbstring(s, tag);
	    hexdump(io, lastoff);
	    printf("file \"%s\"\n", s.data());
	    break;
	
	case BB_FUNCTION:
	    io->read_bbstring(s, tag);
	    hexdump(io, lastoff);
	    printf("func \"%s\"\n", s.data());
	    break;
	
	case BB_ENDOFLIST:
	    hexdump(io, lastoff);
	    printf("eolist\n");
	    break;

	default:
	    hexdump(io, lastoff);
	    printf("line "GNB_U32_DFMT"\n", tag);
	    break;
	}
	lastoff = io->tell();
    }
}

static void
do_file(const char *filename)
{
    covio_t io(filename);
    
    if (!io.open_read())
    {
    	perror(filename);
	return;
    }
    io.set_format(covio_t::FORMAT_OLD);
    
    do_tags(&io);
}


static const char usage_str[] = 
"Usage: bbdump file.bb...\n";

static void
usagef(int ec, const char *fmt, ...)
{
    if (fmt != 0)
    {
    	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputc('\n', stderr);
    }
    fputs(usage_str, stderr);
    fflush(stderr); /* JIC */
    exit(ec);
}

int
main(int argc, char **argv)
{
    int i;
    int nfiles = 0;
    char **files = (char **)malloc(sizeof(char*) * argc);

    argv0 = argv[0];    
    for (i = 1 ; i < argc ; i++)
    {
    	if (argv[i][0] == '-')
	{
	    usagef(1, "unknown option \"%s\"", argv[i]);
	}
	else
	{
	    files[nfiles++] = argv[i];
	}
    }
    if (nfiles == 0)
    	usagef(1, "must specify at least one filename");
    
    for (i = 0 ; i < nfiles ; i++)
    {
    	do_file(files[i]);
    }
    
    return 0;
}
