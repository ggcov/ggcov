#include <stdio.h>
#include <stdlib.h>

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

void
foo::function_one(int x)
{
    printf("    function_one\n");
    if (!x)
    	exit(0);
}

void
foo::function_two(int x)
{
    printf("    function_two\n");
    if (!x)
    	exit(0);
}

void
foo::function_three(int x)
{
    printf("    function_three\n");
    if (!x)
    	exit(0);
}

void (*foo::set_log_function(void (*func)(unsigned, const char *, ...)))(unsigned, const char *, ...)
{
    void (*oldfunc)(unsigned, const char *, ...) = log_function_;
    log_function_ = func;
    return oldfunc;
}

int
main(int argc, char **argv)
{
    int x;
    foo f;
    
    printf("foo running\n");
    x = atoi(argv[1]);
    f.function_one(--x); f.function_two(--x) ; f.function_three(--x);
    x += 4;
    
    return 0;
}
