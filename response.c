#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "response.h"

HttpResponse *http_response_new(void)
{
    HttpResponse *response = calloc(1, sizeof(*response));
    if (!response) {
        return NULL;
    }

    http_response_set_status(response, HTTP_STATUS_OK);
    return response;
}

void http_response_free(HttpResponse *response)
{
    if (!response) {
        return;
    }

    HttpHeader *header = response->headers;

    while (header) {
        HttpHeader *next = header->next;
        free(header);
        header = next;
    }

    free(response->body);
    free(response);
}

void http_response_set_status(HttpResponse *response, int code)
{
    response->status_code = code;

    switch (code) {
        case HTTP_STATUS_OK:
            response->reason = "OK";
            break;
        case HTTP_STATUS_BAD_REQUEST:
            response->reason = "Bad Request";
            break;
        case HTTP_STATUS_NOT_FOUND:
            response->reason = "Not Found";
            break;
        case HTTP_STATUS_INTERNAL_SERVER_ERROR:
            response->reason = "Internal Server Error";
            break;
        default:
            response->reason = "Unknown";
            break;
    }
}

HttpResult http_response_set_header(HttpResponse *response, const char *name, const char *value)
{
    size_t name_len = strlen(name);
    if (name_len >= HTTP_HEADER_NAME_SIZE) {
        return HTTP_INVALID_REQUEST;
    }

    size_t value_len = strlen(value);
    if (value_len >= HTTP_HEADER_VALUE_SIZE) {
        return HTTP_INVALID_REQUEST;
    }

    HttpHeader *header = calloc(1, sizeof(*header));
    if (!header) {
        return HTTP_INTERNAL_ERROR;
    }

    memcpy(header->name, name, name_len + 1);
    memcpy(header->value, value, value_len + 1);

    header->next = response->headers;
    response->headers = header;

    return HTTP_OK;
}

HttpResult http_response_set_body(HttpResponse *response, const char *body)
{
    HttpResult ret = HTTP_OK;

    free(response->body);
    response->body = NULL;
    response->body_length = 0;

    if (body) {
        size_t len = strlen(body);

        response->body = malloc(len + 1);
        if (!response->body) {
            return HTTP_INTERNAL_ERROR;
        }
        memcpy(response->body, body, len + 1);
        response->body_length = len;

        if (response->body_length > 0) {
            char cl[32];

            snprintf(cl, sizeof(cl), "%zu", response->body_length);
            ret = http_response_set_header(response, "Content-Length", cl);
        }
    }

    return ret;
}

HttpResult http_response_write(HttpResponse *response, HttpWriteBuf *buffer)
{
    char status_line[64];
    size_t slen;
    size_t written;
    int ret;

    ret = snprintf(status_line, sizeof(status_line), "HTTP/1.1 %d %s\r\n", response->status_code, response->reason);
    if (ret < 0) {
        return HTTP_INTERNAL_ERROR;
    }

    slen = (size_t)ret;

    written = http_writebuf_write(buffer, status_line);
    if (written < slen) {
        return HTTP_INTERNAL_ERROR;
    }

    HttpHeader *header = response->headers;

    while (header) {
        char line[520];
        size_t llen;

        llen = (size_t)snprintf(line, sizeof(line), "%s: %s\r\n", header->name, header->value);

        written = http_writebuf_write(buffer, line);
        if (written < llen) {
            return HTTP_INTERNAL_ERROR;
        }
        header = header->next;
    }

    written = http_writebuf_write(buffer, "\r\n");
    if (written < 2) {
        return HTTP_INTERNAL_ERROR;
    }

    if (response->body) {
        written = http_writebuf_write(buffer, response->body);
        if (written < response->body_length) {
            return HTTP_INTERNAL_ERROR;
        }
    }

    return HTTP_OK;
}
