#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "readbuf.h"
#include "slice.h"

static void test_zero_size_buffer(void)
{
    HttpReadBuf *buffer = http_readbuf_new(0);

    assert(buffer == NULL);
}

static void test_readbuf1(void)
{
    HttpReadBuf *buffer = http_readbuf_new(1);
    HttpSlice line;
    size_t ret;

    ret = http_readbuf_feed(buffer, "abc");
    assert(ret == 1);
    assert(buffer->buf[0] == 'a');
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 1);

    line = http_readbuf_next_line(buffer);
    assert(slice_empty(&line));

    ret = http_readbuf_feed(buffer, "\n");
    assert(ret == 0);
    assert(buffer->buf[0] == 'a');
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 1);

    http_readbuf_reset(buffer);

    ret = http_readbuf_feed(buffer, "\n");
    assert(ret == 1);
    assert(buffer->buf[0] == '\n');
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 1);

    line = http_readbuf_next_line(buffer);
    assert(slice_eq(&line, "\n"));

    line = http_readbuf_next_line(buffer);
    assert(slice_empty(&line));

    http_readbuf_free(buffer);
}

static void test_readbuf2(void)
{
    HttpReadBuf *buffer = http_readbuf_new(2);
    HttpSlice line;
    size_t ret;

    ret = http_readbuf_feed(buffer, "abc");
    assert(ret == 2);
    assert(buffer->buf[0] == 'a');
    assert(buffer->buf[1] == 'b');
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 2);

    line = http_readbuf_next_line(buffer);
    assert(slice_empty(&line));

    ret = http_readbuf_feed(buffer, "a\n");
    assert(ret == 0);

    http_readbuf_reset(buffer);

    ret = http_readbuf_feed(buffer, "a\n");
    assert(ret == 2);
    assert(buffer->buf[0] == 'a');
    assert(buffer->buf[1] == '\n');
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 2);

    line = http_readbuf_next_line(buffer);
    assert(slice_eq(&line, "a\n"));

    line = http_readbuf_next_line(buffer);
    assert(slice_empty(&line));

    ret = http_readbuf_feed(buffer, "\n\n");
    assert(ret == 2);
    assert(buffer->buf[0] == '\n');
    assert(buffer->buf[1] == '\n');
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 2);

    line = http_readbuf_next_line(buffer);
    assert(slice_eq(&line, "\n"));

    line = http_readbuf_next_line(buffer);
    assert(slice_eq(&line, "\n"));

    line = http_readbuf_next_line(buffer);
    assert(slice_empty(&line));

    http_readbuf_free(buffer);
}

static void test_readbuf3(void)
{
    HttpReadBuf *buffer = http_readbuf_new(128);
    HttpSlice line;
    size_t ret;

    ret = http_readbuf_feed(
        buffer,
        "GET /index.html HTTP/1.0\r\n"
        "Host: www.example.com\r\n"
        "User-Agent: TestAgent/1.0\r\n"
        "\r\n"
    );

    assert(ret == 78);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 78);

    line = http_readbuf_next_line(buffer);
    assert(slice_eq(&line, "GET /index.html HTTP/1.0\r\n"));

    line = http_readbuf_next_line(buffer);
    assert(slice_eq(&line, "Host: www.example.com\r\n"));

    line = http_readbuf_next_line(buffer);
    assert(slice_eq(&line, "User-Agent: TestAgent/1.0\r\n"));

    line = http_readbuf_next_line(buffer);
    assert(slice_eq(&line, "\r\n"));

    line = http_readbuf_next_line(buffer);
    assert(slice_empty(&line));

    http_readbuf_free(buffer);
}

static void test_readbuf4(void)
{
    HttpReadBuf *buffer = http_readbuf_new(128);
    HttpSlice line;
    size_t ret;

    ret = http_readbuf_feed(
        buffer,
        "GET /index.html HTTP/1.0\r\n"
        "Host: www"
    );

    assert(ret == 35);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 35);

    line = http_readbuf_next_line(buffer);
    assert(slice_eq(&line, "GET /index.html HTTP/1.0\r\n"));

    line = http_readbuf_next_line(buffer);
    assert(slice_empty(&line));

    ret = http_readbuf_feed(
        buffer,
        ".example.com\r\n"
        "User-Agent: TestAgent/1.0\r\n"
        "\r\n"
    );

    assert(ret == 43);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 52);

    line = http_readbuf_next_line(buffer);
    assert(slice_eq(&line, "Host: www.example.com\r\n"));

    line = http_readbuf_next_line(buffer);
    assert(slice_eq(&line, "User-Agent: TestAgent/1.0\r\n"));

    line = http_readbuf_next_line(buffer);
    assert(slice_eq(&line, "\r\n"));

    line = http_readbuf_next_line(buffer);
    assert(slice_empty(&line));

    http_readbuf_free(buffer);
}

void test_readbuf(void)
{
    test_zero_size_buffer();
    test_readbuf1();
    test_readbuf2();
    test_readbuf3();
    test_readbuf4();
}
