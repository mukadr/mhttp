#include <string.h>
#include <stdlib.h>

#include "wbuf.h"

struct mhttp_wbuf *mhttp_wbuf_new(size_t cap)
{
	struct mhttp_wbuf *wb;

	if (cap == 0)
		return NULL;

	wb = calloc(1, sizeof(*wb) + cap);
	if (!wb)
		return NULL;

	wb->cap = cap;
	return wb;
}

void mhttp_wbuf_free(struct mhttp_wbuf *wb)
{
	free(wb);
}

void mhttp_wbuf_reset(struct mhttp_wbuf *wb)
{
	wb->len = 0;
}

size_t mhttp_wbuf_write(struct mhttp_wbuf *wb, const char *str)
{
	size_t available, to_write;

	available = wb->cap - wb->len;
	if (!available)
		return 0;

	to_write = strlen(str);
	if (to_write > available)
		to_write = available;

	memcpy(wb->data + wb->len, str, to_write);
	wb->len += to_write;
	return to_write;
}

size_t mhttp_wbuf_remaining(const struct mhttp_wbuf *wb)
{
    return wb->cap - wb->len;
}
