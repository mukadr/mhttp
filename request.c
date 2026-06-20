#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include "rbuf.h"
#include "request.h"

#define CHUNK_NEED_HEADER (-1)
#define CHUNK_BODY_DONE (-2)
#define CHUNK_NEED_CRLF (-3)
#define CHUNK_TRAILER (-4)

struct mhttp_request *mhttp_request_new(void)
{
	struct mhttp_request *req;
       
	req = calloc(1, sizeof(*req));
	if (!req)
		return NULL;

	req->state = MHTTP_PARSE_REQUEST_LINE;
	return req;
}

void mhttp_request_free(struct mhttp_request *req)
{
	struct mhttp_header *hdr, *n;

	if (!req)
		return;

	hdr = req->headers;
	while (hdr) {
		n = hdr->next;
		free(hdr);
		hdr = n;
	}
	free(req);
}

static enum mhttp_result parse_uri(struct mhttp_request *req, struct mhttp_slice *line)
{
	size_t len = 0;
	int c;

	for (;;) {
		c = mhttp_slice_peek(line);
		if (c == '\r' || c == '\n' || c == -1)
			break;

		mhttp_slice_skip(line);

		if (c == ' ')
			break;

		if (c == 0)
			return MHTTP_INVALID_REQUEST;

		if (len == sizeof(req->uri) - 1)
			return MHTTP_INVALID_REQUEST;

		req->uri[len++] = (char)c;
	}
	req->uri[len] = '\0';

	return len ? MHTTP_OK : MHTTP_INVALID_REQUEST;
}

static enum mhttp_result parse_http_version(struct mhttp_request *req, struct mhttp_slice *line)
{
	int c;

	if (mhttp_slice_match(line, "HTTP/")) {
		c = mhttp_slice_peek(line);
		if (c < '0' || c > '9')
			return MHTTP_INVALID_REQUEST;

		req->version = c - '0';

		c = mhttp_slice_next(line);
		if (c != '.')
			return MHTTP_INVALID_REQUEST;

		c = mhttp_slice_next(line);
		if (c < '0' || c > '9')
			return MHTTP_INVALID_REQUEST;

		req->version = (req->version * 10) + (c - '0');

		mhttp_slice_skip(line);
	} else if (mhttp_slice_eol(line)) {
		// HTTP/0.9 (no version present)
		req->version = 9;
	}

	if (!mhttp_slice_eol(line))
		return MHTTP_INVALID_REQUEST;

	return MHTTP_OK;
}

static enum mhttp_result parse_method(struct mhttp_request *req, struct mhttp_rbuf *rb)
{
	struct mhttp_slice line;
	enum mhttp_result res;
	
	line = mhttp_rbuf_next_line(rb);
	if (mhttp_slice_empty(&line))
		return MHTTP_NEED_MORE_INPUT;

	if (mhttp_slice_match(&line, "GET "))
		req->method = MHTTP_METHOD_GET;
	else if (mhttp_slice_match(&line, "HEAD "))
		req->method = MHTTP_METHOD_HEAD;
	else if (mhttp_slice_match(&line, "POST "))
		req->method = MHTTP_METHOD_POST;
	else if (mhttp_slice_match(&line, "PUT "))
		req->method = MHTTP_METHOD_PUT;
	else if (mhttp_slice_match(&line, "DELETE "))
		req->method = MHTTP_METHOD_DELETE;
	else if (mhttp_slice_match(&line, "OPTIONS "))
		req->method = MHTTP_METHOD_OPTIONS;
	else if (mhttp_slice_match(&line, "PATCH "))
		req->method = MHTTP_METHOD_PATCH;
	else
		return MHTTP_INVALID_REQUEST;

	res = parse_uri(req, &line);
	if (res != MHTTP_OK)
		return res;

	return parse_http_version(req, &line);
}

static enum mhttp_result parse_header(struct mhttp_request *req, struct mhttp_slice *line, struct mhttp_header **out_hdr)
{
	struct mhttp_header *hdr;
	int c;
	size_t name_len, value_len;

	hdr = calloc(1, sizeof(*hdr));
	if (!hdr)
		return MHTTP_ERROR;

	name_len = 0;
	for (;;) {
		c = mhttp_slice_peek(line);
		if (c == ':')
			break;
		if (c == '\r' || c == '\n' || c == -1 || c == 0)
			goto invalid;

		if (name_len == sizeof(hdr->name) - 1)
			goto invalid;

		hdr->name[name_len++] = (char)c;
		mhttp_slice_skip(line);
	}
	if (name_len == 0)
		goto invalid;

	c = mhttp_slice_next(line);
	if (c != ' ')
		goto invalid;

	value_len = 0;
	for (;;) {
		c = mhttp_slice_next(line);
		if (c == '\r' || c == '\n' || c == -1)
			break;
		if (c == 0)
			goto invalid;

		if (value_len == sizeof(hdr->value) - 1)
			goto invalid;

		hdr->value[value_len++] = (char)c;
	}

	if (!mhttp_slice_eol(line))
		goto invalid;

	*out_hdr = hdr;
	return MHTTP_OK;

invalid:
	free(hdr);
	return MHTTP_INVALID_REQUEST;
}

static enum mhttp_result parse_headers(struct mhttp_request *req, struct mhttp_rbuf *rb)
{
	struct mhttp_slice line;
	struct mhttp_header *hdr;
	enum mhttp_result res;
	int count;

	count = mhttp_request_header_count(req);

	for (;;) {
		hdr = NULL;

		line = mhttp_rbuf_next_line(rb);
		if (mhttp_slice_empty(&line))
			return MHTTP_NEED_MORE_INPUT;

		if (mhttp_slice_eol(&line))
			break;

		if (count >= MHTTP_MAX_HEADERS)
			return MHTTP_INVALID_REQUEST;

		res = parse_header(req, &line, &hdr);
		if (res != MHTTP_OK)
			return res;

		hdr->next = req->headers;
		req->headers = hdr;
		count++;
	}

	return MHTTP_OK;
}

enum mhttp_result mhttp_request_parse(struct mhttp_request *req, struct mhttp_rbuf *rb)
{
	enum mhttp_result res;
	const char *cl, *te;

	if (req->state == MHTTP_PARSE_REQUEST_LINE) {
		res = parse_method(req, rb);
		if (res != MHTTP_OK)
			return res;
		req->state = MHTTP_PARSE_HEADERS;
	}

	if (req->state == MHTTP_PARSE_HEADERS) {
		res = parse_headers(req, rb);
		if (res != MHTTP_OK)
			return res;
		req->state = MHTTP_PARSE_DONE;

		cl = mhttp_request_get_header(req, "Content-Length");
		if (cl)
			req->content_length = atoi(cl);

		te = mhttp_request_get_header(req, "Transfer-Encoding");
		if (te && !strcasecmp(te, "chunked")) {
			req->body_is_chunked = true;
			req->chunk_size = CHUNK_NEED_HEADER;
		}
	}

	return MHTTP_OK;
}

int mhttp_request_content_length(const struct mhttp_request *req)
{
	return req->content_length;
}

static int parse_hex_digit(int c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

static int parse_chunk_size(struct mhttp_slice *line)
{
	int size = 0;
	int c, d;

	for (;;) {
		c = mhttp_slice_peek(line);
		d = parse_hex_digit(c);
		if (d < 0)
			break;
		mhttp_slice_skip(line);
		size = (size * 16) + d;
	}

	return size;
}

static size_t mhttp_request_read_chunked_body(struct mhttp_request *req, struct mhttp_rbuf *rb, void *dst, size_t len)
{
	struct mhttp_slice line;
	size_t total = 0, available, to_copy;
	int size;
	char *out = dst;

	if (req->body_done)
		return 0;

	while (total < len) {
		if (req->chunk_size == CHUNK_BODY_DONE) {
			req->body_done = true;
			break;
		}

		if (req->chunk_size == CHUNK_NEED_HEADER) {
			line = mhttp_rbuf_next_line(rb);
			if (mhttp_slice_empty(&line))
				break;

			size = parse_chunk_size(&line);
			if (size == 0)
				req->chunk_size = CHUNK_TRAILER;
			else
				req->chunk_size = size;

			continue;
		}

		if (req->chunk_size == CHUNK_TRAILER) {
			line = mhttp_rbuf_next_line(rb);
			if (mhttp_slice_empty(&line))
				break;

			if (mhttp_slice_eol(&line))
				req->chunk_size = CHUNK_BODY_DONE;

			continue;
		}

		if (req->chunk_size == CHUNK_NEED_CRLF) {
			line = mhttp_rbuf_next_line(rb);
			if (mhttp_slice_empty(&line))
				break;

			req->chunk_size = CHUNK_NEED_HEADER;
			continue;
		}

		if (req->chunk_size > 0) {
			available = rb->end - rb->pos;
			if (available == 0)
				break;

			to_copy = len - total;
			if (to_copy > available)
				to_copy = available;
			if (to_copy > (size_t)req->chunk_size)
				to_copy = req->chunk_size;

			memcpy(out + total, rb->pos, to_copy);
			rb->pos += to_copy;
			req->body_received += to_copy;
			req->chunk_size -= to_copy;
			total += to_copy;

			if (req->chunk_size == 0)
				req->chunk_size = CHUNK_NEED_CRLF;
			if (total == len)
				break;
			continue;
		}
	}

	return total;
}

static size_t mhttp_request_read_body_with_content_length(struct mhttp_request *req, struct mhttp_rbuf *rb, void *dst, size_t len)
{
	size_t available, to_copy;
	int remaining;

	if (req->content_length == 0)
		return 0;

	remaining = req->content_length - req->body_received;
	if (remaining <= 0)
		return 0;

	available = rb->end - rb->pos;
	if (available == 0)
		return 0;

	to_copy = len < available ? len : available;
	if (to_copy > (size_t)remaining)
		to_copy = remaining;

	memcpy(dst, rb->pos, to_copy);
	rb->pos += to_copy;
	req->body_received += to_copy;

	return to_copy;
}

size_t mhttp_request_read_body(struct mhttp_request *req, struct mhttp_rbuf *rb, void *dst, size_t len)
{
	if (req->body_is_chunked)
		return mhttp_request_read_chunked_body(req, rb, dst, len);

	return mhttp_request_read_body_with_content_length(req, rb, dst, len);
}

int mhttp_request_header_count(const struct mhttp_request *req)
{
	const struct mhttp_header *hdr;
	int count = 0;

	for (hdr = req->headers; hdr; hdr = hdr->next)
		count++;

	return count;
}

const char *mhttp_request_get_header(const struct mhttp_request *req, const char *name)
{
	const struct mhttp_header *hdr;
       
	for (hdr = req->headers; hdr; hdr = hdr->next)
		if (!strcasecmp(hdr->name, name))
			return hdr->value;

	return NULL;
}
