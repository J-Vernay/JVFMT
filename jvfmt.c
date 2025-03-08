#include "jvfmt.h"

#include <stddef.h>

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
	char* p = f->pBuffer;
	while (*pFormat) {
		_jvfmtParseResult res = {0};
		_jvfmtError error = _jvfmtParse(&pFormat, &res);
		if (error) {
			break; // TODO
		}
		for (size_t i = 0; i < res.lenLiteral; ++i) {
			*p++ = res.pLiteral[i];
		}
		if (!res.pName)
			continue; // No substitution

		break; // TODO
	}

	*p = '\0';
	f->pResult = f->pBuffer;
	f->resultLength = p - f->pResult;
	return f->pResult;
}
