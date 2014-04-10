#include <unistd.h>
#include <stdlib.h>
#include "filename.h"
#include "testfw.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define BASEDIR "/tmp"
#define TESTDIR BASEDIR"/ggcov.filename.test"

/* PATH_MAX not defined on Hurd */
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

static char oldcwd[PATH_MAX];

SETUP
{
    struct stat sb;
    int r;

    r = system("rm -rf " TESTDIR);
    if (r)
	return -1;
    r = stat(TESTDIR, &sb);
    if (r == 0 || errno != ENOENT)
	return -1;
    mkdir(TESTDIR, 0777);
    mkdir(TESTDIR"/dir3", 0777);
    mkdir(TESTDIR"/dir3/dir4", 0777);
    r = stat(TESTDIR, &sb);
    if (r != 0)
	return -1;

    if (getcwd(oldcwd, sizeof(oldcwd)) == NULL)
	return -1;
    if (chdir(TESTDIR"/dir3/dir4") < 0)
	return -1;

    return 0;
}

TEARDOWN
{
    int r;

    if (oldcwd[0] && chdir(oldcwd) < 0)
	return -1;

    r = system("rm -rf " TESTDIR);
    if (r)
	return -1;
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define TESTCASE(in, expected) \
{ \
    const char *out = file_make_absolute(in); \
    check_str_equals(expected, out); \
}

TEST(make_absolute)
{
    TESTCASE("/foo/bar", "/foo/bar");
    TESTCASE("/foo", "/foo");
    TESTCASE("/", "/");
    TESTCASE("foo", TESTDIR"/dir3/dir4/foo");
    TESTCASE("foo/bar", TESTDIR"/dir3/dir4/foo/bar");
    TESTCASE(".", TESTDIR"/dir3/dir4");
    TESTCASE("./foo", TESTDIR"/dir3/dir4/foo");
    TESTCASE("./foo/bar", TESTDIR"/dir3/dir4/foo/bar");
    TESTCASE("./foo/./bar", TESTDIR"/dir3/dir4/foo/bar");
    TESTCASE("./././foo/./bar", TESTDIR"/dir3/dir4/foo/bar");
    TESTCASE("..", TESTDIR"/dir3");
    TESTCASE("../foo", TESTDIR"/dir3/foo");
    TESTCASE("../foo/bar", TESTDIR"/dir3/foo/bar");
    TESTCASE("../../foo", TESTDIR"/foo");
    TESTCASE("../../../foo/bar", BASEDIR"/foo/bar");
    TESTCASE("./../.././../foo/bar", BASEDIR"/foo/bar");
}

#undef TESTCASE

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define TESTCASE(in, expected) \
{ \
    char *out = file_normalise(in); \
    check_str_equals(expected, out); \
    g_free(out); \
}

TEST(normalise)
{
    TESTCASE("/foo/bar", "/foo/bar");
    TESTCASE("//foo////bar", "/foo/bar");
    TESTCASE("/", "/");
    TESTCASE("foo", "foo");
    TESTCASE("./foo", "foo");
    TESTCASE("./././foo", "foo");
    TESTCASE("./foo/./", "foo");
    TESTCASE("foo/bar", "foo/bar");
    TESTCASE("foo////bar", "foo/bar");
    TESTCASE("./foo", "foo");
    TESTCASE("./foo/bar", "foo/bar");
    TESTCASE("./foo///bar", "foo/bar");
    TESTCASE(".//foo/bar", "foo/bar");
    TESTCASE("././././foo/bar", "foo/bar");
    TESTCASE(".", ".");
    TESTCASE("../foo", "../foo");
    TESTCASE("../foo/bar", "../foo/bar");
    TESTCASE("../foo//bar", "../foo/bar");
    TESTCASE("foo/..", ".");
    TESTCASE("foo///..", ".");
    TESTCASE("foo/../bar/..", ".");
    TESTCASE("foo//.././bar/.//..", ".");
    TESTCASE("../../../foo/bar", "../../../foo/bar");
    TESTCASE("./../.././../foo/bar", "../../../foo/bar");
}

#undef TESTCASE

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define TESTCASE(p1, p2, expected) \
{ \
    char *out = file_join2(p1, p2); \
    check_str_equals(expected, out); \
    g_free(out); \
}

TEST(join2)
{
    TESTCASE("a", "b", "a/b");
    TESTCASE("a/", "b", "a/b");
    TESTCASE("a", "/b", "a/b");
    TESTCASE("a/", "/b", "a/b");
    TESTCASE("a////", "///b", "a/b");
    TESTCASE("/foo/bar", "baz", "/foo/bar/baz");
    TESTCASE("/mustache", (const char *)0, "/mustache");
}

#undef TESTCASE

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
