#include <stdio.h>		    /* C(-) */
#include <stdlib.h>		    /* C(-) */
#include "odd.h"		    /* C(-) */
#include "even.h"		    /* C(-) */
				    /* C(-) */
int
do_stuff(int x)
{
    if (x & 1)			    /* C(5) */
	x = do_odd_stuff(x);	    /* C(3) */
    else
	x = do_even_stuff(x);	    /* C(2) */
    x += 1;			    /* C(5) */
    return x;			    /* C(5) */
}
				    /* C(-) */
int
main(int argc, char **argv)
{
    int i;
    int x;
    
    for (i = 1 ; i < argc ; i++)
    {
	x = atoi(argv[i]);	    /* C(5) */
	x = do_stuff(x);	    /* C(5) */
	if (x & 1)		    /* C(5) */
	    return 1;		    /* C(0) */
    }

    return 0;			    /* C(5) */
}
