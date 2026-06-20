#ifndef MHTTP_SLICE_H
#define MHTTP_SLICE_H

#include <stdbool.h>

struct mhttp_slice
{
	char *begin;
	char *end; // 1 after the last valid byte
};

static inline size_t mhttp_slice_len(const struct mhttp_slice *slice)
{
	return slice->end - slice->begin;
}

static inline bool mhttp_slice_empty(const struct mhttp_slice *slice)
{
	return !mhttp_slice_len(slice);
}

static inline int mhttp_slice_peek(struct mhttp_slice *slice)
{
	if (slice->begin == slice->end)
		return -1;
	return *slice->begin;
}

static inline int mhttp_slice_next(struct mhttp_slice *slice)
{
	if (slice->begin + 1 >= slice->end)
		return -1;
	return *(++slice->begin);
}

static inline void mhttp_slice_skip(struct mhttp_slice *slice)
{
	if (mhttp_slice_len(slice))
		slice->begin++;
}

bool mhttp_slice_match(struct mhttp_slice *slice, const char *str);

bool mhttp_slice_eq(const struct mhttp_slice *slice, const char *str);

bool mhttp_slice_eol(const struct mhttp_slice *slice);

#endif // MHTTP_SLICE_H
