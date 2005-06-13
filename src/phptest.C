#include "common.h"
#include "php_serializer.H"

typedef int (*testfunc_t)(int, char **);

static int
test0(int argc, char **argv)
{
    php_serializer_t ser;

    ser.integer(42);

    printf("test0: %s\n", ser.data().data());

    return 0;
}

static int
test1(int argc, char **argv)
{
    php_serializer_t ser;

    ser.string("foobar");

    printf("test1: %s\n", ser.data().data());

    return 0;
}

static int
test2(int argc, char **argv)
{
    php_serializer_t ser;

    ser.begin_array(2);
    ser.next_key(); ser.integer(37);
    ser.next_key(); ser.string("smurf");
    ser.end_array();

    printf("test2: %s\n", ser.data().data());

    return 0;
}

static int
test3(int argc, char **argv)
{
    php_serializer_t ser;

    ser.begin_array();
    ser.next_key(); ser.integer(25);
    ser.next_key(); ser.string("fnoogle");
    ser.end_array();

    printf("test3: %s\n", ser.data().data());

    return 0;
}

static int
test4(int argc, char **argv)
{
    php_serializer_t ser;

    ser.begin_array();
	ser.next_key(); ser.integer(25);
	ser.next_key(); ser.begin_array();
	    ser.next_key(); ser.integer(12);
	    ser.next_key(); ser.begin_array();
		ser.next_key(); ser.integer(123);
	    ser.end_array();
	ser.end_array();
	ser.next_key(); ser.string("fnarp");
    ser.end_array();

    printf("test4: %s\n", ser.data().data());

    return 0;
}


static testfunc_t tests[] = 
{
    test0,
    test1,
    test2,
    test3,
    test4
};
static int num_tests = (sizeof(tests)/sizeof(tests[0]));

int
main(int argc, char **argv)
{
    int testn;

    if (argc > 1)
    {
	testn = atoi(argv[1]);

	if (testn < 0 || testn >= num_tests)
	{
	    fprintf(stderr, "phptest: unknown test number %d\n", testn);
	    exit(1);
	}

	return tests[testn](argc-2, argv+2);
    }
    else
    {
	for (testn = 0 ; testn < num_tests ; testn++)
	{
	    if (tests[testn](0, NULL))
		exit(1);
	}
    }

    return 0;
}
