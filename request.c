#include <string.h>

#include "request.h"
#include "buffer.h"

static HttpResult parse_uri(HttpRequest *request, HttpSlice *line)
{
    size_t uri_len = 0;
    while (true) {
        int c = slice_next(line);
        if (c == ' ' || c == '\r' || c == '\n' || c == -1) {
            break;
        }

        request->uri[uri_len++] = (char)c;

        if (uri_len == sizeof(request->uri) - 1) {
            return HTTP_URI_TOO_LONG;
        }
    }

    request->uri[uri_len] = '\0';

    return uri_len ? HTTP_OK : HTTP_ERROR;
}

static HttpResult parse_http_version(HttpRequest *request, HttpSlice *line)
{
    if (slice_match(line, "HTTP/")) {
        int c = slice_next(line);
        if (c < '0' || c > '9') {
            return HTTP_ERROR;
        }

        int major = c - '0';

        c = slice_next(line);
        if (c != '.') {
            return HTTP_ERROR;
        }

        c = slice_next(line);
        if (c < '0' || c > '9') {
            return HTTP_ERROR;
        }

        int minor = c - '0';

        request->http_major = major;
        request->http_minor = minor;

        c = slice_next(line);
        if (c != '\r' && c != '\n' && c != -1) {
            return HTTP_ERROR;
        }
    } else if (slice_len(line) && (line->begin[0] == '\r' || line->begin[0] == '\n')) {
        // HTTP/0.9 (no version present)
        request->http_major = 0;
        request->http_minor = 9;
    } else {
        return HTTP_ERROR;
    }

    return HTTP_OK;
}

static HttpResult parse_method_get(HttpRequest *request, HttpSlice line)
{
    request->method = HTTP_METHOD_GET;

    HttpResult ret = parse_uri(request, &line);
    if (ret != HTTP_OK) {
        return ret;
    }

    return parse_http_version(request, &line);
}

static HttpResult parse_method_head(HttpRequest *request, HttpSlice line)
{
    request->method = HTTP_METHOD_HEAD;

    HttpResult ret = parse_uri(request, &line);
    if (ret != HTTP_OK) {
        return ret;
    }

    return parse_http_version(request, &line);
}

static HttpResult http_request_parse_method(HttpRequest *request, HttpSlice line)
{
    if (slice_match(&line, "GET ")) {
        return parse_method_get(request, line);
    }

    if (slice_match(&line, "HEAD ")) {
        return parse_method_head(request, line);
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