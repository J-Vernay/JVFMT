
#include "munit.h"

#include "../jvfmt.h"

#define ASSERT_MEM_EQUAL munit_assert_memory_equal
#define ASSERT_STR_EQUAL munit_assert_string_equal
#define ASSERT munit_assert_int64

MunitResult jvfmt_test_usage_lowlevel_spec(MunitParameter const params[], void* fixture)
{
	(void)params;
	(void)fixture;

	/// === BEGIN USAGE_LOWLEVEL_SPEC ===
	///	Whether you want to call directly the low-level API or implement a custom formatter,
	/// you need to provide a format specification, represented by the `JVFMT_SPEC` structure.

	/// === INCLUDE ../jvfmt.h HEADER_LOWLEVEL_SPEC ===

	/// The structure can be obtained by parsing the specification string with `jvfmt_ParseSpec()`.
	///
	/// ```abnf
	/// spec      = [[fill] align] [flags] [width] ["." precision] [quote] [type]
	/// fill      = <any character>
	/// align     = "<" / ">" / "^" / "="
	/// flags     = 0*7<any character except "1" to "9">
	/// width     = <unsigned integer>
	/// precision = <unsigned integer>
	/// quote     = "?"
	/// type      = <any character>
	/// ```
	///
	/// * `width` defines the minimum total field width, including any prefixes, separators,
	///   etc. Note that trailing zeroes are considered part of the `flags`.
	/// * `fill` indicates which character to use for padding, by default a whitespace `' '`.
	/// * `align` indicates how to position the field content within the available space:
	///   * `'<'` for left-alignment (default for strings).
	///   * `'>'` for right-alignment (default for numbers).
	///   * `'^'` for centered-alignment.
	///   * `'='` for right-alignment, except than the initial sign `+` or `-` (if present)
	///     is left-aligned.
	/// * `flags` is a sequence of characters whose presence change the formatting in some way,
	///   dependent on the formatting type.
	/// * `quote` indicates that the result must be quoted with `"` and its
	///   content properly escaped with backslash sequences, i.e. `\n`, `\"`, etc.
	/// * `type` determines how the data should be presented (e.g. binary format...).

	JVFMT_SPEC spec;
	ASSERT(jvfmt_ParseSpec("42.110", &spec), ==, true);
	ASSERT(spec.width, ==, 42);
	ASSERT(spec.precision, ==, 110);
	ASSERT(spec.fill, ==, 0);
	ASSERT(spec.align, ==, 0);
	ASSERT(spec.quote, ==, false);
	ASSERT(spec.type, ==, 0);
	ASSERT_STR_EQUAL(spec.flags, "");

	ASSERT(jvfmt_ParseSpec("x^#_08X", &spec), ==, true);
	ASSERT(spec.width, ==, 8);
	ASSERT(spec.precision, ==, 0);
	ASSERT(spec.fill, ==, 'x');
	ASSERT(spec.align, ==, '^');
	ASSERT(spec.quote, ==, false);
	ASSERT(spec.type, ==, 'X');
	ASSERT_STR_EQUAL(spec.flags, "#_0");

	/// === END ===

	return MUNIT_OK;
}

MunitResult jvfmt_test_usage_lowlevel_putoverwrite(MunitParameter const params[], void* fixture)
{
	(void)params;
	(void)fixture;

	/// === BEGIN USAGE_LOWLEVEL_PUTOVERWRITE ===
	/// `jvfmt_PutOverwrite()` is the lowest-level API exposed by JVFMT.
	/// You are expected to know the output size in advance;
	/// the function reserves an area in the ring buffer for your output,
	/// and also fills the left and right padding for alignment purpose,
	/// according to the spec's `width`, `align` and `fill`.
	///
	/// > [!IMPORTANT]
	/// > The returned area can be smaller than the requested size,
	///   when the `maxLength` limit is reached. Make sure to take
	///   into account the returned size and output only those chars!
	///
	/// After all `jvfmt_PutOverwrite()` operations, use `jvfmt_PutFinalize()`
	/// to add a null-terminator, and get back a pointer to the total string.

	/// === INCLUDE ../jvfmt.h HEADER_LOWLEVEL_PUTOVERWRITE ===

	JVFMT_SPEC spec;
	size_t size;
	char* p;

	char fmtBuffer[64];
	JVFMT f = {0};
	f.pBuffer = fmtBuffer;
	f.bufferSize = sizeof(fmtBuffer);
	f.maxLength = 31;

	spec = (JVFMT_SPEC){0};
	size = 6;
	p = jvfmt_PutOverwrite(&f, spec, &size);
	ASSERT(size, ==, 6);
	memcpy(p, "Hello,", size);

	spec = (JVFMT_SPEC){.width = 12, .fill = '_', .align = '^'};
	size = 5;
	p = jvfmt_PutOverwrite(&f, spec, &size);
	ASSERT(size, ==, 5);
	memcpy(p, "World", size);

	spec = (JVFMT_SPEC){.width = 6, .fill = ' ', .align = '>'};
	size = 3;
	p = jvfmt_PutOverwrite(&f, spec, &size);
	ASSERT(size, ==, 3);
	memcpy(p, "!!!", size);

	spec = (JVFMT_SPEC){0};
	size = 9;
	p = jvfmt_PutOverwrite(&f, spec, &size);
	ASSERT(size, ==, 7); // Reached limit: maxLength = 31
	memcpy(p, "BlaBlaBla", size);

	char const* pResult = jvfmt_PutFinalize(&f);
	ASSERT_STR_EQUAL(pResult, "Hello,___World____   !!!BlaBlaB");

	///
	/// === END ===

	return MUNIT_OK;
}

MunitResult jvfmt_test_usage_lowlevel_putstring(MunitParameter const params[], void* fixture)
{
	(void)params;
	(void)fixture;

	/// === BEGIN USAGE_LOWLEVEL_PUTSTRING ===
	/// `jvfmt_PutString()` is the low-level API for string formatting.
	///	You can use it for writing custom formatters.

	JVFMT_SPEC spec;
	bool bContinue;

	char fmtBuffer[64];
	JVFMT f = {0};
	f.pBuffer = fmtBuffer;
	f.bufferSize = sizeof(fmtBuffer);
	f.maxLength = 31;

	spec = (JVFMT_SPEC){.width = 10, .align = '^', .fill = '.'};
	bContinue = jvfmt_PutString(&f, spec, "Hello", 5);
	ASSERT(bContinue, ==, true);
	ASSERT(f._priv_pos, ==, 10); // === HIDE ===
	ASSERT_MEM_EQUAL(10, fmtBuffer, "..Hello...");

	// The string length can be specified using the "precision" spec.
	spec = (JVFMT_SPEC){.precision = 5};
	bContinue = jvfmt_PutString(&f, spec, " abc def ", SIZE_MAX);
	ASSERT(bContinue, ==, true);
	ASSERT(f._priv_pos, ==, 15); // === HIDE ===
	ASSERT_MEM_EQUAL(15, fmtBuffer, "..Hello... abc ");

	spec = (JVFMT_SPEC){.quote = '?'};
	bContinue = jvfmt_PutString(&f, spec, "Julien!\r\n", 9);
	ASSERT(bContinue, ==, true);
	ASSERT(f._priv_pos, ==, 28); // === HIDE ===
	ASSERT_MEM_EQUAL(28, fmtBuffer, "..Hello... abc \"Julien!\\r\\n\"");

	// Reaching maxLength -> bContinue will be FALSE. You should early exit at this point.
	spec = (JVFMT_SPEC){0};
	bContinue = jvfmt_PutString(&f, spec, "[THE END]", 9);
	ASSERT(bContinue, ==, false);
	ASSERT(f._priv_pos, ==, 31); // === HIDE ===
	ASSERT_MEM_EQUAL(31, fmtBuffer, "..Hello... abc \"Julien!\\r\\n\"[TH");

	// Even though, you can still put strings... nothing will be done.
	spec = (JVFMT_SPEC){.quote = '?', .width = 12, .fill = "#", .align = '<'};
	bContinue = jvfmt_PutString(&f, spec, "--test--", 8);
	ASSERT(bContinue, ==, false);
	ASSERT(f._priv_pos, ==, 31); // === HIDE ===
	ASSERT_MEM_EQUAL(31, fmtBuffer, "..Hello... abc \"Julien!\\r\\n\"[TH");

	char* p = jvfmt_PutFinalize(&f);
	ASSERT_STR_EQUAL(p, "..Hello... abc \"Julien!\\r\\n\"[TH");

	/// === END ===

	return MUNIT_OK;
}

MunitResult jvfmt_test_usage_lowlevel_putptr(MunitParameter const params[], void* fixture)
{
	(void)params;
	(void)fixture;

	/// === BEGIN USAGE_LOWLEVEL_PUTPTR ===
	/// `jvfmt_PutPtr()` is the low-level API for pointer formatting.
	///	You can use it for writing custom formatters.
	/// The format is fixed, except for the usual width/align/fill specifications:
	/// `0x0123456789ABCDEF` (for 64-bit pointers).

	JVFMT_SPEC spec;
	bool bContinue;

	char fmtBuffer[64];
	JVFMT f = {0};
	f.pBuffer = fmtBuffer;
	f.bufferSize = sizeof(fmtBuffer);
	f.maxLength = 31;

	spec = (JVFMT_SPEC){.width = 22, .align = '^', .fill = '-'};
	bContinue = jvfmt_PutPtr(&f, spec, (void*)0x0011223344556677);
	ASSERT(bContinue, ==, true);
	ASSERT(f._priv_pos, ==, 22); // === HIDE ===
	ASSERT_MEM_EQUAL(22, fmtBuffer, "--0x0011223344556677--");

	// Reaching maxLength -> bContinue will be FALSE. You should early exit at this point.
	spec = (JVFMT_SPEC){0};
	bContinue = jvfmt_PutPtr(&f, spec, (void*)0xABCDEABCDEABCDE0);
	ASSERT(bContinue, ==, false);
	ASSERT(f._priv_pos, ==, 31); // === HIDE ===
	ASSERT_MEM_EQUAL(31, fmtBuffer, "--0x0011223344556677--0xABCDEAB");

	char* p = jvfmt_PutFinalize(&f);
	ASSERT_STR_EQUAL(p, "--0x0011223344556677--0xABCDEAB");

	/// === END ===

	return MUNIT_OK;
}