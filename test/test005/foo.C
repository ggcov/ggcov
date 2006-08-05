#include <stdio.h>			    /* C(-) */
#include <stdlib.h>			    /* C(-) */
					    /* C(-) */
struct foo_ex
{
    int code;
    const char *message;

    foo_ex(int c, const char *m)
     :  code(c), message(m)
    {
    }
};
					    /* C(-) */
static void
do_some_detailed_stuff(int x) throw(foo_ex)
{
    printf("Got argument %d\n", x);	    /* C(1,1) C(2,2) C(3,3) C(4,42) C(5,-1) */
    if (x == 42)			    /* C(1,1) C(2,2) C(3,3) C(4,42) C(5,-1) */
	throw foo_ex(x, "argument insufficiently meaningless");	/* C(0,1) C(0,2) C(0,3) C(1,42) C(1,-1) */
    if (x < 0)				    /* C(1,1) C(2,2) C(3,3) C(3,42) C(4,-1) */
	throw foo_ex(x, "argument insufficiently positive");	/* C(0,1) C(0,2) C(0,3) C(0,42) C(1,-1) */
    if (!(x & 1))			    /* C(1,1) C(2,2) C(3,3) C(3,42) C(3,-1) */
	throw foo_ex(x, "argument insufficiently odd");	/* C(0,1) C(1,2) C(1,3) C(1,42) C(1,-1) */
    printf("I like this argument!\n");	    /* C(1,1) C(1,2) C(2,3) C(2,42) C(2,-1) */
}
					    /* C(-) */
void
do_stuff(int x)
{
    try
    {
	do_some_detailed_stuff(x);	    /* C(1,1) C(2,2) C(3,3) C(4,42) C(5,-1) */
    }
    catch (foo_ex ex)
    {
	printf("Caught exception: %d \"%s\"\n", ex.code, ex.message);	/* C(0,1) C(1,2) C(1,3) C(2,42) C(3,-1) */
    }
}
					    /* C(-) */
int
main(int argc, char **argv)
{
    int i;
    
    for (i = 1 ; i < argc ; i++)
    {
	do_stuff(atoi(argv[i]));	    /* C(1,1) C(2,2) C(3,3) C(4,42) C(5,-1) */
    }

    return 0;				    /* C(1,1) C(2,2) C(3,3) C(4,42) C(5,-1) */
}
