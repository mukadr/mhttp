#ifndef MHTTP_SLICE_H
#define MHTTP_SLICE_H

#include <stdbool.h>

typedef struct HttpSlice {
    char *begin;
    char *end; // 1 after the last valid byte
} HttpSlice;

static inline size_t slice_len(const HttpSlice *slice)
{
    return slice->end - slice->begin;
}

static inline bool slice_empty(const HttpSlice *slice)
{
    return !slice_len(slice);
}

static inline int slice_next(HttpSlice *slice)
{
    if (slice->begin == slice->end) {
        return -1;
    }
    return *slice->begin++;
}

static inline void slice_advance(HttpSlice *slice, int n)
{
    if (n > slice_len(slice)) {
        n = slice_len(slice);
    }
    slice->begin += n;
}

static inline bool slice_eq(const HttpSlice *slice, const char *str)
{
    return !memcmp(slice->begin, str, slice_len(slice));
}

bool slice_match(HttpSlice *slice, const char *str);
bool slice_at_endofline(const HttpSlice *slice);

#endif // MHTTP_SLICE_H