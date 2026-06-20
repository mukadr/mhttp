#include <assert.h>
#include <string.h>

#include "wbuf.h"

static void test_wbuf_zero_capacity(void)
{
	struct mhttp_wbuf *wb = mhttp_wbuf_new(0);

	assert(wb == NULL);
}

static void test_wbuf_write_basic(void)
{
	struct mhttp_wbuf *wb = mhttp_wbuf_new(64);
	size_t ret;

	ret = mhttp_wbuf_write(wb, "hello");
	assert(ret == 5);
	assert(wb->len == 5);
	assert(mhttp_wbuf_remaining(wb) == 59);
	assert(memcmp(wb->data, "hello", 5) == 0);

	ret = mhttp_wbuf_write(wb, " world");
	assert(ret == 6);
	assert(wb->len == 11);
	assert(memcmp(wb->data, "hello world", 11) == 0);

	mhttp_wbuf_free(wb);
}

static void test_wbuf_write_exact_fill(void)
{
	struct mhttp_wbuf *wb = mhttp_wbuf_new(5);
	size_t ret;

	ret = mhttp_wbuf_write(wb, "hello");
	assert(ret == 5);
	assert(wb->len == 5);
	assert(mhttp_wbuf_remaining(wb) == 0);
	assert(memcmp(wb->data, "hello", 5) == 0);

	mhttp_wbuf_free(wb);
}

static void test_wbuf_write_truncates(void)
{
	struct mhttp_wbuf *wb = mhttp_wbuf_new(5);
	size_t ret;

	ret = mhttp_wbuf_write(wb, "hello world");
	assert(ret == 5);
	assert(wb->len == 5);
	assert(mhttp_wbuf_remaining(wb) == 0);
	assert(memcmp(wb->data, "hello", 5) == 0);

	mhttp_wbuf_free(wb);
}

static void test_wbuf_write_full(void)
{
	struct mhttp_wbuf *wb = mhttp_wbuf_new(5);
	size_t ret;

	ret = mhttp_wbuf_write(wb, "hello");
	assert(ret == 5);

	ret = mhttp_wbuf_write(wb, " world");
	assert(ret == 0);
	assert(wb->len == 5);

	mhttp_wbuf_free(wb);
}

static void test_wbuf_write_empty_string(void)
{
	struct mhttp_wbuf *wb = mhttp_wbuf_new(64);
	size_t ret;

	ret = mhttp_wbuf_write(wb, "");
	assert(ret == 0);
	assert(wb->len == 0);
	assert(mhttp_wbuf_remaining(wb) == 64);

	mhttp_wbuf_free(wb);
}

static void test_wbuf_write_incremental(void)
{
	struct mhttp_wbuf *wb = mhttp_wbuf_new(32);
	size_t ret;

	ret = mhttp_wbuf_write(wb, "abc");
	assert(ret == 3);

	ret = mhttp_wbuf_write(wb, "def");
	assert(ret == 3);

	ret = mhttp_wbuf_write(wb, "ghi");
	assert(ret == 3);
	assert(wb->len == 9);
	assert(memcmp(wb->data, "abcdefghi", 9) == 0);

	mhttp_wbuf_free(wb);
}

static void test_wbuf_reset(void)
{
	struct mhttp_wbuf *wb = mhttp_wbuf_new(64);
	size_t ret;

	ret = mhttp_wbuf_write(wb, "hello world");
	assert(ret == 11);
	assert(wb->len == 11);

	mhttp_wbuf_reset(wb);
	assert(wb->len == 0);
	assert(mhttp_wbuf_remaining(wb) == 64);

	ret = mhttp_wbuf_write(wb, "new data");
	assert(ret == 8);
	assert(wb->len == 8);
	assert(memcmp(wb->data, "new data", 8) == 0);

	mhttp_wbuf_free(wb);
}

void test_wbuf(void)
{
	test_wbuf_zero_capacity();
	test_wbuf_write_basic();
	test_wbuf_write_exact_fill();
	test_wbuf_write_truncates();
	test_wbuf_write_full();
	test_wbuf_write_empty_string();
	test_wbuf_write_incremental();
	test_wbuf_reset();
}
