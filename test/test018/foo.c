#include <stdio.h>
#include <stdlib.h>

int
nontrivial_tail_recursion2(int x)
{
    if (x <= 2)
	return 1;
    return nontrivial_tail_recursion2(x>>1);
}

int
trivial_tail_recursion(int x)
{
    return nontrivial_tail_recursion2(x>>1);
}

int
trivial_arithmetic1(int x)
{
    return x+1;
}

extern int pure_trivial_arithmetic1(int x) __attribute__((__pure__));
int
pure_trivial_arithmetic1(int x)
{
    return x+1;
}

extern int const_trivial_arithmetic1(int x) __attribute__((__const__));
int
const_trivial_arithmetic1(int x)
{
    return x+1;
}

int
trivial_arithmetic2(int x)
{
    return x+1;
}

int
trivial_arithmetic3(int x)
{
    return x+1;
}

int
trivial_arithmetic4(int x)
{
    return x+1;
}

int
trivial_arithmetic5(int x)
{
    return x+1;
}

int
trivial_arithmetic6(int x)
{
    return x+1;
}

int
trivial_arithmetic7(int x)
{
    return x+1;
}

int
many_simple_calls(int x)
{
    int y;

    y = trivial_arithmetic1(x);
    y += trivial_arithmetic2(x);
    y -= trivial_arithmetic3(x);
    y *= trivial_arithmetic4(x);
    y += trivial_arithmetic5(x);
    y += trivial_arithmetic6(x);
    y -= trivial_arithmetic7(x);
    return y;
}

int
many_calls_much_logic(int x)
{
    int y = 0;

    if (x > 1)
	y = trivial_arithmetic1(x);
    if (y)
	y += trivial_arithmetic2(x);
    if (x || y)
	y -= trivial_arithmetic3(x);
    else
	y *= trivial_arithmetic4(x);
    if (x + y > 3)
	y += trivial_arithmetic5(x);
    y += trivial_arithmetic6(x);
    while (y > 32768)
	y -= trivial_arithmetic7(x);
    return y;
}

int
many_calls_some_logic(int x)
{
    int y;

    y = trivial_arithmetic1(x);
    if (x > 3)
    {
	y += trivial_arithmetic2(x);
	y -= trivial_arithmetic3(x);
	y *= trivial_arithmetic4(x);
    }
    else
    {
	y += trivial_arithmetic5(x);
	y += trivial_arithmetic6(x);
	y -= trivial_arithmetic7(x);
    }
    return y;
}

int
one_call_some_arithmetic(int x)
{
    int y;

    y = 42;
    y += x;
    y *= x;
    y /= 10;
    y += trivial_arithmetic1(x);
    y *= 10;
    y -= x;
    y /= (x+1);
    return y;
}

int
one_pointer_call_some_arithmetic(int x)
{
    int y;
    int (*func)(int) = trivial_arithmetic1;

    y = 42;
    y += x;
    y *= x;
    y /= 10;
    y += func(x);
    y *= 10;
    y -= x;
    y /= (x+1);
    return y;
}

int
one_call_some_arithmetic_some_pure(int x)
{
    int y;

    y = 42;
    y += pure_trivial_arithmetic1(x);
    y *= pure_trivial_arithmetic1(x);
    y /= 10;
    y += trivial_arithmetic1(x);
    y *= 10;
    y -= pure_trivial_arithmetic1(x);
    y /= (pure_trivial_arithmetic1(x)+1);
    return y;
}

int
one_call_some_arithmetic_some_const(int x)
{
    int y;

    y = 42;
    y += const_trivial_arithmetic1(x);
    y *= const_trivial_arithmetic1(x);
    y /= 10;
    y += trivial_arithmetic1(x);
    y *= 10;
    y -= const_trivial_arithmetic1(x);
    y /= (const_trivial_arithmetic1(x)+1);
    return y;
}

int
many_calls_per_line_some_arithmetic(int x)
{
    int y;

    y = 42;
    y += x;
    y *= x;
    y /= 10;
    y += trivial_arithmetic1(x); y *= trivial_arithmetic2(x); y -= trivial_arithmetic3(x);
    y *= 10;
    y -= x;
    y /= (x+1);
    return y;
}

int
main(int argc, char **argv)
{
    int x, y;
    int i;
    
    for (i = 1 ; i < argc ; i++)
    {
    	x = atoi(argv[i]);
    	y = trivial_tail_recursion(x);
    	y += many_simple_calls(x);
    	y += many_calls_much_logic(x);
    	y += many_calls_some_logic(x);
    	y += one_call_some_arithmetic(x);
	y += one_call_some_arithmetic_some_pure(x);
	y += one_call_some_arithmetic_some_const(x);
    	y += many_calls_per_line_some_arithmetic(x);
    	y += one_pointer_call_some_arithmetic(x);
	printf("%d -> %d\n", x, y);
    }
    return 0;
}
