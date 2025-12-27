#include <assert.h>
#include <string.h>

#include "buffer.h"
#include "request.h"

#include "test-request.h"

void test_malformed_request(void)
{
    HttpBuffer *buffer = http_buffer_new(512);
    HttpRequest request = {0};
    HttpResult ret;

    http_buffer_concat(buffer, "X");
    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(buffer, "BLA\n");
    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_ERROR);

    http_buffer_concat(buffer, "GET\n");
    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_ERROR);

    http_buffer_concat(buffer, "GET \n");
    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_ERROR);

    http_buffer_concat(buffer, "GET / HTTP\n");
    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_ERROR);

    http_buffer_concat(buffer, "GET / HTTP2.\n");
    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_ERROR);

    http_buffer_concat(buffer, "GET XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_URI_TOO_LONG);

    http_buffer_free(buffer);
}

void test_get_request(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest request = {0};
    HttpResult ret;

    http_buffer_concat(
        buffer,
        "GET /\r\n"
        "\r\n"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_OK);
    assert(request.http_major == 0);
    assert(request.http_minor == 9);
    assert(request.method == HTTP_METHOD_GET);
    assert(!strcmp(request.uri, "/"));
}

void test_get_request_with_http_version_1_0(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest request = {0};
    HttpResult ret;

    http_buffer_concat(
        buffer,
        "GET /index.html HTTP/1.0\r\n"
        "\r\n"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_OK);
    assert(request.http_major == 1);
    assert(request.http_minor == 0);
    assert(request.method == HTTP_METHOD_GET);
    assert(!strcmp(request.uri, "/index.html"));
}

void test_get_request_with_http_version_1_1(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest request = {0};
    HttpResult ret;

    http_buffer_concat(
        buffer,
        "GET /static/chat.png HTTP/1.1\r\n"
        "\r\n"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_OK);
    assert(request.http_major == 1);
    assert(request.http_minor == 1);
    assert(request.method == HTTP_METHOD_GET);
    assert(!strcmp(request.uri, "/static/chat.png"));

    http_buffer_free(buffer);
}

void test_get_request_with_headers(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest request = {0};
    HttpResult ret;

    http_buffer_concat(
        buffer,
        "GET / HTTP/1.0\r\n"
        "Host: www.example.com\r\n"
        "User-Agent: TestAgent/1.0\r\n"
        "\r\n"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_OK);
    assert(request.http_major == 1);
    assert(request.http_minor == 0);
    assert(request.method == HTTP_METHOD_GET);
    assert(!strcmp(request.uri, "/"));

    bool found_host = false;
    bool found_user_agent = false;
    HttpHeader *header = request.headers;
    while (header) {
        if (!strcmp(header->name, "Host")) {
            assert(!found_host);
            assert(!strcmp(header->value, "www.example.com"));
            found_host = true;
        } else if (!strcmp(header->name, "User-Agent")) {
            assert(!found_user_agent);
            assert(!strcmp(header->value, "TestAgent/1.0"));
            found_user_agent = true;
        } else {
            assert(false);
        }
        header = header->next;
    }

    assert(found_host);
    assert(found_user_agent);

    http_buffer_free(buffer);
}

void test_head_request(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest request = {0};
    HttpResult ret;

    http_buffer_concat(
        buffer,
        "HEAD /\r\n"
        "\r\n"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_OK);
    assert(request.http_major == 0);
    assert(request.http_minor == 9);
    assert(request.method == HTTP_METHOD_HEAD);
    assert(!strcmp(request.uri, "/"));
}

void test_head_request_with_http_version_1_0(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest request = {0};
    HttpResult ret;

    http_buffer_concat(
        buffer,
        "HEAD /index.html HTTP/1.0\r\n"
        "\r\n"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_OK);
    assert(request.http_major == 1);
    assert(request.http_minor == 0);
    assert(request.method == HTTP_METHOD_HEAD);
    assert(!strcmp(request.uri, "/index.html"));
}

void test_head_request_with_http_version_1_1(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest request = {0};
    HttpResult ret;

    http_buffer_concat(
        buffer,
        "HEAD /static/chat.png HTTP/1.1\r\n"
        "\r\n"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_OK);
    assert(request.http_major == 1);
    assert(request.http_minor == 1);
    assert(request.method == HTTP_METHOD_HEAD);
    assert(!strcmp(request.uri, "/static/chat.png"));

    http_buffer_free(buffer);
}

void test_head_request_with_headers_requiring_more_data(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest request = {0};
    HttpResult ret;

    http_buffer_concat(
        buffer,
        "HEAD /tralala"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        ".html HTTP/1.0\r\n"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        "Host: www.example"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        ".com\r\nApi-Key: 123456\r\nUse"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        "r-Agen"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        "t: TestAgent"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        "/1.0\r\n"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_REQUIRES_MORE_DATA);

    http_buffer_concat(
        buffer,
        "\r\n"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_OK);

    assert(request.http_major == 1);
    assert(request.http_minor == 0);
    assert(request.method == HTTP_METHOD_HEAD);
    assert(!strcmp(request.uri, "/tralala.html"));

    bool found_host = false;
    bool found_apikey = false;
    bool found_user_agent = false;
    HttpHeader *header = request.headers;
    while (header) {
        if (!strcmp(header->name, "Host")) {
            assert(!found_host);
            assert(!strcmp(header->value, "www.example.com"));
            found_host = true;
        } else if (!strcmp(header->name, "Api-Key")) {
            assert(!found_apikey);
            assert(!strcmp(header->value, "123456"));
            found_apikey = true;
        } else if (!strcmp(header->name, "User-Agent")) {
            assert(!found_user_agent);
            assert(!strcmp(header->value, "TestAgent/1.0"));
            found_user_agent = true;
        } else {
            assert(false);
        }
        header = header->next;
    }

    assert(found_host);
    assert(found_apikey);
    assert(found_user_agent);

    http_buffer_free(buffer);
}

void test_request(void)
{
    test_malformed_request();
    test_get_request();
    test_get_request_with_http_version_1_0();
    test_get_request_with_http_version_1_1();
    test_get_request_with_headers();
    test_head_request();
    test_head_request_with_http_version_1_0();
    test_head_request_with_http_version_1_1();
    test_head_request_with_headers_requiring_more_data();
}