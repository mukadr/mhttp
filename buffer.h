#ifndef MHTTP_BUFFER_H
#define MHTTP_BUFFER_H

#include <stdlib.h>

typedef struct HttpBuffer {
    size_t size;
    char *pos;
    char *end; // 1 after the last valid byte
    char buf[];
} HttpBuffer;

typedef struct HttpSlice {
    char *begin;
    char *end; // 1 after the last valid byte
} HttpSlice;

HttpBuffer *http_buffer_new(size_t size);

void http_buffer_free(HttpBuffer *buffer);

size_t http_buffer_fill(HttpBuffer *buffer, const char *s);

HttpSlice http_buffer_next_line(HttpBuffer *buffer);

#endif // MHTTP_BUFFER_H