#include <stdio.h>
#include <stdlib.h>

int function_one(int x);

int
function_two(int x)
{
    return function_one(x>>1);
}

int
function_three(int x)
{
    x += 4;
    x -= 5;
    x++;
    return x>>1;
}

int
function_one(int x)
{
    if (x & 1)
    	x += function_two(x);
    else
    	x += function_three(x);
    return x;
}

#ifdef FOO
int
unused_function_FOO(int x)
{
    x += 24;
    x -= 56;
    return x;
}
#endif

# if BAR
int
unused_function_BAR(int x)
{
    x += 24;
    x -= 56;
    return x;
}
#endif/* BAR */

# if defined(BAZ)
int
unused_function_BAZ(int x)
{
    x += 24;
    x -= 56;
    return x;
}
#else	/*!BAZ*/
int
unused_function_notBAZ(int x)
{
    x += 24;
    x -= 56;
    return x;
}
#endif	/*BAZ*/

#           	if  /*comm/*ent1*/\
    defined(/* comm//ent2 */QUUX) &&  \
    !defined(FOONLY)/*comment3*/
int
unused_function_QUUX(int x)
{
    x += 24;
    x -= 56;
    return x;
}
#endif

#ifndef FARNSWORTH
int
unused_function_notFARNSWORTH(int x)
{
    x += 24;
    x -= 56;
    return x;
}
#endif

int
main(int argc, char **argv)
{
    int x, y;
    int i;
    
    for (i = 1 ; i < argc ; i++)
    {
    	x = atoi(argv[i]);
    	y = function_one(x);
#ifdef FOO
    	if (x == 42 && x == 31415926)
	    unused_function_FOO(x);
#endif
	printf("%d -> %d\n", x, y);
    }
    return 0;
}
