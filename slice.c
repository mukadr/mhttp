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

    slice_advance(slice, len);
    return true;
}

bool slice_at_endofline(const HttpSlice *slice)
{
    if (slice_len(slice) > 0) {
        if (slice->begin[0] == '\n') {
            return true;
        }
        if (slice_len(slice) > 1 && slice->begin[0] == '\r' && slice->begin[1] == '\n') {
            return true;
        }
    }
    return false;
}