
# JVFMT (currently WIP!)

**jvfmt** is a C11 library for 


## High-level API


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


## Low-level API


### The JVFMT_SPEC structure

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



### jvfmt_PutOverwrite() and jvfmt_PutFinalize()

`jvfmt_PutOverwrite()` is the lowest-level API exposed by JVFMT.
You are expected to know the output size in advance;
the function reserves an area in the ring buffer for your output,
and also fills the left and right padding for alignment purpose,
according to the spec's `width`, `align` and `fill`.

> [!IMPORTANT]
> The returned area can be smaller than the requested size,
  when the `maxLength` limit is reached. Make sure to take
  into account the returned size and output only those chars!

After all `jvfmt_PutOverwrite()` operations, use `jvfmt_PutFinalize()`
to add a null-terminator, and get back a pointer to the total string.


```h
// Gives back a buffer where a formatter can output bytes.
// Formatters need to precompute their output size first, then they call
// `jvfmt_PutOverwrite()` to obtain a buffer for this output.
// The spec's width/align/fill are handled by this function.
char* jvfmt_PutOverwrite(JVFMT* f, JVFMT_SPEC spec, size_t* inout_pCharCount);

// Indicates that all PutOverwrite() are considered done.
// The next PutOverwrite() will start a new string.
// Returns a pointer to the total string written.
char const* jvfmt_PutFinalize(JVFMT* f);
```


```c
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
```




### jvfmt_PutString()

`jvfmt_PutString()` is the low-level API for string formatting.
   You can use it for writing custom formatters.

```c
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
ASSERT_MEM_EQUAL(10, fmtBuffer, "..Hello...");

// The string length can be specified using the "precision" spec.
spec = (JVFMT_SPEC){.precision = 5};
bContinue = jvfmt_PutString(&f, spec, " abc def ", SIZE_MAX);
ASSERT(bContinue, ==, true);
ASSERT_MEM_EQUAL(15, fmtBuffer, "..Hello... abc ");

spec = (JVFMT_SPEC){.quote = '?'};
bContinue = jvfmt_PutString(&f, spec, "Julien!\r\n", 9);
ASSERT(bContinue, ==, true);
ASSERT_MEM_EQUAL(28, fmtBuffer, "..Hello... abc \"Julien!\\r\\n\"");

// Reaching maxLength -> bContinue will be FALSE. You should early exit at this point.
spec = (JVFMT_SPEC){0};
bContinue = jvfmt_PutString(&f, spec, "[THE END]", 9);
ASSERT(bContinue, ==, false);
ASSERT_MEM_EQUAL(31, fmtBuffer, "..Hello... abc \"Julien!\\r\\n\"[TH");

// Even though you can still put strings... nothing will be done.
spec = (JVFMT_SPEC){.quote = '?', .width = 12, .fill = "#", .align = '<'};
bContinue = jvfmt_PutString(&f, spec, "--test--", 8);
ASSERT(bContinue, ==, false);
ASSERT_MEM_EQUAL(31, fmtBuffer, "..Hello... abc \"Julien!\\r\\n\"[TH");

char* p = jvfmt_PutFinalize(&f);
ASSERT_STR_EQUAL(p, "..Hello... abc \"Julien!\\r\\n\"[TH");
```
