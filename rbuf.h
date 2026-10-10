#ifndef MHTTP_RBUF_H
#define MHTTP_RBUF_H

#include <stdlib.h>

#include "span.h"

struct mhttp_rbuf {
	size_t size;
	char *pos;
	char *end;
	char buf[];
};

struct mhttp_rbuf *mhttp_rbuf_new(size_t size);
void mhttp_rbuf_free(struct mhttp_rbuf *rb);
void mhttp_rbuf_reset(struct mhttp_rbuf *rb);
size_t mhttp_rbuf_feed(struct mhttp_rbuf *rb, const char *str);
struct mhttp_span mhttp_rbuf_next_line(struct mhttp_rbuf *rb);

#endif // MHTTP_RBUF_H
