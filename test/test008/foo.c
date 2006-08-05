#include <stdio.h>			    /* C(-) */
#include <stdlib.h>			    /* C(-) */
#include <string.h>			    /* C(-) */
					    /* C(-) */
char blocktrace[1024];			    /* C(-) */
char *bp = blocktrace;			    /* C(-) */
					    /* C(-) */
int
main(int argc, char **argv)
{
    int N;
    int i;

    printf("foo running\n");		    /* C(1,0) C(2,1) C(3,3) C(4,10) */
    N = atoi(argv[1]);			    /* C(1,0) C(2,1) C(3,3) C(4,10) */
    for (*bp++ = '1', i = 0 ; *bp++ = '2', i < N ; *bp++ = '3', i++) { /* C(1,0) C(3,1) C(7,3) C(18,10) */
	printf("loop: %d\n", i);	    /* C(0,0) C(1,1) C(4,3) C(14,10) */
	*bp++ = 'L';			    /* C(0,0) C(1,1) C(4,3) C(14,10) */
    }

    printf("blocktrace[] = \"%s\"\n", blocktrace);  /* C(1,0) C(2,1) C(3,3) C(4,10) */

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

    return 0;				    /* C(1,0) C(2,1) C(3,3) C(4,10) */
}
