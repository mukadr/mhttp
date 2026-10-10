#ifndef MHTTP_SPAN_H
#define MHTTP_SPAN_H

#include <stdbool.h>

struct mhttp_span {
	char *begin;
	char *end; // 1 after the last valid byte
};

static inline size_t mhttp_span_len(const struct mhttp_span *span)
{
	return span->end - span->begin;
}

static inline bool mhttp_span_empty(const struct mhttp_span *span)
{
	return !mhttp_span_len(span);
}

static inline int mhttp_span_peek(struct mhttp_span *span)
{
	if (span->begin == span->end)
		return -1;
	return *span->begin;
}

static inline int mhttp_span_next(struct mhttp_span *span)
{
	if (span->begin + 1 >= span->end)
		return -1;
	return *(++span->begin);
}

static inline void mhttp_span_skip(struct mhttp_span *span)
{
	if (mhttp_span_len(span))
		span->begin++;
}

bool mhttp_span_match(struct mhttp_span *span, const char *str);

bool mhttp_span_eq(const struct mhttp_span *span, const char *str);

bool mhttp_span_eol(const struct mhttp_span *span);

#endif // MHTTP_SPAN_H
