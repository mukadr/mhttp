#ifndef MHTTP_REQUEST_H
#define MHTTP_REQUEST_H

#include <stdbool.h>

#include "http.h"
#include "buffer.h"

typedef enum HttpMethod {
    HTTP_METHOD_GET = 0,
    HTTP_METHOD_HEAD
} HttpMethod;

typedef struct HttpHeader {
    char name[256];
    char value[256];
    struct HttpHeader *next;
} HttpHeader;

typedef enum HttpRequestState {
    HTTP_STATE_INITIAL = 0,
    HTTP_STATE_REQUEST_HEADERS,
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

HttpResult http_request_parse(HttpRequest *request, HttpBuffer *buffer);

#endif // MHTTP_REQUEST_H