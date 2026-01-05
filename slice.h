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

static inline int slice_peek(HttpSlice *slice)
{
    if (slice->begin == slice->end) {
        return -1;
    }
    return *slice->begin;
}

static inline int slice_next(HttpSlice *slice)
{
    if (slice->begin == slice->end) {
        return -1;
    }
    return *(++slice->begin);
}

static inline void slice_skip(HttpSlice *slice)
{
    if (slice_len(slice)) {
        slice->begin++;
    }
}

bool slice_match(HttpSlice *slice, const char *str);

bool slice_eq(const HttpSlice *slice, const char *str);

bool slice_eol(const HttpSlice *slice);

#endif // MHTTP_SLICE_H