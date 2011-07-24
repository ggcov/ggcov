#include "foo.h"

static int
munge(int x)
{
    x += (x>>2);			    /* C(3) */
    return x;				    /* C(3) */
}

struct foo *
quux_new(void)
{
    static struct foo f = { munge };	    /* C(-) */
    return &f;				    /* C(3) */
}

