#ifndef MHTTP_WBUF_H
#define MHTTP_WBUF_H

#include <stdlib.h>

struct mhttp_wbuf {
	size_t cap;
	size_t len;
	char data[];
};

struct mhttp_wbuf *mhttp_wbuf_new(size_t cap);

void mhttp_wbuf_free(struct mhttp_wbuf *wb);

void mhttp_wbuf_reset(struct mhttp_wbuf *wb);

size_t mhttp_wbuf_write(struct mhttp_wbuf *wb, const char *str);

size_t mhttp_wbuf_remaining(const struct mhttp_wbuf *wb);

#endif // MHTTP_WBUF_H
