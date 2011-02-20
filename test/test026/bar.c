#include "quux.h"			    /* C(-) */
					    /* C(-) */
int
main(int argc, char **argv)
{
    int x, y;
    int i;

    for (i = 1 ; i < argc ; i++)
    {
    	x = atoi(argv[i]);		    /* C(0) */
	if (x == 37334)			    /* C(0) */
	    unused_function(x);		    /* C(0) */
    	y = function_one(x);		    /* C(0) */
	printf("%d -> %d\n", x, y);	    /* C(0) */
    }
    return 0;				    /* C(0) */
}
