#include <stdio.h>			/* C(-) */
#include <stdlib.h>			/* C(-) */
					/* C(-) */
class foo
{
private:
    void (*log_function_)(unsigned, const char *, ...);

public:
    void function_one(int);
    void function_two(int);
    void function_three(int);
    void (*set_log_function(void (*)(unsigned, const char *, ...)))(unsigned, const char *, ...);
};
					/* C(-) */
void
foo::function_one(int x)
{
    printf("    function_one\n");	/* C(4) */
    if (!x)				/* C(4) */
    	exit(0);			/* C(1) */
}
					/* C(-) */
void
foo::function_two(int x)
{
    printf("    function_two\n");	/* C(3) */
    if (!x)				/* C(3) */
    	exit(0);			/* C(1) */
}
					/* C(-) */
void
foo::function_three(int x)
{
    printf("    function_three\n");	/* C(2) */
    if (!x)				/* C(2) */
    	exit(0);			/* C(1) */
}
					/* C(-) */
void (*foo::set_log_function(void (*func)(unsigned, const char *, ...)))(unsigned, const char *, ...)
{
    void (*oldfunc)(unsigned, const char *, ...) = log_function_;
    log_function_ = func;
    return oldfunc;
}
					/* C(-) */
int
main(int argc, char **argv)
{
    int x;
    foo f;
    
    printf("foo running\n");		/* C(4) */
    x = atoi(argv[1]);			/* C(4) */
    f.function_one(--x); f.function_two(--x) ; f.function_three(--x); /* C(4) */
    x += 4;				/* C(1) */
    
    return 0;				/* C(1) */
}
