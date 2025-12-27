#include <assert.h>
#include <string.h>

#include "buffer.h"
#include "request.h"

#include "test-request.h"

void test_malformed_request(void)
{
    HttpBuffer *buffer = http_buffer_new(512);
    HttpRequest request;
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

    http_buffer_concat(buffer, "GET XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_URI_TOO_LONG);

    http_buffer_free(buffer);
}

void test_request_get(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest request;
    HttpResult ret;

    http_buffer_concat(
        buffer,
        "GET /\r\n"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_OK);
    assert(request.http_major == 0);
    assert(request.http_minor == 9);
    assert(request.method == HTTP_METHOD_GET);
    assert(!strcmp(request.uri, "/"));

    http_buffer_concat(
        buffer,
        "GET /index.html HTTP/1.0\r\n"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_OK);
    assert(request.http_major == 1);
    assert(request.http_minor == 0);
    assert(request.method == HTTP_METHOD_GET);
    assert(!strcmp(request.uri, "/index.html"));

    http_buffer_free(buffer);
}

void test_request_head(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest request;
    HttpResult ret;

    http_buffer_concat(
        buffer,
        "HEAD /\r\n"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_OK);
    assert(request.http_major == 0);
    assert(request.http_minor == 9);
    assert(request.method == HTTP_METHOD_HEAD);
    assert(!strcmp(request.uri, "/"));

    http_buffer_concat(
        buffer,
        "HEAD /index.html HTTP/1.0\r\n"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_OK);
    assert(request.http_major == 1);
    assert(request.http_minor == 0);
    assert(request.method == HTTP_METHOD_HEAD);
    assert(!strcmp(request.uri, "/index.html"));

    http_buffer_free(buffer);
}

void test_request(void)
{
    test_malformed_request();
    test_request_get();
    test_request_head();
}