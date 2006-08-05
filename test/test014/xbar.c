#include <stdio.h>			    /* C(-) */
#include <stdlib.h>			    /* C(-) */
#include "bar.h"			    /* C(-) */
					    /* C(-) */
void
function_one(int x)
{
    printf("    function_one\n");	    /* C(4) */
    if (!x)				    /* C(4) */
    	exit(0);			    /* C(1) */
}

