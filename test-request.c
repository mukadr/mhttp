#include <assert.h>
#include <string.h>

#include "readbuf.h"
#include "request.h"

#include "test-request.h"

// Bypass null checking from http_readbuf_feed
static void buffer_write_raw(HttpReadBuf *buffer, const void *data, size_t len)
{
    size_t remaining = buffer->end - buffer->pos;
    memmove(buffer->buf, buffer->pos, remaining);
    memcpy(buffer->buf + remaining, data, len);
    buffer->pos = buffer->buf;
    buffer->end = buffer->buf + remaining + len;
}

static void test_request_with_malformed_content(void)
{
    HttpReadBuf *buffer = http_readbuf_new(512);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(buffer, "");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_readbuf_feed(buffer, "X");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);
    http_readbuf_reset(buffer);

    http_readbuf_feed(buffer, "BLA\n");
    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_readbuf_feed(buffer, "GET\n");
    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_readbuf_feed(buffer, "GET \n");
    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_readbuf_feed(buffer, "GET / HTTP\n");
    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_readbuf_feed(buffer, "GET / HTTP2.\n");
    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_readbuf_feed(buffer,
        "GET "
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "\r\n");
    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_with_bad_line_ending(void)
{
    HttpReadBuf *buffer = http_readbuf_new(128);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "GET /index.html HTTP/1.0\n"
    );
    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_readbuf_feed(
        buffer,
        "GET /index.html HTTP/1.0\r"
    );
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);
    http_readbuf_reset(buffer);

    http_readbuf_feed(
        buffer,
        "GET /index.html HTTP/1.0 \r\n"
    );
    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_readbuf_feed(
        buffer,
        "GET /index.html HTTP/1.0\r \n"
    );
    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_readbuf_feed(
        buffer,
        "GET /index.html HTTP/1.0\r\n"
        "\n"
    );
    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_readbuf_feed(
        buffer,
        "GET /index.html HTTP/1.0\r\n"
        "Host: www.example.com\r \n"
        "\r\n"
    );
    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_with_null_byte_in_uri(void)
{
    HttpReadBuf *buffer = http_readbuf_new(64);
    HttpRequest *request = http_request_new();

    static const char req[] = "GET /pa\x00th HTTP/1.1\r\n\r\n";
    buffer_write_raw(buffer, req, sizeof(req) - 1);

    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_with_null_byte_in_header_name(void)
{
    HttpReadBuf *buffer = http_readbuf_new(64);
    HttpRequest *request = http_request_new();

    static const char req[] = "GET / HTTP/1.1\r\nNa\x00me: value\r\n\r\n";
    buffer_write_raw(buffer, req, sizeof(req) - 1);

    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_with_null_byte_in_header_value(void)
{
    HttpReadBuf *buffer = http_readbuf_new(64);
    HttpRequest *request = http_request_new();

    static const char req[] = "GET / HTTP/1.1\r\nName: va\x00lue\r\n\r\n";
    buffer_write_raw(buffer, req, sizeof(req) - 1);

    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_get_request(void)
{
    HttpReadBuf *buffer = http_readbuf_new(128);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "GET /\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(request->method == HTTP_METHOD_GET);
    assert(!strcmp(request->uri, "/"));
    assert(request->version == 9);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_post_request(void)
{
    HttpReadBuf *buffer = http_readbuf_new(128);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(request->method == HTTP_METHOD_POST);
    assert(!strcmp(request->uri, "/"));
    assert(request->version == 9);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_head_request(void)
{
    HttpReadBuf *buffer = http_readbuf_new(128);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "HEAD /\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(request->method == HTTP_METHOD_HEAD);
    assert(!strcmp(request->uri, "/"));
    assert(request->version == 9);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_with_http_version_1_0(void)
{
    HttpReadBuf *buffer = http_readbuf_new(128);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "GET /index.html HTTP/1.0\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(request->method == HTTP_METHOD_GET);
    assert(!strcmp(request->uri, "/index.html"));
    assert(request->version == 10);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_with_http_version_1_1(void)
{
    HttpReadBuf *buffer = http_readbuf_new(128);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /static/chat.png HTTP/1.1\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(!strcmp(request->uri, "/static/chat.png"));
    assert(request->version == 11);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_with_headers(void)
{
    HttpReadBuf *buffer = http_readbuf_new(256);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST / HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "Content-Type: application/json\r\n"
        "User-Agent: TestAgent/1.0\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(!strcmp(request->uri, "/"));
    assert(request->version == 11);
    assert(http_request_header_count(request) == 3);

    const char *host = http_request_get_header(request, "Host");
    assert(host);
    assert(!strcmp(host, "www.example.com"));

    const char *ct = http_request_get_header(request, "Content-Type");
    assert(ct);
    assert(!strcmp(ct, "application/json"));

    const char *ua = http_request_get_header(request, "User-Agent");
    assert(ua);
    assert(!strcmp(ua, "TestAgent/1.0"));

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_with_headers_needing_more_input(void)
{
    HttpReadBuf *buffer = http_readbuf_new(128);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(buffer, "HEAD /tralala");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_readbuf_feed(buffer, ".html HTT");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_readbuf_feed(buffer, "P/1.0\r\n");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_readbuf_feed(buffer, "Host");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_readbuf_feed(buffer, ":");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_readbuf_feed(buffer, " www.example");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_readbuf_feed(buffer, ".com\r\nApi-Key: 123456\r\nUse");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_readbuf_feed(buffer, "r-Agen");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_readbuf_feed(buffer, "t: TestAgent");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_readbuf_feed(buffer, "/1.0\r\n");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_readbuf_feed(buffer, "\r\n");
    assert(http_request_parse(request, buffer) == HTTP_OK);

    // Request
    assert(request->method == HTTP_METHOD_HEAD);
    assert(!strcmp(request->uri, "/tralala.html"));
    assert(request->version == 10);

    // Headers
    assert(http_request_header_count(request) == 3);

    const char *host = http_request_get_header(request, "Host");
    assert(host);
    assert(!strcmp(host, "www.example.com"));

    const char *api_key = http_request_get_header(request, "Api-Key");
    assert(api_key);
    assert(!strcmp(api_key, "123456"));

    const char *user_agent = http_request_get_header(request, "User-Agent");
    assert(user_agent);
    assert(!strcmp(user_agent, "TestAgent/1.0"));

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_with_missing_header_name(void)
{
    HttpReadBuf *buffer = http_readbuf_new(128);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(buffer, "HEAD /hello\r\n");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_readbuf_feed(buffer, ": world\r");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_readbuf_feed(buffer, "\n");
    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_with_header_count_below_limit_succeeds(void)
{
    HttpReadBuf *buffer = http_readbuf_new(1024);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(buffer, "GET / HTTP/1.1\r\n");
    for (int i = 0; i < 100; i++) {
        http_readbuf_feed(buffer, "H: v\r\n");
    }
    http_readbuf_feed(buffer, "\r\n");

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_header_count(request) == 100);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_with_header_count_above_limit_fails(void)
{
    HttpReadBuf *buffer = http_readbuf_new(1024);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(buffer, "GET / HTTP/1.1\r\n");
    for (int i = 0; i < 50; i++) {
        http_readbuf_feed(buffer, "H: v\r\n");
    }

    // Make sure the count is tracked across multiple calls, not just within a single call
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);
    assert(http_request_header_count(request) == 50);

    for (int i = 0; i < 51; i++) {
        http_readbuf_feed(buffer, "H: v\r\n");
    }
    http_readbuf_feed(buffer, "\r\n");

    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_with_header_name_below_length_limit_succeeds(void)
{
    HttpReadBuf *buffer = http_readbuf_new(512);
    HttpRequest *request = http_request_new();

    char header[270];
    memset(header, 'A', 255);
    strcpy(header + 255, ": v\r\n\r\n");

    http_readbuf_feed(buffer, "GET / HTTP/1.1\r\n");
    http_readbuf_feed(buffer, header);

    assert(http_request_parse(request, buffer) == HTTP_OK);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_with_header_name_above_length_limit_fails(void)
{
    HttpReadBuf *buffer = http_readbuf_new(512);
    HttpRequest *request = http_request_new();

    char header[270];
    memset(header, 'A', 256);
    strcpy(header + 256, ": v\r\n\r\n");

    http_readbuf_feed(buffer, "GET / HTTP/1.1\r\n");
    http_readbuf_feed(buffer, header);

    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_with_header_value_below_length_limit_succeeds(void)
{
    HttpReadBuf *buffer = http_readbuf_new(512);
    HttpRequest *request = http_request_new();

    char header[270];
    strcpy(header, "N: ");
    memset(header + 3, 'B', 255);
    strcpy(header + 3 + 255, "\r\n\r\n");

    http_readbuf_feed(buffer, "GET / HTTP/1.1\r\n");
    http_readbuf_feed(buffer, header);

    assert(http_request_parse(request, buffer) == HTTP_OK);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_with_header_value_above_length_limit_fails(void)
{
    HttpReadBuf *buffer = http_readbuf_new(512);
    HttpRequest *request = http_request_new();

    char header[270];
    strcpy(header, "N: ");
    memset(header + 3, 'B', 256);
    strcpy(header + 3 + 256, "\r\n\r\n");

    http_readbuf_feed(buffer, "GET / HTTP/1.1\r\n");
    http_readbuf_feed(buffer, header);

    assert(http_request_parse(request, buffer) == HTTP_INVALID_REQUEST);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_put_request(void)
{
    HttpReadBuf *buffer = http_readbuf_new(128);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "PUT /items/42 HTTP/1.1\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(request->method == HTTP_METHOD_PUT);
    assert(!strcmp(request->uri, "/items/42"));
    assert(request->version == 11);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_delete_request(void)
{
    HttpReadBuf *buffer = http_readbuf_new(128);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "DELETE /items/42 HTTP/1.1\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(request->method == HTTP_METHOD_DELETE);
    assert(!strcmp(request->uri, "/items/42"));
    assert(request->version == 11);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_options_request(void)
{
    HttpReadBuf *buffer = http_readbuf_new(128);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "OPTIONS * HTTP/1.1\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(request->method == HTTP_METHOD_OPTIONS);
    assert(!strcmp(request->uri, "*"));
    assert(request->version == 11);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_patch_request(void)
{
    HttpReadBuf *buffer = http_readbuf_new(128);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "PATCH /items/42 HTTP/1.1\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(request->method == HTTP_METHOD_PATCH);
    assert(!strcmp(request->uri, "/items/42"));
    assert(request->version == 11);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_body_full_in_buffer(void)
{
    HttpReadBuf *buffer = http_readbuf_new(256);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /submit HTTP/1.1\r\n"
        "Content-Length: 7\r\n"
        "\r\n"
        "abcdefg"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_content_length(request) == 7);

    char body[16] = { 0 };
    size_t ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 7);
    assert(!memcmp(body, "abcdefg", 7));

    ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 0);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_body_incremental(void)
{
    HttpReadBuf *buffer = http_readbuf_new(256);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /upload HTTP/1.1\r\n"
        "Content-Length: 10\r\n"
        "\r\n"
        "hello"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_content_length(request) == 10);

    char body[16] = { 0 };
    size_t ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 5);
    assert(!memcmp(body, "hello", 5));

    http_readbuf_feed(buffer, " worl");

    ret = http_request_read_body(request, buffer, body + 5, sizeof(body) - 5);
    assert(ret == 5);
    assert(!memcmp(body + 5, " worl", 5));

    ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 0);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_no_content_length(void)
{
    HttpReadBuf *buffer = http_readbuf_new(256);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "GET / HTTP/1.1\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_content_length(request) == 0);

    char body[16] = { 0 };
    size_t ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 0);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_content_length_zero(void)
{
    HttpReadBuf *buffer = http_readbuf_new(256);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /empty HTTP/1.1\r\n"
        "Content-Length: 0\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_content_length(request) == 0);

    char body[16] = { 0 };
    size_t ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 0);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_body_clamped_to_content_length(void)
{
    HttpReadBuf *buffer = http_readbuf_new(256);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /data HTTP/1.1\r\n"
        "Content-Length: 3\r\n"
        "\r\n"
        "abcdef"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_content_length(request) == 3);

    char body[16] = { 0 };
    size_t ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 3);
    assert(!memcmp(body, "abc", 3));

    ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 0);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_body_buffer_smaller_than_content(void)
{
    HttpReadBuf *buffer = http_readbuf_new(64);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /data HTTP/1.1\r\n"
        "Content-Length: 50\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_content_length(request) == 50);

    char body[60] = { 0 };
    int total = 0;
    size_t ret;

    http_readbuf_feed(buffer, "AAAAAAAAAABBBBBBBBBB");
    ret = http_request_read_body(request, buffer, body + total, sizeof(body) - total);
    assert(ret == 20);
    total += ret;
    assert(!memcmp(body, "AAAAAAAAAABBBBBBBBBB", 20));

    http_readbuf_feed(buffer, "CCCCCCCCCCDDDDDDDDDD");
    ret = http_request_read_body(request, buffer, body + total, sizeof(body) - total);
    assert(ret == 20);
    total += ret;
    assert(!memcmp(body + 20, "CCCCCCCCCCDDDDDDDDDD", 20));

    http_readbuf_feed(buffer, "EEEEEEEEEE");
    ret = http_request_read_body(request, buffer, body + total, sizeof(body) - total);
    assert(ret == 10);
    total += ret;
    assert(!memcmp(body + 40, "EEEEEEEEEE", 10));

    ret = http_request_read_body(request, buffer, body + total, sizeof(body) - total);
    assert(ret == 0);
    assert(total == 50);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_body_chunked_single(void)
{
    HttpReadBuf *buffer = http_readbuf_new(256);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /data HTTP/1.1\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "7\r\n"
        "abcdefg\r\n"
        "0\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_is_chunked(request));

    char body[16] = { 0 };
    size_t ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 7);
    assert(!memcmp(body, "abcdefg", 7));

    ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 0);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_body_chunked_multiple(void)
{
    HttpReadBuf *buffer = http_readbuf_new(256);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /data HTTP/1.1\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "hello\r\n"
        "6\r\n"
        " world\r\n"
        "0\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_is_chunked(request));

    char body[32] = { 0 };
    size_t ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 11);
    assert(!memcmp(body, "hello world", 11));

    ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 0);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_body_chunked_incremental(void)
{
    HttpReadBuf *buffer = http_readbuf_new(128);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /data HTTP/1.1\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "A\r\n"
        "hello"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_is_chunked(request));

    char body[32] = { 0 };
    int total = 0;
    size_t ret;

    ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 5);
    total += ret;

    http_readbuf_feed(buffer, " wor");

    ret = http_request_read_body(request, buffer, body + total, sizeof(body) - total);
    assert(ret == 4);
    total += ret;

    http_readbuf_feed(buffer, "l");

    ret = http_request_read_body(request, buffer, body + total, sizeof(body) - total);
    assert(ret == 1);
    total += ret;

    http_readbuf_feed(buffer, "d\r\n0\r\n\r\n");

    ret = http_request_read_body(request, buffer, body + total, sizeof(body) - total);
    assert(ret == 0);
    assert(total == 10);
    assert(!memcmp(body, "hello worl", 10));

    ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 0);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_body_chunked_no_header(void)
{
    HttpReadBuf *buffer = http_readbuf_new(256);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /data HTTP/1.1\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(!http_request_is_chunked(request));

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_body_chunked_empty(void)
{
    HttpReadBuf *buffer = http_readbuf_new(256);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /data HTTP/1.1\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "0\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_is_chunked(request));

    char body[16] = { 0 };
    size_t ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 0);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_body_chunked_no_header_data_yet(void)
{
    HttpReadBuf *buffer = http_readbuf_new(256);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /data HTTP/1.1\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_is_chunked(request));

    char body[16] = { 0 };
    size_t ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 0);

    http_readbuf_feed(buffer, "5\r\nhello\r\n0\r\n\r\n");

    ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 5);
    assert(!memcmp(body, "hello", 5));

    ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 0);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_body_chunked_trailer_split(void)
{
    HttpReadBuf *buffer = http_readbuf_new(256);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /data HTTP/1.1\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "hello\r\n"
        "0\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_is_chunked(request));

    char body[16] = { 0 };
    size_t ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 5);
    assert(!memcmp(body, "hello", 5));

    http_readbuf_feed(buffer, "\r\n");

    ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 0);

    ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 0);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_body_chunked_trailer_header(void)
{
    HttpReadBuf *buffer = http_readbuf_new(256);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /data HTTP/1.1\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "4\r\n"
        "body\r\n"
        "0\r\n"
        "X-Info: ok\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_is_chunked(request));

    char body[16] = { 0 };
    size_t ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 4);
    assert(!memcmp(body, "body", 4));

    ret = http_request_read_body(request, buffer, body, sizeof(body));
    assert(ret == 0);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_body_chunked_user_buffer_small(void)
{
    HttpReadBuf *buffer = http_readbuf_new(256);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "POST /data HTTP/1.1\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "A\r\n"
        "0123456789\r\n"
        "0\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_is_chunked(request));

    char body[16] = { 0 };
    int total = 0;
    size_t ret;

    ret = http_request_read_body(request, buffer, body, 3);
    assert(ret == 3);
    assert(!memcmp(body, "012", 3));
    total += ret;

    ret = http_request_read_body(request, buffer, body + total, 3);
    assert(ret == 3);
    assert(!memcmp(body + total, "345", 3));
    total += ret;

    ret = http_request_read_body(request, buffer, body + total, 3);
    assert(ret == 3);
    assert(!memcmp(body + total, "678", 3));
    total += ret;

    ret = http_request_read_body(request, buffer, body + total, 1);
    assert(ret == 1);
    assert(!memcmp(body + total, "9", 1));
    total += ret;

    ret = http_request_read_body(request, buffer, body + total, 10);
    assert(ret == 0);
    assert(total == 10);

    http_request_free(request);
    http_readbuf_free(buffer);
}

static void test_request_case_insensitive_header_name_lookup(void)
{
    HttpReadBuf *buffer = http_readbuf_new(256);
    HttpRequest *request = http_request_new();

    http_readbuf_feed(
        buffer,
        "GET / HTTP/1.1\r\n"
        "Content-Type: text/html\r\n"
        "X-Custom-Header: secret\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);

    assert(!strcmp(http_request_get_header(request, "Content-Type"), "text/html"));
    assert(!strcmp(http_request_get_header(request, "content-type"), "text/html"));
    assert(!strcmp(http_request_get_header(request, "CONTENT-TYPE"), "text/html"));
    assert(!strcmp(http_request_get_header(request, "cOnTeNt-TyPe"), "text/html"));

    assert(!strcmp(http_request_get_header(request, "x-custom-header"), "secret"));
    assert(!strcmp(http_request_get_header(request, "X-CUSTOM-HEADER"), "secret"));

    assert(http_request_get_header(request, "Authorization") == NULL);

    http_request_free(request);
    http_readbuf_free(buffer);
}

void test_request(void)
{
    test_request_with_malformed_content();
    test_request_with_bad_line_ending();

    test_request_with_null_byte_in_uri();
    test_request_with_null_byte_in_header_name();
    test_request_with_null_byte_in_header_value();

    test_get_request();
    test_post_request();
    test_head_request();
    test_put_request();
    test_delete_request();
    test_options_request();
    test_patch_request();

    test_request_with_http_version_1_0();
    test_request_with_http_version_1_1();

    test_request_with_headers();
    test_request_with_headers_needing_more_input();
    test_request_with_missing_header_name();
    test_request_with_header_count_below_limit_succeeds();
    test_request_with_header_count_above_limit_fails();
    test_request_with_header_name_below_length_limit_succeeds();
    test_request_with_header_name_above_length_limit_fails();
    test_request_with_header_value_below_length_limit_succeeds();
    test_request_with_header_value_above_length_limit_fails();
    test_request_case_insensitive_header_name_lookup();

    test_request_body_full_in_buffer();
    test_request_body_incremental();
    test_request_no_content_length();
    test_request_content_length_zero();
    test_request_body_clamped_to_content_length();
    test_request_body_buffer_smaller_than_content();

    test_request_body_chunked_single();
    test_request_body_chunked_multiple();
    test_request_body_chunked_incremental();
    test_request_body_chunked_no_header();
    test_request_body_chunked_empty();

    test_request_body_chunked_no_header_data_yet();
    test_request_body_chunked_trailer_split();
    test_request_body_chunked_trailer_header();
    test_request_body_chunked_user_buffer_small();
}