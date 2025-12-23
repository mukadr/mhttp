#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "buffer.h"

HttpBuffer *http_buffer_new(size_t size)
{
    HttpBuffer *buffer;

    assert(size > 0);

    buffer = malloc(sizeof(*buffer) + size);

    if (buffer) {
        buffer->size = size;
        buffer->pos = buffer->buf;
        buffer->end = buffer->buf;
    }

    return buffer;
}

void http_buffer_free(HttpBuffer *buffer)
{
    free(buffer);
}

size_t http_buffer_fill(HttpBuffer *buffer, const char *s)
{
    size_t remaining = buffer->end - buffer->pos;
    size_t available = buffer->size - remaining;

    if (available > 0) {
        memmove(buffer->buf, buffer->pos, remaining);
    } else {
        // If there's no space left, discard the existing data, since the line is greater than buffer size
        // In this situation we should return BAD REQUEST
        available = buffer->size;
        remaining = 0;
    }

    size_t len = strlen(s);
    if (len > available) {
        len = available;
    }

    memcpy(buffer->buf + remaining, s, len);

    buffer->pos = buffer->buf;
    buffer->end = buffer->buf + remaining + len;

    return len;
}

HttpSlice http_buffer_next_line(HttpBuffer *buffer)
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
