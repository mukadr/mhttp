#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "response.h"

HttpResponse *http_response_new(void)
{
    HttpResponse *response = calloc(1, sizeof(*response));

    if (response) {
        response->status_code = HTTP_STATUS_OK;
        response->reason = "OK";
    }

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
    HttpHeader *header = calloc(1, sizeof(*header));

    if (!header) {
        return HTTP_INTERNAL_ERROR;
    }

    strncpy(header->name, name, sizeof(header->name) - 1);
    header->name[sizeof(header->name) - 1] = '\0';
    strncpy(header->value, value, sizeof(header->value) - 1);
    header->value[sizeof(header->value) - 1] = '\0';
    header->next = response->headers;
    response->headers = header;

    return HTTP_OK;
}

void http_response_set_body(HttpResponse *response, const char *body)
{
    free(response->body);
    response->body = NULL;
    response->body_length = 0;

    if (body) {
        size_t len = strlen(body);

        response->body = malloc(len + 1);
        if (response->body) {
            memcpy(response->body, body, len + 1);
            response->body_length = len;
        }

        if (response->body_length > 0) {
            char cl[32];

            snprintf(cl, sizeof(cl), "%d", response->body_length);
            http_response_set_header(response, "Content-Length", cl);
        }
    }
}

size_t http_response_write(HttpResponse *response, HttpBuffer *buffer)
{
    char status_line[64];

    snprintf(status_line, sizeof(status_line), "HTTP/1.1 %d %s\r\n", response->status_code, response->reason);

    size_t total = http_buffer_concat(buffer, status_line);

    HttpHeader *header = response->headers;

    while (header) {
        char line[520];

        snprintf(line, sizeof(line), "%s: %s\r\n", header->name, header->value);
        total += http_buffer_concat(buffer, line);
        header = header->next;
    }

    total += http_buffer_concat(buffer, "\r\n");

    if (response->body) {
        total += http_buffer_concat(buffer, response->body);
    }

    return total;
}
