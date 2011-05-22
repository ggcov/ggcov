#include <stdio.h>			    /* C(-) */
#include <stdlib.h>			    /* C(-) */
					    /* C(-) */
int function_one(int x);		    /* C(-) */

int
unused_function(int x)
{
    x += 24;				    /* C(0) */
    x -= 56;				    /* C(0) */
    return x;				    /* C(0) */
}
					    /* C(-) */
int
main(int argc, char **argv)
{
    int x, y;
    int i;

    for (i = 1 ; i < argc ; i++)
    {
	x = atoi(argv[i]);		    /* C(4) */
	if (x == 42)			    /* C(4) */
	    unused_function(x);		    /* C(0) */
	y = function_one(x);		    /* C(4) */
	printf("%d -> %d\n", x, y);	    /* C(4) */
    }
    return 0;				    /* C(3) */
}
