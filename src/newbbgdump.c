/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2003-2004 Greg Banks <gnb@alphalink.com.au>
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
    off_t n = 0;
    
    here = io->tell();
    assert((here - lastoff) % 4 == 0);
    io->seek(lastoff);
    for ( ; lastoff < here ; lastoff += 4, n += 4)
    {
    	if (!(n & 0xf))
	    printf("%s%08lx: ", (n ? "\n" : ""), lastoff);
    	io->read_u32(d);
	printf(GNB_U32_XFMT" ", d);
    }
    assert(here == lastoff);
    assert(lastoff == io->tell());
}

static void
fskip(covio_t *io, off_t len)
{
    if (!io->skip(len))
    {
    	fprintf(stderr, "short file while skipping data\n");
	exit(1);
    }
}

#define GCOV_TAG_FUNCTION	 ((gnb_u32_t)0x01000000)
#define GCOV_TAG_BLOCKS		 ((gnb_u32_t)0x01410000)
#define GCOV_TAG_ARCS		 ((gnb_u32_t)0x01430000)
#define GCOV_TAG_LINES		 ((gnb_u32_t)0x01450000)
#define GCOV_TAG_COUNTER_BASE 	 ((gnb_u32_t)0x01a10000)
#define GCOV_TAG_OBJECT_SUMMARY  ((gnb_u32_t)0xa1000000)
#define GCOV_TAG_PROGRAM_SUMMARY ((gnb_u32_t)0xa3000000)

static const struct 
{
    const char *name;
    gnb_u32_t value;
}
gcov_tags[] = 
{
{"GCOV_TAG_FUNCTION",		GCOV_TAG_FUNCTION},
{"GCOV_TAG_BLOCKS",		GCOV_TAG_BLOCKS},
{"GCOV_TAG_ARCS",		GCOV_TAG_ARCS},
{"GCOV_TAG_LINES",		GCOV_TAG_LINES},
{"GCOV_TAG_COUNTER_BASE",	GCOV_TAG_COUNTER_BASE},
{"GCOV_TAG_OBJECT_SUMMARY",	GCOV_TAG_OBJECT_SUMMARY},
{"GCOV_TAG_PROGRAM_SUMMARY",	GCOV_TAG_PROGRAM_SUMMARY},
{0, 0}
};

static const char *
gcov_tag_as_string(gnb_u32_t tag)
{
    int i;
    
    for (i = 0 ; gcov_tags[i].name != 0 ; i++)
    {
    	if (gcov_tags[i].value == tag)
	    return gcov_tags[i].name;
    }
    return "unknown";
}


static void
do_tags(covio_t *io)
{
    gnb_u32_t tag, length;
    off_t lastoff = io->tell();
    off_t chunkoff;
    
    while (io->read_u32(tag))
    {
	if (!io->read_u32(length))
	{
    	    fprintf(stderr, "%s: short file while reading tag header\n",
		    io->filename());
	    exit(1);
	}

	hexdump(io, lastoff);
	printf("tag=%s length=%u\n", gcov_tag_as_string(tag), length);
	chunkoff = lastoff = io->tell();

    	switch (tag)
	{
	case GCOV_TAG_FUNCTION:
	    {
	    	gnb_u32_t checksum;
		estring name;
		
		if (!io->read_string(name) ||
		    !io->read_u32(checksum))
		{
    		    fprintf(stderr, "%s: short file while reading function\n",
		    	    io->filename());
		    exit(1);
		}
		hexdump(io, lastoff);
		printf("name=\"%s\" checksum=0x%08x\n",
		    	name.data(), checksum);
		assert((off_t)(lastoff+length) == io->tell());
	    }
	    break;
	case GCOV_TAG_BLOCKS:
	    fskip(io, (off_t)length);
	    hexdump(io, lastoff);
	    printf("nblocks=%u\n", length/4);
	    break;
	case GCOV_TAG_ARCS:
	    {
	    	gnb_u32_t src, dst, flags;
		gnb_u32_t i;

		if (!io->read_u32(src))
		{
    		    fprintf(stderr, "%s: short file while reading src block\n",
		    	    io->filename());
		    exit(1);
		}
		hexdump(io, lastoff);
		printf("source-block=%u\n", src);
		lastoff = io->tell();

		i = 4;
		while (i < length)
		{
		    if (!io->read_u32(dst) ||
		    	!io->read_u32(flags))
		    {
    			fprintf(stderr, "%s: short file while reading arc\n",
		    		io->filename());
			exit(1);
		    }
		    i += 8;
		    hexdump(io, lastoff);
		    printf("dest-block=%u flags=0x%x\n", dst, flags);
		    lastoff = io->tell();
		}
	    }
	    break;
	case GCOV_TAG_LINES:
	    {
	    	gnb_u32_t block, line;

		if (!io->read_u32(block))
		{
    		    fprintf(stderr, "%s: short file while reading block number\n",
		    	    io->filename());
		    exit(1);
		}
		hexdump(io, lastoff);
		printf("block=%u\n", block);
		lastoff = io->tell();

		while (io->read_u32(line))
		{
		    if (line == 0)
		    {
		    	estring srcfilename;
			if (!io->read_string(srcfilename))
			{
    			    fprintf(stderr, "%s: short file while reading filename\n",
		    		    io->filename());
			    exit(1);
			}
			hexdump(io, lastoff);
			lastoff = io->tell();

			if (srcfilename.length() == 0)
			{
			    /* end of LINES block */
			    printf("end-of-lines\n");
			    break;
			}
			printf("srcfilename=\"%s\"\n", srcfilename.data());
		    }
		    else
		    {
			hexdump(io, lastoff);
			lastoff = io->tell();
			printf("line=%u\n", line);
		    }
		}
	    }
	    break;
	case GCOV_TAG_COUNTER_BASE:
	case GCOV_TAG_OBJECT_SUMMARY:
	case GCOV_TAG_PROGRAM_SUMMARY:
	default:
	    fskip(io, (off_t)length);
	    hexdump(io, lastoff);
	    printf("???\n");
	    break;
	}
	assert((off_t)(chunkoff + length) == io->tell());
	lastoff = io->tell();
    }
}

static void
do_file(const char *filename)
{
    covio_gcc33_t io(filename);
    gnb_u32_t magic, version;

    if (!io.open_read())
    {
    	perror(filename);
	return;
    }
    
    if (!io.read_u32(magic) ||
    	!io.read_u32(version))
    {
    	fprintf(stderr, "%s: short file while reading header\n", filename);
	exit(1);
    }
    hexdump(&io, 0UL);
    printf("magic=\"%c%c%c%c\" ",
	(magic>>24)&0xff,
	(magic>>16)&0xff,
	(magic>>8)&0xff,
	(magic)&0xff);
    printf("version=\"%c%c%c%c\"\n",
	(version>>24)&0xff,
	(version>>16)&0xff,
	(version>>8)&0xff,
	(version)&0xff);

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
