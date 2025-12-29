#include <assert.h>
#include <string.h>

#include "buffer.h"
#include "request.h"

#include "test-request.h"

void test_malformed_request(void)
{
    HttpBuffer *buffer = http_buffer_new(512);
    HttpRequest *request = http_request_new();

    http_buffer_concat(buffer, "X");
    assert(http_request_parse(request, buffer) == HTTP_REQUIRES_MORE_DATA);

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
        "\r \n"
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
    assert(request->http_major == 0);
    assert(request->http_minor == 9);

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
    assert(request->http_major == 1);
    assert(request->http_minor == 0);

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
    assert(request->http_major == 1);
    assert(request->http_minor == 1);

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
    assert(request->http_major == 1);
    assert(request->http_minor == 0);

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
    assert(request->http_major == 0);
    assert(request->http_minor == 9);

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
    assert(request->http_major == 1);
    assert(request->http_minor == 0);

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
    assert(request->http_major == 1);
    assert(request->http_minor == 1);

    http_request_free(request);
    http_buffer_free(buffer);
}

void test_head_request_with_headers_requiring_more_data(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest *request = http_request_new();

    http_buffer_concat(
        buffer,
        "HEAD /tralala"
    );

    assert(http_request_parse(request, buffer) == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        ".html HTT"
    );

    assert(http_request_parse(request, buffer) == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        "P/1.0\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        "Host"
    );

    assert(http_request_parse(request, buffer) == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        ":"
    );

    assert(http_request_parse(request, buffer) == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        " www.example"
    );

    assert(http_request_parse(request, buffer) == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        ".com\r\nApi-Key: 123456\r\nUse"
    );

    assert(http_request_parse(request, buffer) == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        "r-Agen"
    );

    assert(http_request_parse(request, buffer) == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        "t: TestAgent"
    );

    assert(http_request_parse(request, buffer) == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        "/1.0\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        "\r\n"
    );

    assert(http_request_parse(request, buffer) == HTTP_OK);

    // Request
    assert(request->method == HTTP_METHOD_HEAD);
    assert(!strcmp(request->uri, "/tralala.html"));
    assert(request->http_major == 1);
    assert(request->http_minor == 0);

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
    test_head_request_with_headers_requiring_more_data();
}