#include <assert.h>
#include <string.h>

#include "buffer.h"
#include "request.h"

#include "test-request.h"

// Bypass null checking from http_buffer_concat
static void buffer_write_raw(HttpBuffer *buffer, const void *data, size_t len)
{
    size_t remaining = buffer->end - buffer->pos;
    memmove(buffer->buf, buffer->pos, remaining);
    memcpy(buffer->buf + remaining, data, len);
    buffer->pos = buffer->buf;
    buffer->end = buffer->buf + remaining + len;
}

void test_malformed_request(void)
{
    HttpBuffer *buffer = http_buffer_new(512);
    HttpRequest *request = http_request_new();

    http_buffer_concat(buffer, "");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_buffer_concat(buffer, "X");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);
    http_buffer_reset(buffer);

    http_buffer_concat(buffer, "BLA\n");
    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_buffer_concat(buffer, "GET\n");
    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_buffer_concat(buffer, "GET \n");
    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_buffer_concat(buffer, "GET / HTTP\n");
    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_buffer_concat(buffer, "GET / HTTP2.\n");
    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_buffer_concat(buffer,
        "GET "
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
        "\r\n");
    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_request_free(request);
    http_buffer_free(buffer);
}

void test_bad_line_ending(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest *request = http_request_new();

    http_buffer_concat(
        buffer,
        "GET /index.html HTTP/1.0\n"
    );
    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_buffer_concat(
        buffer,
        "GET /index.html HTTP/1.0\r"
    );
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);
    http_buffer_reset(buffer);

    http_buffer_concat(
        buffer,
        "GET /index.html HTTP/1.0 \r\n"
    );
    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_buffer_concat(
        buffer,
        "GET /index.html HTTP/1.0\r \n"
    );
    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_buffer_concat(
        buffer,
        "GET /index.html HTTP/1.0\r\n"
        "\n"
    );
    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_buffer_concat(
        buffer,
        "GET /index.html HTTP/1.0\r\n"
        "Host: www.example.com\r \n"
        "\r\n"
    );
    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_request_free(request);
    http_buffer_free(buffer);
}

void test_get_request(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest *request = http_request_new();

    http_buffer_concat(
        buffer,
        "GET /\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(request->method == HTTP_METHOD_GET);
    assert(!strcmp(request->uri, "/"));
    assert(request->version == 9);

    http_request_free(request);
    http_buffer_free(buffer);
}

void test_get_request_with_http_version_1_0(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest *request = http_request_new();

    http_buffer_concat(
        buffer,
        "GET /index.html HTTP/1.0\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(request->method == HTTP_METHOD_GET);
    assert(!strcmp(request->uri, "/index.html"));
    assert(request->version == 10);

    http_request_free(request);
    http_buffer_free(buffer);
}

void test_get_request_with_http_version_1_1(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest *request = http_request_new();

    http_buffer_concat(
        buffer,
        "GET /static/chat.png HTTP/1.1\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(request->method == HTTP_METHOD_GET);
    assert(!strcmp(request->uri, "/static/chat.png"));
    assert(request->version == 11);

    http_request_free(request);
    http_buffer_free(buffer);
}

void test_get_request_with_headers(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest *request = http_request_new();

    http_buffer_concat(
        buffer,
        "GET / HTTP/1.0\r\n"
        "Host: www.example.com\r\n"
        "User-Agent: TestAgent/1.0\r\n"
        "\r\n"
    );

    // Request
    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(request->method == HTTP_METHOD_GET);
    assert(!strcmp(request->uri, "/"));
    assert(request->version == 10);

    // Headers
    assert(http_request_header_count(request) == 2);

    const char *host = http_request_get_header(request, "Host");
    assert(host);
    assert(!strcmp(host, "www.example.com"));

    const char *user_agent = http_request_get_header(request, "User-Agent");
    assert(user_agent);
    assert(!strcmp(user_agent, "TestAgent/1.0"));

    http_request_free(request);
    http_buffer_free(buffer);
}

void test_head_request(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest *request = http_request_new();

    http_buffer_concat(
        buffer,
        "HEAD /\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(request->method == HTTP_METHOD_HEAD);
    assert(!strcmp(request->uri, "/"));
    assert(request->version == 9);

    http_request_free(request);
    http_buffer_free(buffer);
}

void test_head_request_with_http_version_1_0(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest *request = http_request_new();

    http_buffer_concat(
        buffer,
        "HEAD /index.html HTTP/1.0\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(request->method == HTTP_METHOD_HEAD);
    assert(!strcmp(request->uri, "/index.html"));
    assert(request->version == 10);

    http_request_free(request);
    http_buffer_free(buffer);
}

void test_head_request_with_http_version_1_1(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest *request = http_request_new();

    http_buffer_concat(
        buffer,
        "HEAD /static/chat.png HTTP/1.1\r\n"
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(request->method == HTTP_METHOD_HEAD);
    assert(!strcmp(request->uri, "/static/chat.png"));
    assert(request->version == 11);

    http_request_free(request);
    http_buffer_free(buffer);
}

void test_head_request_with_headers_needing_more_input(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest *request = http_request_new();

    http_buffer_concat(buffer, "HEAD /tralala");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_buffer_concat(buffer, ".html HTT");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_buffer_concat(buffer, "P/1.0\r\n");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_buffer_concat(buffer, "Host");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_buffer_concat(buffer, ":");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_buffer_concat(buffer, " www.example");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_buffer_concat(buffer, ".com\r\nApi-Key: 123456\r\nUse");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_buffer_concat(buffer, "r-Agen");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_buffer_concat(buffer, "t: TestAgent");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_buffer_concat(buffer, "/1.0\r\n");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_buffer_concat(buffer, "\r\n");
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
    http_buffer_free(buffer);
}

void test_head_request_with_missing_header_name(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest *request = http_request_new();

    http_buffer_concat(buffer, "HEAD /hello\r\n");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_buffer_concat(buffer, ": world\r");
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);

    http_buffer_concat(buffer, "\n");
    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_request_free(request);
    http_buffer_free(buffer);
}

static void test_header_count_below_limit_succeeds(void)
{
    HttpBuffer *buffer = http_buffer_new(1024);
    HttpRequest *request = http_request_new();

    http_buffer_concat(buffer, "GET / HTTP/1.1\r\n");
    for (int i = 0; i < 100; i++) {
        http_buffer_concat(buffer, "H: v\r\n");
    }
    http_buffer_concat(buffer, "\r\n");

    assert(http_request_parse(request, buffer) == HTTP_OK);
    assert(http_request_header_count(request) == 100);

    http_request_free(request);
    http_buffer_free(buffer);
}

static void test_header_count_above_limit_fails(void)
{
    HttpBuffer *buffer = http_buffer_new(1024);
    HttpRequest *request = http_request_new();

    http_buffer_concat(buffer, "GET / HTTP/1.1\r\n");
    for (int i = 0; i < 50; i++) {
        http_buffer_concat(buffer, "H: v\r\n");
    }

    // Make sure the count is tracked across multiple calls, not just within a single call
    assert(http_request_parse(request, buffer) == HTTP_NEED_MORE_INPUT);
    assert(http_request_header_count(request) == 50);

    for (int i = 0; i < 51; i++) {
        http_buffer_concat(buffer, "H: v\r\n");
    }
    http_buffer_concat(buffer, "\r\n");

    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_request_free(request);
    http_buffer_free(buffer);
}

static void test_null_byte_in_uri(void)
{
    HttpBuffer *buffer = http_buffer_new(64);
    HttpRequest *request = http_request_new();

    static const char req[] = "GET /pa\x00th HTTP/1.1\r\n\r\n";
    buffer_write_raw(buffer, req, sizeof(req) - 1);

    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_request_free(request);
    http_buffer_free(buffer);
}

static void test_null_byte_in_header_name(void)
{
    HttpBuffer *buffer = http_buffer_new(64);
    HttpRequest *request = http_request_new();

    static const char req[] = "GET / HTTP/1.1\r\nNa\x00me: value\r\n\r\n";
    buffer_write_raw(buffer, req, sizeof(req) - 1);

    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_request_free(request);
    http_buffer_free(buffer);
}

static void test_null_byte_in_header_value(void)
{
    HttpBuffer *buffer = http_buffer_new(64);
    HttpRequest *request = http_request_new();

    static const char req[] = "GET / HTTP/1.1\r\nName: va\x00lue\r\n\r\n";
    buffer_write_raw(buffer, req, sizeof(req) - 1);

    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_request_free(request);
    http_buffer_free(buffer);
}

static void test_header_name_below_length_limit_succeeds(void)
{
    HttpBuffer *buffer = http_buffer_new(512);
    HttpRequest *request = http_request_new();

    char header[270];
    memset(header, 'A', 255);
    strcpy(header + 255, ": v\r\n\r\n");

    http_buffer_concat(buffer, "GET / HTTP/1.1\r\n");
    http_buffer_concat(buffer, header);

    assert(http_request_parse(request, buffer) == HTTP_OK);

    http_request_free(request);
    http_buffer_free(buffer);
}

static void test_header_name_above_length_limit_fails(void)
{
    HttpBuffer *buffer = http_buffer_new(512);
    HttpRequest *request = http_request_new();

    char header[270];
    memset(header, 'A', 256);
    strcpy(header + 256, ": v\r\n\r\n");

    http_buffer_concat(buffer, "GET / HTTP/1.1\r\n");
    http_buffer_concat(buffer, header);

    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_request_free(request);
    http_buffer_free(buffer);
}

static void test_header_value_below_length_limit_succeeds(void)
{
    HttpBuffer *buffer = http_buffer_new(512);
    HttpRequest *request = http_request_new();

    char header[270];
    strcpy(header, "N: ");
    memset(header + 3, 'B', 255);
    strcpy(header + 3 + 255, "\r\n\r\n");

    http_buffer_concat(buffer, "GET / HTTP/1.1\r\n");
    http_buffer_concat(buffer, header);

    assert(http_request_parse(request, buffer) == HTTP_OK);

    http_request_free(request);
    http_buffer_free(buffer);
}

static void test_header_value_above_length_limit_fails(void)
{
    HttpBuffer *buffer = http_buffer_new(512);
    HttpRequest *request = http_request_new();

    char header[270];
    strcpy(header, "N: ");
    memset(header + 3, 'B', 256);
    strcpy(header + 3 + 256, "\r\n\r\n");

    http_buffer_concat(buffer, "GET / HTTP/1.1\r\n");
    http_buffer_concat(buffer, header);

    assert(http_request_parse(request, buffer) == HTTP_BAD_REQUEST);

    http_request_free(request);
    http_buffer_free(buffer);
}

static void test_case_insensitive_header_name_lookup(void)
{
    HttpBuffer *buffer = http_buffer_new(256);
    HttpRequest *request = http_request_new();

    http_buffer_concat(
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
    http_buffer_free(buffer);
}

void test_request(void)
{
    test_malformed_request();
    test_bad_line_ending();
    test_get_request();
    test_get_request_with_http_version_1_0();
    test_get_request_with_http_version_1_1();
    test_get_request_with_headers();
    test_head_request();
    test_head_request_with_http_version_1_0();
    test_head_request_with_http_version_1_1();
    test_head_request_with_headers_needing_more_input();
    test_head_request_with_missing_header_name();
    test_header_count_below_limit_succeeds();
    test_header_count_above_limit_fails();
    test_header_name_below_length_limit_succeeds();
    test_header_name_above_length_limit_fails();
    test_header_value_below_length_limit_succeeds();
    test_header_value_above_length_limit_fails();
    test_null_byte_in_uri();
    test_null_byte_in_header_name();
    test_null_byte_in_header_value();
    test_case_insensitive_header_name_lookup();
}