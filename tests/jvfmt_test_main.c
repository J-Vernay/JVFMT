
#include "munit.h"

MunitResult jvfmt_test_strings(MunitParameter const[], void*);
MunitResult jvfmt_test_usage(MunitParameter const[], void*);
MunitResult jvfmt_test_usage_lowlevel_spec(MunitParameter const[], void*);

MunitTest jvfmt_tests[] = {
	{
		"/usage",				/* name */
		jvfmt_test_usage,		/* test */
		NULL,					/* setup */
		NULL,					/* tear_down */
		MUNIT_TEST_OPTION_NONE, /* options */
		NULL					/* parameters */
	},
	{
		"/usage_lowlevel_spec",			/* name */
		jvfmt_test_usage_lowlevel_spec, /* test */
		NULL,							/* setup */
		NULL,							/* tear_down */
		MUNIT_TEST_OPTION_NONE,			/* options */
		NULL							/* parameters */
	},
	{NULL}
};

static const MunitSuite jvfmt_testsuite = {
	"/jvfmt",				/* name */
	jvfmt_tests,			/* tests */
	NULL,					/* suites */
	1,						/* iterations */
	MUNIT_SUITE_OPTION_NONE /* options */
};

int main(int argc, char** argv)
{
	return munit_suite_main(&jvfmt_testsuite, NULL, argc, argv);
}