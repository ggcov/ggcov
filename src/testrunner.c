#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "testfw.h"

/* some of the code needs this to exist */
const char *argv0;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void usage(void) __attribute__((noreturn));

static void
usage(void)
{
    fprintf(stderr, "Usage: testrunner [-v] [-d] [suite[:test]...]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "-v         be more verbose");
    fprintf(stderr, "-l         list all tests and exit");
    fprintf(stderr, "-d         run tests without forking (useful under gdb)");
    exit(1);
}

int
main(int argc, char **argv)
{
    int i;
    testrunner_t runner;

    while ((i = getopt(argc, argv, "vld")) >= 0)
    {
	switch (i)
	{
	case 'v':
	    runner.set_verbose(1);
	    break;
	case 'd':
	    runner.set_forking(false);
	    break;
	case 'l':
	    runner.list();
	    exit(0);
	default:
	    usage();
	}
    }

    for (i = optind ; i < argc ; i++)
    {
	if (!runner.schedule(argv[i]))
	    fprintf(stderr, "Unknown suite or name: %s\n", argv[i]);
    }

    if (!runner.run())
	return 1;

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
