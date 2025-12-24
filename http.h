#ifndef MHTTP_HTTP_H
#define MHTTP_HTTP_H

typedef enum HttpResult {
    HTTP_OK,
    HTTP_REQUIRES_MORE_DATA,
    HTTP_URI_TOO_LONG,
    HTTP_ERROR
} HttpResult;

#endif // MHTTP_HTTP_H