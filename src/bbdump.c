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
    
    here = ftell(fp);
    assert((here - lastoff) % 4 == 0);
    fseek(fp, lastoff, SEEK_SET);
    for ( ; lastoff < here ; lastoff += 4)
    {
    	covio_read_lu32(fp, &d);
	printf(GNB_U32_XFMT" ", d);
    }
    assert(here == lastoff);
    assert(lastoff == ftell(fp));
}


#define BB_FILENAME 	0x80000001
#define BB_FUNCTION 	0x80000002
#define BB_ENDOFLIST	0x00000000

static void
do_tags(const char *filename, FILE *fp)
{
    gnb_u32_t tag;
    off_t lastoff = 0;
    char *s;
    
    while (covio_read_lu32(fp, &tag))
    {
    	printf("%08lx: ", lastoff);
    	switch (tag)
	{
	case BB_FILENAME:
	    s = covio_read_bbstring(fp, tag);
	    hexdump(fp, lastoff);
	    printf("file \"%s\"\n", s);
	    g_free(s);
	    break;
	
	case BB_FUNCTION:
	    s = covio_read_bbstring(fp, tag);
	    hexdump(fp, lastoff);
	    printf("func \"%s\"\n", s);
	    g_free(s);
	    break;
	
	case BB_ENDOFLIST:
	    hexdump(fp, lastoff);
	    printf("eolist\n");
	    break;

	default:
	    hexdump(fp, lastoff);
	    printf("line "GNB_U32_DFMT"\n", tag);
	    break;
	}
	lastoff = ftell(fp);
    }
}

static void
do_file(const char *filename)
{
    FILE *fp;
    
    if ((fp = fopen(filename, "r")) == 0)
    {
    	perror(filename);
	return;
    }
    
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
