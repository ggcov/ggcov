#include <stdio.h>
#include <stdlib.h>

struct foo_ex
{
    int code;
    const char *message;

    foo_ex(int c, const char *m)
     :  code(c), message(m)
    {
    }
};

static void
do_some_detailed_stuff(int x) throw(foo_ex)
{
    printf("Got argument %d\n", x);
    if (x == 42)
	throw foo_ex(x, "argument insufficiently meaningless");
    if (x < 0)
	throw foo_ex(x, "argument insufficiently positive");
    if (!(x & 1))
	throw foo_ex(x, "argument insufficiently odd");
    printf("I like this argument!\n");
}

void
do_stuff(int x)
{
    try
    {
	do_some_detailed_stuff(x);
    }
    catch (foo_ex ex)
    {
	printf("Caught exception: %d \"%s\"\n", ex.code, ex.message);
    }
}

int
main(int argc, char **argv)
{
    int i;
    
    for (i = 1 ; i < argc ; i++)
    {
	do_stuff(atoi(argv[i]));
    }

    return 0;
}
