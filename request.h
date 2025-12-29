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

typedef enum HttpRequestState {
    HTTP_STATE_REQUEST_LINE,
    HTTP_STATE_HEADERS,
    HTTP_STATE_DONE
} HttpRequestState;

typedef struct HttpRequest {
    HttpRequestState state;
    int http_major;
    int http_minor;
    HttpMethod method;
    char uri[256];
    HttpHeader *headers;
} HttpRequest;

HttpRequest *http_request_new(void);

void http_request_free(HttpRequest *request);

HttpResult http_request_parse(HttpRequest *request, HttpBuffer *buffer);

#endif // MHTTP_REQUEST_H