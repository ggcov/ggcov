#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

static jmp_buf jb;

void
function_one(int x)
{
    printf("    function_one begins\n");
    if (!x)
    	longjmp(jb, 1);
    printf("    function_one ends\n");
}

void
function_two(int x)
{
    printf("    function_two begins\n");
    if (!x)
    	longjmp(jb, 2);
    printf("    function_two ends\n");
}

void
function_three(int x)
{
    printf("    function_three begins\n");
    if (!x)
    	longjmp(jb, 3);
    printf("    function_three ends\n");
}

int
main(int argc, char **argv)
{
    int x;
    
    printf("foo running\n");
    x = atoi(argv[1]);
    if (!setjmp(jb))
    {
	function_one(--x);
	function_two(--x);
	function_three(--x);
    }
    x += 4;
    printf("foo returning naturally\n");
    return x;
}
