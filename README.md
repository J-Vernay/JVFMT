
# JVFMT

## Usage


Contrary to most C formatting string APIs, **jvfmt** is explicit about its state.
This lets you control the memory allocation strategy. You are responsible
for zero-initializing `JVFMT`, and give it a buffer to use for storing
generated strings, and a maximum length (excluding null-terminator) for generated strings.
If unsure about these parameters, use the constants `JVFMT_RECOMMENDED_BUFFER_SIZE`
and `JVFMT_RECOMMENDED_MAX_LENGTH`.

```c
char fmtBuffer[JVFMT_RECOMMENDED_BUFFER_SIZE];
JVFMT f = {0};
f.pBuffer = fmtBuffer;
f.bufferSize = JVFMT_RECOMMENDED_BUFFER_SIZE;
f.maxLength = JVFMT_RECOMMENDED_MAX_LENGTH;
```

> [!TIP]
> `pBuffer` is used as a ring buffer: it is filled from left to right,
> until it reaches the end, then it overwrites the first generated string.
> By tweaking `maxLength` and `bufferSize`, you can guarantee that the last N
> generated strings are not overriden.
>
> `JVFMT_RECOMMENDED_BUFFER_SIZE = 4096` and  `JVFMT_RECOMMENDED_MAX_LENGTH = 511`.
> This mean that the last 8 generated strings are guaranteed to be valid.

> [!IMPORTANT]
> **jvfmt** uses no global state, persistent state is explicitly stored in `JVFMT`.
> For multithreading applications, this mean that **jvfmt** is thread-compatible:
> concurrent accesses to independent `JVFMT` instances do not need to be synchronized.
> However, you must synchronize concurrent accesses to a same `JVFMT` instance.
>
> One possible way is to make `fmtBuffer` and `f` global variables **thread_local**.
> In which case, each thread is guaranteed to access a different instance, thus
> no synchronization is needed.

Format strings are inspired by [Python](https://docs.python.org/3/library/string.html).
They consist of a normal ASCII-compatible string containing "replacement fields",
delimited by curly braces `{}`. Anything outside these braces is considered literal text,
and will be copied unchanged to the output. If you need to include a brace character in
the literal text, you can double them with `{{` and `}}`.

```c
pString = jvfmt0(&f, "Hello!");
ASSERT_STR_EQUAL(pString, "Hello!");

pString = jvfmt0(&f, "Hello {{World}}!");
ASSERT_STR_EQUAL(pString, "Hello {World}!");
```

The replacement fields follow this grammar:

```abnf
replacement_field = "{" [arg_name] ["!" conv] [":" spec] "}"
arg_idx           = *DIGIT               ; Unsigned decimal integer.
conv              = *(ALPHA / DIGIT)     ; Sequence of alphanumerical ASCII chars.
spec              = *(VCHAR)             ; Sequence of visible ASCII chars.
```
* `arg_idx` identifies the argument provided by caller to use for replacement.
  If unspecified, the argument used is the next after the one used in previous replacement.
* `conv` identifies a custom user-provided function to call for custom formatting.
  If unspecified, the builtin formatters for primitive types are used.
* `spec` customizes the generated output, the effect depending on `conv`.
```c
"First, thou shalt count to {0}"  // References first argument.
"Bring me a {}"                   // Implicitly references the first argument.
"From {} to {}"                   // Same as "From {0} to {1}".
"My quest is {:5}"                // References first argument, spec is "5".
"NTSTATUS = {0!ntstatus:FR}"      // Applies the "ntstatus" converter, with spec "FR".
```

Replacement fields access arguments. In `jvfmt`, these arguments are specified
with the macros `jvfmt0()`, `jvfmt1()`, `jvfmt2()`, etc. The number corresponds
to the number of arguments given by the caller.

```c
pString = jvfmt0(&f, "Hello!");
ASSERT_STR_EQUAL(pString, "Hello!");

pString = jvfmt1(&f, "Hello {}!", "World");
ASSERT_STR_EQUAL(pString, "Hello World!");

pString = jvfmt2(&f, "{1} {0} !", "le monde", "Bonjour");
ASSERT_STR_EQUAL(pString, "Bonjour le monde !");

pString = jvfmt2(&f, "{} {} {0} {} {0} {0} {}", "tic", "tac");
ASSERT_STR_EQUAL(pString, "tic tac tic tac tic tic tac");
```

**jvfmt** has built-in support for primitive C data types:
signed and unsigned integers, floats and doubles, pointers
and null-terminated char strings.

```c
pString = jvfmt4(&f, "{} {} {} {}", 0.25f, 1.125e300, INT64_MIN, UINT64_MAX);
```


### Low-level API

   Whether you want to call directly the low-level API or implement a custom formatter,
you need to provide a format specification, represented by the `JVFMT_SPEC` structure.


```h
struct JVFMT_SPEC {
    unsigned short width;
    unsigned short precision;
    char fill;
    char align;
    char quote;
    char type;
    char flags[8];
};

// Utility for implementing custom formatters with some decent support for specifiers.
// @returns Whether the entire specification have been consumed, else an error occurred.
bool jvfmtParseSpec(char const* p, JVFMT_SPEC* pSpec);
```


The structure can be obtained by parsing the specification string with `jvfmtParseSpec()`.

```abnf
spec      = [[fill] align] [flags] [width] ["." precision] [quote] [type]
fill      = <any character>
align     = "<" / ">" / "^" / "="
flags     = 0*7<any character except "1" to "9">
width     = <unsigned integer>
precision = <unsigned integer>
quote     = "?"
type      = <any character>
```

* `width` defines the minimum total field width, including any prefixes, separators,
  etc. Note that trailing zeroes are considered part of the `flags`.
* `fill` indicates which character to use for padding, by default a whitespace `' '`.
* `align` indicates how to position the field content within the available space:
  * `'<'` for left-alignment (default for strings).
  * `'>'` for right-alignment (default for numbers).
  * `'^'` for centered-alignment.
  * `'='` for right-alignment, except than the initial sign `+` or `-` (if present)
    is left-aligned.
* `flags` is a sequence of characters whose presence change the formatting in some way,
  dependent on the formatting type.
* `quote` indicates that the result must be quoted with `"` and its
  content properly escaped with backslash sequences, i.e. `\n`, `\"`, etc.
* `type` determines how the data should be presented (e.g. binary format...).

```c
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
```



**jvfmt** exposes a low-level API consisting of direct element concatenation.
Note that these APIs do not null-terminate their output.


```h
void jvfmtConcatPtr(JVFMT* f, JVFMT_SPEC spec, void const* value);
void jvfmtConcatInt(JVFMT* f, JVFMT_SPEC spec, long long value);
void jvfmtConcatUint(JVFMT* f, JVFMT_SPEC spec, unsigned long long value);
void jvfmtConcatFloat(JVFMT* f, JVFMT_SPEC spec, float value);
void jvfmtConcatDouble(JVFMT* f, JVFMT_SPEC spec, double value);
void jvfmtConcatString(JVFMT* f, JVFMT_SPEC spec, char const* value);
void jvfmtConcatRawBytes(JVFMT* f, char const* pBytes, size_t byteCount);
```


Among these functions, `jvfmtConcatRawBytes()` is the most basic: it directly
copies bytes to the `JVFMT` buffer.

```c
char fmtBuffer[JVFMT_RECOMMENDED_BUFFER_SIZE];
JVFMT f = {0};
f.pBuffer = fmtBuffer;
f.bufferSize = JVFMT_RECOMMENDED_BUFFER_SIZE;
f.maxLength = JVFMT_RECOMMENDED_MAX_LENGTH;

jvfmtConcatRawBytes(&f, "Hello,", 6);
ASSERT_MEM_EQUAL(6, f.pBuffer, "Hello,");

jvfmtConcatRawBytes(&f, " World!", 7);
ASSERT_MEM_EQUAL(13, f.pBuffer, "Hello, World!");
```
