#include "argparse.H"
#include "testfw.h"

#define TESTCASE(...) \
    argparse::simple_params_t params; \
    argparse::parser_t parser(params); \
    parser.add_option('s', "seitan").without_arg(); \
    parser.add_option('b', "brooklyn").with_arg(); \
    static const char * const _argv[] = { __VA_ARGS__, NULL }; \
    dmsg("line %d", __LINE__); \
    int r = parser.parse(sizeof(_argv)/sizeof(_argv[0])-1, (char **)_argv)

TEST(simple_no_files)
{
    TESTCASE("/bin/the-program");
    check_num_equals(r, 0);
    check(!params.has("seitan"));
    check(!params.has("brooklyn"));
    check_num_equals(params.num_files(), 0);
}

TEST(simple_one_file)
{
    TESTCASE("/bin/the-program", "mustache");
    check_num_equals(r, 0);
    check(!params.has("seitan"));
    check(!params.has("brooklyn"));
    check_num_equals(params.num_files(), 1);
    check_str_equals(params.nth_file(0), "mustache");
}

TEST(simple_two_files)
{
    TESTCASE("/bin/the-program", "mustache", "dreamcatcher");
    check_num_equals(r, 0);
    check(!params.has("seitan"));
    check(!params.has("brooklyn"));
    check_num_equals(params.num_files(), 2);
    check_str_equals(params.nth_file(0), "mustache");
    check_str_equals(params.nth_file(1), "dreamcatcher");
}

TEST(simple_short_option_with_value)
{
    TESTCASE("/bin/the-program", "-b", "shoreditch");
    check_num_equals(r, 0);
    check(!params.has("seitan"));
    check(params.has("brooklyn"));
    check_str_equals(params.value("brooklyn"), "shoreditch");
    check_num_equals(params.num_files(), 0);
}

TEST(simple_long_option_with_value)
{
    TESTCASE("/bin/the-program", "--brooklyn", "williamsburg");
    check_num_equals(r, 0);
    check(!params.has("seitan"));
    check(params.has("brooklyn"));
    check_str_equals(params.value("brooklyn"), "williamsburg");
    check_num_equals(params.num_files(), 0);
}

TEST(simple_short_option_without_value)
{
    TESTCASE("/bin/the-program", "-s");
    check_num_equals(r, 0);
    check(params.has("seitan"));
    check(!params.has("brooklyn"));
    check_num_equals(params.num_files(), 0);
}

TEST(simple_long_option_without_value)
{
    TESTCASE("/bin/the-program", "--seitan");
    check_num_equals(r, 0);
    check(params.has("seitan"));
    check(!params.has("brooklyn"));
    check_num_equals(params.num_files(), 0);
}

TEST(simple_short_both_options)
{
    TESTCASE("/bin/the-program", "-b", "leggings", "-s");
    check_num_equals(r, 0);
    check(params.has("seitan"));
    check(params.has("brooklyn"));
    check_str_equals(params.value("brooklyn"), "leggings");
    check_num_equals(params.num_files(), 0);
}

TEST(simple_short_both_options_one_file)
{
    TESTCASE("/bin/the-program", "-b", "leggings", "-s", "freegan");
    check_num_equals(r, 0);
    check(params.has("seitan"));
    check(params.has("brooklyn"));
    check_str_equals(params.value("brooklyn"), "leggings");
    check_num_equals(params.num_files(), 1);
    check_str_equals(params.nth_file(0), "freegan");
}

TEST(simple_unexpected_option)
{
    TESTCASE("/bin/the-program", "--bogus-option");
    check_num_equals(r, -1);
    check(!params.has("seitan"));
    check(!params.has("brooklyn"));
    check_num_equals(params.num_files(), 0);
}

TEST(simple_no_value_for_option_with_value)
{
    TESTCASE("/bin/the-program", "-b");
    check_num_equals(r, -1);
    check(!params.has("seitan"));
    check(!params.has("brooklyn"));
    check_num_equals(params.num_files(), 0);
}

TEST(simple_doubledash)
{
    /* the -- option turns all following arguments into files */
    TESTCASE("/bin/the-program", "-b", "kombucha", "--", "biodiesel", "-s", "mustache");
    check_num_equals(r, 0);
    check(!params.has("seitan"));
    check(params.has("brooklyn"));
    check_str_equals(params.value("brooklyn"), "kombucha");
    check_num_equals(params.num_files(), 3);
    check_str_equals(params.nth_file(0), "biodiesel");
    check_str_equals(params.nth_file(1), "-s");   /* -- makes this a file not an option */
    check_str_equals(params.nth_file(2), "mustache");
}

#undef TESTCASE

class argstest_params_t : public argparse::params_t
{
public:
    argstest_params_t();
    ~argstest_params_t();

    ARGPARSE_STRING_PROPERTY(brooklyn);
    ARGPARSE_BOOL_PROPERTY(seitan);

    void setup_parser(argparse::parser_t &parser)
    {
	parser.add_option('s', "seitan")
	      .setter((argparse::noarg_setter_t)&argstest_params_t::set_seitan);
	parser.add_option('b', "brooklyn")
	      .setter((argparse::arg_setter_t)&argstest_params_t::set_brooklyn);
    }
};

argstest_params_t::argstest_params_t()
 :  seitan_(false)
{
}

argstest_params_t::~argstest_params_t()
{
}

#define TESTCASE(...) \
    argstest_params_t params; \
    argparse::parser_t parser(params); \
    static const char * const _argv[] = { __VA_ARGS__, NULL }; \
    dmsg("line %d", __LINE__); \
    int r = parser.parse(sizeof(_argv)/sizeof(_argv[0])-1, (char **)_argv)

TEST(setter_no_files)
{
    TESTCASE("/bin/the-program");
    check_num_equals(r, 0);
    check(!params.get_seitan());
    check_null(params.get_brooklyn());
    check_num_equals(params.num_files(), 0);
}

TEST(setter_one_file)
{
    TESTCASE("/bin/the-program", "mustache");
    check_num_equals(r, 0);
    check(!params.get_seitan());
    check_null(params.get_brooklyn());
    check_num_equals(params.num_files(), 1);
    check_str_equals(params.nth_file(0), "mustache");
}

TEST(setter_two_files)
{
    TESTCASE("/bin/the-program", "mustache", "dreamcatcher");
    check_num_equals(r, 0);
    check(!params.get_seitan());
    check_null(params.get_brooklyn());
    check_num_equals(params.num_files(), 2);
    check_str_equals(params.nth_file(0), "mustache");
    check_str_equals(params.nth_file(1), "dreamcatcher");
}

TEST(setter_short_option_with_value)
{
    TESTCASE("/bin/the-program", "-b", "shoreditch");
    check_num_equals(r, 0);
    check(!params.get_seitan());
    check_str_equals(params.get_brooklyn(), "shoreditch");
    check_num_equals(params.num_files(), 0);
}

TEST(setter_long_option_with_value)
{
    TESTCASE("/bin/the-program", "--brooklyn", "williamsburg");
    check_num_equals(r, 0);
    check(!params.get_seitan());
    check_str_equals(params.get_brooklyn(), "williamsburg");
    check_num_equals(params.num_files(), 0);
}

TEST(setter_short_option_without_value)
{
    TESTCASE("/bin/the-program", "-s");
    check_num_equals(r, 0);
    check(params.get_seitan());
    check_null(params.get_brooklyn());
    check_num_equals(params.num_files(), 0);
}

TEST(setter_long_option_without_value)
{
    TESTCASE("/bin/the-program", "--seitan");
    check_num_equals(r, 0);
    check(params.get_seitan());
    check_null(params.get_brooklyn());
    check_num_equals(params.num_files(), 0);
}

TEST(setter_short_both_options)
{
    TESTCASE("/bin/the-program", "-b", "leggings", "-s");
    check_num_equals(r, 0);
    check(params.get_seitan());
    check_str_equals(params.get_brooklyn(), "leggings");
    check_num_equals(params.num_files(), 0);
}

TEST(setter_short_both_options_one_file)
{
    TESTCASE("/bin/the-program", "-b", "leggings", "-s", "freegan");
    check_num_equals(r, 0);
    check(params.get_seitan());
    check_str_equals(params.get_brooklyn(), "leggings");
    check_num_equals(params.num_files(), 1);
    check_str_equals(params.nth_file(0), "freegan");
}

TEST(setter_unexpected_option)
{
    TESTCASE("/bin/the-program", "--bogus-option");
    check_num_equals(r, -1);
    check(!params.get_seitan());
    check_null(params.get_brooklyn());
    check_num_equals(params.num_files(), 0);
}

TEST(setter_no_value_for_option_with_value)
{
    /* an option which requires an argument but doesn't have one */
    TESTCASE("/bin/the-program", "-b");
    check_num_equals(r, -1);
    check(!params.get_seitan());
    check_null(params.get_brooklyn());
    check_num_equals(params.num_files(), 0);
}

TEST(setter_doubledash)
{
    /* the -- option turns all following arguments into files */
    TESTCASE("/bin/the-program", "-b", "kombucha", "--", "biodiesel", "-s", "mustache");
    check_num_equals(r, 0);
    check(!params.get_seitan());
    check_str_equals(params.get_brooklyn(), "kombucha");
    check_num_equals(params.num_files(), 3);
    check_str_equals(params.nth_file(0), "biodiesel");
    check_str_equals(params.nth_file(1), "-s");   /* -- makes this a file not an option */
    check_str_equals(params.nth_file(2), "mustache");
}

#undef TESTCASE
