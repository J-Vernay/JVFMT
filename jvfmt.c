#include "jvfmt.h"

#include <stddef.h>
#include <string.h>

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

void jvfmtConcatPtr(JVFMT* f, JVFMT_SPEC spec, void const* value)
{
	jvfmtConcatRawBytes(f, "JVERR-NOIMPL", 12);
}

void jvfmtConcatInt(JVFMT* f, JVFMT_SPEC spec, long long value)
{
	jvfmtConcatRawBytes(f, "JVERR-NOIMPL", 12);
}

void jvfmtConcatUint(JVFMT* f, JVFMT_SPEC spec, unsigned long long value)
{
	jvfmtConcatRawBytes(f, "JVERR-NOIMPL", 12);
}

void jvfmtConcatFloat(JVFMT* f, JVFMT_SPEC spec, float value)
{
	jvfmtConcatRawBytes(f, "JVERR-NOIMPL", 12);
}

void jvfmtConcatDouble(JVFMT* f, JVFMT_SPEC spec, double value)
{
	jvfmtConcatRawBytes(f, "JVERR-NOIMPL", 12);
}

void jvfmtConcatString(JVFMT* f, JVFMT_SPEC spec, char const* value)
{
	jvfmtConcatRawBytes(f, "JVERR-NOIMPL", 12);
}

void jvfmtConcatRawBytes(JVFMT* f, char const* pBytes, size_t byteCount)
{
	// Check whether we reach the end of the ring buffer.
	// In this case, move the current string at start of ring buffer.
	// This same check works also when `f` has just been zero-initialized,
	// to init `_priv_posEnd` to its relevant value `maxLength`.
	size_t remaining = f->_priv_posEnd - f->_priv_pos;
	if (byteCount > remaining								  //
		&& f->_priv_posEnd - f->_priv_posBegin < f->maxLength //
	) {
		char* pSrc = f->pBuffer + f->_priv_posBegin;
		char* pDst = f->pBuffer;
		size_t count = f->_priv_pos - f->_priv_posBegin;
		memmove(pDst, pSrc, count);
		f->_priv_posBegin = 0;
		f->_priv_pos = count;
		f->_priv_posEnd = f->maxLength;
		remaining = f->maxLength - count;
	}
	if (byteCount > remaining)
		byteCount = remaining;
	memcpy(f->pBuffer + f->_priv_pos, pBytes, byteCount);
	f->_priv_pos += byteCount;
}