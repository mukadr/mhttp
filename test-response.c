#include <assert.h>
#include <string.h>

#include "response.h"
#include "http.h"

#include "test-response.h"

static void test_response_new_and_free(void)
{
    HttpResponse *response = http_response_new();

    assert(response);
    assert(response->status_code == 200);
    assert(!strcmp(response->reason, "OK"));

    http_response_free(response);
}

static void test_response_free_null(void)
{
    http_response_free(NULL);
}

static void test_response_set_status_known(void)
{
    HttpResponse *response = http_response_new();

    http_response_set_status(response, 404);
    assert(response->status_code == 404);
    assert(!strcmp(response->reason, "Not Found"));

    http_response_set_status(response, 500);
    assert(response->status_code == 500);
    assert(!strcmp(response->reason, "Internal Server Error"));

    http_response_set_status(response, 400);
    assert(response->status_code == 400);
    assert(!strcmp(response->reason, "Bad Request"));

    http_response_set_status(response, 200);
    assert(response->status_code == 200);
    assert(!strcmp(response->reason, "OK"));

    http_response_free(response);
}

static void test_response_set_status_unknown(void)
{
    HttpResponse *response = http_response_new();

    http_response_set_status(response, 999);
    assert(response->status_code == 999);
    assert(!strcmp(response->reason, "Unknown"));

    http_response_free(response);
}

static void test_response_set_header(void)
{
    HttpResponse *response = http_response_new();

    assert(http_response_set_header(response, "Content-Type", "text/html") == HTTP_OK);

    assert(response->headers);
    assert(!strcmp(response->headers->name, "Content-Type"));
    assert(!strcmp(response->headers->value, "text/html"));
    assert(response->headers->next == NULL);

    http_response_free(response);
}

static void test_response_set_header_multiple(void)
{
    HttpResponse *response = http_response_new();

    assert(http_response_set_header(response, "Content-Type", "text/html") == HTTP_OK);
    assert(http_response_set_header(response, "Server", "mhttp") == HTTP_OK);

    assert(response->headers);
    assert(!strcmp(response->headers->name, "Server"));
    assert(!strcmp(response->headers->value, "mhttp"));
    assert(response->headers->next);
    assert(!strcmp(response->headers->next->name, "Content-Type"));
    assert(!strcmp(response->headers->next->value, "text/html"));
    assert(response->headers->next->next == NULL);

    http_response_free(response);
}

static void test_response_set_body(void)
{
    HttpResponse *response = http_response_new();

    assert(http_response_set_body(response, "hello world") == HTTP_OK);

    assert(response->body);
    assert(!strcmp(response->body, "hello world"));
    assert(response->body_length == 11);

    http_response_free(response);
}

static void test_response_set_body_replaces(void)
{
    HttpResponse *response = http_response_new();

    assert(http_response_set_body(response, "first") == HTTP_OK);
    assert(http_response_set_body(response, "second") == HTTP_OK);

    assert(!strcmp(response->body, "second"));
    assert(response->body_length == 6);

    http_response_free(response);
}

static void test_response_set_body_null(void)
{
    HttpResponse *response = http_response_new();

    assert(http_response_set_body(response, NULL) == HTTP_OK);

    assert(response->body == NULL);
    assert(response->body_length == 0);

    http_response_free(response);
}

static void test_response_write_minimal(void)
{
    HttpResponse *response = http_response_new();
    HttpWriteBuf *buffer = http_writebuf_new(128);

    HttpResult ret = http_response_write(response, buffer);
    const char *expected = "HTTP/1.1 200 OK\r\n\r\n";

    assert(ret == HTTP_OK);
    assert(buffer->len == strlen(expected));
    assert(memcmp(buffer->data, expected, buffer->len) == 0);

    http_response_free(response);
    http_writebuf_free(buffer);
}

static void test_response_write_with_headers(void)
{
    HttpResponse *response = http_response_new();
    HttpWriteBuf *buffer = http_writebuf_new(256);

    http_response_set_status(response, 404);
    assert(http_response_set_header(response, "Content-Type", "text/plain") == HTTP_OK);

    HttpResult ret = http_response_write(response, buffer);
    const char *expected =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n";

    assert(ret == HTTP_OK);
    assert(buffer->len == strlen(expected));
    assert(memcmp(buffer->data, expected, buffer->len) == 0);

    http_response_free(response);
    http_writebuf_free(buffer);
}

static void test_response_write_with_body(void)
{
    HttpResponse *response = http_response_new();
    HttpWriteBuf *buffer = http_writebuf_new(256);

    assert(http_response_set_header(response, "Content-Type", "application/json") == HTTP_OK);
    assert(http_response_set_body(response, "{\"ok\":true}") == HTTP_OK);

    HttpResult ret = http_response_write(response, buffer);
    const char *expected =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 11\r\n"
        "Content-Type: application/json\r\n"
        "\r\n"
        "{\"ok\":true}";

    assert(ret == HTTP_OK);
    assert(buffer->len == strlen(expected));
    assert(memcmp(buffer->data, expected, buffer->len) == 0);

    http_response_free(response);
    http_writebuf_free(buffer);
}

static void test_response_write_partial(void)
{
    HttpResponse *response = http_response_new();
    HttpWriteBuf *buffer = http_writebuf_new(32);

    assert(http_response_set_body(response, "hello world") == HTTP_OK);

    HttpResult ret = http_response_write(response, buffer);

    assert(ret == HTTP_INTERNAL_ERROR);

    http_response_free(response);
    http_writebuf_free(buffer);
}

static void test_response_write_no_body(void)
{
    HttpResponse *response = http_response_new();
    HttpWriteBuf *buffer = http_writebuf_new(128);

    assert(http_response_set_header(response, "Server", "mhttp") == HTTP_OK);

    HttpResult ret = http_response_write(response, buffer);
    const char *expected =
        "HTTP/1.1 200 OK\r\n"
        "Server: mhttp\r\n"
        "\r\n";

    assert(ret == HTTP_OK);
    assert(buffer->len == strlen(expected));
    assert(memcmp(buffer->data, expected, buffer->len) == 0);

    http_response_free(response);
    http_writebuf_free(buffer);
}

void test_response(void)
{
    test_response_new_and_free();
    test_response_free_null();
    test_response_set_status_known();
    test_response_set_status_unknown();
    test_response_set_header();
    test_response_set_header_multiple();
    test_response_set_body();
    test_response_set_body_replaces();
    test_response_set_body_null();
    test_response_write_minimal();
    test_response_write_with_headers();
    test_response_write_with_body();
    test_response_write_partial();
    test_response_write_no_body();
}
