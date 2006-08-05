#include <stdio.h>			    /* C(-) */
#include <stdlib.h>			    /* C(-) */
#include <setjmp.h>			    /* C(-) */
					    /* C(-) */
static jmp_buf jb;			    /* C(-) */
					    /* C(-) */
void
function_one(int x)
{
    printf("    function_one begins\n");    /* C(4) */
    if (!x)				    /* C(4) */
    	longjmp(jb, 1);			    /* C(1) */
    printf("    ...ends\n");		    /* C(3) */
}
					    /* C(-) */
void
function_two(int x)
{
    printf("    function_two begins\n");    /* C(3) */
    if (!x)				    /* C(3) */
    	longjmp(jb, 2);			    /* C(1) */
    printf("    ...ends\n");		    /* C(2) */
}
					    /* C(-) */
void
function_three(int x)
{
    printf("    function_three begins\n");  /* C(2) */
    if (!x)				    /* C(2) */
    	longjmp(jb, 3);			    /* C(1) */
    printf("    ...ends\n");		    /* C(1) */
}
					    /* C(-) */
int
main(int argc, char **argv)
{
    int x;
    
    printf("foo running\n");		    /* C(4) */
    x = atoi(argv[1]);			    /* C(4) */
    if (!setjmp(jb))
    {
	function_one(--x);		    /* C(4) */
	function_two(--x);		    /* C(3) */
	function_three(--x);		    /* C(2) */
	printf("function completed\n");     /* C(1) */
    }
    else
    {
	printf("long jump triggered\n");    /* C(3) */
    }
    x += 4;				    /* C(4) */
    return 0;				    /* C(4) */
}
