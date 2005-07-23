#include <stdio.h>
#include <stdlib.h>

/* single line comment in the middle of nowhere */
int function_one(int x);

/* multiple line
 * comment in
 * the middle
 * of nowhere */

/* first *//*and*/ /*     second     */  /* comments*/

int
function_two(int x)
{
    return function_one(x>>1);
}

/* FOO */
int
unused_function_FOO1(int x)
{
    x += 24;
    x -= 56;
    return x;
}
/*BAR*/

int
function_three(int x)
{
    x += 4;
    x -= 5;
    x++;
    return x>>1;
}

/*crud*//* FOO *//*random crap*/
int
unused_function_FOO2(int x)
{
    x += 24;
    // BAR/*FULOUS*/
    x -= 56;
    return x;
}
/*BAR*/

int
function_one(int x)
{
    if (x & 1)
    	x += function_two(x);
    else
    	x += function_three(x);
    return x;
}

int
unused_function_BAZ(int x)  // BAZ
{			    // BAZ
    x += 24;		    // BAZ
    x -= 56;		    // BAZ
    return x;		    // BAZ
}			    // BAZ

int
main(int argc, char **argv)
{
    int x, y;
    int i;
    
    for (i = 1 ; i < argc ; i++)
    {
    	x = atoi(argv[i]);
    	y = function_one(x);
	// FOO
    	if (x == 42 && x == 31415926)
	{
	    unused_function_FOO1(x);
	    unused_function_FOO2(x);
	}
	// BAR 
	printf("%d -> %d\n", x, y);
    }
    return 0;
}
