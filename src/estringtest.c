#include "filename.h"
#include "estring.H"
#include "testfw.H"

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
    estring e;

    check(e.length() == 0);
    check(e.data() == 0);
}

TEST(const_string_ctor)
{
    static const char x[] = "hello";
    estring e(x);

    check(e.length() == 5);
    check(!strcmp(e.data(), x));
    check(e.data()[5] == '\0');
    check(strlen(e.data()) == 5);
    check(e.data() != x);           // argument must be copied
    check(is_heap(e.data()));
}

TEST(const_string_ctor_with_null)
{
    estring e((const char *)0);

    check(e.length() == 0);
    check(e.data() == 0);
}

TEST(non_const_string_ctor)
{
    char *x = strdup("abra cadabra");
    estring e(x);

    check(e.length() == 12);
    check(!strcmp(e.data(), x));
    check(strlen(e.data()) == 12);
    check(e.data() == x);           // argument must be taken over
}

TEST(non_const_string_ctor_with_null)
{
    estring e((char *)0);

    check(e.length() == 0);
    check(e.data() == 0);
}

TEST(const_string_and_length_ctor)
{
    static const char x[] = "world";
    estring e(x, 3);

    check(e.length() == 3);
    check(!strncmp(e.data(), x, 3));
    check(e.data()[3] == '\0');
    check(strlen(e.data()) == 3);
    check(e.data() != x);           // argument must be copied
    check(is_heap(e.data()));
}

TEST(const_string_and_length_ctor_with_null)
{
    estring e((const char *)0, 27);

    check(e.length() == 0);
    check(e.data() == 0);
}

TEST(non_const_string_and_length_ctor)
{
    char *x = strdup("foonly quux");
    estring e(x, 6);

    check(e.length() == 6);
    check(!strncmp(e.data(), x, 6));
    check(e.data()[6] == '\0');
    check(strlen(e.data()) == 6);
    check(e.data() == x);           // argument must be taken over
}

TEST(non_const_string_and_length_ctor_with_null)
{
    estring e((char *)0, 27);

    check(e.length() == 0);
    check(e.data() == 0);
}

TEST(assignment_by_const_string)
{
    static const char x[] = "sign for abs";
    estring e;

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
    estring e(x1);

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
    estring e;

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
    estring e(x1);

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

TEST(const_cast_operator)
{
    char *x = g_strdup("our time is now");
    const char *p;
    estring e(x);

    check(e.length() == 15);
    check(e.data() != 0);
    check(is_heap(e.data()));
    check(e.data() == x);
    check(strlen(e.data()) == 15);
    check(!strcmp(e.data(), "our time is now"));

    p = e;

    check(e.length() == 15);
    check(e.data() != 0);
    check(is_heap(e.data()));
    check(e.data() == x);
    check(strlen(e.data()) == 15);
    check(!strcmp(e.data(), "our time is now"));

    check(p != 0);
    check(is_heap(p));
    check(p == x);
    check(p == e.data());
    check(strlen(p) == 15);
    check(!strcmp(p, "our time is now"));
}

TEST(take)
{
    char *x = g_strdup("our lives have just begun");
    char *p;
    estring e(x);

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

static void
call_append_vprintf(estring &e, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	e.append_vprintf(fmt, args);
	va_end(args);
}

TEST(append_string)
{
    estring e;

    check(e.length() == 0);
    check(e.data() == 0);

    e.append_string(0);
    check(e.length() == 0);
    check(e.data() == 0);

    e.append_string("");
    check(e.length() == 0);
    check(e.data() == 0);

    e.append_string("foo");
    check(e.length() == 3);
    check(e.data() != 0);
    check(strlen(e.data()) == 3);
    check(!strcmp(e.data(), "foo"));
    check(is_heap(e.data()));

    e.append_string(" bar");
    check(e.length() == 7);
    check(e.data() != 0);
    check(strlen(e.data()) == 7);
    check(!strcmp(e.data(), "foo bar"));
    check(is_heap(e.data()));
}

TEST(append_char)
{
    estring e;

    check(e.length() == 0);
    check(e.data() == 0);

    e.append_char('b');
    check(e.length() == 1);
    check(e.data() != 0);
    check(strlen(e.data()) == 1);
    check(!strcmp(e.data(), "b"));
    check(is_heap(e.data()));

    e.append_char('a');
    check(e.length() == 2);
    check(e.data() != 0);
    check(strlen(e.data()) == 2);
    check(!strcmp(e.data(), "ba"));
    check(is_heap(e.data()));

    e.append_char('z');
    check(e.length() == 3);
    check(e.data() != 0);
    check(strlen(e.data()) == 3);
    check(!strcmp(e.data(), "baz"));
    check(is_heap(e.data()));
}

TEST(append_chars)
{
    static const char x[] = "quux foonly";
    estring e;

    check(e.length() == 0);
    check(e.data() == 0);

    e.append_chars(x, 4);
    check(e.length() == 4);
    check(e.data() != 0);
    check(strlen(e.data()) == 4);
    check(!strcmp(e.data(), "quux"));
    check(is_heap(e.data()));

    e.append_chars(x+4, 7);
    check(e.length() == 11);
    check(e.data() != 0);
    check(strlen(e.data()) == 11);
    check(!strcmp(e.data(), "quux foonly"));
    check(is_heap(e.data()));
}

TEST(append_printf)
{
    estring e;

    check(e.length() == 0);
    check(e.data() == 0);

    e.append_printf("%sw", "co");
    check(e.length() == 3);
    check(e.data() != 0);
    check(strlen(e.data()) == 3);
    check(!strcmp(e.data(), "cow"));
    check(is_heap(e.data()));

    e.append_printf(",%x", 0xbeef);
    check(e.length() == 8);
    check(e.data() != 0);
    check(strlen(e.data()) == 8);
    check(!strcmp(e.data(), "cow,beef"));
    check(is_heap(e.data()));
}

TEST(append_vprintf)
{
    estring e;

    check(e.length() == 0);
    check(e.data() == 0);

    call_append_vprintf(e, "%sw", "co");
    check(e.length() == 3);
    check(e.data() != 0);
    check(strlen(e.data()) == 3);
    check(!strcmp(e.data(), "cow"));
    check(is_heap(e.data()));

    call_append_vprintf(e, ",%x", 0xbeef);
    check(e.length() == 8);
    check(e.data() != 0);
    check(strlen(e.data()) == 8);
    check(!strcmp(e.data(), "cow,beef"));
    check(is_heap(e.data()));
}

TEST(replace_string)
{
    static const char x[] = "I look pretty good but my heels are high";
    estring e(x);

    check(e.length() == 40);
    check(!strcmp(e.data(), x));
    check(e.data()[40] == '\0');
    check(strlen(e.data()) == 40);
    check(e.data() != x);           // argument must be copied
    check(is_heap(e.data()));

    // replace some chars from the middle
    e.replace_string(7, 6, "real");

    check(e.length() == 38);
    check(!strcmp(e.data(), "I look real good but my heels are high"));
    check(e.data()[38] == '\0');
    check(strlen(e.data()) == 38);

    // remove some chars from the middle
    e.replace_string(12, 5, (const char *)0);

    check(e.length() == 33);
    check(!strcmp(e.data(), "I look real but my heels are high"));
    check(e.data()[33] == '\0');
    check(strlen(e.data()) == 33);

    // replace some chars at the start
    e.replace_string(0, 6, "I'm");

    check(e.length() == 30);
    check(!strcmp(e.data(), "I'm real but my heels are high"));
    check(e.data()[30] == '\0');
    check(strlen(e.data()) == 30);

    // replace some chars at the end
    e.replace_string(26, 4, "low");

    check(e.length() == 29);
    check(!strcmp(e.data(), "I'm real but my heels are low"));
    check(e.data()[29] == '\0');
    check(strlen(e.data()) == 29);

    // replace and expand some chars at the start
    e.replace_string(0, 3, "Meesa");

    check(e.length() == 31);
    check(!strcmp(e.data(), "Meesa real but my heels are low"));
    check(e.data()[31] == '\0');
    check(strlen(e.data()) == 31);

    // replace and expand some chars at the end
    e.replace_string(28, 3, "fantastic");

    check(e.length() == 37);
    check(!strcmp(e.data(), "Meesa real but my heels are fantastic"));
    check(e.data()[37] == '\0');
    check(strlen(e.data()) == 37);
}

#if 0
    replace_char
    replace_chars
    replace_vprintf
    replace_printf
#endif

TEST(remove)
{
    static const char x[] = "love removal machine";
    estring e(x);
    const char *p;

    check(e.length() == 20);
    check(!strcmp(e.data(), x));
    check(e.data()[20] == '\0');
    check(strlen(e.data()) == 20);
    check(e.data() != x);           // argument must be copied
    check(is_heap(e.data()));
    p = e.data();

    // remove some chars from the middle
    e.remove(7, 3);

    check(e.length() == 17);
    check(e.data() == p);           // not been reallocated
    check(!strcmp(e.data(), "love real machine"));
    check(e.data()[17] == '\0');
    check(strlen(e.data()) == 17);

    // remove a single char
    e.remove(4, 1);

    check(e.length() == 16);
    check(e.data() == p);           // not been reallocated
    check(!strcmp(e.data(), "lovereal machine"));
    check(e.data()[16] == '\0');
    check(strlen(e.data()) == 16);

    // remove no chars at all
    e.remove(4, 0);

    check(e.length() == 16);
    check(e.data() == p);           // not been reallocated
    check(!strcmp(e.data(), "lovereal machine"));
    check(e.data()[16] == '\0');
    check(strlen(e.data()) == 16);

    // remove first 4 chars
    e.remove(0, 4);

    check(e.length() == 12);
    check(e.data() == p);           // not been reallocated
    check(!strcmp(e.data(), "real machine"));
    check(e.data()[12] == '\0');
    check(strlen(e.data()) == 12);

    // remove last 3 chars
    e.remove(9, 3);

    check(e.length() == 9);
    check(e.data() == p);           // not been reallocated
    check(!strcmp(e.data(), "real mach"));
    check(e.data()[9] == '\0');
    check(strlen(e.data()) == 9);

    // remove all remaining chars
    e.remove(0, 9);

    check(e.length() == 0);
    check(e.data() == p);           // not been reallocated
    check(e.data()[0] == '\0');
    check(strlen(e.data()) == 0);
}

TEST(truncate_of_zero_length)
{
    estring e;

    check(e.length() == 0);
    check(e.data() == 0);
    e.truncate();
    check(e.length() == 0);
    check(e.data() == 0);
}

TEST(truncate_of_nonzero_length)
{
    static const char x[] = "shake like jello";
    estring e(x);

    check(e.length() == 16);
    check(e.data() != 0);
    check(strlen(e.data()) == 16);
    e.truncate();
    check(e.length() == 0);
    check(e.data() != 0);
    check(strlen(e.data()) == 0);
}

TEST(truncate_to_of_zero_length_to_zero)
{
    estring e;

    check(e.length() == 0);
    check(e.data() == 0);
    e.truncate_to(0);
    check(e.length() == 0);
    check(e.data() == 0);
}

TEST(truncate_to_of_zero_length_to_non_zero_expanding)
{
    static const char zero[] = {0,0,0};
    estring e;

    check(e.length() == 0);
    check(e.data() == 0);
    e.truncate_to(3);
    check(e.length() == 3);
    check(e.data() != 0);
    check(is_heap(e.data()));
    check(!memcmp(e.data(), zero, 3));
}

TEST(truncate_to_of_non_zero_length_to_zero)
{
    static const char x[] = "hand me the mike";
    estring e(x);

    check(e.length() == 16);
    check(e.data() != 0);
    check(strlen(e.data()) == 16);
    e.truncate_to(0);
    check(e.length() == 0);
    check(e.data() != 0);
    check(strlen(e.data()) == 0);
}

TEST(truncate_to_of_non_zero_length_to_non_zero_truncating)
{
    static const char x[] = "and watch me rip it up";
    estring e(x);

    check(e.length() == 22);
    check(e.data() != 0);
    e.truncate_to(9);
    check(e.length() == 9);
    check(e.data() != 0);
    check(is_heap(e.data()));
    check(strlen(e.data()) == 9);
    check(!strcmp(e.data(), "and watch"));
}

TEST(truncate_to_of_non_zero_length_to_non_zero_expanding)
{
    static const char x[] = "the house is a-rockin'";
    static const char zero[] = {0,0,0,0,0};
    estring e(x);

    check(e.length() == 22);
    check(e.data() != 0);
    e.truncate_to(27);
    check(e.length() == 27);
    check(e.data() != 0);
    check(is_heap(e.data()));
    check(strlen(e.data()) == 22);
    check(!strcmp(e.data(), x));
    check(!memcmp(e.data()+22, zero, 5));
}

TEST(chomp_empty_string)
{
    estring e;

    check(e.length() == 0);
    check(e.data() == 0);
    e.chomp();
    check(e.length() == 0);
    check(e.data() == 0);
}

TEST(chomp_just_a_single_newline)
{
    static const char x[] = "\n";
    estring e(x);

    check(e.length() == 1);
    check(e.data() != 0);
    e.chomp();
    check(e.length() == 0);
    check(e.data() != 0);
    check(strlen(e.data()) == 0);
}

TEST(chomp_single_newline)
{
    static const char x[] = "I beg to differ\n";
    estring e(x);

    check(e.length() == 16);
    check(e.data() != 0);
    e.chomp();
    check(e.length() == 15);
    check(e.data() != 0);
    check(strlen(e.data()) == 15);
    check(!strncmp(e.data(), x, 15));
}

TEST(chomp_multiple_newlines)
{
    static const char x[] = "I have no belief\n\n\n\n";
    estring e(x);

    check(e.length() == 20);
    check(e.data() != 0);
    e.chomp();
    check(e.length() == 16);
    check(e.data() != 0);
    check(strlen(e.data()) == 16);
    check(!strncmp(e.data(), x, 16));
}

TEST(chomp_just_multiple_whitespace)
{
    static const char x[] = "\r \r\t  \t";
    estring e(x);

    check(e.length() == 7);
    check(e.data() != 0);
    e.chomp();
    check(e.length() == 0);
    check(e.data() != 0);
    check(strlen(e.data()) == 0);
}

TEST(chomp_multiple_whitespace)
{
    static const char x[] = "walking contradiction \t \r\n  \t";
    estring e(x);

    check(e.length() == 29);
    check(e.data() != 0);
    e.chomp();
    check(e.length() == 21);
    check(e.data() != 0);
    check(strlen(e.data()) == 21);
    check(!strncmp(e.data(), x, 21));
}

TEST(trim_nuls_empty_string)
{
    estring e;

    check(e.length() == 0);
    check(e.data() == 0);
    e.trim_nuls();
    check(e.length() == 0);
    check(e.data() == 0);
}

TEST(trim_nuls_non_empty_string)
{
    const char x[] = "trapped beneath my piano";
    estring e(x);

    check(e.length() == 24);
    check(!strcmp(e.data(), x));
    check(e.data()[24] == '\0');
    check(strlen(e.data()) == 24);
    check(e.data() != x);           // argument must be copied
    check(is_heap(e.data()));

    e.trim_nuls();

    check(e.length() == 24);
    check(!strcmp(e.data(), x));
    check(e.data()[24] == '\0');
    check(strlen(e.data()) == 24);
    check(e.data() != x);           // argument must be copied
    check(is_heap(e.data()));
}

TEST(trim_nuls_non_empty_string_with_trailing_nuls)
{
    char *x = (char *)calloc(33, 1);
    strcpy(x, "scares the");
    estring e(x, 32);

    check(e.length() == 32);
    check(e.data() == x);           // argument must be taken over
    check(!strcmp(e.data(), "scares the"));
    check(strlen(e.data()) == 10);

    e.trim_nuls();

    check(e.length() == 10);
    check(e.data() == x);           // argument must be taken over
    check(!strcmp(e.data(), "scares the"));
    check(strlen(e.data()) == 10);
}

TEST(find)
{
    estring e("the quick brown fox jumped over the lazy dog");

    check(e.find_char('t') == 0);
    check(e.find_char('q') == 4);
    check(e.find_char('X') == -1);

    check(e.find_last_char('t') == 32);
    check(e.find_last_char('q') == 4);
    check(e.find_last_char('X') == -1);

    check(e.find_string("the") == 0);
    check(e.find_string("quick") == 4);
    check(e.find_string("Xanadu") == -1);

    check(e.find_last_string("the") == 32);
    check(e.find_last_string("quick") == 4);
    check(e.find_last_string("Xanadu") == -1);
}
