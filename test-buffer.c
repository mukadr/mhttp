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
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 1);

    line = http_buffer_next_line(buffer);
    assert(!line.begin);
    assert(!line.end);

    ret = http_buffer_concat(buffer, "\n");
    assert(ret == 1);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 1);

    line = http_buffer_next_line(buffer);
    assert(line.begin == buffer->buf);
    assert(line.end == buffer->buf + 1);

    line = http_buffer_next_line(buffer);
    assert(!line.begin);
    assert(!line.end);

    http_buffer_free(buffer);
}

static void test_buffer2(void)
{
    HttpBuffer *buffer = http_buffer_new(2);
    HttpSlice line;
    size_t ret;

    ret = http_buffer_concat(buffer, "abc");
    assert(ret == 2);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 2);

    line = http_buffer_next_line(buffer);
    assert(!line.begin);
    assert(!line.end);

    ret = http_buffer_concat(buffer, "a\n");
    assert(ret == 2);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 2);

    line = http_buffer_next_line(buffer);
    assert(line.begin == buffer->buf);
    assert(line.end == buffer->buf + 2);

    line = http_buffer_next_line(buffer);
    assert(!line.begin);
    assert(!line.end);

    ret = http_buffer_concat(buffer, "\n\n");
    assert(ret == 2);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 2);

    line = http_buffer_next_line(buffer);
    assert(line.begin == buffer->buf);
    assert(line.end == buffer->buf + 1);

    line = http_buffer_next_line(buffer);
    assert(line.begin == buffer->buf + 1);
    assert(line.end == buffer->buf + 2);

    line = http_buffer_next_line(buffer);
    assert(!line.begin);
    assert(!line.end);

    http_buffer_free(buffer);
}

static void test_buffer3(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpSlice line;
    size_t ret;

    ret = http_buffer_concat(buffer, "GET /index.html HTTP/1.0\r\nHost: www.example.com\r\nUser-Agent: TestAgent/1.0\r\n\r\n");
    assert(ret == 78);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 78);

    line = http_buffer_next_line(buffer);
    assert(!memcmp(line.begin, "GET /index.html HTTP/1.0\r\n", slice_len(&line)));

    line = http_buffer_next_line(buffer);
    assert(!memcmp(line.begin, "Host: www.example.com\r\n", slice_len(&line)));

    line = http_buffer_next_line(buffer);
    assert(!memcmp(line.begin, "User-Agent: TestAgent/1.0\r\n", slice_len(&line)));

    line = http_buffer_next_line(buffer);
    assert(!memcmp(line.begin, "\r\n", slice_len(&line)));

    line = http_buffer_next_line(buffer);
    assert(!line.begin);
    assert(!line.end);

    http_buffer_free(buffer);
}

static void test_buffer4(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpSlice line;
    size_t ret;

    ret = http_buffer_concat(buffer, "GET /index.html HTTP/1.0\r\nHost: www");
    assert(ret == 35);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 35);

    line = http_buffer_next_line(buffer);
    assert(!memcmp(line.begin, "GET /index.html HTTP/1.0\r\n", slice_len(&line)));

    line = http_buffer_next_line(buffer);
    assert(!line.begin);
    assert(!line.end);

    ret = http_buffer_concat(buffer, ".example.com\r\nUser-Agent: TestAgent/1.0\r\n\r\n");
    assert(ret == 43);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 52);

    line = http_buffer_next_line(buffer);
    assert(!memcmp(line.begin, "Host: www.example.com\r\n", slice_len(&line)));

    line = http_buffer_next_line(buffer);
    assert(!memcmp(line.begin, "User-Agent: TestAgent/1.0\r\n", slice_len(&line)));

    line = http_buffer_next_line(buffer);
    assert(!memcmp(line.begin, "\r\n", slice_len(&line)));

    line = http_buffer_next_line(buffer);
    assert(!line.begin);
    assert(!line.end);

    http_buffer_free(buffer);
}

void test_buffer(void)
{
    test_buffer1();
    test_buffer2();
    test_buffer3();
    test_buffer4();
}