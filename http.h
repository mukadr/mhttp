#ifndef MHTTP_HTTP_H
#define MHTTP_HTTP_H

typedef enum HttpResult {
    HTTP_OK,
    HTTP_REQUIRES_MORE_DATA,
    HTTP_URI_TOO_LONG,
    HTTP_ERROR
} HttpResult;

#ifdef __unix__
#include <sys/types.h>
#else
typedef int ssize_t;
#endif

#endif // MHTTP_HTTP_H