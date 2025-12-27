#include <string.h>

#include "request.h"
#include "buffer.h"

static HttpResult http_request_parse_method(HttpRequest *request, HttpSlice line)
{
    if (slice_match(&line, "GET ")) {
        request->method = HTTP_METHOD_GET;

        // parse URI
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

        if (!uri_len) {
            return HTTP_ERROR;
        }

        // parse HTTP version
        if (slice_match(&line, "HTTP/")) {
            int c = slice_next(&line);
            if (c < '0' || c > '9') {
                return HTTP_ERROR;
            }

            int major = c - '0';

            c = slice_next(&line);
            if (c != '.') {
                return HTTP_ERROR;
            }

            c = slice_next(&line);
            if (c < '0' || c > '9') {
                return HTTP_ERROR;
            }

            int minor = c - '0';

            request->http_major = major;
            request->http_minor = minor;

            c = slice_next(&line);
            if (c != '\r' && c != '\n' && c != -1) {
                return HTTP_ERROR;
            }
        } else if (slice_len(&line) && (line.begin[0] == '\r' || line.begin[0] == '\n')) {
            // HTTP/0.9 (no version present)
            request->http_major = 0;
            request->http_minor = 9;
        } else {
            return HTTP_ERROR;
        }

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