
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

	/// The structure can be obtained by parsing the specification string with `jvfmtParseSpec()`.
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
	ASSERT(jvfmtParseSpec("42.110", &spec), ==, true);
	ASSERT(spec.width, ==, 42);
	ASSERT(spec.precision, ==, 110);
	ASSERT(spec.fill, ==, 0);
	ASSERT(spec.align, ==, 0);
	ASSERT(spec.quote, ==, false);
	ASSERT(spec.type, ==, 0);
	ASSERT_STR_EQUAL(spec.flags, "");

	ASSERT(jvfmtParseSpec("x^#_08X", &spec), ==, true);
	ASSERT(spec.width, ==, 8);
	ASSERT(spec.precision, ==, 0);
	ASSERT(spec.fill, ==, 'x');
	ASSERT(spec.align, ==, '^');
	ASSERT(spec.quote, ==, false);
	ASSERT(spec.type, ==, 'X');
	ASSERT_STR_EQUAL(spec.flags, "#_0");

	/// === END ===

	/// === BEGIN USAGE_LOWLEVEL_CONCAT ===
	///
	/// **jvfmt** exposes a low-level API consisting of direct element concatenation.
	/// Note that these APIs do not null-terminate their output.
	///
	/// === INCLUDE ../jvfmt.h HEADER_LOWLEVEL_CONCAT ===
	///
	/// Among these functions, `jvfmtConcatRawBytes()` is the most basic: it directly
	/// copies bytes to the `JVFMT` buffer.

	char fmtBuffer[JVFMT_RECOMMENDED_BUFFER_SIZE];
	JVFMT f = {0};
	f.pBuffer = fmtBuffer;
	f.bufferSize = JVFMT_RECOMMENDED_BUFFER_SIZE;
	f.maxLength = JVFMT_RECOMMENDED_MAX_LENGTH;

	jvfmtConcatRawBytes(&f, "Hello,", 6);
	ASSERT_MEM_EQUAL(6, f.pBuffer, "Hello,");
	ASSERT(f._priv_pos, ==, 6); /// === HIDE ===

	jvfmtConcatRawBytes(&f, " World!", 7);
	ASSERT_MEM_EQUAL(13, f.pBuffer, "Hello, World!");
	ASSERT(f._priv_pos, ==, 13); /// === HIDE ===

	/// === END ===

	return MUNIT_OK;
}