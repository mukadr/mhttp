#ifndef MHTTP_REQUEST_H
#define MHTTP_REQUEST_H

#include <stdbool.h>

#include "http.h"
#include "buffer.h"

typedef enum HttpMethod {
    HTTP_METHOD_GET,
    HTTP_METHOD_HEAD
} HttpMethod;

typedef struct HttpHeader {
    char name[256];
    char value[256];
    struct HttpHeader *next;
} HttpHeader;

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
} HttpRequest;

HttpRequest *http_request_new(void);

void http_request_free(HttpRequest *request);

HttpResult http_request_parse(HttpRequest *request, HttpBuffer *buffer);

int http_request_header_count(const HttpRequest *request);

const char *http_request_get_header(const HttpRequest *request, const char *name);

#endif // MHTTP_REQUEST_H