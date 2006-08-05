#include <stdio.h>			/* C(-) */
#include <stdlib.h>			/* C(-) */
					/* C(-) */
void
function_one(int x)
{
    printf("    function_one\n");	/* C(4) */
    if (!x)				/* C(4) */
    	exit(0);			/* C(1) */
    printf("    ...ends\n");		/* C(3) */
}
					/* C(-) */
void
function_two(int x)
{
    printf("    function_two\n");	/* C(3) */
    if (!x)				/* C(3) */
    	exit(0);			/* C(1) */
    printf("    ...ends\n");		/* C(2) */
}
					/* C(-) */
void
function_three(int x)
{
    printf("    function_three\n");	/* C(2) */
    if (!x)				/* C(2) */
    	exit(0);			/* C(1) */
    printf("    ...ends\n");		/* C(1) */
}
					/* C(-) */
int
main(int argc, char **argv)
{
    int x;
    
    printf("foo running\n");		/* C(4) */
    x = atoi(argv[1]);			/* C(4) */
    function_one(--x);			/* C(4) */
    function_two(--x);			/* C(3) */
    function_three(--x);		/* C(2) */
    x += 4;				/* C(1) */
    
    return 0;				/* C(1) */
}
