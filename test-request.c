#include <assert.h>
#include <string.h>

#include "buffer.h"
#include "request.h"

#include "test-request.h"

void test_request1(void)
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

void test_request2(void)
{
    HttpBuffer *buffer = http_buffer_new(128);
    HttpRequest request;
    HttpResult ret;

    http_buffer_concat(
        buffer,
        "GET /index.html HTTP/1.0\r\n"
        "\r\n"
    );

    ret = http_request_parse(&request, buffer);
    assert(ret == HTTP_OK);
    assert(request.method == HTTP_GET);
    assert(!strcmp(request.uri, "/index.html"));

    http_buffer_free(buffer);
}

void test_request(void)
{
    test_request1();
    test_request2();
}