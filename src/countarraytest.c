#include "filename.h"
#include "countarray.H"

const char *argv0;
extern int end;
int verbose = 0;

static inline void
__assfail(int res, const char *expr, const char *file, int line)
{
    if (verbose)
	fprintf(stderr, "%s:%d: checking %s\n", file, line, expr);
    if (!res)
    {
	fprintf(stderr, "%s:%d: ASSERT FAILED: %s\n", file, line, expr);
	abort();
    }
}

#define ASSERT(expr)	__assfail(!!(expr), #expr, __FUNCTION__, __LINE__)

static inline void
subtest(const char *desc)
{
    if (verbose)
	fprintf(stderr, "=== %s ===\n", desc);
}

static void
test_ctor(void)
{
    countarray_t ca;

    ASSERT(ca.length() == 0);
    ASSERT(ca.get(0) == COV_COUNT_INVALID);
    ASSERT(ca.get(1) == COV_COUNT_INVALID);
    ASSERT(ca.get(1023) == COV_COUNT_INVALID);
    ASSERT(ca.get(1000000) == COV_COUNT_INVALID);
}

static void
test_append(void)
{
    countarray_t ca;

    ASSERT(ca.length() == 0);
    ASSERT(ca.get(0) == COV_COUNT_INVALID);
    ASSERT(ca.get(1) == COV_COUNT_INVALID);
    ASSERT(ca.get(1023) == COV_COUNT_INVALID);
    ASSERT(ca.get(1000000) == COV_COUNT_INVALID);

    ca.append(123);
    ASSERT(ca.length() == 1);
    ASSERT(ca.get(0) == 123);
    ASSERT(ca.get(1) == COV_COUNT_INVALID);
    ASSERT(ca.get(1023) == COV_COUNT_INVALID);
    ASSERT(ca.get(1000000) == COV_COUNT_INVALID);

    ca.append(456);
    ASSERT(ca.length() == 2);
    ASSERT(ca.get(0) == 123);
    ASSERT(ca.get(1) == 456);
    ASSERT(ca.get(2) == COV_COUNT_INVALID);
    ASSERT(ca.get(1023) == COV_COUNT_INVALID);
    ASSERT(ca.get(1000000) == COV_COUNT_INVALID);
}

static void
test_append_invalid(void)
{
    countarray_t ca;

    ASSERT(ca.length() == 0);
    ASSERT(ca.get(0) == COV_COUNT_INVALID);
    ASSERT(ca.get(1) == COV_COUNT_INVALID);
    ASSERT(ca.get(1023) == COV_COUNT_INVALID);
    ASSERT(ca.get(1000000) == COV_COUNT_INVALID);

    ca.append(COV_COUNT_INVALID);
    ASSERT(ca.length() == 1);
    ASSERT(ca.get(0) == COV_COUNT_INVALID);
    ASSERT(ca.get(1) == COV_COUNT_INVALID);
    ASSERT(ca.get(1023) == COV_COUNT_INVALID);
    ASSERT(ca.get(1000000) == COV_COUNT_INVALID);

    ca.append(COV_COUNT_INVALID);
    ASSERT(ca.length() == 2);
    ASSERT(ca.get(0) == COV_COUNT_INVALID);
    ASSERT(ca.get(1) == COV_COUNT_INVALID);
    ASSERT(ca.get(2) == COV_COUNT_INVALID);
    ASSERT(ca.get(1023) == COV_COUNT_INVALID);
    ASSERT(ca.get(1000000) == COV_COUNT_INVALID);
}

static void
test_set(void)
{
    countarray_t ca;

    ASSERT(ca.length() == 0);
    ASSERT(ca.get(0) == COV_COUNT_INVALID);
    ASSERT(ca.get(1) == COV_COUNT_INVALID);
    ASSERT(ca.get(1023) == COV_COUNT_INVALID);
    ASSERT(ca.get(1000000) == COV_COUNT_INVALID);

    ca.set(1, 123);
    ASSERT(ca.length() == 2);
    ASSERT(ca.get(0) == COV_COUNT_INVALID);
    ASSERT(ca.get(1) == 123);
    ASSERT(ca.get(2) == COV_COUNT_INVALID);
    ASSERT(ca.get(1023) == COV_COUNT_INVALID);
    ASSERT(ca.get(1000000) == COV_COUNT_INVALID);

    ca.set(0, 456);
    ASSERT(ca.length() == 2);
    ASSERT(ca.get(0) == 456);
    ASSERT(ca.get(1) == 123);
    ASSERT(ca.get(2) == COV_COUNT_INVALID);
    ASSERT(ca.get(1023) == COV_COUNT_INVALID);
    ASSERT(ca.get(1000000) == COV_COUNT_INVALID);
}

static void
test_allocate(void)
{
    countarray_t ca;
    unsigned int i;

    ASSERT(ca.length() == 0);
    ASSERT(ca.get(0) == COV_COUNT_INVALID);
    ASSERT(ca.get(1) == COV_COUNT_INVALID);
    ASSERT(ca.get(1023) == COV_COUNT_INVALID);
    ASSERT(ca.get(1000000) == COV_COUNT_INVALID);

    i = ca.allocate();
    ASSERT(i == 0);
    ASSERT(ca.length() == 1);
    ASSERT(ca.get(0) == COV_COUNT_INVALID);
    ASSERT(ca.get(1) == COV_COUNT_INVALID);
    ASSERT(ca.get(2) == COV_COUNT_INVALID);
    ASSERT(ca.get(1023) == COV_COUNT_INVALID);
    ASSERT(ca.get(1000000) == COV_COUNT_INVALID);

    i = ca.allocate();
    ASSERT(i == 1);
    ASSERT(ca.length() == 2);
    ASSERT(ca.get(0) == COV_COUNT_INVALID);
    ASSERT(ca.get(1) == COV_COUNT_INVALID);
    ASSERT(ca.get(2) == COV_COUNT_INVALID);
    ASSERT(ca.get(1023) == COV_COUNT_INVALID);
    ASSERT(ca.get(1000000) == COV_COUNT_INVALID);
}

int
main(int argc, char **argv)
{
    argv0 = file_basename_c(argv[0]);

    if (argc > 1 && !strcmp(argv[1], "--verbose"))
	verbose++;

    test_ctor();
    test_append();
    test_append_invalid();
    test_set();
    test_allocate();
    return 0;
}

