#ifndef MHTTP_WRITEBUF_H
#define MHTTP_WRITEBUF_H

#include <stdlib.h>

typedef struct HttpWriteBuf {
    size_t capacity;
    size_t len;
    char data[];
} HttpWriteBuf;

HttpWriteBuf *http_writebuf_new(size_t capacity);

void http_writebuf_free(HttpWriteBuf *buf);

void http_writebuf_reset(HttpWriteBuf *buf);

size_t http_writebuf_write(HttpWriteBuf *buf, const char *str);

size_t http_writebuf_remaining(const HttpWriteBuf *buf);

#endif // MHTTP_WRITEBUF_H
