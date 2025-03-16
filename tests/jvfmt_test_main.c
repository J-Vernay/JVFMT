
#include "munit.h"

MunitResult jvfmt_test_strings(MunitParameter const[], void*);
MunitResult jvfmt_test_usage(MunitParameter const[], void*);
MunitResult jvfmt_test_usage_lowlevel_spec(MunitParameter const[], void*);
MunitResult jvfmt_test_usage_lowlevel_putoverwrite(MunitParameter const[], void*);
MunitResult jvfmt_test_usage_lowlevel_putstring(MunitParameter const[], void*);
MunitResult jvfmt_test_usage_lowlevel_putptr(MunitParameter const params[], void* fixture);

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
	{
		"/usage_lowlevel_putoverwrite",			/* name */
		jvfmt_test_usage_lowlevel_putoverwrite, /* test */
		NULL,									/* setup */
		NULL,									/* tear_down */
		MUNIT_TEST_OPTION_NONE,					/* options */
		NULL									/* parameters */
	},
	{
		"/usage_lowlevel_putstring",		 /* name */
		jvfmt_test_usage_lowlevel_putstring, /* test */
		NULL,								 /* setup */
		NULL,								 /* tear_down */
		MUNIT_TEST_OPTION_NONE,				 /* options */
		NULL								 /* parameters */
	},
	{
		"/usage_lowlevel_putptr",		  /* name */
		jvfmt_test_usage_lowlevel_putptr, /* test */
		NULL,							  /* setup */
		NULL,							  /* tear_down */
		MUNIT_TEST_OPTION_NONE,			  /* options */
		NULL							  /* parameters */
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