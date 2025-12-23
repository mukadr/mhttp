#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct HttpBuffer {
    size_t size;
    char *pos;
    char *end; // 1 after the last valid byte
    char buf[];
} HttpBuffer;

typedef struct HttpSlice {
    char *begin;
    char *end; // 1 after the last valid byte
} HttpSlice;

HttpBuffer *http_buffer_new(size_t size)
{
    assert(size > 0);

    HttpBuffer *buffer = malloc(sizeof(HttpBuffer) + size);
    if (!buffer) {
        return NULL;
    }

    buffer->size = size;
    buffer->pos = buffer->buf;
    buffer->end = buffer->buf;

    return buffer;
}

size_t http_buffer_fill(HttpBuffer *buffer, const char *s)
{
    size_t len = strlen(s);
    if (len > buffer->size) {
        len = buffer->size;
    }

    memcpy(buffer->buf, s, len);

    buffer->pos = buffer->buf;
    buffer->end = buffer->buf + len;

    return len;
}

void http_buffer_free(HttpBuffer *buffer)
{
    free(buffer);
}

HttpSlice http_buffer_next_line(HttpBuffer *buffer)
{
    HttpSlice line = { NULL, NULL };

    char *ptr = memchr(buffer->pos, '\n', buffer->end - buffer->pos);
    if (ptr) {
        line.begin = buffer->pos;
        line.end = ptr + 1;
        buffer->pos = line.end;
    }

    return line;
}

void test_buffer1(void)
{
    HttpBuffer *buffer = http_buffer_new(1);
    HttpSlice line;
    size_t ret;

    ret = http_buffer_fill(buffer, "abc");
    assert(ret == 1);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 1);

    line = http_buffer_next_line(buffer);
    assert(line.begin == NULL);
    assert(line.end == NULL);

    ret = http_buffer_fill(buffer, "\n");
    assert(ret == 1);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 1);

    line = http_buffer_next_line(buffer);
    assert(line.begin == buffer->buf);
    assert(line.end == buffer->buf + 1);

    line = http_buffer_next_line(buffer);
    assert(line.begin == NULL);
    assert(line.end == NULL);

    http_buffer_free(buffer);
}

void test_buffer2(void)
{
    HttpBuffer *buffer = http_buffer_new(2);
    HttpSlice line;
    size_t ret;

    ret = http_buffer_fill(buffer, "abc");
    assert(ret == 2);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 2);

    line = http_buffer_next_line(buffer);
    assert(line.begin == NULL);
    assert(line.end == NULL);

    ret = http_buffer_fill(buffer, "a\n");
    assert(ret == 2);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 2);

    line = http_buffer_next_line(buffer);
    assert(line.begin == buffer->buf);
    assert(line.end == buffer->buf + 2);

    line = http_buffer_next_line(buffer);
    assert(line.begin == NULL);
    assert(line.end == NULL);

    ret = http_buffer_fill(buffer, "\n\n");
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
    assert(line.begin == NULL);
    assert(line.end == NULL);

    http_buffer_free(buffer);
}

void test_buffer3(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpSlice line;
    size_t ret;

    ret = http_buffer_fill(buffer, "GET /index.html HTTP/1.0\r\nHost: www.example.com\r\nUser-Agent: TestAgent/1.0\r\n\r\n");
    assert(ret == 78);
    assert(buffer->pos == buffer->buf);
    assert(buffer->end == buffer->buf + 78);

    line = http_buffer_next_line(buffer);
    assert(!memcmp(line.begin, "GET /index.html HTTP/1.0\r\n", line.end - line.begin));

    line = http_buffer_next_line(buffer);
    assert(!memcmp(line.begin, "Host: www.example.com\r\n", line.end - line.begin));

    line = http_buffer_next_line(buffer);
    assert(!memcmp(line.begin, "User-Agent: TestAgent/1.0\r\n", line.end - line.begin));

    line = http_buffer_next_line(buffer);
    assert(!memcmp(line.begin, "\r\n", line.end - line.begin));

    line = http_buffer_next_line(buffer);
    assert(line.begin == NULL);
    assert(line.end == NULL);

    http_buffer_free(buffer);
}

int main()
{
    test_buffer1();
    test_buffer2();
    test_buffer3();

    return 0;
}