#ifndef MHTTP_SLICE_H
#define MHTTP_SLICE_H

typedef struct HttpSlice {
    char *begin;
    char *end; // 1 after the last valid byte
} HttpSlice;

static inline size_t slice_len(const HttpSlice *slice)
{
    return slice->end - slice->begin;
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
    if (n > slice->end - slice->begin) {
        n = slice->end - slice->begin;
    }
    slice->begin += n;
}

#endif // MHTTP_SLICE_H