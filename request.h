#ifndef MHTTP_REQUEST_H
#define MHTTP_REQUEST_H

#include <stdbool.h>

#include "http.h"
#include "readbuf.h"

typedef enum HttpParseState {
    HTTP_PARSE_REQUEST_LINE,
    HTTP_PARSE_HEADERS,
    HTTP_PARSE_DONE
} HttpParseState;

typedef struct HttpRequest {
    HttpParseState state;
    int version;
    HttpMethod method;
    char uri[256];
    HttpHeader *headers;
    int content_length;
    int body_received;
    bool body_is_chunked;
    int chunk_size;
    bool body_done;
} HttpRequest;

HttpRequest *http_request_new(void);

void http_request_free(HttpRequest *request);

HttpResult http_request_parse(HttpRequest *request, HttpReadBuf *buffer);

int http_request_header_count(const HttpRequest *request);

const char *http_request_get_header(const HttpRequest *request, const char *name);

int http_request_content_length(const HttpRequest *request);

size_t http_request_read_body(HttpRequest *request, HttpReadBuf *buffer, void *dst, size_t len);

static inline bool http_request_is_chunked(const HttpRequest *request)
{
    return request->body_is_chunked;
}

#endif // MHTTP_REQUEST_H