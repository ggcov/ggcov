#include <stdio.h>
#include <stdlib.h>

void
function_one(int x)
{
    printf("    function_one\n");
    if (!x)
    	exit(0);
    printf("    ...ends\n");
}

void
function_two(int x)
{
    printf("    function_two\n");
    if (!x)
    	exit(0);
    printf("    ...ends\n");
}

void
function_three(int x)
{
    printf("    function_three\n");
    if (!x)
    	exit(0);
    printf("    ...ends\n");
}

int
main(int argc, char **argv)
{
    int x;
    
    printf("foo running\n");
    x = atoi(argv[1]);
    function_one(--x);
    function_two(--x);
    function_three(--x);
    x += 4;
    
    return x;
}
