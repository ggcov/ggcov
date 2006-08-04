#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char blocktrace[1024];
char *bp = blocktrace;

int
main(int argc, char **argv)
{
    int N;
    int i;

    printf("foo running\n");
    N = atoi(argv[1]);
    for (*bp++ = '1', i = 0 ; *bp++ = '2', i < N ; *bp++ = '3', i++) {
	printf("loop: %d\n", i);
	*bp++ = 'L';
    }

    printf("blocktrace[] = \"%s\"\n", blocktrace);

    if (argc > 1)
    {
	printf("expected blocktrace[] = \"%s\"\n", argv[2]);
	if (strcmp(blocktrace, argv[2]))
	{
	    fprintf(stderr, "%s: blocktrace mismatch: got \"%s\" expected \"%s\"\n",
		    argv[0], blocktrace, argv[2]);
	    return 1;
	}
    }

    return 0;
}
