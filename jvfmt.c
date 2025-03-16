#include "jvfmt.h"

#include <stddef.h>
#include <string.h>

// Adapted to 64-bits from:
// https://graphics.stanford.edu/~seander/bithacks.html#ValueInWord
#define haszero64(v) (((v) - 0x0101010101010101ull) & ~(v) & 0x8080808080808080ull)
#define hasvalue64(x, n) (haszero64((x) ^ (0x0101010101010101ull * (n))))

#if _MSC_VER
#define JVIMPL_NOINLINE __declspec(noinline)
#else
#define JVIMPL_NOINLINE __attribute__((noinline))
#endif

typedef enum _jvfmtError {
	_jvfmt_OK = 0,
	_jvfmt_CLOSE_BRACE_NOT_FOUND,
} _jvfmtError;

typedef struct _jvfmtParseResult {
	char const* pLiteral;
	size_t lenLiteral;
	char const* pName;
	size_t lenName;
	char const* pConv;
	size_t lenConv;
	char const* pSpec;
	size_t lenSpec;
} _jvfmtParseResult;

_jvfmtError _jvfmtParse(char const** ppFormat, _jvfmtParseResult* pResult)
{
	// Assuming pResult is already constructed zero-initialized.

	char const* p = *ppFormat;
	unsigned char c = *p;

	// 1. Search for literal end.
	pResult->pLiteral = p;
	while (c && c != '{' && c != '}')
		c = *++p;
	pResult->lenLiteral = p - pResult->pLiteral;

	if (!c) {
		// Reached end of string.
		*ppFormat = p;
		return _jvfmt_OK;
	}
	++p;
	if (c == *p) {
		// Reached escaped '{' or '}'
		++pResult->lenLiteral;
		*ppFormat = ++p;
		return _jvfmt_OK;
	}

	// 2. Search for arg index.
	pResult->pName = p;
	c = *p;
	while (c && c != '{' && c != '}' && c != '!' && c != ':')
		c = *++p;
	pResult->lenName = p - pResult->pName;

	// 3. Search for conversion function.
	if (c == '!') {
		c = *++p;
		pResult->pConv = p;
		while (c && c != '{' && c != '}' && c != ':')
			c = *++p;
		pResult->lenConv = p - pResult->pName;
	}

	// 4. Search for specification.
	if (c == ':') {
		pResult->pSpec = p + 1;
		while (c && c != '{' && c != '}')
			c = *++p;
		pResult->lenSpec = p - pResult->pSpec;
	}

	if (c != '}') {
		*ppFormat = pResult->pLiteral + pResult->lenLiteral;
		return _jvfmt_CLOSE_BRACE_NOT_FOUND;
	}
	++p;
	*ppFormat = p;
	return _jvfmt_OK;
}

/*
static unsigned long _jvfmt_read_small_uint(char const** ppStr)
{
	unsigned long v = 0;
	char const* p = *ppStr;
	for (; *p >= '0' && *p <= '9'; ++p) {
		v *= 10;
		v += (*p - '0');
		if (v >= 1000000)
			return ULONG_MAX;
	}
	*ppStr = p;
	return v;
}
*/

char const* jvfmt_end(JVFMT* f, char const* pFormat)
{
	unsigned short argIdx = 0;
	char* p = f->pBuffer;

	for (; *pFormat; ++argIdx) {
		_jvfmtParseResult res = {0};
		_jvfmtError error = _jvfmtParse(&pFormat, &res);
		if (error) {
			break; // TODO
		}
		for (size_t i = 0; i < res.lenLiteral; ++i) {
			*p++ = res.pLiteral[i];
		}
		if (!res.pName) {
			continue; // No substitution
		}

		if (res.lenName) {
			size_t lenDigits = jvfmt_impl_readUint16(res.pName, &argIdx);
			if (lenDigits != res.lenName)
				break; // TODO
		}

		if (argIdx >= f->argCount) {
			break; // TODO
		}

		char argKind = f->argKinds[argIdx];

		if (argKind != 's') {
			break; // TODO
		}
		char const* pSrc = f->args[argIdx].str;
		while (*pSrc)
			*p++ = *pSrc++;
	}

	*p = '\0';
	f->pResult = f->pBuffer;
	f->resultLength = p - f->pResult;
	return f->pResult;
}

bool jvfmtParseSpec(char const* pIn, JVFMT_SPEC* pSpec)
{
	JVFMT_SPEC spec = {0};
	size_t flagCount = 0;

	char const* p = pIn;
	char c0 = p[0];
	if (!c0)
		goto end_parse;

	char c = p[1];
	if (c == '<' || c == '>' || c == '^' || c == '=') {
		spec.fill = c0;
		spec.align = c;
		p += 2;
	}
	else if (c0 == '<' || c0 == '>' || c0 == '^' || c0 == '=') {
		spec.fill = ' ';
		spec.align = c0;
		p += 1;
	}

	for (;; ++p) {
		c = *p;
		if (!c)
			goto end_parse;

		if (c >= '1' && c <= '9' || c == '.')
			break;
		// Leading zeroes are part of the prefix.
		if (flagCount == sizeof(spec.flags) - 1)
			goto end_parse; // Too many flags.
		spec.flags[flagCount++] = c;
	}

	if (*p != '.')
		p += jvfmt_impl_readUint16(p, &spec.width);
	if (*p == '.')
		p += jvfmt_impl_readUint16(p + 1, &spec.precision) + 1;
	if (*p == '?')
		spec.quote = *p++;
	if (*p)
		spec.type = *p++;

end_parse:
	if (*p)
		return false; // Error: not all specification consumed.
	*pSpec = spec;
	return true;
}

char* jvfmt_PutOverwrite(JVFMT* f, JVFMT_SPEC spec, size_t* inout_pCharCount)
{
	size_t valueLength = *inout_pCharCount;

	// 1. Apply spec.width argument.

	size_t outputLength = valueLength;
	if (outputLength < spec.width)
		outputLength = spec.width;

	// 2. Grow the output buffer if needed.

	size_t remaining = f->_priv_posEnd - f->_priv_pos;
	if (remaining < outputLength) {
		// Reached the end of the buffer. Are we already at "maxLength" limit?
		if (f->_priv_posEnd - f->_priv_posBegin < f->maxLength) {
			// We have reached the ring buffer's end.
			// We move the current string to the begin of the ring buffer.
			size_t currentLength = f->_priv_pos - f->_priv_posBegin;
			memmove(f->pBuffer, f->pBuffer + f->_priv_pos, currentLength);
			f->_priv_posBegin = 0;
			f->_priv_pos = currentLength;
			f->_priv_posEnd = f->maxLength;
			remaining = f->_priv_posEnd - f->_priv_pos;
		}
	}
	char* p = f->pBuffer + f->_priv_pos;
	char* pEnd = p + remaining;

	// 3. Compute padding.

	// Default is left-alignment.
	size_t leftPad = 0;
	size_t rightPad = outputLength - valueLength;
	if (spec.align == '>') {
		// Right-alignment.
		leftPad = rightPad;
		rightPad = 0;
	}
	else if (spec.align == '^') {
		// Center-alignment.
		leftPad = rightPad / 2;
		rightPad -= leftPad;
	}

	// 4. Fill the padding.

	if (leftPad > 0) {
		if (leftPad > remaining)
			leftPad = remaining;
		memset(p, spec.fill, leftPad);
		p += leftPad;
		remaining -= leftPad;
	}

	if (valueLength > remaining)
		valueLength = remaining;
	pEnd = p + valueLength;
	remaining -= valueLength;

	if (rightPad > 0) {
		if (rightPad > remaining)
			rightPad = remaining;
		memset(pEnd, spec.fill, rightPad);
		remaining -= rightPad;
	}

	// 5. Finalize.

	f->_priv_pos = f->_priv_posEnd - remaining;
	*inout_pCharCount = pEnd - p;
	return p;
}

char const* jvfmt_PutFinalize(JVFMT* f)
{
	char const* p = f->pBuffer + f->_priv_posBegin;
	char* pEnd = f->pBuffer + f->_priv_pos;
	// ASSERT(pEnd - p <= f->maxLength);

	*pEnd = '\0';
	++f->_priv_pos;
	f->_priv_posBegin = f->_priv_posBegin;
	if (f->bufferSize - f->_priv_pos >= f->maxLength)
		f->_priv_posEnd = f->_priv_posBegin + f->maxLength;
	else
		f->_priv_posEnd = f->bufferSize;

	return p;
}

#pragma region jvfmt_PutString()

// \t=0x09 \n=0x0A \r=0x0D \"=0x22 \\=0x5C
#define _jvimpl_IsStrEscape(c) hasvalue64(0x090A0D225C5C5C5Cull, c)

size_t _jvimpl_GetQuoteContentLength(JVFMT* f, JVFMT_SPEC spec, char const* p, size_t valueLength)
{
	// Chars matching bLUT are \r \n \t etc : one extra char to output.
	// Other control chars are \u0000 etc : five extra chars to output.

	size_t escapedLength = valueLength;
	char const* pEnd = p + valueLength;
	while (true) {
		// Fast path
		while (true) {
			if (p == pEnd)
				return escapedLength;
			unsigned char c = *p++;
			if (_jvimpl_IsStrEscape(c))
				goto char_escape;
			if (c <= 0x20)
				break;
		}
		escapedLength += 4;
	char_escape:
		escapedLength += 1;
	}
}

static char* _jvimpl_EscapeString_SlowPath(char* p, char* pEnd, char const* pIn,
										   char const* pInEnd);

JVIMPL_NOINLINE static char* _jvimpl_EscapeString(char* p, char* pEnd, char const* pIn,
												  char const* pInEnd)
{
	// Fast path, for long sequences with nothing to escape.
	for (; p != pEnd && pIn != pInEnd; ++pIn) {
		unsigned char c = *pIn;
		if (c < 0x20 || c == '"' || c == '\\')
			return _jvimpl_EscapeString_SlowPath(p, pEnd, pIn, pInEnd);
		*p++ = c;
	}
	return p;
}

JVIMPL_NOINLINE static char* _jvimpl_EscapeString_SlowPath(char* p, char* pEnd, char const* pIn,
														   char const* pInEnd)
{
	char c = *pIn++;
	if (_jvimpl_IsStrEscape(c)) {
		*p++ = '\\';
		if (p == pEnd)
			return p;
		switch (c) {
		case '\t':
			*p++ = 't';
			break;
		case '\n':
			*p++ = 'n';
			break;
		case '\r':
			*p++ = 'r';
			break;
		case '"':
			*p++ = '"';
			break;
		case '\\':
			*p++ = '\\';
			break;
		}
		return _jvimpl_EscapeString(p, pEnd, pIn, pInEnd);
	}
	else {
		*p++ = '\\';
		if (p == pEnd)
			return p;
		*p++ = 'u';
		if (p == pEnd)
			return p;
		*p++ = '0';
		if (p == pEnd)
			return p;
		*p++ = '0';
		if (p == pEnd)
			return p;
		*p++ = "0123456789ABCDEF"[c / 16];
		if (p == pEnd)
			return p;
		*p++ = "0123456789ABCDEF"[c % 16];
		return _jvimpl_EscapeString(p, pEnd, pIn, pInEnd);
	}
}

bool jvfmt_PutString(JVFMT* f, JVFMT_SPEC spec, char const* value, size_t valueLength)
{
	if (spec.type && spec.type != 's' && spec.type != 'g')
		return false; // ERROR

	if (spec.precision && valueLength > spec.precision)
		valueLength = spec.precision;

	size_t contentLength = valueLength;
	if (spec.quote) {
		contentLength = _jvimpl_GetQuoteContentLength(f, spec, value, valueLength);
		contentLength += 2; // Quotes.
	}

	size_t actualCount = contentLength;
	char* p = jvfmt_PutOverwrite(f, spec, &actualCount);
	char* pEnd = p + actualCount;
	if (p == pEnd)
		return false;

	if (!spec.quote) {
		memcpy(p, value, actualCount);
		return actualCount == valueLength;
	}

	if (spec.type == 'c')
		*p++ = '\'';
	else
		*p++ = '"';
	if (p == pEnd)
		return false;
	p = _jvimpl_EscapeString(p, pEnd, value, value + valueLength);
	if (p == pEnd)
		return false;
	if (spec.type == 'c')
		*p++ = '\'';
	else
		*p++ = '"';
	return true;
}

#pragma endregion
