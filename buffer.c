#include <string.h>
#include <stdlib.h>

#include "buffer.h"

HttpBuffer *http_buffer_new(size_t size)
{
    if (size == 0) {
        return NULL;
    }

    HttpBuffer *buffer = calloc(1, sizeof(*buffer) + size);

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

void http_buffer_reset(HttpBuffer *buffer)
{
    buffer->pos = buffer->buf;
    buffer->end = buffer->buf;
}

size_t http_buffer_concat(HttpBuffer *buffer, const char *str)
{
    size_t remaining = buffer->end - buffer->pos;
    size_t available = buffer->size - remaining;

    if (!available) {
        return 0;
    }

    memmove(buffer->buf, buffer->pos, remaining);

    size_t len = strlen(str);
    if (len > available) {
        len = available;
    }

    memcpy(buffer->buf + remaining, str, len);

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
