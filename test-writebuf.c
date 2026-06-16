#include <assert.h>
#include <string.h>

#include "writebuf.h"

static void test_writebuf_zero_capacity(void)
{
    HttpWriteBuf *buf = http_writebuf_new(0);

    assert(buf == NULL);
}

static void test_writebuf_write_basic(void)
{
    HttpWriteBuf *buf = http_writebuf_new(64);
    size_t ret;

    ret = http_writebuf_write(buf, "hello");
    assert(ret == 5);
    assert(buf->len == 5);
    assert(http_writebuf_remaining(buf) == 59);
    assert(memcmp(buf->data, "hello", 5) == 0);

    ret = http_writebuf_write(buf, " world");
    assert(ret == 6);
    assert(buf->len == 11);
    assert(memcmp(buf->data, "hello world", 11) == 0);

    http_writebuf_free(buf);
}

static void test_writebuf_write_exact_fill(void)
{
    HttpWriteBuf *buf = http_writebuf_new(5);
    size_t ret;

    ret = http_writebuf_write(buf, "hello");
    assert(ret == 5);
    assert(buf->len == 5);
    assert(http_writebuf_remaining(buf) == 0);
    assert(memcmp(buf->data, "hello", 5) == 0);

    http_writebuf_free(buf);
}

static void test_writebuf_write_truncates(void)
{
    HttpWriteBuf *buf = http_writebuf_new(5);
    size_t ret;

    ret = http_writebuf_write(buf, "hello world");
    assert(ret == 5);
    assert(buf->len == 5);
    assert(http_writebuf_remaining(buf) == 0);
    assert(memcmp(buf->data, "hello", 5) == 0);

    http_writebuf_free(buf);
}

static void test_writebuf_write_full(void)
{
    HttpWriteBuf *buf = http_writebuf_new(5);
    size_t ret;

    ret = http_writebuf_write(buf, "hello");
    assert(ret == 5);

    ret = http_writebuf_write(buf, " world");
    assert(ret == 0);
    assert(buf->len == 5);

    http_writebuf_free(buf);
}

static void test_writebuf_write_empty_string(void)
{
    HttpWriteBuf *buf = http_writebuf_new(64);
    size_t ret;

    ret = http_writebuf_write(buf, "");
    assert(ret == 0);
    assert(buf->len == 0);
    assert(http_writebuf_remaining(buf) == 64);

    http_writebuf_free(buf);
}

static void test_writebuf_write_incremental(void)
{
    HttpWriteBuf *buf = http_writebuf_new(32);
    size_t ret;

    ret = http_writebuf_write(buf, "abc");
    assert(ret == 3);

    ret = http_writebuf_write(buf, "def");
    assert(ret == 3);

    ret = http_writebuf_write(buf, "ghi");
    assert(ret == 3);
    assert(buf->len == 9);
    assert(memcmp(buf->data, "abcdefghi", 9) == 0);

    http_writebuf_free(buf);
}

static void test_writebuf_reset(void)
{
    HttpWriteBuf *buf = http_writebuf_new(64);
    size_t ret;

    ret = http_writebuf_write(buf, "hello world");
    assert(ret == 11);
    assert(buf->len == 11);

    http_writebuf_reset(buf);
    assert(buf->len == 0);
    assert(http_writebuf_remaining(buf) == 64);

    ret = http_writebuf_write(buf, "new data");
    assert(ret == 8);
    assert(buf->len == 8);
    assert(memcmp(buf->data, "new data", 8) == 0);

    http_writebuf_free(buf);
}

void test_writebuf(void)
{
    test_writebuf_zero_capacity();
    test_writebuf_write_basic();
    test_writebuf_write_exact_fill();
    test_writebuf_write_truncates();
    test_writebuf_write_full();
    test_writebuf_write_empty_string();
    test_writebuf_write_incremental();
    test_writebuf_reset();
}
