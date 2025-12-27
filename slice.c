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