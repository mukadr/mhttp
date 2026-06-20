#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rbuf.h"
#include "slice.h"

static void test_zero_size_rbuf(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(0);

	assert(rb == NULL);
}

static void test_rbuf1(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(1);
	struct mhttp_slice line;
	size_t ret;

	ret = mhttp_rbuf_feed(rb, "abc");
	assert(ret == 1);
	assert(rb->buf[0] == 'a');
	assert(rb->pos == rb->buf);
	assert(rb->end == rb->buf + 1);

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_empty(&line));

	ret = mhttp_rbuf_feed(rb, "\n");
	assert(ret == 0);
	assert(rb->buf[0] == 'a');
	assert(rb->pos == rb->buf);
	assert(rb->end == rb->buf + 1);

	mhttp_rbuf_reset(rb);

	ret = mhttp_rbuf_feed(rb, "\n");
	assert(ret == 1);
	assert(rb->buf[0] == '\n');
	assert(rb->pos == rb->buf);
	assert(rb->end == rb->buf + 1);

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_eq(&line, "\n"));

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_empty(&line));

	mhttp_rbuf_free(rb);
}

static void test_rbuf2(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(2);
	struct mhttp_slice line;
	size_t ret;

	ret = mhttp_rbuf_feed(rb, "abc");
	assert(ret == 2);
	assert(rb->buf[0] == 'a');
	assert(rb->buf[1] == 'b');
	assert(rb->pos == rb->buf);
	assert(rb->end == rb->buf + 2);

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_empty(&line));

	ret = mhttp_rbuf_feed(rb, "a\n");
	assert(ret == 0);

	mhttp_rbuf_reset(rb);

	ret = mhttp_rbuf_feed(rb, "a\n");
	assert(ret == 2);
	assert(rb->buf[0] == 'a');
	assert(rb->buf[1] == '\n');
	assert(rb->pos == rb->buf);
	assert(rb->end == rb->buf + 2);

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_eq(&line, "a\n"));

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_empty(&line));

	ret = mhttp_rbuf_feed(rb, "\n\n");
	assert(ret == 2);
	assert(rb->buf[0] == '\n');
	assert(rb->buf[1] == '\n');
	assert(rb->pos == rb->buf);
	assert(rb->end == rb->buf + 2);

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_eq(&line, "\n"));

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_eq(&line, "\n"));

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_empty(&line));

	mhttp_rbuf_free(rb);
}

static void test_rbuf3(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(128);
	struct mhttp_slice line;
	size_t ret;

	ret = mhttp_rbuf_feed(
		rb,
		"GET /index.html HTTP/1.0\r\n"
		"Host: www.example.com\r\n"
		"User-Agent: TestAgent/1.0\r\n"
		"\r\n");

	assert(ret == 78);
	assert(rb->pos == rb->buf);
	assert(rb->end == rb->buf + 78);

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_eq(&line, "GET /index.html HTTP/1.0\r\n"));

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_eq(&line, "Host: www.example.com\r\n"));

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_eq(&line, "User-Agent: TestAgent/1.0\r\n"));

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_eq(&line, "\r\n"));

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_empty(&line));

	mhttp_rbuf_free(rb);
}

static void test_rbuf4(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(128);
	struct mhttp_slice line;
	size_t ret;

	ret = mhttp_rbuf_feed(
		rb,
		"GET /index.html HTTP/1.0\r\n"
		"Host: www");

	assert(ret == 35);
	assert(rb->pos == rb->buf);
	assert(rb->end == rb->buf + 35);

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_eq(&line, "GET /index.html HTTP/1.0\r\n"));

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_empty(&line));

	ret = mhttp_rbuf_feed(
		rb,
		".example.com\r\n"
		"User-Agent: TestAgent/1.0\r\n"
		"\r\n");

	assert(ret == 43);
	assert(rb->pos == rb->buf);
	assert(rb->end == rb->buf + 52);

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_eq(&line, "Host: www.example.com\r\n"));

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_eq(&line, "User-Agent: TestAgent/1.0\r\n"));

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_eq(&line, "\r\n"));

	line = mhttp_rbuf_next_line(rb);
	assert(mhttp_slice_empty(&line));

	mhttp_rbuf_free(rb);
}

void test_rbuf(void)
{
	test_zero_size_rbuf();
	test_rbuf1();
	test_rbuf2();
	test_rbuf3();
	test_rbuf4();
}
