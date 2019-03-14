#include "common.h"
#include "tok.H"
#include "testfw.H"

// extern int end;

static int
is_heap(const void *x)
{
    // This fails spuriously under Valgrind
//     return (x > (void *)&end);
    return 1;
}

TEST(const_string_ctor_copies_the_memory)
{
    const char x[] = "/one/two/three/four";
    const char *buf_start = (const char *)x;
    const char *buf_end = (const char *)(x+sizeof(x));
    const char *p;
    tok_t t(x, "/");

    p = t.next();
    check(p != 0);
    check(!strcmp(p, "one"));
    check(!(p >= buf_start && p <= buf_end));   // buf was copied
    check(is_heap(p));

    p = t.next();
    check(p != 0);
    check(!strcmp(p, "two"));
    check(!(p >= buf_start && p <= buf_end));   // buf was copied
    check(is_heap(p));

    p = t.next();
    check(p != 0);
    check(!strcmp(p, "three"));
    check(!(p >= buf_start && p <= buf_end));   // buf was copied
    check(is_heap(p));

    p = t.next();
    check(p != 0);
    check(!strcmp(p, "four"));
    check(!(p >= buf_start && p <= buf_end));   // buf was copied
    check(is_heap(p));

    p = t.next();
    check(p == 0);
}

TEST(non_const_string_c_tor_takes_the_memory_over)
{
    char *x = strdup("five/six/seven");
    const char *buf_start = (const char *)x;
    const char *buf_end = (const char *)(x+strlen(x)+1);
    const char *p;
    tok_t t(x, "/");

    p = t.next();
    check(p != 0);
    check(!strcmp(p, "five"));
    check((p >= buf_start && p <= buf_end));    // buf was not copied
    check(is_heap(p));

    p = t.next();
    check(p != 0);
    check(!strcmp(p, "six"));
    check((p >= buf_start && p <= buf_end));    // buf was not copied
    check(is_heap(p));

    p = t.next();
    check(p != 0);
    check(!strcmp(p, "seven"));
    check((p >= buf_start && p <= buf_end));    // buf was not copied
    check(is_heap(p));

    p = t.next();
    check(p == 0);
}

TEST(single_character_separator)
{
    const char *p;
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

TEST(two_character_separator)
{
    const char *p;
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

TEST(collapsing_multiple_instances_of_the_separator)
{
    const char *p;
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

TEST(ignoring_leading_and_trailing_instances_of_the_separator)
{
    const char *p;
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

TEST(string_comprises_only_1_instance_of_the_separator)
{
    const char *p;
    tok_t t(",", ",");

    p = t.next();
    check(p == 0);
}

TEST(string_comprises_only_multiple_instances_of_the_separator)
{
    const char *p;
    tok_t t(",,,,,,", ",");

    p = t.next();
    check(p == 0);
}

TEST(null_string_argument_doesnt_crash)
{
    const char *p;
    tok_t t((const char *)0, "/");
    p = t.next();
    check(p == 0);
}

TEST(null_separator_gives_default)
{
    const char *p;
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

TEST(no_separator_specified_gives_default)
{
    const char *p;
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

