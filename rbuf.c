#include <string.h>
#include <stdlib.h>

#include "rbuf.h"

struct mhttp_rbuf *mhttp_rbuf_new(size_t size)
{
	struct mhttp_rbuf *rb;

	if (size == 0)
		return NULL;

	rb = calloc(1, sizeof(*rb) + size);
	if (!rb)
		return NULL;

	rb->size = size;
	rb->pos = rb->buf;
	rb->end = rb->buf;
	return rb;
}

void mhttp_rbuf_free(struct mhttp_rbuf *rb)
{
	free(rb);
}

void mhttp_rbuf_reset(struct mhttp_rbuf *rb)
{
	rb->pos = rb->buf;
	rb->end = rb->buf;
}

size_t mhttp_rbuf_feed(struct mhttp_rbuf *rb, const char *str)
{
	size_t remaining = rb->end - rb->pos;
	size_t available = rb->size - remaining;
	size_t to_read;

	if (!available)
		return 0;

	memmove(rb->buf, rb->pos, remaining);

	to_read = strlen(str);
	if (to_read > available)
		to_read = available;

	memcpy(rb->buf + remaining, str, to_read);
	rb->pos = rb->buf;
	rb->end = rb->buf + remaining + to_read;
	return to_read;
}

struct mhttp_slice mhttp_rbuf_next_line(struct mhttp_rbuf *rb)
{
	struct mhttp_slice line = { NULL, NULL };
	char *ptr;

	ptr = memchr(rb->pos, '\n', rb->end - rb->pos);
	if (ptr) {
		line.begin = rb->pos;
		line.end = ptr + 1;
		rb->pos = line.end;
	}

	return line;
}
