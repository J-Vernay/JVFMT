
#include "munit.h"

#include "../jvfmt.h"

MunitResult jvfmt_test_strings(MunitParameter const params[], void* fixture)
{
	(void)params;
	(void)fixture;
#if 0
	enum {
		SIZE_RANDOM_STRING = 8,
		SIZE_RANDOM_FORMAT = 8,
		SIZE_FMT_BUFFER = 256,
	};
	char randomString[SIZE_RANDOM_STRING];
	char randomFormat[SIZE_RANDOM_FORMAT];

	char fmtBuffer[SIZE_FMT_BUFFER];
	JVFMT f = {fmtBuffer, SIZE_FMT_BUFFER};

	munit_rand_memory(SIZE_RANDOM_STRING, (uint8_t*)randomString);
	for (int i = 0; i < SIZE_RANDOM_STRING; ++i)
		if (randomString[i] == '\0')
			randomString[i] = ' '; // Ensure no null terminators in middle.

	munit_rand_memory(SIZE_RANDOM_FORMAT, (uint8_t*)randomFormat);
	for (int i = 0; i < SIZE_RANDOM_FORMAT; ++i)
		if (randomFormat[i] == '\0' || randomFormat[i] == '{' || randomFormat[i] == '}')
			randomFormat[i] = ' '; // Ensure no null terminators in middle nor {}.
	randomFormat[SIZE_RANDOM_FORMAT - 1] = '\0';

	fmt0(&f, randomFormat);
	munit_assert_size(f.resultLength, ==, SIZE_RANDOM_FORMAT - 1);
	munit_assert_string_equal(f.pResult, randomFormat);

	for (int lenString = SIZE_RANDOM_STRING - 1; lenString >= 0; --lenString) {
		randomString[lenString] = '0';

		int posArg = munit_rand_int_range(0, SIZE_RANDOM_FORMAT - 3);
		char c0 = randomFormat[posArg];
		char c1 = randomFormat[posArg + 1];
		randomFormat[posArg] = '{';
		randomFormat[posArg + 1] = '}';

		fmt1(&f, randomFormat, randomString);
		munit_assert_size(f.resultLength, ==, SIZE_RANDOM_FORMAT - 3 + lenString);

		randomFormat[posArg] = c0;
		randomFormat[posArg + 1] = c1;
	}
#endif
	return MUNIT_OK;
}