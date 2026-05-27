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
 * LSP Server main loop
 */
#include "lsp.h"
#include <stdlib.h>
#include <string.h>

int g_log_level = LOG_INFO;

/*
 * Create a new LSP server
 */
LSPServer *server_new(FILE *in, FILE *out) {
    LSPServer *server = calloc(1, sizeof(LSPServer));
    server->state = SERVER_UNINITIALIZED;
    server->in = in;
    server->out = out;
    server->documents = docstore_new();
    server->copy_index = copy_index_new();
    server->unreferenced_style = 2;  /* Default: strikethrough */
    return server;
}

/*
 * Free server resources
 */
void server_free(LSPServer *server) {
    if (!server) return;

    free(server->root_uri);

    for (int i = 0; i < server->workspace_folder_count; i++) {
        free(server->workspace_folders[i]);
    }
    free(server->workspace_folders);

    for (int i = 0; i < server->include_path_count; i++) {
        free(server->include_paths[i]);
    }
    free(server->include_paths);

    docstore_free(server->documents);
    copy_index_free(server->copy_index);

    free(server);
}

/*
 * Dispatch a request/notification to the appropriate handler
 */
static void dispatch(LSPServer *server, cJSON *msg) {
    cJSON *id = cJSON_GetObjectItem(msg, "id");
    cJSON *method = cJSON_GetObjectItem(msg, "method");
    cJSON *params = cJSON_GetObjectItem(msg, "params");

    /* Check if this is a response to a server->client request */
    if (id && !method) {
        /* This is a response, not a request */
        cJSON *result = cJSON_GetObjectItem(msg, "result");
        cJSON *error = cJSON_GetObjectItem(msg, "error");

        int response_id = cJSON_IsNumber(id) ? (int)id->valuedouble : -1;
        log_debug("Received response to request id=%d\n", response_id);

        /* Check if this is the configuration response we're waiting for */
        if (response_id == server->pending_config_request_id && response_id > 0) {
            server->pending_config_request_id = 0;  /* Clear pending */
            if (error) {
                cJSON *msg = cJSON_GetObjectItem(error, "message");
                log_warn("Configuration request failed: %s\n",
                         cJSON_IsString(msg) ? msg->valuestring : "unknown error");
            } else {
                handle_configuration_response(server, result);
            }
        } else {
            log_debug("Ignoring unexpected response id=%d\n", response_id);
        }
        return;
    }

    if (!cJSON_IsString(method)) {
        log_error("Message has no method\n");
        return;
    }

    const char *m = method->valuestring;
    bool is_request = (id != NULL);

    log_debug("Dispatch: %s (request=%d)\n", m, is_request);

    /* Lifecycle methods */
    if (strcmp(m, "initialize") == 0) {
        if (server->state != SERVER_UNINITIALIZED) {
            cJSON *err = jsonrpc_error(id, LSP_ERROR_INVALID_REQUEST, "Already initialized");
            char *str = cJSON_PrintUnformatted(err);
            message_write(server->out, str);
            free(str);
            cJSON_Delete(err);
            return;
        }
        cJSON *result = handle_initialize(server, params);
        cJSON *response = jsonrpc_result(id, result);
        char *str = cJSON_PrintUnformatted(response);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(response);
        return;
    }

    if (strcmp(m, "initialized") == 0) {
        handle_initialized(server, params);
        return;
    }

    if (strcmp(m, "shutdown") == 0) {
        cJSON *result = handle_shutdown(server, params);
        cJSON *response = jsonrpc_result(id, result);
        char *str = cJSON_PrintUnformatted(response);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(response);
        return;
    }

    if (strcmp(m, "exit") == 0) {
        handle_exit(server, params);
        return;  /* handle_exit calls exit() */
    }

    /* Check that server is initialized for other methods */
    if (server->state != SERVER_RUNNING) {
        if (is_request) {
            cJSON *err = jsonrpc_error(id, LSP_ERROR_SERVER_NOT_INITIALIZED, "Server not initialized");
            char *str = cJSON_PrintUnformatted(err);
            message_write(server->out, str);
            free(str);
            cJSON_Delete(err);
        }
        return;
    }

    /* Text document notifications */
    if (strcmp(m, "textDocument/didOpen") == 0) {
        handle_did_open(server, params);
        return;
    }

    if (strcmp(m, "textDocument/didChange") == 0) {
        handle_did_change(server, params);
        return;
    }

    if (strcmp(m, "textDocument/didClose") == 0) {
        handle_did_close(server, params);
        return;
    }

    if (strcmp(m, "textDocument/didSave") == 0) {
        handle_did_save(server, params);
        return;
    }

    /* Language feature requests */
    if (strcmp(m, "textDocument/hover") == 0) {
        cJSON *result = handle_hover(server, params);
        cJSON *response = jsonrpc_result(id, result);
        char *str = cJSON_PrintUnformatted(response);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(response);
        return;
    }

    if (strcmp(m, "textDocument/completion") == 0) {
        cJSON *result = handle_completion(server, params);
        cJSON *response = jsonrpc_result(id, result);
        char *str = cJSON_PrintUnformatted(response);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(response);
        return;
    }

    if (strcmp(m, "textDocument/definition") == 0) {
        cJSON *result = handle_definition(server, params);
        cJSON *response = jsonrpc_result(id, result);
        char *str = cJSON_PrintUnformatted(response);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(response);
        return;
    }

    if (strcmp(m, "textDocument/references") == 0) {
        cJSON *result = handle_references(server, params);
        cJSON *response = jsonrpc_result(id, result);
        char *str = cJSON_PrintUnformatted(response);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(response);
        return;
    }

    if (strcmp(m, "textDocument/documentSymbol") == 0) {
        cJSON *result = handle_document_symbol(server, params);
        cJSON *response = jsonrpc_result(id, result);
        char *str = cJSON_PrintUnformatted(response);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(response);
        return;
    }

    if (strcmp(m, "workspace/symbol") == 0) {
        cJSON *result = handle_workspace_symbol(server, params);
        cJSON *response = jsonrpc_result(id, result);
        char *str = cJSON_PrintUnformatted(response);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(response);
        return;
    }

    if (strcmp(m, "textDocument/prepareRename") == 0) {
        cJSON *result = handle_prepare_rename(server, params);
        cJSON *response = jsonrpc_result(id, result);
        char *str = cJSON_PrintUnformatted(response);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(response);
        return;
    }

    if (strcmp(m, "textDocument/rename") == 0) {
        cJSON *result = handle_rename(server, params);
        cJSON *response = jsonrpc_result(id, result);
        char *str = cJSON_PrintUnformatted(response);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(response);
        return;
    }

    if (strcmp(m, "textDocument/signatureHelp") == 0) {
        cJSON *result = handle_signature_help(server, params);
        cJSON *response = jsonrpc_result(id, result);
        char *str = cJSON_PrintUnformatted(response);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(response);
        return;
    }

    if (strcmp(m, "textDocument/foldingRange") == 0) {
        cJSON *result = handle_folding_range(server, params);
        cJSON *response = jsonrpc_result(id, result);
        char *str = cJSON_PrintUnformatted(response);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(response);
        return;
    }

    if (strcmp(m, "textDocument/semanticTokens/full") == 0) {
        cJSON *result = handle_semantic_tokens_full(server, params);
        cJSON *response = jsonrpc_result(id, result);
        char *str = cJSON_PrintUnformatted(response);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(response);
        return;
    }

    if (strcmp(m, "textDocument/selectionRange") == 0) {
        cJSON *result = handle_selection_range(server, params);
        cJSON *response = jsonrpc_result(id, result);
        char *str = cJSON_PrintUnformatted(response);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(response);
        return;
    }

    if (strcmp(m, "textDocument/formatting") == 0) {
        cJSON *result = handle_formatting(server, params);
        cJSON *response = jsonrpc_result(id, result);
        char *str = cJSON_PrintUnformatted(response);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(response);
        return;
    }

    /* Unknown method */
    if (is_request) {
        log_warn("Unknown method: %s\n", m);
        cJSON *err = jsonrpc_error(id, LSP_ERROR_METHOD_NOT_FOUND, "Method not found");
        char *str = cJSON_PrintUnformatted(err);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(err);
    }
}

/*
 * Main server loop
 */
int server_run(LSPServer *server) {
    log_info("TBOL Language Server starting\n");

    while (1) {
        Message *msg = message_read(server->in);
        if (!msg) {
            log_error("Failed to read message, exiting\n");
            break;
        }

        cJSON *json = cJSON_Parse(msg->content);
        if (!json) {
            log_error("Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
            cJSON *err = jsonrpc_error(NULL, LSP_ERROR_PARSE_ERROR, "Parse error");
            char *str = cJSON_PrintUnformatted(err);
            message_write(server->out, str);
            free(str);
            cJSON_Delete(err);
            message_free(msg);
            continue;
        }

        dispatch(server, json);

        cJSON_Delete(json);
        message_free(msg);
    }

    return 0;
}
