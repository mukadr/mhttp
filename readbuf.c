#include <string.h>
#include <stdlib.h>

#include "readbuf.h"

HttpReadBuf *http_readbuf_new(size_t size)
{
    if (size == 0) {
        return NULL;
    }

    HttpReadBuf *buffer = calloc(1, sizeof(*buffer) + size);

    if (buffer) {
        buffer->size = size;
        buffer->pos = buffer->buf;
        buffer->end = buffer->buf;
    }

    return buffer;
}

void http_readbuf_free(HttpReadBuf *buffer)
{
    free(buffer);
}

void http_readbuf_reset(HttpReadBuf *buffer)
{
    buffer->pos = buffer->buf;
    buffer->end = buffer->buf;
}

size_t http_readbuf_feed(HttpReadBuf *buffer, const char *str)
{
    size_t remaining = buffer->end - buffer->pos;
    size_t available = buffer->size - remaining;

    if (!available) {
        return 0;
    }

    memmove(buffer->buf, buffer->pos, remaining);

    size_t to_read = strlen(str);
    if (to_read > available) {
        to_read = available;
    }

    memcpy(buffer->buf + remaining, str, to_read);

    buffer->pos = buffer->buf;
    buffer->end = buffer->buf + remaining + to_read;

    return to_read;
}

HttpSlice http_readbuf_next_line(HttpReadBuf *buffer)
{
    HttpSlice line = { 0 };

    char *ptr = memchr(buffer->pos, '\n', buffer->end - buffer->pos);
    if (ptr) {
        line.begin = buffer->pos;
        line.end = ptr + 1;
        buffer->pos = line.end;
    }

    return line;
}
