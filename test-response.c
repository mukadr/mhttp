#include <assert.h>
#include <string.h>

#include "http.h"
#include "response.h"
#include "test-response.h"

static void test_response_new_and_free(void)
{
	struct mhttp_response *rsp = mhttp_response_new();

	assert(rsp);
	assert(rsp->status_code == 200);
	assert(!strcmp(rsp->reason, "OK"));

	mhttp_response_free(rsp);
}

static void test_response_free_null(void)
{
	mhttp_response_free(NULL);
}

static void test_response_set_status_known(void)
{
	struct mhttp_response *rsp = mhttp_response_new();

	mhttp_response_set_status(rsp, 404);
	assert(rsp->status_code == 404);
	assert(!strcmp(rsp->reason, "Not Found"));

	mhttp_response_set_status(rsp, 500);
	assert(rsp->status_code == 500);
	assert(!strcmp(rsp->reason, "Internal Server Error"));

	mhttp_response_set_status(rsp, 400);
	assert(rsp->status_code == 400);
	assert(!strcmp(rsp->reason, "Bad Request"));

	mhttp_response_set_status(rsp, 200);
	assert(rsp->status_code == 200);
	assert(!strcmp(rsp->reason, "OK"));

	mhttp_response_free(rsp);
}

static void test_response_set_status_unknown(void)
{
	struct mhttp_response *rsp = mhttp_response_new();

	mhttp_response_set_status(rsp, 999);
	assert(rsp->status_code == 999);
	assert(!strcmp(rsp->reason, "Unknown"));

	mhttp_response_free(rsp);
}

static void test_response_set_header(void)
{
	struct mhttp_response *rsp = mhttp_response_new();

	assert(mhttp_response_set_header(rsp, "Content-Type", "text/html") == MHTTP_OK);

	assert(rsp->headers);
	assert(!strcmp(rsp->headers->name, "Content-Type"));
	assert(!strcmp(rsp->headers->value, "text/html"));
	assert(rsp->headers->next == NULL);

	mhttp_response_free(rsp);
}

static void test_response_set_header_multiple(void)
{
	struct mhttp_response *rsp = mhttp_response_new();

	assert(mhttp_response_set_header(rsp, "Content-Type", "text/html") == MHTTP_OK);
	assert(mhttp_response_set_header(rsp, "Server", "mhttp") == MHTTP_OK);

	assert(rsp->headers);
	assert(!strcmp(rsp->headers->name, "Server"));
	assert(!strcmp(rsp->headers->value, "mhttp"));
	assert(rsp->headers->next);
	assert(!strcmp(rsp->headers->next->name, "Content-Type"));
	assert(!strcmp(rsp->headers->next->value, "text/html"));
	assert(rsp->headers->next->next == NULL);

	mhttp_response_free(rsp);
}

static void test_response_set_body(void)
{
	struct mhttp_response *rsp = mhttp_response_new();

	assert(mhttp_response_set_body(rsp, "hello world") == MHTTP_OK);

	assert(rsp->body);
	assert(!strcmp(rsp->body, "hello world"));
	assert(rsp->body_length == 11);

	mhttp_response_free(rsp);
}

static void test_response_set_body_replaces(void)
{
	struct mhttp_response *rsp = mhttp_response_new();

	assert(mhttp_response_set_body(rsp, "first") == MHTTP_OK);
	assert(mhttp_response_set_body(rsp, "second") == MHTTP_OK);

	assert(!strcmp(rsp->body, "second"));
	assert(rsp->body_length == 6);

	mhttp_response_free(rsp);
}

static void test_response_set_body_null(void)
{
	struct mhttp_response *rsp = mhttp_response_new();

	assert(mhttp_response_set_body(rsp, NULL) == MHTTP_OK);

	assert(rsp->body == NULL);
	assert(rsp->body_length == 0);

	mhttp_response_free(rsp);
}

static void test_response_write_minimal(void)
{
	struct mhttp_response *rsp = mhttp_response_new();
	struct mhttp_wbuf *wbuf = mhttp_wbuf_new(128);
	enum mhttp_result ret;
	const char *expected;

	ret = mhttp_response_write(rsp, wbuf);
	expected = "HTTP/1.1 200 OK\r\n\r\n";

	assert(ret == MHTTP_OK);
	assert(wbuf->len == strlen(expected));
	assert(memcmp(wbuf->data, expected, wbuf->len) == 0);

	mhttp_response_free(rsp);
	mhttp_wbuf_free(wbuf);
}

static void test_response_write_with_headers(void)
{
	struct mhttp_response *rsp = mhttp_response_new();
	struct mhttp_wbuf *wbuf = mhttp_wbuf_new(256);
	enum mhttp_result ret;
	const char *expected;

	mhttp_response_set_status(rsp, 404);
	assert(mhttp_response_set_header(rsp, "Content-Type", "text/plain") == MHTTP_OK);

	ret = mhttp_response_write(rsp, wbuf);
	expected =
		"HTTP/1.1 404 Not Found\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n";

	assert(ret == MHTTP_OK);
	assert(wbuf->len == strlen(expected));
	assert(memcmp(wbuf->data, expected, wbuf->len) == 0);

	mhttp_response_free(rsp);
	mhttp_wbuf_free(wbuf);
}

static void test_response_write_with_body(void)
{
	struct mhttp_response *rsp = mhttp_response_new();
	struct mhttp_wbuf *wbuf = mhttp_wbuf_new(256);
	enum mhttp_result ret;
	const char *expected;

	assert(mhttp_response_set_header(rsp, "Content-Type", "application/json") == MHTTP_OK);
	assert(mhttp_response_set_body(rsp, "{\"ok\":true}") == MHTTP_OK);

	ret = mhttp_response_write(rsp, wbuf);
	expected =
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 11\r\n"
		"Content-Type: application/json\r\n"
		"\r\n"
		"{\"ok\":true}";

	assert(ret == MHTTP_OK);
	assert(wbuf->len == strlen(expected));
	assert(memcmp(wbuf->data, expected, wbuf->len) == 0);

	mhttp_response_free(rsp);
	mhttp_wbuf_free(wbuf);
}

static void test_response_write_partial(void)
{
	struct mhttp_response *rsp = mhttp_response_new();
	struct mhttp_wbuf *wbuf = mhttp_wbuf_new(32);
	enum mhttp_result ret;

	assert(mhttp_response_set_body(rsp, "hello world") == MHTTP_OK);

	ret = mhttp_response_write(rsp, wbuf);

	assert(ret == MHTTP_ERROR);

	mhttp_response_free(rsp);
	mhttp_wbuf_free(wbuf);
}

static void test_response_write_no_body(void)
{
	struct mhttp_response *rsp = mhttp_response_new();
	struct mhttp_wbuf *wbuf = mhttp_wbuf_new(128);
	enum mhttp_result ret;
	const char *expected;

	assert(mhttp_response_set_header(rsp, "Server", "mhttp") == MHTTP_OK);

	ret = mhttp_response_write(rsp, wbuf);
	expected =
		"HTTP/1.1 200 OK\r\n"
		"Server: mhttp\r\n"
		"\r\n";

	assert(ret == MHTTP_OK);
	assert(wbuf->len == strlen(expected));
	assert(memcmp(wbuf->data, expected, wbuf->len) == 0);

	mhttp_response_free(rsp);
	mhttp_wbuf_free(wbuf);
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
