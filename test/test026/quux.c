#include <stdio.h>			    /* C(-) */
#include <stdlib.h>			    /* C(-) */
					    /* C(-) */
int function_one(int);			    /* C(-) */
					    /* C(-) */
int
function_two(int x)
{
    return function_one(x>>1);		    /* C(7) */
}
					    /* C(-) */
int
function_three(int x)
{
    x += 4;				    /* C(4) */
    x -= 5;				    /* C(4) */
    x++;				    /* C(4) */
    return x>>1;			    /* C(4) */
}
					    /* C(-) */
int
function_one(int x)
{
    if (x & 1)				    /* C(11) */
    	x += function_two(x);		    /* C(7) */
    else
    	x += function_three(x);		    /* C(4) */
    return x;				    /* C(11) */
}

int
unused_function(int x)
{
    x += 24;				    /* C(0) */
    x -= 56;				    /* C(0) */
    return x;				    /* C(0) */
}
