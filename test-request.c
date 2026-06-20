#include <assert.h>
#include <string.h>

#include "rbuf.h"
#include "request.h"

#include "test-request.h"

// Bypass null checking from mhttp_rbuf_feed
static void rbuf_write_raw(struct mhttp_rbuf *rb, const void *data, size_t len)
{
	size_t remaining = rb->end - rb->pos;

	memmove(rb->buf, rb->pos, remaining);
	memcpy(rb->buf + remaining, data, len);
	rb->pos = rb->buf;
	rb->end = rb->buf + remaining + len;
}

static void test_request_with_malformed_content(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(512);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(rb, "");
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);

	mhttp_rbuf_feed(rb, "X");
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);
	mhttp_rbuf_reset(rb);

	mhttp_rbuf_feed(rb, "BLA\n");
	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_rbuf_feed(rb, "GET\n");
	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_rbuf_feed(rb, "GET \n");
	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_rbuf_feed(rb, "GET / HTTP\n");
	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_rbuf_feed(rb, "GET / HTTP2.\n");
	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_rbuf_feed(rb,
		"GET "
		"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
		"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
		"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
		"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
		"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
		"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
		"\r\n");
	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_with_bad_line_ending(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(128);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(
		rb,
		"GET /index.html HTTP/1.0\n");
	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_rbuf_feed(
		rb,
		"GET /index.html HTTP/1.0\r");
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);
	mhttp_rbuf_reset(rb);

	mhttp_rbuf_feed(
		rb,
		"GET /index.html HTTP/1.0 \r\n");
	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_rbuf_feed(
		rb,
		"GET /index.html HTTP/1.0\r \n");
	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_rbuf_feed(
		rb,
		"GET /index.html HTTP/1.0\r\n"
		"\n");
	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_rbuf_feed(
		rb,
		"GET /index.html HTTP/1.0\r\n"
		"Host: www.example.com\r \n"
		"\r\n");
	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_with_null_byte_in_uri(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(64);
	struct mhttp_request *req = mhttp_request_new();
	static const char r[] = "GET /pa\x00th HTTP/1.1\r\n\r\n";

	rbuf_write_raw(rb, r, sizeof(r) - 1);

	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_with_null_byte_in_header_name(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(64);
	struct mhttp_request *req = mhttp_request_new();
	static const char r[] = "GET / HTTP/1.1\r\nNa\x00me: value\r\n\r\n";

	rbuf_write_raw(rb, r, sizeof(r) - 1);

	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_with_null_byte_in_header_value(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(64);
	struct mhttp_request *req = mhttp_request_new();
	static const char r[] = "GET / HTTP/1.1\r\nName: va\x00lue\r\n\r\n";

	rbuf_write_raw(rb, r, sizeof(r) - 1);

	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_get_request(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(128);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(
		rb,
		"GET /\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(req->method == MHTTP_METHOD_GET);
	assert(!strcmp(req->uri, "/"));
	assert(req->version == 9);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_post_request(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(128);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(
		rb,
		"POST /\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(req->method == MHTTP_METHOD_POST);
	assert(!strcmp(req->uri, "/"));
	assert(req->version == 9);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_head_request(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(128);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(
		rb,
		"HEAD /\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(req->method == MHTTP_METHOD_HEAD);
	assert(!strcmp(req->uri, "/"));
	assert(req->version == 9);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_with_http_version_1_0(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(128);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(
		rb,
		"GET /index.html HTTP/1.0\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(req->method == MHTTP_METHOD_GET);
	assert(!strcmp(req->uri, "/index.html"));
	assert(req->version == 10);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_with_http_version_1_1(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(128);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(
		rb,
		"POST /static/chat.png HTTP/1.1\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(!strcmp(req->uri, "/static/chat.png"));
	assert(req->version == 11);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_with_headers(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(256);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(
		rb,
		"POST / HTTP/1.1\r\n"
		"Host: www.example.com\r\n"
		"Content-Type: application/json\r\n"
		"User-Agent: TestAgent/1.0\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(!strcmp(req->uri, "/"));
	assert(req->version == 11);
	assert(mhttp_request_header_count(req) == 3);

	const char *host = mhttp_request_get_header(req, "Host");
	assert(host);
	assert(!strcmp(host, "www.example.com"));

	const char *ct = mhttp_request_get_header(req, "Content-Type");
	assert(ct);
	assert(!strcmp(ct, "application/json"));

	const char *ua = mhttp_request_get_header(req, "User-Agent");
	assert(ua);
	assert(!strcmp(ua, "TestAgent/1.0"));

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_with_headers_needing_more_input(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(128);
	struct mhttp_request *req = mhttp_request_new();
	const char *host, *api_key, *user_agent;

	mhttp_rbuf_feed(rb, "HEAD /tralala");
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);

	mhttp_rbuf_feed(rb, ".html HTT");
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);

	mhttp_rbuf_feed(rb, "P/1.0\r\n");
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);

	mhttp_rbuf_feed(rb, "Host");
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);

	mhttp_rbuf_feed(rb, ":");
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);

	mhttp_rbuf_feed(rb, " www.example");
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);

	mhttp_rbuf_feed(rb, ".com\r\nApi-Key: 123456\r\nUse");
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);

	mhttp_rbuf_feed(rb, "r-Agen");
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);

	mhttp_rbuf_feed(rb, "t: TestAgent");
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);

	mhttp_rbuf_feed(rb, "/1.0\r\n");
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);

	mhttp_rbuf_feed(rb, "\r\n");
	assert(mhttp_request_parse(req, rb) == MHTTP_OK);

	// Request
	assert(req->method == MHTTP_METHOD_HEAD);
	assert(!strcmp(req->uri, "/tralala.html"));
	assert(req->version == 10);

	// Headers
	assert(mhttp_request_header_count(req) == 3);

	host = mhttp_request_get_header(req, "Host");
	assert(host);
	assert(!strcmp(host, "www.example.com"));

	api_key = mhttp_request_get_header(req, "Api-Key");
	assert(api_key);
	assert(!strcmp(api_key, "123456"));

	user_agent = mhttp_request_get_header(req, "User-Agent");
	assert(user_agent);
	assert(!strcmp(user_agent, "TestAgent/1.0"));

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_with_missing_header_name(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(128);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(rb, "HEAD /hello\r\n");
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);

	mhttp_rbuf_feed(rb, ": world\r");
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);

	mhttp_rbuf_feed(rb, "\n");
	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_with_header_count_below_limit_succeeds(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(1024);
	struct mhttp_request *req = mhttp_request_new();
	int i;

	mhttp_rbuf_feed(rb, "GET / HTTP/1.1\r\n");
	for (i = 0; i < 100; i++)
		mhttp_rbuf_feed(rb, "H: v\r\n");
	mhttp_rbuf_feed(rb, "\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(mhttp_request_header_count(req) == 100);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_with_header_count_above_limit_fails(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(1024);
	struct mhttp_request *req = mhttp_request_new();
	int i;

	mhttp_rbuf_feed(rb, "GET / HTTP/1.1\r\n");
	for (i = 0; i < 50; i++)
		mhttp_rbuf_feed(rb, "H: v\r\n");

	// Make sure the count is tracked across multiple calls, not just within a single call
	assert(mhttp_request_parse(req, rb) == MHTTP_NEED_MORE_INPUT);
	assert(mhttp_request_header_count(req) == 50);

	for (i = 0; i < 51; i++)
		mhttp_rbuf_feed(rb, "H: v\r\n");
	mhttp_rbuf_feed(rb, "\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_with_header_name_below_length_limit_succeeds(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(512);
	struct mhttp_request *req = mhttp_request_new();
	char h[270];

	memset(h, 'A', 255);
	strcpy(h + 255, ": v\r\n\r\n");

	mhttp_rbuf_feed(rb, "GET / HTTP/1.1\r\n");
	mhttp_rbuf_feed(rb, h);

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_with_header_name_above_length_limit_fails(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(512);
	struct mhttp_request *req = mhttp_request_new();
	char h[270];

	memset(h, 'A', 256);
	strcpy(h + 256, ": v\r\n\r\n");

	mhttp_rbuf_feed(rb, "GET / HTTP/1.1\r\n");
	mhttp_rbuf_feed(rb, h);

	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_with_header_value_below_length_limit_succeeds(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(512);
	struct mhttp_request *req = mhttp_request_new();
	char h[270];

	strcpy(h, "N: ");
	memset(h + 3, 'B', 255);
	strcpy(h + 3 + 255, "\r\n\r\n");

	mhttp_rbuf_feed(rb, "GET / HTTP/1.1\r\n");
	mhttp_rbuf_feed(rb, h);

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_with_header_value_above_length_limit_fails(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(512);
	struct mhttp_request *req = mhttp_request_new();
	char h[270];

	strcpy(h, "N: ");
	memset(h + 3, 'B', 256);
	strcpy(h + 3 + 256, "\r\n\r\n");

	mhttp_rbuf_feed(rb, "GET / HTTP/1.1\r\n");
	mhttp_rbuf_feed(rb, h);

	assert(mhttp_request_parse(req, rb) == MHTTP_INVALID_REQUEST);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_put_request(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(128);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(
		rb,
		"PUT /items/42 HTTP/1.1\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(req->method == MHTTP_METHOD_PUT);
	assert(!strcmp(req->uri, "/items/42"));
	assert(req->version == 11);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_delete_request(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(128);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(
		rb,
		"DELETE /items/42 HTTP/1.1\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(req->method == MHTTP_METHOD_DELETE);
	assert(!strcmp(req->uri, "/items/42"));
	assert(req->version == 11);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_options_request(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(128);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(
		rb,
		"OPTIONS * HTTP/1.1\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(req->method == MHTTP_METHOD_OPTIONS);
	assert(!strcmp(req->uri, "*"));
	assert(req->version == 11);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_patch_request(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(128);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(
		rb,
		"PATCH /items/42 HTTP/1.1\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(req->method == MHTTP_METHOD_PATCH);
	assert(!strcmp(req->uri, "/items/42"));
	assert(req->version == 11);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_body_full_in_rbuf(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(256);
	struct mhttp_request *req = mhttp_request_new();
	char body[16] = {0};
	size_t ret;

	mhttp_rbuf_feed(
		rb,
		"POST /submit HTTP/1.1\r\n"
		"Content-Length: 7\r\n"
		"\r\n"
		"abcdefg");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(mhttp_request_content_length(req) == 7);

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 7);
	assert(!memcmp(body, "abcdefg", 7));

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 0);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_body_incremental(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(256);
	struct mhttp_request *req = mhttp_request_new();
	char body[16] = {0};
	size_t ret;

	mhttp_rbuf_feed(
		rb,
		"POST /upload HTTP/1.1\r\n"
		"Content-Length: 10\r\n"
		"\r\n"
		"hello");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(mhttp_request_content_length(req) == 10);

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 5);
	assert(!memcmp(body, "hello", 5));

	mhttp_rbuf_feed(rb, " worl");

	ret = mhttp_request_read_body(req, rb, body + 5, sizeof(body) - 5);
	assert(ret == 5);
	assert(!memcmp(body + 5, " worl", 5));

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 0);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_no_content_length(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(256);
	struct mhttp_request *req = mhttp_request_new();
	char body[16] = {0};
	size_t ret;

	mhttp_rbuf_feed(
		rb,
		"GET / HTTP/1.1\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(mhttp_request_content_length(req) == 0);

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 0);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_content_length_zero(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(256);
	struct mhttp_request *req = mhttp_request_new();
	char body[16] = {0};
	size_t ret;

	mhttp_rbuf_feed(
		rb,
		"POST /empty HTTP/1.1\r\n"
		"Content-Length: 0\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(mhttp_request_content_length(req) == 0);

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 0);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_body_clamped_to_content_length(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(256);
	struct mhttp_request *req = mhttp_request_new();
	char body[16] = {0};
	size_t ret;

	mhttp_rbuf_feed(
		rb,
		"POST /data HTTP/1.1\r\n"
		"Content-Length: 3\r\n"
		"\r\n"
		"abcdef");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(mhttp_request_content_length(req) == 3);

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 3);
	assert(!memcmp(body, "abc", 3));

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 0);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_body_rbuf_smaller_than_content(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(64);
	struct mhttp_request *req = mhttp_request_new();
	char body[60] = {0};
	size_t ret, total = 0;

	mhttp_rbuf_feed(
		rb,
		"POST /data HTTP/1.1\r\n"
		"Content-Length: 50\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(mhttp_request_content_length(req) == 50);

	mhttp_rbuf_feed(rb, "AAAAAAAAAABBBBBBBBBB");
	ret = mhttp_request_read_body(req, rb, body + total, sizeof(body) - total);
	assert(ret == 20);
	total += ret;
	assert(!memcmp(body, "AAAAAAAAAABBBBBBBBBB", 20));

	mhttp_rbuf_feed(rb, "CCCCCCCCCCDDDDDDDDDD");
	ret = mhttp_request_read_body(req, rb, body + total, sizeof(body) - total);
	assert(ret == 20);
	total += ret;
	assert(!memcmp(body + 20, "CCCCCCCCCCDDDDDDDDDD", 20));

	mhttp_rbuf_feed(rb, "EEEEEEEEEE");
	ret = mhttp_request_read_body(req, rb, body + total, sizeof(body) - total);
	assert(ret == 10);
	total += ret;
	assert(!memcmp(body + 40, "EEEEEEEEEE", 10));

	ret = mhttp_request_read_body(req, rb, body + total, sizeof(body) - total);
	assert(ret == 0);
	assert(total == 50);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_body_chunked_single(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(256);
	struct mhttp_request *req = mhttp_request_new();
	char body[16] = {0};
	size_t ret;

	mhttp_rbuf_feed(
		rb,
		"POST /data HTTP/1.1\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"7\r\n"
		"abcdefg\r\n"
		"0\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(mhttp_request_is_chunked(req));

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 7);
	assert(!memcmp(body, "abcdefg", 7));

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 0);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_body_chunked_multiple(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(256);
	struct mhttp_request *req = mhttp_request_new();
	char body[32] = {0};
	size_t ret;

	mhttp_rbuf_feed(
		rb,
		"POST /data HTTP/1.1\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"5\r\n"
		"hello\r\n"
		"6\r\n"
		" world\r\n"
		"0\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(mhttp_request_is_chunked(req));

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 11);
	assert(!memcmp(body, "hello world", 11));

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 0);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_body_chunked_incremental(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(128);
	struct mhttp_request *req = mhttp_request_new();
	char body[32] = {0};
	size_t ret, total = 0;

	mhttp_rbuf_feed(
		rb,
		"POST /data HTTP/1.1\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"A\r\n"
		"hello");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(mhttp_request_is_chunked(req));

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 5);
	total += ret;

	mhttp_rbuf_feed(rb, " wor");

	ret = mhttp_request_read_body(req, rb, body + total, sizeof(body) - total);
	assert(ret == 4);
	total += ret;

	mhttp_rbuf_feed(rb, "l");

	ret = mhttp_request_read_body(req, rb, body + total, sizeof(body) - total);
	assert(ret == 1);
	total += ret;

	mhttp_rbuf_feed(rb, "d\r\n0\r\n\r\n");

	ret = mhttp_request_read_body(req, rb, body + total, sizeof(body) - total);
	assert(ret == 0);
	assert(total == 10);
	assert(!memcmp(body, "hello worl", 10));

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 0);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_body_chunked_no_header(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(256);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(
		rb,
		"POST /data HTTP/1.1\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"hello");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(!mhttp_request_is_chunked(req));

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_body_chunked_empty(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(256);
	struct mhttp_request *req = mhttp_request_new();
	char body[16] = {0};
	size_t ret;

	mhttp_rbuf_feed(
		rb,
		"POST /data HTTP/1.1\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"0\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(mhttp_request_is_chunked(req));

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 0);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_body_chunked_no_header_data_yet(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(256);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(
		rb,
		"POST /data HTTP/1.1\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(mhttp_request_is_chunked(req));

	char body[16] = {0};
	size_t ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 0);

	mhttp_rbuf_feed(rb, "5\r\nhello\r\n0\r\n\r\n");

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 5);
	assert(!memcmp(body, "hello", 5));

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 0);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_body_chunked_trailer_split(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(256);
	struct mhttp_request *req = mhttp_request_new();
	char body[16] = {0};
	size_t ret;

	mhttp_rbuf_feed(
		rb,
		"POST /data HTTP/1.1\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"5\r\n"
		"hello\r\n"
		"0\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(mhttp_request_is_chunked(req));

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 5);
	assert(!memcmp(body, "hello", 5));

	mhttp_rbuf_feed(rb, "\r\n");

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 0);

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 0);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_body_chunked_trailer_header(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(256);
	struct mhttp_request *req = mhttp_request_new();
	char body[16] = {0};
	size_t ret;

	mhttp_rbuf_feed(
		rb,
		"POST /data HTTP/1.1\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"4\r\n"
		"body\r\n"
		"0\r\n"
		"X-Info: ok\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(mhttp_request_is_chunked(req));

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 4);
	assert(!memcmp(body, "body", 4));

	ret = mhttp_request_read_body(req, rb, body, sizeof(body));
	assert(ret == 0);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_body_chunked_user_rbuf_small(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(256);
	struct mhttp_request *req = mhttp_request_new();
	char body[16] = {0};
	size_t ret, total = 0;

	mhttp_rbuf_feed(
		rb,
		"POST /data HTTP/1.1\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"A\r\n"
		"0123456789\r\n"
		"0\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);
	assert(mhttp_request_is_chunked(req));

	ret = mhttp_request_read_body(req, rb, body, 3);
	assert(ret == 3);
	assert(!memcmp(body, "012", 3));
	total += ret;

	ret = mhttp_request_read_body(req, rb, body + total, 3);
	assert(ret == 3);
	assert(!memcmp(body + total, "345", 3));
	total += ret;

	ret = mhttp_request_read_body(req, rb, body + total, 3);
	assert(ret == 3);
	assert(!memcmp(body + total, "678", 3));
	total += ret;

	ret = mhttp_request_read_body(req, rb, body + total, 1);
	assert(ret == 1);
	assert(!memcmp(body + total, "9", 1));
	total += ret;

	ret = mhttp_request_read_body(req, rb, body + total, 10);
	assert(ret == 0);
	assert(total == 10);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
}

static void test_request_case_insensitive_header_name_lookup(void)
{
	struct mhttp_rbuf *rb = mhttp_rbuf_new(256);
	struct mhttp_request *req = mhttp_request_new();

	mhttp_rbuf_feed(
		rb,
		"GET / HTTP/1.1\r\n"
		"Content-Type: text/html\r\n"
		"X-Custom-Header: secret\r\n"
		"\r\n");

	assert(mhttp_request_parse(req, rb) == MHTTP_OK);

	assert(!strcmp(mhttp_request_get_header(req, "Content-Type"), "text/html"));
	assert(!strcmp(mhttp_request_get_header(req, "content-type"), "text/html"));
	assert(!strcmp(mhttp_request_get_header(req, "CONTENT-TYPE"), "text/html"));
	assert(!strcmp(mhttp_request_get_header(req, "cOnTeNt-TyPe"), "text/html"));

	assert(!strcmp(mhttp_request_get_header(req, "x-custom-header"), "secret"));
	assert(!strcmp(mhttp_request_get_header(req, "X-CUSTOM-HEADER"), "secret"));

	assert(mhttp_request_get_header(req, "Authorization") == NULL);

	mhttp_request_free(req);
	mhttp_rbuf_free(rb);
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

	test_request_body_full_in_rbuf();
	test_request_body_incremental();
	test_request_no_content_length();
	test_request_content_length_zero();
	test_request_body_clamped_to_content_length();
	test_request_body_rbuf_smaller_than_content();

	test_request_body_chunked_single();
	test_request_body_chunked_multiple();
	test_request_body_chunked_incremental();
	test_request_body_chunked_no_header();
	test_request_body_chunked_empty();

	test_request_body_chunked_no_header_data_yet();
	test_request_body_chunked_trailer_split();
	test_request_body_chunked_trailer_header();
	test_request_body_chunked_user_rbuf_small();
}
