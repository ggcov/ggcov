#include "covio.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>

const char *argv0;

static void
hexdump(FILE *fp, off_t lastoff)
{
    off_t here;
    gnb_u32_t d;
    off_t n = 0;
    
    here = ftell(fp);
    assert((here - lastoff) % 4 == 0);
    fseek(fp, lastoff, SEEK_SET);
    for ( ; lastoff < here ; lastoff += 4, n += 4)
    {
    	if (!(n & 0xf))
	    printf("%s%08lx: ", (n ? "\n" : ""), lastoff);
    	covio_read_bu32(fp, &d);
	printf(GNB_U32_XFMT" ", d);
    }
    assert(here == lastoff);
    assert(lastoff == ftell(fp));
}

static void
fskip(FILE *fp, off_t len)
{
    for ( ; len > 0 ; --len)
    {
    	if (fgetc(fp) == EOF)
	{
    	    fprintf(stderr, "short file while skipping data\n");
	    exit(1);
	}
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
do_tags(const char *filename, FILE *fp)
{
    gnb_u32_t tag, length;
    off_t lastoff = ftell(fp);
    off_t chunkoff;
    
    while (covio_read_bu32(fp, &tag))
    {
	if (!covio_read_bu32(fp, &length))
	{
    	    fprintf(stderr, "%s: short file while reading tag header\n",
		    filename);
	    exit(1);
	}

	hexdump(fp, lastoff);
	printf("tag=%s length=%u\n", gcov_tag_as_string(tag), length);
	chunkoff = lastoff = ftell(fp);

    	switch (tag)
	{
	case GCOV_TAG_FUNCTION:
	    {
	    	gnb_u32_t checksum;
		char *name;
		
		if ((name = covio_read_string(fp)) == 0 ||
		    !covio_read_bu32(fp, &checksum))
		{
    		    fprintf(stderr, "%s: short file while reading function\n",
		    	    filename);
		    exit(1);
		}
		hexdump(fp, lastoff);
		printf("name=\"%s\" checksum=0x%08x\n",
		    	name, checksum);
		assert((off_t)(lastoff+length) == ftell(fp));
		g_free(name);
	    }
	    break;
	case GCOV_TAG_BLOCKS:
	    fskip(fp, (off_t)length);
	    hexdump(fp, lastoff);
	    printf("nblocks=%u\n", length/4);
	    break;
	case GCOV_TAG_ARCS:
	    {
	    	gnb_u32_t src, dst, flags;
		gnb_u32_t i;

		if (!covio_read_bu32(fp, &src))
		{
    		    fprintf(stderr, "%s: short file while reading src block\n",
		    	    filename);
		    exit(1);
		}
		hexdump(fp, lastoff);
		printf("source-block=%u\n", src);
		lastoff = ftell(fp);

		i = 4;
		while (i < length)
		{
		    if (!covio_read_bu32(fp, &dst) ||
		    	!covio_read_bu32(fp, &flags))
		    {
    			fprintf(stderr, "%s: short file while reading arc\n",
		    		filename);
			exit(1);
		    }
		    i += 8;
		    hexdump(fp, lastoff);
		    printf("dest-block=%u flags=0x%x\n", dst, flags);
		    lastoff = ftell(fp);
		}
	    }
	    break;
	case GCOV_TAG_LINES:
	    {
	    	gnb_u32_t block, line;

		if (!covio_read_bu32(fp, &block))
		{
    		    fprintf(stderr, "%s: short file while reading block number\n",
		    	    filename);
		    exit(1);
		}
		hexdump(fp, lastoff);
		printf("block=%u\n", block);
		lastoff = ftell(fp);

		while (covio_read_bu32(fp, &line))
		{
		    if (line == 0)
		    {
		    	char *filename = covio_read_string(fp);
			hexdump(fp, lastoff);
			lastoff = ftell(fp);

			if (filename == 0)
			{
    			    fprintf(stderr, "%s: short file while reading filename\n",
		    		    filename);
			    exit(1);
			}
			if (*filename == '\0')
			{
			    /* end of LINES block */
			    printf("end-of-lines\n");
			    g_free(filename);
			    break;
			}
			printf("filename=\"%s\"\n", filename);
			g_free(filename);
		    }
		    else
		    {
			hexdump(fp, lastoff);
			lastoff = ftell(fp);
			printf("line=%u\n", line);
		    }
		}
	    }
	    break;
	case GCOV_TAG_COUNTER_BASE:
	case GCOV_TAG_OBJECT_SUMMARY:
	case GCOV_TAG_PROGRAM_SUMMARY:
	default:
	    fskip(fp, (off_t)length);
	    hexdump(fp, lastoff);
	    printf("???\n");
	    break;
	}
	assert((off_t)(chunkoff + length) == ftell(fp));
	lastoff = ftell(fp);
    }
}

static void
do_file(const char *filename)
{
    FILE *fp;
    gnb_u32_t magic, version;
    
    if ((fp = fopen(filename, "r")) == 0)
    {
    	perror(filename);
	return;
    }
    
    if (!covio_read_bu32(fp, &magic) ||
    	!covio_read_bu32(fp, &version))
    {
    	fprintf(stderr, "%s: short file while reading header\n", filename);
	exit(1);
    }
    hexdump(fp, 0UL);
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

    do_tags(filename, fp);
    
    fclose(fp);
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
