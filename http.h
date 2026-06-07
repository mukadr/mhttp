#ifndef MHTTP_HTTP_H
#define MHTTP_HTTP_H

typedef enum HttpResult {
    HTTP_OK,
    HTTP_NEED_MORE_INPUT,
    HTTP_BAD_REQUEST,
    HTTP_INTERNAL_ERROR
} HttpResult;

typedef enum HttpMethod {
    HTTP_METHOD_GET,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_PATCH
} HttpMethod;

typedef struct HttpHeader {
    char name[256];
    char value[256];
    struct HttpHeader *next;
} HttpHeader;

#endif // MHTTP_HTTP_H