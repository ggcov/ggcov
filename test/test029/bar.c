#include "foo.h"

static int
munge(int x)
{					/* C(4) */
    x += 42;				/* C(4) */
    return x;				/* C(4) */
}

struct foo *
bar_new(void)
{
    static struct foo f = { munge };	/* C(-) */
    return &f;				/* C(4) */
}

