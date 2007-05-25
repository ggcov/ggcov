#include "common.h"
#include "tok.H"

const char *argv0;
extern int end;

static int
is_heap(const void *x)
{
    return (x > (void *)&end);
}

int
__check_failed(const char *expr, const char *file, int line)
{
    fprintf(stderr, "%s:%d: check failed: %s\n", file, line, expr);
    abort();
    return 0;
}

#define check(expr) \
    ((expr) ? 0 :  __check_failed(#expr, __FILE__, __LINE__))

void
test_ctors()
{
    const char *p;
    const char *buf_start, *buf_end;

    // const string c'tor copies the memory
    {
	const char x[] = "/one/two/three/four";
	tok_t t(x, "/");
	buf_start = (const char *)x;
	buf_end = (const char *)(x+sizeof(x));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "one"));
	check(!(p >= buf_start && p <= buf_end));	// buf was copied
	check(is_heap(p));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "two"));
	check(!(p >= buf_start && p <= buf_end));	// buf was copied
	check(is_heap(p));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "three"));
	check(!(p >= buf_start && p <= buf_end));	// buf was copied
	check(is_heap(p));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "four"));
	check(!(p >= buf_start && p <= buf_end));	// buf was copied
	check(is_heap(p));

	p = t.next();
	check(p == 0);
    }

    // non-const string c'tor takes the memory over
    {
	char *x = strdup("five/six/seven");
	tok_t t(x, "/");
	buf_start = (const char *)x;
	buf_end = (const char *)(x+strlen(x)+1);

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "five"));
	check((p >= buf_start && p <= buf_end));	// buf was not copied
	check(is_heap(p));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "six"));
	check((p >= buf_start && p <= buf_end));	// buf was not copied
	check(is_heap(p));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "seven"));
	check((p >= buf_start && p <= buf_end));	// buf was not copied
	check(is_heap(p));

	p = t.next();
	check(p == 0);
    }
}

void
test_separators()
{
    const char *p;

    // single character separator
    {
	tok_t t("one/two three,four", "/");

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "one"));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "two three,four"));

	p = t.next();
	check(p == 0);
    }

    // two character separator
    {
	tok_t t("one/two three,four", " ,");

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "one/two"));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "three"));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "four"));

	p = t.next();
	check(p == 0);
    }

    // collapsing multiple instances of the separator
    {
	tok_t t("one,,,,,two,,three", ",");

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "one"));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "two"));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "three"));

	p = t.next();
	check(p == 0);
    }

    // ignoring leading and trailing instances of the separator
    {
	tok_t t(",,,,one,two,three,,,", ",");

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "one"));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "two"));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "three"));

	p = t.next();
	check(p == 0);
    }

    // string comprises only 1 instance of the separator
    {
	tok_t t(",", ",");

	p = t.next();
	check(p == 0);
    }

    // string comprises only multiple instances of the separator
    {
	tok_t t(",,,,,,", ",");

	p = t.next();
	check(p == 0);
    }
}

void
test_corner_cases()
{
    const char *p;

    // null string argument doesn't crash
    {
	tok_t t((const char *)0, "/");
	p = t.next();
	check(p == 0);
    }

    // null separator gives default
    {
	tok_t t("alpha beta\tgamma\rdelta\nepsilon", 0);

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "alpha"));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "beta"));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "gamma"));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "delta"));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "epsilon"));

	p = t.next();
	check(p == 0);
    }

    // no separator specified gives default
    {
	tok_t t("alpha beta\tgamma\rdelta\nepsilon");

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "alpha"));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "beta"));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "gamma"));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "delta"));

	p = t.next();
	check(p != 0);
	check(!strcmp(p, "epsilon"));

	p = t.next();
	check(p == 0);
    }
}

int
main(int argc, char **argv)
{
    test_ctors();
    test_separators();
    test_corner_cases();
    return 0;
}
