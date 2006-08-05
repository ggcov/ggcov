#include <stdio.h>			    /* C(-) */
#include <stdlib.h>			    /* C(-) */
#include "bar.h"			    /* C(-) */
#include "baz.h"			    /* C(-) */
					    /* C(-) */
int
main(int argc, char **argv)
{
    int x;

    printf("foo running\n");		    /* C(4) */
    x = atoi(argv[1]);			    /* C(4) */
    function_one(--x); function_two(--x) ; function_three(--x);	/* C(4) */
    x += 4;				    /* C(1) */

    return 0;				    /* C(1) */
}
