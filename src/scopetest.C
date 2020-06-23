#include "common.h"
#include "cov_types.H"
#include "cov_scope.H"
#include "cov_file.H"
#include "cov_function.H"
#include "cov_line.H"
#include "testfw.H"
#include "teststarter.h"

#define LINE(f, n, expected_status) \
    { \
	cov_line_t *_l = f->nth_line((n)); \
	check_not_null(_l); \
	by_line.add_line(_l->status()); \
	if ((int)expected_status >= 0) \
	{ \
	    check_num_equals(_l->status(), expected_status); \
	} \
    }

TEST(one_function)
{
    test_starter_t starter;

    starter.add_sourcefile("foo.c").source(
"#ifdef __cplusplus\n"
"extern \"C\" int function_one(int);\n"
"#endif\n"
"\n"
"int\n"
"function_one(int x)\n"
"{\n"
"    x += 4;\n"
"    if (x & 1)\n"
"        x -= 5;\n"
"    x++;\n"
"    return x>>1;\n"
"}\n"
"\n");
    starter.add_run().arg(2);

    check(!starter.start());

    /* Test that individual lines report what we expect them to, with
     * some leeway around function start and end whose behavior varies
     * with gcc version. */
    cov_file_t *f = cov_file_t::find("foo.c");
    check_not_null(f);
    cov_stats_t by_line;
    LINE(f, 1, cov::UNINSTRUMENTED);
    LINE(f, 2, cov::UNINSTRUMENTED);
    LINE(f, 3, cov::UNINSTRUMENTED);
    LINE(f, 4, cov::UNINSTRUMENTED);
    LINE(f, 5, cov::UNINSTRUMENTED);
    LINE(f, 6, -1);
    LINE(f, 7, -1);
    LINE(f, 8, cov::COVERED);
    LINE(f, 9, cov::COVERED);
    LINE(f, 10, cov::UNCOVERED);
    LINE(f, 11, cov::COVERED);
    LINE(f, 12, cov::COVERED);
    LINE(f, 13, -1);
    LINE(f, 14, cov::UNINSTRUMENTED);

    /* Test the Overall scope */
    cov_overall_scope_t oscope;
    const cov_stats_t *overall = oscope.get_stats();
    check_num_equals(overall->lines_total(), by_line.lines_total());
    check_num_equals(overall->lines_full(), by_line.lines_full());
    check_num_equals(overall->lines_partial(), by_line.lines_partial());
    // check_num_equals(overall->blocks_total(), 3);
    // check_num_equals(overall->blocks_executed(), 2);
    check_num_equals(overall->functions_total(), 1);
    check_num_equals(overall->functions_executed(), 1);
    check_num_equals(overall->functions_full(), 0);
    check_num_equals(overall->functions_partial(), 1);
    check_num_equals(overall->status_by_lines(), cov::PARTCOVERED);

    /* Test the File scope. Easy because there's only one file. */
    cov_file_scope_t fscope(f);
    const cov_stats_t *file = fscope.get_stats();
    check(*file == *overall);

    /* Test the Function scope.  Also easy because there's only one
     * function */
    cov_function_t *fn = f->find_function("function_one");
    check_not_null(fn);
    cov_function_scope_t fnscope(fn);
    const cov_stats_t *func = fnscope.get_stats();
    if (testrunner_t::verbose())
    {
	fprintf(stderr, "func = "); func->dump(stderr);
	fprintf(stderr, "overall = "); overall->dump(stderr);
    }
    check(*func == *overall);

    /* Test a Range scope scoped to the whole file */
    cov_range_scope_t rscope_entire(f, 1, 14);
    const cov_stats_t *entire = rscope_entire.get_stats();
    if (testrunner_t::verbose())
    {
	fprintf(stderr, "entire = "); entire->dump(stderr);
	fprintf(stderr, "overall = "); overall->dump(stderr);
    }
    check(*entire == *overall);
}

#undef LINE
