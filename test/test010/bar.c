/* this file is #included into foo.c */

void
function_one(int x)
{
    printf("    function_one\n");
    if (!x)
    	exit(0);
}

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


