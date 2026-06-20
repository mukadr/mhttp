#ifndef MHTTP_RESPONSE_H
#define MHTTP_RESPONSE_H

#include <stdlib.h>

#include "http.h"
#include "wbuf.h"

struct mhttp_response {
	int status_code;
	const char *reason;
	struct mhttp_header *headers;
	char *body;
	size_t body_length;
};

struct mhttp_response *mhttp_response_new(void);

void mhttp_response_free(struct mhttp_response *rsp);

void mhttp_response_set_status(struct mhttp_response *rsp, int code);

enum mhttp_result mhttp_response_set_header(struct mhttp_response *rsp, const char *name, const char *value);

enum mhttp_result mhttp_response_set_body(struct mhttp_response *rsp, const char *body);

enum mhttp_result mhttp_response_write(struct mhttp_response *rsp, struct mhttp_wbuf *wb);

#endif // MHTTP_RESPONSE_H
