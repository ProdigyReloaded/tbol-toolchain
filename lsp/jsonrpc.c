/*
 * Copyright 2025-2026, Phillip Heller
 *
 * This file is part of Prodigy Reloaded.
 *
 * Prodigy Reloaded is free software: you can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * Prodigy Reloaded is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with Prodigy Reloaded. If not,
 * see <https://www.gnu.org/licenses/>.
 */
/*
 * JSON-RPC message handling for LSP
 */
#include "lsp.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * Read a JSON-RPC message from input stream
 *
 * LSP uses HTTP-like headers:
 *   Content-Length: <length>\r\n
 *   \r\n
 *   <json content>
 */
Message *message_read(FILE *in) {
    char line[256];
    int content_length = -1;

    /* Read headers */
    while (fgets(line, sizeof(line), in)) {
        /* Check for end of headers */
        if (strcmp(line, "\r\n") == 0 || strcmp(line, "\n") == 0) {
            break;
        }

        /* Parse Content-Length header */
        if (strncasecmp(line, "Content-Length:", 15) == 0) {
            content_length = atoi(line + 15);
        }
        /* Ignore other headers (Content-Type, etc.) */
    }

    if (content_length < 0) {
        log_error("No Content-Length header\n");
        return NULL;
    }

    /* Read content */
    char *content = malloc(content_length + 1);
    if (!content) {
        log_error("Failed to allocate %d bytes\n", content_length);
        return NULL;
    }

    size_t read = fread(content, 1, content_length, in);
    if (read != (size_t)content_length) {
        log_error("Expected %d bytes, got %zu\n", content_length, read);
        free(content);
        return NULL;
    }
    content[content_length] = '\0';

    Message *msg = malloc(sizeof(Message));
    msg->content = content;
    msg->length = content_length;

    log_debug("Received: %s\n", content);

    return msg;
}

void message_free(Message *msg) {
    if (msg) {
        free(msg->content);
        free(msg);
    }
}

/*
 * Write a JSON-RPC message to output stream
 */
void message_write(FILE *out, const char *content) {
    size_t length = strlen(content);

    log_debug("Sending: %s\n", content);

    fprintf(out, "Content-Length: %zu\r\n", length);
    fprintf(out, "\r\n");
    fwrite(content, 1, length, out);
    fflush(out);
}

/*
 * Build a JSON-RPC success response
 */
cJSON *jsonrpc_result(cJSON *id, cJSON *result) {
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "jsonrpc", "2.0");

    if (id) {
        cJSON_AddItemToObject(response, "id", cJSON_Duplicate(id, true));
    }

    if (result) {
        cJSON_AddItemToObject(response, "result", result);
    } else {
        cJSON_AddNullToObject(response, "result");
    }

    return response;
}

/*
 * Build a JSON-RPC error response
 */
cJSON *jsonrpc_error(cJSON *id, int code, const char *message) {
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "jsonrpc", "2.0");

    if (id) {
        cJSON_AddItemToObject(response, "id", cJSON_Duplicate(id, true));
    } else {
        cJSON_AddNullToObject(response, "id");
    }

    cJSON *error = cJSON_CreateObject();
    cJSON_AddNumberToObject(error, "code", code);
    cJSON_AddStringToObject(error, "message", message);
    cJSON_AddItemToObject(response, "error", error);

    return response;
}

/*
 * Build a JSON-RPC notification (no id, no response expected)
 */
cJSON *jsonrpc_notification(const char *method, cJSON *params) {
    cJSON *notif = cJSON_CreateObject();
    cJSON_AddStringToObject(notif, "jsonrpc", "2.0");
    cJSON_AddStringToObject(notif, "method", method);

    if (params) {
        cJSON_AddItemToObject(notif, "params", params);
    }

    return notif;
}

/*
 * Build a JSON-RPC request (server->client, expects response)
 */
cJSON *jsonrpc_request(int id, const char *method, cJSON *params) {
    cJSON *request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(request, "id", id);
    cJSON_AddStringToObject(request, "method", method);

    if (params) {
        cJSON_AddItemToObject(request, "params", params);
    }

    return request;
}
