#ifndef MHTTP_READBUF_H
#define MHTTP_READBUF_H

#include <stdlib.h>

#include "slice.h"

typedef struct HttpReadBuf {
    size_t size;
    char *pos;
    char *end; // 1 after the last valid byte
    char buf[];
} HttpReadBuf;

HttpReadBuf *http_readbuf_new(size_t size);

void http_readbuf_free(HttpReadBuf *buffer);

void http_readbuf_reset(HttpReadBuf *buffer);

size_t http_readbuf_feed(HttpReadBuf *buffer, const char *str);

HttpSlice http_readbuf_next_line(HttpReadBuf *buffer);

#endif // MHTTP_READBUF_H