#include <string.h>

#include "request.h"
#include "buffer.h"

static HttpResult http_request_parse_method(HttpRequest *request, HttpSlice line)
{
    if (slice_len(&line) < 4) {
        return HTTP_ERROR;
    }

    if (!memcmp(line.begin, "GET ", 4)) {
        request->method = HTTP_GET;

        slice_advance(&line, 4);

        size_t uri_len = 0;
        while (1) {
            if (uri_len == sizeof(request->uri) - 1) {
                return HTTP_URI_TOO_LONG;
            }

            int c = slice_next(&line);
            if (c == ' ' || c == '\r' || c == '\n') {
                break;
            }

            request->uri[uri_len++] = (char)c;
        }

        request->uri[uri_len] = '\0';

        return HTTP_OK;
    }

    return HTTP_ERROR;
}

HttpResult http_request_parse(HttpRequest *request, HttpBuffer *buffer)
{
    HttpSlice line = http_buffer_next_line(buffer);

    if (!slice_len(&line)) {
        return HTTP_REQUIRES_MORE_DATA;
    }

    return http_request_parse_method(request, line);
}