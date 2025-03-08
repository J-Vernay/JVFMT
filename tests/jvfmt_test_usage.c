
#include "munit.h"

#include "../jvfmt.h"

#define ASSERT_STR_EQUAL munit_assert_string_equal

MunitResult jvfmt_test_usage(MunitParameter const params[], void* fixture)
{
	(void)params;
	(void)fixture;
	char const* pString = NULL;

	/// Contrary to most C formatting string APIs, **jvfmt** is explicit about its state.
	/// This lets you control the memory allocation strategy. You are responsible
	/// for zero-initializing `JVFMT`, and give it a buffer to use for storing
	/// generated strings, and a maximum length (excluding null-terminator) for generated strings.
	/// If unsure about these parameters, use the constants `JVFMT_RECOMMENDED_BUFFER_SIZE`
	/// and `JVFMT_RECOMMENDED_MAX_LENGTH`.

	char fmtBuffer[JVFMT_RECOMMENDED_BUFFER_SIZE];
	JVFMT f = {0};
	f.pBuffer = fmtBuffer;
	f.bufferSize = JVFMT_RECOMMENDED_BUFFER_SIZE;
	f.maxLength = JVFMT_RECOMMENDED_MAX_LENGTH;

	/// > [!TIP]
	/// > `pBuffer` is used as a ring buffer: it is filled from left to right,
	/// > until it reaches the end, then it overwrites the first generated string.
	/// > By tweaking `maxLength` and `bufferSize`, you can guarantee that the last N
	/// > generated strings are not overriden.
	/// >
	/// > `JVFMT_RECOMMENDED_BUFFER_SIZE = 4096` and  `JVFMT_RECOMMENDED_MAX_LENGTH = 511`.
	/// > This mean that the last 8 generated strings are guaranteed to be valid.

	/// > [!IMPORTANT]
	/// > **jvfmt** uses no global state, persistent state is explicitly stored in `JVFMT`.
	/// > For multithreading applications, this mean that **jvfmt** is thread-compatible:
	/// > concurrent accesses to independent `JVFMT` instances do not need to be synchronized.
	/// > However, you must synchronize concurrent accesses to a same `JVFMT` instance.
	/// >
	/// > One possible way is to make `fmtBuffer` and `f` global variables **thread_local**.
	/// > In which case, each thread is guaranteed to access a different instance, thus
	/// > no synchronization is needed.

	/// Format strings are inspired by [Python](https://docs.python.org/3/library/string.html).
	/// They consist of a normal ASCII-compatible string containing "replacement fields",
	/// delimited by curly braces `{}`. Anything outside these braces is considered literal text,
	/// and will be copied unchanged to the output. If you need to include a brace character in
	/// the literal text, you can double them with `{{` and `}}`.

	pString = jvfmt0(&f, "Hello!");
	ASSERT_STR_EQUAL(pString, "Hello!");

	pString = jvfmt0(&f, "Hello {{World}}!");
	ASSERT_STR_EQUAL(pString, "Hello {World}!");

	/// The replacement fields follow this grammar:
	///
	/// ```abnf
	/// replacement_field = "{" [arg_name] ["!" conv] [":" spec] "}"
	/// arg_idx           = *DIGIT               ; Unsigned decimal integer.
	/// conv              = *(ALPHA / DIGIT)     ; Sequence of alphanumerical ASCII chars.
	/// spec              = *(VCHAR)             ; Sequence of visible ASCII chars.
	/// ```
	/// * `arg_idx` identifies the argument provided by caller to use for replacement.
	///   If unspecified, the argument used is the next after the one used in previous replacement.
	/// * `conv` identifies a custom user-provided function to call for custom formatting.
	///   If unspecified, the builtin formatters for primitive types are used.
	/// * `spec` customizes the generated output, the effect depending on `conv`.
	/// ```c
	/// "First, thou shalt count to {0}"  // References first argument.
	/// "Bring me a {}"                   // Implicitly references the first argument.
	/// "From {} to {}"                   // Same as "From {0} to {1}".
	/// "My quest is {:5}"                // References first argument, spec is "5".
	/// "NTSTATUS = {0!ntstatus:FR}"      // Applies the "ntstatus" converter, with spec "FR".
	/// ```

	return MUNIT_OK;
}