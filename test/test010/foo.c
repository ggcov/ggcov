#include <stdio.h>
#include <stdlib.h>

#include "bar.c"

int
main(int argc, char **argv)
{
    int x;
    
    printf("foo running\n");
    x = atoi(argv[1]);
    function_one(--x); function_two(--x) ; function_three(--x);
    x += 4;
    
    return x;
}
