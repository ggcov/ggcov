#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int
function_one(int x)
{
    assert(x >= 0);			    /* S(CO) */
    assert(x < 1024);			    /* S(CO) */
    if (x & 1)				    /* S(CO) */
	x += 42;			    /* S(CO) */
    else
	x += 13;			    /* S(CO) */
    assert(x & 1);			    /* S(CO) */
    return x;				    /* S(CO) */
}

int
main(int argc, char **argv)
{
    int x, y;
    int i;

    for (i = 1 ; i < argc ; i++)
    {
	x = atoi(argv[i]);		    /* S(CO) */
	y = function_one(x);		    /* S(CO) */
	printf("%d -> %d\n", x, y);	    /* S(CO) */
    }
    return 0;				    /* S(CO) */
}
