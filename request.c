#include <string.h>

#include "request.h"
#include "buffer.h"

HttpRequest *http_request_new(void)
{
    HttpRequest *request = calloc(1, sizeof(*request));

    if (request) {
        request->state = HTTP_PARSE_REQUEST_LINE;
    }

    return request;
}

void http_request_free(HttpRequest *request)
{
    if (!request) {
        return;
    }

    HttpHeader *header = request->headers;

    while (header) {
        HttpHeader *next = header->next;
        free(header);
        header = next;
    }

    free(request);
}

static HttpResult parse_uri(HttpRequest *request, HttpSlice *line)
{
    size_t len = 0;
    while (true) {
        int c = slice_peek(line);
        if (c == '\r' || c == '\n' || c == -1) {
            break;
        }

        slice_skip(line);

        if (c == ' ') {
            break;
        }

        if (c == 0) {
            return HTTP_BAD_REQUEST;
        }

        if (len == sizeof(request->uri) - 1) {
            return HTTP_BAD_REQUEST;
        }

        request->uri[len++] = (char)c;
    }

    request->uri[len] = '\0';

    return len ? HTTP_OK : HTTP_BAD_REQUEST;
}

static HttpResult parse_http_version(HttpRequest *request, HttpSlice *line)
{
    if (slice_match(line, "HTTP/")) {
        int c = slice_peek(line);
        if (c < '0' || c > '9') {
            return HTTP_BAD_REQUEST;
        }

        request->version = c - '0';

        c = slice_next(line);
        if (c != '.') {
            return HTTP_BAD_REQUEST;
        }

        c = slice_next(line);
        if (c < '0' || c > '9') {
            return HTTP_BAD_REQUEST;
        }

        request->version = request->version * 10 + c - '0';

        slice_skip(line);

        if (!slice_eol(line)) {
            return HTTP_BAD_REQUEST;
        }
    } else if (slice_eol(line)) {
        // HTTP/0.9 (no version present)
        request->version = 9;
    } else {
        return HTTP_BAD_REQUEST;
    }

    return HTTP_OK;
}

static HttpResult parse_method_get(HttpRequest *request, HttpSlice *line)
{
    request->method = HTTP_METHOD_GET;

    HttpResult ret = parse_uri(request, line);
    if (ret != HTTP_OK) {
        return ret;
    }

    return parse_http_version(request, line);
}

static HttpResult parse_method_head(HttpRequest *request, HttpSlice *line)
{
    request->method = HTTP_METHOD_HEAD;

    HttpResult ret = parse_uri(request, line);
    if (ret != HTTP_OK) {
        return ret;
    }

    return parse_http_version(request, line);
}

static HttpResult parse_method(HttpRequest *request, HttpBuffer *buffer)
{
    HttpSlice line = http_buffer_next_line(buffer);
    if (slice_empty(&line)) {
        return HTTP_NEED_MORE_INPUT;
    }

    if (slice_match(&line, "GET ")) {
        return parse_method_get(request, &line);
    }

    if (slice_match(&line, "HEAD ")) {
        return parse_method_head(request, &line);
    }

    return HTTP_BAD_REQUEST;
}

static HttpResult parse_header(HttpRequest *request, HttpSlice *line, HttpHeader **out_header)
{
    int c;

    HttpHeader *header = calloc(1, sizeof(*header));
    if (!header) {
        return HTTP_INTERNAL_ERROR;
    }

    size_t name_len = 0;
    while (true) {
        c = slice_peek(line);
        if (c == ':') {
            break;
        }
        if (c == '\r' || c == '\n' || c == -1) {
            free(header);
            return HTTP_BAD_REQUEST;
        }
        if (c == 0) {
            free(header);
            return HTTP_BAD_REQUEST;
        }
        if (name_len == sizeof(header->name) - 1) {
            free(header);
            return HTTP_BAD_REQUEST;
        }
        header->name[name_len++] = (char)c;
        slice_skip(line);
    }

    if (name_len == 0) {
        free(header);
        return HTTP_BAD_REQUEST;
    }
    c = slice_next(line);
    if (c != ' ') {
        free(header);
        return HTTP_BAD_REQUEST;
    }

    size_t value_len = 0;
    while (true) {
        c = slice_next(line);
        if (c == '\r' || c == '\n' || c == -1) {
            break;
        }
        if (c == 0) {
            free(header);
            return HTTP_BAD_REQUEST;
        }
        if (value_len == sizeof(header->value) - 1) {
            free(header);
            return HTTP_BAD_REQUEST;
        }
        header->value[value_len++] = (char)c;
    }

    if (!slice_eol(line)) {
        free(header);
        return HTTP_BAD_REQUEST;
    }

    *out_header = header;

    return HTTP_OK;
}

#define HTTP_MAX_HEADERS 100

static HttpResult parse_headers(HttpRequest *request, HttpBuffer *buffer)
{
    HttpHeader **header_ptr = &request->headers;
    int count = 0;

    while (true) {
        HttpResult ret;
        HttpHeader *header = NULL;

        HttpSlice line = http_buffer_next_line(buffer);
        if (slice_empty(&line)) {
            return HTTP_NEED_MORE_INPUT;
        }
        if (slice_eol(&line)) {
            break;
        }
        if (count >= HTTP_MAX_HEADERS) {
            return HTTP_BAD_REQUEST;
        }
        ret = parse_header(request, &line, &header);
        if (ret != HTTP_OK) {
            return ret;
        }

        header->next = *header_ptr;
        *header_ptr = header;
        count++;
    }

    return HTTP_OK;
}

HttpResult http_request_parse(HttpRequest *request, HttpBuffer *buffer)
{
    if (request->state == HTTP_PARSE_REQUEST_LINE) {
        HttpResult ret = parse_method(request, buffer);
        if (ret != HTTP_OK) {
            return ret;
        }
        request->state = HTTP_PARSE_HEADERS;
    }

    if (request->state == HTTP_PARSE_HEADERS) {
        HttpResult ret = parse_headers(request, buffer);
        if (ret != HTTP_OK) {
            return ret;
        }
        request->state = HTTP_PARSE_DONE;
    }

    return HTTP_OK;
}

int http_request_header_count(const HttpRequest *request)
{
    const HttpHeader *header = request->headers;
    int count = 0;

    while (header) {
        count++;
        header = header->next;
    }

    return count;
}

const char *http_request_get_header(const HttpRequest *request, const char *name)
{
    const HttpHeader *header = request->headers;

    while (header) {
        if (!strcmp(header->name, name)) {
            return header->value;
        }
        header = header->next;
    }

    return NULL;
}