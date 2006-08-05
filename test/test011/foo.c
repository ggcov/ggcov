#include <stdio.h>			/* C(-) */
#include <stdlib.h>			/* C(-) */
					/* C(-) */
int function_one(int x);		/* C(-) */
					/* C(-) */
int
function_two(int x)
{
    return function_one(x>>1);		/* C(7) */
}
					/* C(-) */
int
function_three(int x)
{
    x += 4;				/* C(4) */
    x -= 5;				/* C(4) */
    x++;				/* C(4) */
    return x>>1;			/* C(4) */
}
					/* C(-) */
int
function_one(int x)
{
    if (x & 1)				/* C(11) */
    	x += function_two(x);		/* C(7) */
    else
    	x += function_three(x);		/* C(4) */
    return x;				/* C(11) */
}
					/* C(-) */
#ifdef FOO
int
unused_function_FOO(int x)
{
    x += 24;				/* C(0,1) C(-,2) */
    x -= 56;				/* C(0,1) C(-,2) */
    return x;				/* C(0,1) C(-,2) */
}
#endif
					/* C(-) */
# if BAR
int
unused_function_BAR(int x)
{
    x += 24;				/* C(-) */
    x -= 56;				/* C(-) */
    return x;				/* C(-) */
}
#endif/* BAR */
					/* C(-) */
# if defined(BAZ)
int
unused_function_BAZ(int x)
{
    x += 24;				/* C(-) */
    x -= 56;				/* C(-) */
    return x;				/* C(-) */
}
#else	/*!BAZ*/
int
unused_function_notBAZ(int x)
{
    x += 24;				/* C(0) */
    x -= 56;				/* C(0) */
    return x;				/* C(0) */
}
#endif	/*BAZ*/
					/* C(-) */
#           	if  /*comm/*ent1*/\
    defined(/* comm//ent2 */QUUX) &&  \
    !defined(FOONLY)/*comment3*/
int
unused_function_QUUX(int x)
{
    x += 24;				/* C(0) */
    x -= 56;				/* C(0) */
    return x;				/* C(0) */
}
#endif
					/* C(-) */
#ifndef FARNSWORTH
int
unused_function_notFARNSWORTH(int x)
{
    x += 24;				/* C(0) */
    x -= 56;				/* C(0) */
    return x;				/* C(0) */
}
#endif
					/* C(-) */
int
main(int argc, char **argv)
{
    int x, y;
    int i;
    
    for (i = 1 ; i < argc ; i++)
    {
    	x = atoi(argv[i]);		/* C(4) */
    	y = function_one(x);		/* C(4) */
#ifdef FOO
    	if (x == 42 && x == 31415926)	/* C(4,1) C(-,2) */
	    unused_function_FOO(x);	/* C(0,1) C(-,2) */
#endif
	printf("%d -> %d\n", x, y);	/* C(4) */
    }
    return 0;				/* C(3) */
}
