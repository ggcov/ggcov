#include <stdio.h>
#include <stdlib.h>
#include "baz.h"

void
function_two(int x)
{
    printf("    function_two\n");
    if (!x)
    	exit(0);
}

void
function_three(int x)
{
    printf("    function_three\n");
    if (!x)
    	exit(0);
}

