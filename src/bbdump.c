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
    	io->read_u32(&d);
	printf(GNB_U32_XFMT" ", d);
    }
    assert(here == lastoff);
    assert(lastoff == io->tell());
}


#define BB_FILENAME 	0x80000001
#define BB_FUNCTION 	0x80000002
#define BB_ENDOFLIST	0x00000000

static void
do_tags(covio_old_t *io)
{
    gnb_u32_t tag;
    off_t lastoff = 0;
    char *s;
    
    while (io->read_u32(&tag))
    {
    	printf("%08lx: ", lastoff);
    	switch (tag)
	{
	case BB_FILENAME:
	    s = io->read_bbstring(tag);
	    hexdump(io, lastoff);
	    printf("file \"%s\"\n", s);
	    g_free(s);
	    break;
	
	case BB_FUNCTION:
	    s = io->read_bbstring(tag);
	    hexdump(io, lastoff);
	    printf("func \"%s\"\n", s);
	    g_free(s);
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
    covio_old_t io(filename);
    
    if (!io.open_read())
    {
    	perror(filename);
	return;
    }
    
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
