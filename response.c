#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "response.h"

struct mhttp_response *mhttp_response_new(void)
{
	struct mhttp_response *rsp;
	
	rsp = calloc(1, sizeof(*rsp));
	if (!rsp)
		return NULL;

	mhttp_response_set_status(rsp, MHTTP_STATUS_OK);
	return rsp;
}

void mhttp_response_free(struct mhttp_response *rsp)
{
	struct mhttp_header *hdr, *n;

	if (!rsp)
		return;

	hdr = rsp->headers;
	while (hdr) {
		n = hdr->next;
		free(hdr);
		hdr = n;
	}

	free(rsp->body);
	free(rsp);
}

void mhttp_response_set_status(struct mhttp_response *rsp, int code)
{
	rsp->status_code = code;

	switch (code) {
	case MHTTP_STATUS_OK:
		rsp->reason = "OK";
		break;
	case MHTTP_STATUS_BAD_REQUEST:
		rsp->reason = "Bad Request";
		break;
	case MHTTP_STATUS_NOT_FOUND:
		rsp->reason = "Not Found";
		break;
	case MHTTP_STATUS_INTERNAL_SERVER_ERROR:
		rsp->reason = "Internal Server Error";
		break;
	default:
		rsp->reason = "Unknown";
	}
}

enum mhttp_result mhttp_response_set_header(struct mhttp_response *rsp, const char *name, const char *value)
{
	struct mhttp_header *hdr;
	size_t name_len, value_len;
       
	name_len = strlen(name);
	if (name_len >= MHTTP_HEADER_NAME_SIZE)
		return MHTTP_INVALID_REQUEST;

	value_len = strlen(value);
	if (value_len >= MHTTP_HEADER_VALUE_SIZE)
		return MHTTP_INVALID_REQUEST;

	hdr = calloc(1, sizeof(*hdr));
	if (!hdr)
		return MHTTP_ERROR;

	memcpy(hdr->name, name, name_len + 1);
	memcpy(hdr->value, value, value_len + 1);
	hdr->next = rsp->headers;
	rsp->headers = hdr;
	return MHTTP_OK;
}

enum mhttp_result mhttp_response_set_body(struct mhttp_response *rsp, const char *body)
{
	enum mhttp_result res = MHTTP_OK;
	size_t len;
	char cl[32];

	free(rsp->body);
	rsp->body = NULL;
	rsp->body_length = 0;

	if (body) {
		len = strlen(body);
		rsp->body = malloc(len + 1);
		if (!rsp->body)
			return MHTTP_ERROR;

		memcpy(rsp->body, body, len + 1);
		rsp->body_length = len;

		if (rsp->body_length > 0) {
			snprintf(cl, sizeof(cl), "%zu", rsp->body_length);
			res = mhttp_response_set_header(rsp, "Content-Length", cl);
		}
	}

	return res;
}

enum mhttp_result mhttp_response_write(struct mhttp_response *rsp, struct mhttp_wbuf *wb)
{
	struct mhttp_header *hdr;
	char status_line[64];
	size_t slen;
	size_t written;
	int ret;

	ret = snprintf(status_line, sizeof(status_line), "HTTP/1.1 %d %s\r\n", rsp->status_code, rsp->reason);
	if (ret < 0)
		return MHTTP_ERROR;

	slen = (size_t)ret;
	written = mhttp_wbuf_write(wb, status_line);
	if (written < slen)
		return MHTTP_ERROR;

	for (hdr = rsp->headers; hdr; hdr = hdr->next) {
		char line[520];
		size_t llen;

		llen = (size_t)snprintf(line, sizeof(line), "%s: %s\r\n", hdr->name, hdr->value);
		written = mhttp_wbuf_write(wb, line);
		if (written < llen)
			return MHTTP_ERROR;
	}

	written = mhttp_wbuf_write(wb, "\r\n");
	if (written < 2)
		return MHTTP_ERROR;

	if (rsp->body) {
		written = mhttp_wbuf_write(wb, rsp->body);
		if (written < rsp->body_length)
			return MHTTP_ERROR;
	}

	return MHTTP_OK;
}
