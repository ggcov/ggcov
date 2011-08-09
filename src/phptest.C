#include "common.h"
#include "php_serializer.H"
#include "testfw.h"

TEST(integer)
{
    php_serializer_t ser;
    ser.integer(42);
    check_str_equals("i:42;", ser.data().data());
}

TEST(string)
{
    php_serializer_t ser;
    ser.string("foobar");
    check_str_equals("s:6:\"foobar\";", ser.data().data());
}

TEST(presized_array)
{
    php_serializer_t ser;
    ser.begin_array(2);
    ser.next_key(); ser.integer(37);
    ser.next_key(); ser.string("smurf");
    ser.end_array();
    check_str_equals("a:2:{i:0;i:37;i:1;s:5:\"smurf\";}", ser.data().data());
}

TEST(unpresized_array)
{
    php_serializer_t ser;
    ser.begin_array();
    ser.next_key(); ser.integer(25);
    ser.next_key(); ser.string("fnoogle");
    ser.end_array();
    check_str_equals("a:2:{i:0;i:25;i:1;s:7:\"fnoogle\";}", ser.data().data());
}

TEST(nested_arrays)
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
    check_str_equals("a:3:{i:0;i:25;i:1;a:2:{i:0;i:12;i:1;a:1:{i:0;i:123;}}i:2;s:5:\"fnarp\";}", ser.data().data());
}

TEST(floating)
{
    php_serializer_t ser;
    ser.floating(3.1415926535);
    check_str_equals("d:3.141593;", ser.data().data());
}

TEST(null)
{
    php_serializer_t ser;
    ser.null();
    check_str_equals("N;", ser.data().data());
}

