#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include "request.h"
#include "readbuf.h"

enum {
    CHUNK_NEED_HEADER = -1,
    CHUNK_BODY_DONE   = -2,
    CHUNK_NEED_CRLF   = -3,
    CHUNK_TRAILER     = -4
};

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
            return HTTP_INVALID_REQUEST;
        }

        if (len == sizeof(request->uri) - 1) {
            return HTTP_INVALID_REQUEST;
        }

        request->uri[len++] = (char)c;
    }

    request->uri[len] = '\0';

    return len ? HTTP_OK : HTTP_INVALID_REQUEST;
}

static HttpResult parse_http_version(HttpRequest *request, HttpSlice *line)
{
    if (slice_match(line, "HTTP/")) {
        int c = slice_peek(line);
        if (c < '0' || c > '9') {
            return HTTP_INVALID_REQUEST;
        }

        request->version = c - '0';

        c = slice_next(line);
        if (c != '.') {
            return HTTP_INVALID_REQUEST;
        }

        c = slice_next(line);
        if (c < '0' || c > '9') {
            return HTTP_INVALID_REQUEST;
        }

        request->version = request->version * 10 + c - '0';

        slice_skip(line);

        if (!slice_eol(line)) {
            return HTTP_INVALID_REQUEST;
        }
    } else if (slice_eol(line)) {
        // HTTP/0.9 (no version present)
        request->version = 9;
    } else {
        return HTTP_INVALID_REQUEST;
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

static HttpResult parse_method_post(HttpRequest *request, HttpSlice *line)
{
    request->method = HTTP_METHOD_POST;

    HttpResult ret = parse_uri(request, line);
    if (ret != HTTP_OK) {
        return ret;
    }

    return parse_http_version(request, line);
}

static HttpResult parse_method_put(HttpRequest *request, HttpSlice *line)
{
    request->method = HTTP_METHOD_PUT;

    HttpResult ret = parse_uri(request, line);
    if (ret != HTTP_OK) {
        return ret;
    }

    return parse_http_version(request, line);
}

static HttpResult parse_method_delete(HttpRequest *request, HttpSlice *line)
{
    request->method = HTTP_METHOD_DELETE;

    HttpResult ret = parse_uri(request, line);
    if (ret != HTTP_OK) {
        return ret;
    }

    return parse_http_version(request, line);
}

static HttpResult parse_method_options(HttpRequest *request, HttpSlice *line)
{
    request->method = HTTP_METHOD_OPTIONS;

    HttpResult ret = parse_uri(request, line);
    if (ret != HTTP_OK) {
        return ret;
    }

    return parse_http_version(request, line);
}

static HttpResult parse_method_patch(HttpRequest *request, HttpSlice *line)
{
    request->method = HTTP_METHOD_PATCH;

    HttpResult ret = parse_uri(request, line);
    if (ret != HTTP_OK) {
        return ret;
    }

    return parse_http_version(request, line);
}

static HttpResult parse_method(HttpRequest *request, HttpReadBuf *buffer)
{
    HttpSlice line = http_readbuf_next_line(buffer);
    if (slice_empty(&line)) {
        return HTTP_NEED_MORE_INPUT;
    }

    if (slice_match(&line, "GET ")) {
        return parse_method_get(request, &line);
    }

    if (slice_match(&line, "HEAD ")) {
        return parse_method_head(request, &line);
    }

    if (slice_match(&line, "POST ")) {
        return parse_method_post(request, &line);
    }

    if (slice_match(&line, "PUT ")) {
        return parse_method_put(request, &line);
    }

    if (slice_match(&line, "DELETE ")) {
        return parse_method_delete(request, &line);
    }

    if (slice_match(&line, "OPTIONS ")) {
        return parse_method_options(request, &line);
    }

    if (slice_match(&line, "PATCH ")) {
        return parse_method_patch(request, &line);
    }

    return HTTP_INVALID_REQUEST;
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
            goto err_bad_request;
        }
        if (c == 0) {
            goto err_bad_request;
        }
        if (name_len == sizeof(header->name) - 1) {
            goto err_bad_request;
        }
        header->name[name_len++] = (char)c;
        slice_skip(line);
    }

    if (name_len == 0) {
        goto err_bad_request;
    }
    c = slice_next(line);
    if (c != ' ') {
        goto err_bad_request;
    }

    size_t value_len = 0;
    while (true) {
        c = slice_next(line);
        if (c == '\r' || c == '\n' || c == -1) {
            break;
        }
        if (c == 0) {
            goto err_bad_request;
        }
        if (value_len == sizeof(header->value) - 1) {
            goto err_bad_request;
        }
        header->value[value_len++] = (char)c;
    }

    if (!slice_eol(line)) {
        goto err_bad_request;
    }

    *out_header = header;

    return HTTP_OK;

err_bad_request:
    free(header);
    return HTTP_INVALID_REQUEST;
}

#define HTTP_MAX_HEADERS 100

static HttpResult parse_headers(HttpRequest *request, HttpReadBuf *buffer)
{
    HttpHeader **header_ptr = &request->headers;
    int count = http_request_header_count(request);

    while (true) {
        HttpResult ret;
        HttpHeader *header = NULL;

        HttpSlice line = http_readbuf_next_line(buffer);
        if (slice_empty(&line)) {
            return HTTP_NEED_MORE_INPUT;
        }
        if (slice_eol(&line)) {
            break;
        }
        if (count >= HTTP_MAX_HEADERS) {
            return HTTP_INVALID_REQUEST;
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

HttpResult http_request_parse(HttpRequest *request, HttpReadBuf *buffer)
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

        const char *cl = http_request_get_header(request, "Content-Length");
        if (cl) {
            request->content_length = atoi(cl);
        }

        const char *te = http_request_get_header(request, "Transfer-Encoding");
        if (te && !strcasecmp(te, "chunked")) {
            request->body_is_chunked = true;
            request->chunk_size = CHUNK_NEED_HEADER;
        }
    }

    return HTTP_OK;
}

int http_request_content_length(const HttpRequest *request)
{
    return request->content_length;
}

static int parse_hex_digit(int c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

static int parse_chunk_size(HttpSlice *line)
{
    int size = 0;

    while (true) {
        int c = slice_peek(line);
        int d = parse_hex_digit(c);
        if (d < 0) {
            break;
        }
        slice_skip(line);
        size = size * 16 + d;
    }

    return size;
}

static size_t http_request_read_chunked_body(HttpRequest *request, HttpReadBuf *buffer, void *dst, size_t len)
{
    if (request->body_done) {
        return 0;
    }

    size_t total = 0;
    char *out = dst;

    while (total < len) {
        if (request->chunk_size == CHUNK_BODY_DONE) {
            request->body_done = true;
            break;
        }

        if (request->chunk_size == CHUNK_NEED_HEADER) {
            HttpSlice line = http_readbuf_next_line(buffer);
            if (slice_empty(&line)) {
                break;
            }
            int size = parse_chunk_size(&line);
            if (size == 0) {
                request->chunk_size = CHUNK_TRAILER;
            } else {
                request->chunk_size = size;
            }
            continue;
        }

        if (request->chunk_size == CHUNK_TRAILER) {
            HttpSlice line = http_readbuf_next_line(buffer);
            if (slice_empty(&line)) {
                break;
            }
            if (slice_eol(&line)) {
                request->chunk_size = CHUNK_BODY_DONE;
            }
            continue;
        }

        if (request->chunk_size == CHUNK_NEED_CRLF) {
            HttpSlice line = http_readbuf_next_line(buffer);
            if (slice_empty(&line)) {
                break;
            }
            request->chunk_size = CHUNK_NEED_HEADER;
            continue;
        }

        if (request->chunk_size > 0) {
            size_t available = buffer->end - buffer->pos;
            if (available == 0) {
                break;
            }
            size_t to_copy = len - total;
            if (to_copy > available) {
                to_copy = available;
            }
            if (to_copy > (size_t)request->chunk_size) {
                to_copy = request->chunk_size;
            }

            memcpy(out + total, buffer->pos, to_copy);
            buffer->pos += to_copy;
            request->body_received += to_copy;
            request->chunk_size -= to_copy;
            total += to_copy;

            if (request->chunk_size == 0) {
                request->chunk_size = CHUNK_NEED_CRLF;
            }
            if (total == len) {
                break;
            }
            continue;
        }
    }

    return total;
}

static size_t http_request_read_body_with_content_length(HttpRequest *request, HttpReadBuf *buffer, void *dst, size_t len)
{
    if (request->content_length == 0) {
        return 0;
    }

    int remaining = request->content_length - request->body_received;
    if (remaining <= 0) {
        return 0;
    }

    size_t available = buffer->end - buffer->pos;
    if (available == 0) {
        return 0;
    }

    size_t to_copy = len < available ? len : available;
    if (to_copy > (size_t)remaining) {
        to_copy = remaining;
    }

    memcpy(dst, buffer->pos, to_copy);
    buffer->pos += to_copy;
    request->body_received += to_copy;

    return to_copy;
}

size_t http_request_read_body(HttpRequest *request, HttpReadBuf *buffer, void *dst, size_t len)
{
    if (request->body_is_chunked) {
        return http_request_read_chunked_body(request, buffer, dst, len);
    }

    return http_request_read_body_with_content_length(request, buffer, dst, len);
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
        if (!strcasecmp(header->name, name)) {
            return header->value;
        }
        header = header->next;
    }

    return NULL;
}
