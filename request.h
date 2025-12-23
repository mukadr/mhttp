#ifndef MHTTP_REQUEST_H
#define MHTTP_REQUEST_H

#include "buffer.h"

typedef enum HttpMethod {
    HTTP_UNKNOWN,
    HTTP_GET
} HttpMethod;

typedef struct HttpRequest {
    HttpMethod method;
    char uri[256];
} HttpRequest;

void http_request_parse(HttpRequest *request, HttpBuffer *buffer);

#endif // MHTTP_REQUEST_H