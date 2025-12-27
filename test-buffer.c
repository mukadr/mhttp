#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "slice.h"

static void test_buffer1(void)
{
    HttpBuffer *buffer = http_buffer_new(1);
    HttpSlice line;
    size_t ret;

    ret = http_buffer_concat(buffer, "abc");
    assert(ret == 1);
    assert(buffer->buf[0] == 'a');
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 1);

    line = http_buffer_next_line(buffer);
    assert(slice_empty(&line));

    ret = http_buffer_concat(buffer, "\n");
    assert(ret == -1);
    assert(buffer->buf[0] == 'a');
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 1);

    http_buffer_reset(buffer);

    ret = http_buffer_concat(buffer, "\n");
    assert(ret == 1);
    assert(buffer->buf[0] == '\n');
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 1);

    line = http_buffer_next_line(buffer);
    assert(slice_eq(&line, "\n"));

    line = http_buffer_next_line(buffer);
    assert(slice_empty(&line));

    http_buffer_free(buffer);
}

static void test_buffer2(void)
{
    HttpBuffer *buffer = http_buffer_new(2);
    HttpSlice line;
    size_t ret;

    ret = http_buffer_concat(buffer, "abc");
    assert(ret == 2);
    assert(buffer->buf[0] == 'a');
    assert(buffer->buf[1] == 'b');
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 2);

    line = http_buffer_next_line(buffer);
    assert(slice_empty(&line));

    ret = http_buffer_concat(buffer, "a\n");
    assert(ret == -1);

    http_buffer_reset(buffer);

    ret = http_buffer_concat(buffer, "a\n");
    assert(ret == 2);
    assert(buffer->buf[0] == 'a');
    assert(buffer->buf[1] == '\n');
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 2);

    line = http_buffer_next_line(buffer);
    assert(slice_eq(&line, "a\n"));

    line = http_buffer_next_line(buffer);
    assert(slice_empty(&line));

    ret = http_buffer_concat(buffer, "\n\n");
    assert(ret == 2);
    assert(buffer->buf[0] == '\n');
    assert(buffer->buf[1] == '\n');
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 2);

    line = http_buffer_next_line(buffer);
    assert(slice_eq(&line, "\n"));

    line = http_buffer_next_line(buffer);
    assert(slice_eq(&line, "\n"));

    line = http_buffer_next_line(buffer);
    assert(slice_empty(&line));

    http_buffer_free(buffer);
}

static void test_buffer3(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpSlice line;
    size_t ret;

    ret = http_buffer_concat(
        buffer,
        "GET /index.html HTTP/1.0\r\n"
        "Host: www.example.com\r\n"
        "User-Agent: TestAgent/1.0\r\n"
        "\r\n"
    );

    assert(ret == 78);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 78);

    line = http_buffer_next_line(buffer);
    assert(slice_eq(&line, "GET /index.html HTTP/1.0\r\n"));

    line = http_buffer_next_line(buffer);
    assert(slice_eq(&line, "Host: www.example.com\r\n"));

    line = http_buffer_next_line(buffer);
    assert(slice_eq(&line, "User-Agent: TestAgent/1.0\r\n"));

    line = http_buffer_next_line(buffer);
    assert(slice_eq(&line, "\r\n"));

    line = http_buffer_next_line(buffer);
    assert(slice_empty(&line));

    http_buffer_free(buffer);
}

static void test_buffer4(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpSlice line;
    size_t ret;

    ret = http_buffer_concat(
        buffer,
        "GET /index.html HTTP/1.0\r\n"
        "Host: www"
    );

    assert(ret == 35);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 35);

    line = http_buffer_next_line(buffer);
    assert(slice_eq(&line, "GET /index.html HTTP/1.0\r\n"));

    line = http_buffer_next_line(buffer);
    assert(slice_empty(&line));

    ret = http_buffer_concat(
        buffer,
        ".example.com\r\n"
        "User-Agent: TestAgent/1.0\r\n"
        "\r\n"
    );

    assert(ret == 43);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 52);

    line = http_buffer_next_line(buffer);
    assert(slice_eq(&line, "Host: www.example.com\r\n"));

    line = http_buffer_next_line(buffer);
    assert(slice_eq(&line, "User-Agent: TestAgent/1.0\r\n"));

    line = http_buffer_next_line(buffer);
    assert(slice_eq(&line, "\r\n"));

    line = http_buffer_next_line(buffer);
    assert(slice_empty(&line));

    http_buffer_free(buffer);
}

void test_buffer(void)
{
    test_buffer1();
    test_buffer2();
    test_buffer3();
    test_buffer4();
}