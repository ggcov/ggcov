#include <stdio.h>
#include <stdlib.h>
#include "odd.h"
#include "even.h"

int
do_stuff(int x)
{
    if (x & 1)
	x = do_odd_stuff(x);
    else
	x = do_even_stuff(x);
    x += 1;
    return x;
}

int
main(int argc, char **argv)
{
    int i;
    int x;
    
    for (i = 1 ; i < argc ; i++)
    {
	x = atoi(argv[i]);
	x = do_stuff(x);
	if (x & 1)
	    return 1;
    }

    return 0;
}
