#include <stdlib.h>
#include <string.h>

#include "span.h"

bool mhttp_span_match(struct mhttp_span *span, const char *str)
{
	size_t len = strlen(str);

	if (mhttp_span_len(span) < len)
		return false;

	if (memcmp(span->begin, str, len))
		return false;

	span->begin += len;

	return true;
}

bool mhttp_span_eq(const struct mhttp_span *span, const char *str)
{
	size_t len = strlen(str);

	if (mhttp_span_len(span) != len)
		return false;

	return !memcmp(span->begin, str, len);
}

bool mhttp_span_eol(const struct mhttp_span *span)
{
	return mhttp_span_len(span) == 2 && span->begin[0] == '\r' && span->begin[1] == '\n';
}
