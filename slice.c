#include <stdlib.h>
#include <string.h>

#include "slice.h"

bool slice_match(HttpSlice *slice, const char *str)
{
    size_t len = strlen(str);
    if (slice_len(slice) < len) {
        return false;
    }

    if (memcmp(slice->begin, str, len)) {
        return false;
    }

    slice->begin += len;
    return true;
}

bool slice_eq(const HttpSlice *slice, const char *str)
{
    size_t len = strlen(str);
    if (slice_len(slice) != len) {
        return false;
    }
    return !memcmp(slice->begin, str, len);
}

bool slice_eol(const HttpSlice *slice)
{
    return slice_len(slice) == 2 && slice->begin[0] == '\r' && slice->begin[1] == '\n';
}