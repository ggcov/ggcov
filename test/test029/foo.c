#include <stdio.h>			    /* C(-) */
#include <string.h>			    /* C(-) */
#include "foo.h"			    /* C(-) */

static struct foo *
foo_factory(const char *name)
{					    /* C(7) */
    if (!strcmp(name, "bar"))		    /* C(7) */
	return bar_new();		    /* C(4) */
    if (!strcmp(name, "baz"))		    /* C(3) */
	return baz_new();		    /* C(0) */
    if (!strcmp(name, "quux"))		    /* C(3) */
	return quux_new();		    /* C(3) */
    return 0;				    /* C(0) */
}

int
main(int argc, char **argv)
{					    /* C(3) */
    int x = 42;				    /* C(3) */
    int i;				    /* C(-) */
    struct foo *f;			    /* C(-) */
					    /* C(-) */
    for (i = 1 ; i < argc ; i++)	    /* C(10) */
    {					    /* C(-) */
	f = foo_factory(argv[i]);	    /* C(7) */
	if (f)				    /* C(7) */
	{				    /* C(-) */
	    x = f->munge(x);		    /* C(7) */
	    printf("%s -> %d\n", argv[i], x);/* C(7) */
	}				    /* C(-) */
    }					    /* C(-) */
    return 0;				    /* C(3) */
}
