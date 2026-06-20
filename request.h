#ifndef MHTTP_REQUEST_H
#define MHTTP_REQUEST_H

#include <stdbool.h>

#include "http.h"
#include "rbuf.h"

enum mhttp_parse_state {
	MHTTP_PARSE_REQUEST_LINE,
	MHTTP_PARSE_HEADERS,
	MHTTP_PARSE_DONE
};

struct mhttp_request {
	enum mhttp_parse_state state;
	int version;
	enum mhttp_method method;
	char uri[256];
	struct mhttp_header *headers;
	int content_length;
	int body_received;
	bool body_is_chunked;
	int chunk_size;
	bool body_done;
};

struct mhttp_request *mhttp_request_new(void);

void mhttp_request_free(struct mhttp_request *req);

enum mhttp_result mhttp_request_parse(struct mhttp_request *req, struct mhttp_rbuf *buf);

int mhttp_request_header_count(const struct mhttp_request *req);

const char *mhttp_request_get_header(const struct mhttp_request *req, const char *name);

int mhttp_request_content_length(const struct mhttp_request *req);

size_t mhttp_request_read_body(struct mhttp_request *req, struct mhttp_rbuf *buf, void *dst, size_t len);

static inline bool mhttp_request_is_chunked(const struct mhttp_request *req)
{
	return req->body_is_chunked;
}

#endif // MHTTP_REQUEST_H
