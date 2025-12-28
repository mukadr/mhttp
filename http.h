#ifndef MHTTP_HTTP_H
#define MHTTP_HTTP_H

typedef enum HttpResult {
    HTTP_OK,
    HTTP_REQUIRES_MORE_DATA,
    HTTP_BAD_REQUEST,
    HTTP_INTERNAL_ERROR
} HttpResult;

#endif // MHTTP_HTTP_H