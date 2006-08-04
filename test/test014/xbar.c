#include <stdio.h>
#include <stdlib.h>
#include "bar.h"

void
function_one(int x)
{
    printf("    function_one\n");
    if (!x)
    	exit(0);
}

