#pragma once
#ifndef JVFMT_H_
#define JVFMT_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct JVFMT JVFMT;
typedef union JVFMT_ARG JVFMT_ARG;
typedef struct JVFMT_SPEC JVFMT_SPEC;

#define jvfmt0(p, pFormat) (jvfmt_begin(p), jvfmt_end(p, pFormat))

#define jvfmt1(p, pFormat, a0) (jvfmt_begin(p), jvfmt_arg(p, a0), jvfmt_end(p, pFormat))

#define jvfmt2(p, pFormat, a0, a1)                                                                 \
	(jvfmt_begin(p), jvfmt_arg(p, a0), jvfmt_arg(p, a1), jvfmt_end(p, pFormat))

#define jvfmt3(p, pFormat, a0, a1, a2)                                                             \
	(jvfmt_begin(p), jvfmt_arg(p, a0), jvfmt_arg(p, a1), jvfmt_arg(p, a2), jvfmt_end(p, pFormat))

#define jvfmt4(p, pFormat, a0, a1, a2, a3)                                                         \
	(jvfmt_begin(p), jvfmt_arg(p, a0), jvfmt_arg(p, a1), jvfmt_arg(p, a2), jvfmt_arg(p, a3),       \
	 jvfmt_end(p, pFormat))

#define jvfmt5(p, pFormat, a0, a1, a2, a3, a4)                                                     \
	(jvfmt_begin(p), jvfmt_arg(p, a0), jvfmt_arg(p, a1), jvfmt_arg(p, a2), jvfmt_arg(p, a3),       \
	 jvfmt_arg(p, a4), jvfmt_end(p, pFormat))

#define jvfmt6(p, pFormat, a0, a1, a2, a3, a4, a5)                                                 \
	(jvfmt_begin(p), jvfmt_arg(p, a0), jvfmt_arg(p, a1), jvfmt_arg(p, a2), jvfmt_arg(p, a3),       \
	 jvfmt_arg(p, a4), jvfmt_arg(p, a5), jvfmt_end(p, pFormat))

#define jvfmt7(p, pFormat, a0, a1, a2, a3, a4, a5, a6)                                             \
	(jvfmt_begin(p), jvfmt_arg(p, a0), jvfmt_arg(p, a1), jvfmt_arg(p, a2), jvfmt_arg(p, a3),       \
	 jvfmt_arg(p, a4), jvfmt_arg(p, a5), jvfmt_arg(p, a6), jvfmt_end(p, pFormat))

/// @name Low-level API
/// @{

/// Resets the argument list to empty.
void jvfmt_begin(JVFMT* p);

void jvfmt_arg_llong(JVFMT* p, long long v);
void jvfmt_arg_ullong(JVFMT* p, unsigned long long v);
void jvfmt_arg_float(JVFMT* p, float v);
void jvfmt_arg_double(JVFMT* p, double v);
void jvfmt_arg_str(JVFMT* p, char const* v);
void jvfmt_arg_ptr(JVFMT* p, void const* v);

#define jvfmt_arg(p, v)                                                                            \
	_Generic(v + 0ll,                                                                              \
		long long: jvfmt_arg_llong,                                                                \
		unsigned long long: jvfmt_arg_ullong,                                                      \
		float: jvfmt_arg_float,                                                                    \
		double: jvfmt_arg_double,                                                                  \
		long double: jvfmt_arg_double,                                                             \
		char const*: jvfmt_arg_str,                                                                \
		char*: jvfmt_arg_str,                                                                      \
		default: jvfmt_arg_ptr)(p, v)

char const* jvfmt_end(JVFMT* p, char const* pFormat);

/// @}

#define JVFMT_MAX_ARGS 7
#define JVFMT_TMP_SIZE 128

#define JVFMT_RECOMMENDED_BUFFER_SIZE 4096
#define JVFMT_RECOMMENDED_MAX_LENGTH 511

union JVFMT_ARG {
	long long llong;		   ///< Kind = 'q'
	unsigned long long ullong; ///< Kind = 'Q'
	float float_;			   ///< Kind = 'f'
	double double_;			   ///< Kind = 'd'
	char const* str;		   ///< Kind = 's'
	void const* ptr;		   ///< Kind = 'p'
};

struct JVFMT {
	/// **(CONFIG)** Memory area where to put formatted output, used as ring buffer.
	char* pBuffer;
	/// **(CONFIG)** Number of code units writable in `pBuffer`.
	size_t bufferSize;
	/// **(CONFIG)** Maximum length, in code units, of each formatted output.
	/// 0 for no limit other than buffer size. For instance,
	/// if `bufferLength == 4096` and `maxLength == 511`, you are sure to keep
	/// in memory at least the last 8 formatted strings.
	size_t maxLength;

	/// **(RESULT)** Pointer to last formatted string, null-terminated.
	char const* pResult;
	/// **(RESULT)** Length, in code units, of the last formatted string.
	size_t resultLength;

	/// **(CALL)** Number of arguments.
	size_t argCount;
	/// **(CALL)** Type of each argument, encoded as a single ASCII byte per argument.
	char argKinds[JVFMT_MAX_ARGS];
	/// **(CALL)** Storage for each argument.
	JVFMT_ARG args[JVFMT_MAX_ARGS];

	size_t _priv_posBegin;
	size_t _priv_posEnd;
	size_t _priv_pos;
	char _priv_tmpBuffer[JVFMT_TMP_SIZE];
};

inline void jvfmt_begin(JVFMT* p)
{
	p->argCount = 0;
}

inline void jvfmt_arg_llong(JVFMT* p, long long v)
{
	p->argKinds[p->argCount] = 'q';
	p->args[p->argCount].llong = v;
	++p->argCount;
}

inline void jvfmt_arg_ullong(JVFMT* p, unsigned long long v)
{
	p->argKinds[p->argCount] = 'Q';
	p->args[p->argCount].ullong = v;
	++p->argCount;
}

inline void jvfmt_arg_float(JVFMT* p, float v)
{
	p->argKinds[p->argCount] = 'f';
	p->args[p->argCount].float_ = v;
	++p->argCount;
}

inline void jvfmt_arg_double(JVFMT* p, double v)
{
	p->argKinds[p->argCount] = 'd';
	p->args[p->argCount].double_ = v;
	++p->argCount;
}

inline void jvfmt_arg_str(JVFMT* p, char const* v)
{
	p->argKinds[p->argCount] = 's';
	p->args[p->argCount].str = v;
	++p->argCount;
}

inline void jvfmt_arg_ptr(JVFMT* p, void const* v)
{
	p->argKinds[p->argCount] = 'p';
	p->args[p->argCount].ptr = v;
	++p->argCount;
}

/// Reads as much ASCII digits as possible from `pStr`.
/// Not needed for `jvfmt` API, but can be useful for implementing custom formatters.
/// @returns Number of ASCII digits read, 0 in case of overflow.
static inline size_t jvfmt_impl_readUint16(char const* pStr, unsigned short* pOut)
{
	unsigned long v = 0;
	char const* p = pStr;
	for (; *p >= '0' && *p <= '9'; ++p) {
		v *= 10;
		v += (*p - '0');
		if (v >= 0x10000)
			return 0;
	}
	*pOut = (unsigned short)v;
	return p - pStr;
}

/// === BEGIN HEADER_LOWLEVEL_SPEC ===

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
bool jvfmt_ParseSpec(char const* p, JVFMT_SPEC* pSpec);

/// === END ===

bool jvfmt_PutPtr(JVFMT* f, JVFMT_SPEC spec, void const* value);
bool jvfmt_PutInt(JVFMT* f, JVFMT_SPEC spec, long long value);
bool jvfmt_PutUint(JVFMT* f, JVFMT_SPEC spec, unsigned long long value);
bool jvfmt_PutFloat(JVFMT* f, JVFMT_SPEC spec, float value);
bool jvfmt_PutDouble(JVFMT* f, JVFMT_SPEC spec, double value);
bool jvfmt_PutString(JVFMT* f, JVFMT_SPEC spec, char const* value, size_t valueLength);

/// === BEGIN HEADER_LOWLEVEL_PUTOVERWRITE ===

// Gives back a buffer where a formatter can output bytes.
// Formatters need to precompute their output size first, then they call
// `jvfmt_PutOverwrite()` to obtain a buffer for this output.
// The spec's width/align/fill are handled by this function.
char* jvfmt_PutOverwrite(JVFMT* f, JVFMT_SPEC spec, size_t* inout_pCharCount);

// Indicates that all PutOverwrite() are considered done.
// The next PutOverwrite() will start a new string.
// Returns a pointer to the total string written.
char const* jvfmt_PutFinalize(JVFMT* f);

/// === END ===

#endif
