#pragma once
#ifndef JVFMT_H_
#define JVFMT_H_

typedef struct JVFMT JVFMT;

#define jvfmt0(p, pFormat) (jvfmt_begin(p), jvfmt_end(p, pFormat))

#define jvfmt1(p, pFormat, a0) (jvfmt_begin(p), jvfmt_arg(p, a0), jvfmt_end(p, pFormat))

#define jvfmt2(p, pFormat, a0, a1)                                                                 \
	(jvfmt_begin(p), jvfmt_arg(p, a0), jvfmt_arg(p, a1), jvfmt_end(p, pFormat))

#define jvfmt3(p, pFormat, a0, a1, a2)                                                             \
	(jvfmt_begin(p), jvfmt_arg(p, a0), jvfmt_arg(p, a1), jvfmt_arg(p, a2), jvfmt_end(p, pFormat))

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

#define FMT_MAX_ARGS 7

#define JVFMT_RECOMMENDED_BUFFER_SIZE 4096
#define JVFMT_RECOMMENDED_MAX_LENGTH 511

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
	char argKinds[FMT_MAX_ARGS];
	/// **(CALL)** Storage for each argument.
	union {
		long long llong;		   ///< Kind = 'q'
		unsigned long long ullong; ///< Kind = 'Q'
		float float_;			   ///< Kind = 'f'
		double double_;			   ///< Kind = 'd'
		char const* str;		   ///< Kind = 's'
		void const* ptr;		   ///< Kind = 'p'
	} args[FMT_MAX_ARGS];

	/// **(INTERNAL)** Beginning for next formatted output.
	size_t _priv_pos;
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

#endif
