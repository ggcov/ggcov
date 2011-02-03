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

static void
test_group(void)
{
    countarray_t ca;
    unsigned int i;
    countarray_t::group_t group1, group2, group3;

    ASSERT(ca.length() == 0);
    ASSERT(ca.get(0) == COV_COUNT_INVALID);
    ASSERT(ca.get(1) == COV_COUNT_INVALID);
    ASSERT(ca.get(1023) == COV_COUNT_INVALID);
    ASSERT(ca.get(1000000) == COV_COUNT_INVALID);

    ca.begin_group();

    i = ca.allocate();
    ASSERT(i == 0);
    ASSERT(ca.length() == 1);

    i = ca.allocate();
    ASSERT(i == 1);
    ASSERT(ca.length() == 2);

    ca.end_group(group1);

    ca.begin_group();

    i = ca.allocate();
    ASSERT(i == 2);
    ASSERT(ca.length() == 3);

    ca.end_group(group2);

    ca.begin_group();

    i = ca.allocate();
    ASSERT(i == 3);
    ASSERT(ca.length() == 4);

    i = ca.allocate();
    ASSERT(i == 4);
    ASSERT(ca.length() == 5);

    i = ca.allocate();
    ASSERT(i == 5);
    ASSERT(ca.length() == 6);

    ca.end_group(group3);

    ASSERT(group1.first_ == 0);
    ASSERT(group1.last_ == 1);
    ASSERT(group2.first_ == 2);
    ASSERT(group2.last_ == 2);
    ASSERT(group3.first_ == 3);
    ASSERT(group3.last_ == 5);

    ca.set(0, 1000);
    ca.set(1, 1001);
    ca.set(2, 1002);
    ca.set(3, 1003);
    ca.set(4, 1004);
    ca.set(5, 1005);

    ca.invalidate(group2);

    ASSERT(ca.length() == 6);
    ASSERT(ca.get(0) == 1000);
    ASSERT(ca.get(1) == 1001);
    ASSERT(ca.get(2) == COV_COUNT_INVALID);
    ASSERT(ca.get(3) == 1003);
    ASSERT(ca.get(4) == 1004);
    ASSERT(ca.get(5) == 1005);

    ca.invalidate(group1);

    ASSERT(ca.length() == 6);
    ASSERT(ca.get(0) == COV_COUNT_INVALID);
    ASSERT(ca.get(1) == COV_COUNT_INVALID);
    ASSERT(ca.get(2) == COV_COUNT_INVALID);
    ASSERT(ca.get(3) == 1003);
    ASSERT(ca.get(4) == 1004);
    ASSERT(ca.get(5) == 1005);

    ca.invalidate(group3);

    ASSERT(ca.length() == 6);
    ASSERT(ca.get(0) == COV_COUNT_INVALID);
    ASSERT(ca.get(1) == COV_COUNT_INVALID);
    ASSERT(ca.get(2) == COV_COUNT_INVALID);
    ASSERT(ca.get(3) == COV_COUNT_INVALID);
    ASSERT(ca.get(4) == COV_COUNT_INVALID);
    ASSERT(ca.get(5) == COV_COUNT_INVALID);
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
    test_group();
    return 0;
}

