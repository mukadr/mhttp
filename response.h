#ifndef MHTTP_RESPONSE_H
#define MHTTP_RESPONSE_H

#include "http.h"
#include "buffer.h"

typedef struct HttpResponse {
    int status_code;
    const char *reason;
    HttpHeader *headers;
    char *body;
    int body_length;
} HttpResponse;

HttpResponse *http_response_new(void);

void http_response_free(HttpResponse *response);

void http_response_set_status(HttpResponse *response, int code);

HttpResult http_response_set_header(HttpResponse *response, const char *name, const char *value);

void http_response_set_body(HttpResponse *response, const char *body);

size_t http_response_write(HttpResponse *response, HttpBuffer *buffer);

#endif // MHTTP_RESPONSE_H
