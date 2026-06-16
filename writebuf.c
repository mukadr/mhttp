#include <string.h>
#include <stdlib.h>

#include "writebuf.h"

HttpWriteBuf *http_writebuf_new(size_t capacity)
{
    if (capacity == 0) {
        return NULL;
    }

    HttpWriteBuf *buf = calloc(1, sizeof(*buf) + capacity);
    if (!buf) {
        return NULL;
    }

    buf->capacity = capacity;

    return buf;
}

void http_writebuf_free(HttpWriteBuf *buf)
{
    free(buf);
}

void http_writebuf_reset(HttpWriteBuf *buf)
{
    buf->len = 0;
}

size_t http_writebuf_write(HttpWriteBuf *buf, const char *str)
{
    size_t available = buf->capacity - buf->len;

    if (!available) {
        return 0;
    }

    size_t to_write = strlen(str);
    if (to_write > available) {
        to_write = available;
    }

    memcpy(buf->data + buf->len, str, to_write);
    buf->len += to_write;

    return to_write;
}

size_t http_writebuf_remaining(const HttpWriteBuf *buf)
{
    return buf->capacity - buf->len;
}
