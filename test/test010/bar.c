/* this file is #included into foo.c */
					/* C(-) */
void
function_one(int x)
{
    printf("    function_one\n");	/* C(4) */
    if (!x)				/* C(4) */
    	exit(0);			/* C(1) */
}
					/* C(-) */
void
function_two(int x)
{
    printf("    function_two\n");	/* C(3) */
    if (!x)				/* C(3) */
    	exit(0);			/* C(1) */
}
					/* C(-) */
void
function_three(int x)
{
    printf("    function_three\n");	/* C(2) */
    if (!x)				/* C(2) */
    	exit(0);			/* C(1) */
}
					/* C(-) */
					/* C(-) */
