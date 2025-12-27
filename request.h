#ifndef MHTTP_REQUEST_H
#define MHTTP_REQUEST_H

#include <stdbool.h>

#include "http.h"
#include "buffer.h"

typedef enum HttpMethod {
    HTTP_METHOD_GET,
    HTTP_METHOD_HEAD
} HttpMethod;

typedef struct HttpRequest {
    int http_major;
    int http_minor;
    HttpMethod method;
    char uri[256];
} HttpRequest;

HttpResult http_request_parse(HttpRequest *request, HttpBuffer *buffer);

#endif // MHTTP_REQUEST_H