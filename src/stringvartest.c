#include "string_var.H"
#include <map>
#include "testfw.h"

// extern int end;

static int
is_heap(const void *x)
{
    // This fails spuriously under Valgrind
//     return (x > (void *)&end);
    return 1;
}

TEST(default_ctor)
{
    string_var e;

    check(e.length() == 0);
    check(e.data() == 0);
}

TEST(const_string_ctor)
{
    static const char x[] = "hello";
    string_var e(x);

    check(e.length() == 5);
    check(!strcmp(e.data(), x));
    check(e.data()[5] == '\0');
    check(strlen(e.data()) == 5);
    check(e.data() != x);           // argument must be copied
    check(is_heap(e.data()));
}

TEST(const_string_ctor_with_null)
{
    string_var e((const char *)0);

    check(e.length() == 0);
    check(e.data() == 0);
}

TEST(non_const_string_ctor)
{
    char *x = strdup("abra cadabra");
fprintf(stderr, "XXX x=%p\"%s\"\n", x, x);
    string_var e(x);
fprintf(stderr, "XXX e=%p\"%s\"\n", (void*)&e, e.data());

    check(e.length() == 12);
    check(!strcmp(e.data(), x));
    check(strlen(e.data()) == 12);
    check(e.data() == x);           // argument must be taken over
}

TEST(non_const_string_ctor_with_null)
{
    string_var e((char *)0);

    check(e.length() == 0);
    check(e.data() == 0);
}

TEST(assignment_by_const_string)
{
    static const char x[] = "sign for abs";
    string_var e;

    check(e.length() == 0);
    check(e.data() == 0);

    e = x;

    check(e.length() == 12);
    check(e.data() != 0);
    check(e.data() != x);
    check(is_heap(e.data()));
    check(strlen(e.data()) == 12);
    check(!strcmp(e.data(), x));
}

TEST(re_assignment_by_const_string)
{
    static const char x1[] = "won't let you";
    static const char x2[] = "smother it";
    static const char x3[] = "I will be singing";
    string_var e(x1);

    check(e.length() == 13);
    check(e.data() != 0);
    check(e.data() != x1);
    check(is_heap(e.data()));
    check(strlen(e.data()) == 13);
    check(!strcmp(e.data(), x1));

    e = x2;             // non-expanding re-assignment

    check(e.length() == 10);
    check(e.data() != 0);
    check(e.data() != x2);
    check(is_heap(e.data()));
    check(strlen(e.data()) == 10);
    check(!strcmp(e.data(), x2));

    e = x3;             // non-expanding re-assignment

    check(e.length() == 17);
    check(e.data() != 0);
    check(e.data() != x3);
    check(is_heap(e.data()));
    check(strlen(e.data()) == 17);
    check(!strcmp(e.data(), x3));

    e = (const char *)0;
    check(e.length() == 0);
    check(e.data() == 0);
}

TEST(assignment_by_non_const_string)
{
    char *x = g_strdup("time is running out");
    string_var e;

    check(e.length() == 0);
    check(e.data() == 0);

    e = x;

    check(e.length() == 19);
    check(e.data() != 0);
    check(e.data() == x);
    check(is_heap(e.data()));
    check(strlen(e.data()) == 19);
    check(!strcmp(e.data(), "time is running out"));
}

TEST(re_assignment_by_non_const_string)
{
    char *x1 = g_strdup("I won't stand");
    char *x2 = g_strdup("in your way");
    char *x3 = g_strdup("as you scream as you shout");
    string_var e(x1);

    check(e.length() == 13);
    check(e.data() != 0);
    check(e.data() == x1);
    check(is_heap(e.data()));
    check(strlen(e.data()) == 13);
    check(!strcmp(e.data(), "I won't stand"));

    e = x2;             // non-expanding re-assignment

    check(e.length() == 11);
    check(e.data() != 0);
    check(e.data() == x2);
    check(is_heap(e.data()));
    check(strlen(e.data()) == 11);
    check(!strcmp(e.data(), "in your way"));

    e = x3;             // non-expanding re-assignment

    check(e.length() == 26);
    check(e.data() != 0);
    check(e.data() == x3);
    check(is_heap(e.data()));
    check(strlen(e.data()) == 26);
    check(!strcmp(e.data(), "as you scream as you shout"));

    e = (char *)0;
    check(e.length() == 0);
    check(e.data() == 0);
}

TEST(take)
{
    char *x = g_strdup("our lives have just begun");
    char *p;
    string_var e(x);

    check(e.length() == 25);
    check(e.data() != 0);
    check(is_heap(e.data()));
    check(e.data() == x);
    check(strlen(e.data()) == 25);
    check(!strcmp(e.data(), "our lives have just begun"));

    p = e.take();

    check(e.length() == 0);
    check(e.data() == 0);

    check(p != 0);
    check(is_heap(p));
    check(p == x);
    check(strlen(p) == 25);
    check(!strcmp(p, "our lives have just begun"));
    g_free(p);
}

TEST(compare)
{
    string_var alpha("alpha");
    string_var beta("beta");
    check(alpha < beta);
    check(!(alpha > beta));
    check(!(alpha == beta));

    string_var also_alpha("alpha");
    check(!(alpha < also_alpha));
    check(!(alpha > also_alpha));
    check(alpha == also_alpha);
}

TEST(map)
{
    std::map<string_var, int> m;
    check(m.size() == 0);

    string_var k("hello");
    int v = 42;
    m.insert(std::make_pair(k, v));
    check(m.size() == 1);
    // the key "nope" is not found
    std::map<string_var, int>::iterator i = m.find(string_var("nope"));
    check(i == m.end());

    // the key "hello" is found
    i = m.find(string_var("hello"));
    check(i != m.end());
    check(i->second == 42);
    // the key was copied
    check(!strcmp(i->first.data(), k.data()));
    check((void *)&i->first != (void*)&k);

    m.insert(std::make_pair(string_var("world"), 37));
    check(m.size() == 2);
    // the key "hello" is found
    i = m.find(string_var("hello"));
    check(i != m.end());
    check(i->second == 42);
    // the key "world" is found
    i = m.find(string_var("world"));
    check(i != m.end());
    check(i->second == 37);
}
