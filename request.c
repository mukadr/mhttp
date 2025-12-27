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

        if (uri_len == sizeof(request->uri) - 1) {
            return HTTP_URI_TOO_LONG;
        }

        request->uri[uri_len++] = (char)c;
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

        slice_next(line);

        if (!slice_empty(line)) {
            return HTTP_ERROR;
        }
    } else if (slice_empty(line)) {
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

static HttpResult parse_method(HttpRequest *request, HttpSlice line)
{
    if (slice_match(&line, "GET ")) {
        return parse_method_get(request, line);
    }

    if (slice_match(&line, "HEAD ")) {
        return parse_method_head(request, line);
    }

    return HTTP_ERROR;
}

static HttpResult parse_header(HttpRequest *request, HttpSlice *line, HttpHeader **out_header)
{
    int c;

    HttpHeader *header = malloc(sizeof(*header));
    if (!header) {
        return HTTP_ERROR;
    }

    size_t name_len = 0;
    while (true) {
        c = slice_next(line);
        if (c == ':') {
            break;
        }
        if (c == '\r' || c == '\n' || c == -1) {
            free(header);
            return HTTP_ERROR;
        }
        if (name_len == sizeof(header->name) - 1) {
            free(header);
            return HTTP_ERROR;
        }
        header->name[name_len++] = (char)c;
    }
    header->name[name_len] = '\0';

    c = slice_next(line);
    if (c != ' ') {
        free(header);
        return HTTP_ERROR;
    }

    size_t value_len = 0;
    while (true) {
        c = slice_next(line);
        if (c == '\r' || c == '\n' || c == -1) {
            break;
        }
        if (value_len == sizeof(header->value) - 1) {
            free(header);
            return HTTP_ERROR;
        }
        header->value[value_len++] = (char)c;
    }
    header->value[value_len] = '\0';

    header->next = NULL;
    *out_header = header;

    return HTTP_OK;
}

static HttpResult parse_headers(HttpRequest *request, HttpBuffer *buffer)
{
    HttpHeader **header_ptr = &request->headers;

    while (true) {
        HttpResult ret;
        HttpHeader *header = NULL;

        HttpSlice line = http_buffer_next_line(buffer);
        if (!line.begin) {
            return HTTP_REQUIRES_MORE_DATA;
        }
        if (slice_empty(&line)) {
            break;
        }
        ret = parse_header(request, &line, &header);
        if (ret != HTTP_OK) {
            return ret;
        }

        *header_ptr = header;
        header_ptr = &header->next;
    }

    return HTTP_OK;
}

HttpResult http_request_parse(HttpRequest *request, HttpBuffer *buffer)
{
    HttpSlice line = http_buffer_next_line(buffer);
    if (!slice_len(&line)) {
        return HTTP_REQUIRES_MORE_DATA;
    }

    HttpResult ret = parse_method(request, line);
    if (ret != HTTP_OK) {
        return ret;
    }

    return parse_headers(request, buffer);
}