#include "foo.h"

static int
munge(int x)
{
    x *= 2;				    /* C(0) */
    return x;				    /* C(0) */
}

struct foo *
baz_new(void)
{
    static struct foo f = { munge };	    /* C(-) */
    return &f;				    /* C(0) */
}

