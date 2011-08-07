#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "testfw.h"

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

    testfw_init();
    while ((i = getopt(argc, argv, "vl")) >= 0)
    {
	switch (i)
	{
	case 'v':
	    testfw_set_verbose(1);
	    break;
	case 'l':
	    testfw_list();
	    exit(0);
	default:
	    usage();
	}
    }

    for (i = optind ; i < argc ; i++)
    {
	if (!testfw_schedule(argv[i]))
	    fprintf(stderr, "Unknown suite or name: %s\n", argv[i]);
    }

    testfw_run();

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
