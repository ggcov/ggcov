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
    fprintf(stderr, "Usage: testrunner [-v] [suite[:test]...]\n");
    exit(1);
}

int
main(int argc, char **argv)
{
    int i;
    testrunner_t runner;

    while ((i = getopt(argc, argv, "vl")) >= 0)
    {
	switch (i)
	{
	case 'v':
	    runner.set_verbose(1);
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

    runner.run();

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
