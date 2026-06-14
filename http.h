#ifndef MHTTP_HTTP_H
#define MHTTP_HTTP_H

typedef enum HttpResult {
    HTTP_OK,
    HTTP_NEED_MORE_INPUT,
    HTTP_INVALID_REQUEST,
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

typedef enum HttpStatusCode {
    HTTP_STATUS_OK                    = 200,
    HTTP_STATUS_BAD_REQUEST           = 400,
    HTTP_STATUS_NOT_FOUND             = 404,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500
} HttpStatusCode;

#define HTTP_HEADER_NAME_SIZE 256
#define HTTP_HEADER_VALUE_SIZE 256

typedef struct HttpHeader {
    char name[HTTP_HEADER_NAME_SIZE];
    char value[HTTP_HEADER_VALUE_SIZE];
    struct HttpHeader *next;
} HttpHeader;

#endif // MHTTP_HTTP_H
