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
 * Protocol-level tests for LSP
 * Tests full message round-trips
 */
#include "test.h"
#include "../lsp.h"
#include <stdlib.h>
#include <string.h>

/* Test that proper error codes are used */
TEST(error_codes_defined) {
    TEST_INIT;
    ASSERT_EQ(LSP_ERROR_PARSE_ERROR, -32700);
    ASSERT_EQ(LSP_ERROR_INVALID_REQUEST, -32600);
    ASSERT_EQ(LSP_ERROR_METHOD_NOT_FOUND, -32601);
    ASSERT_EQ(LSP_ERROR_INVALID_PARAMS, -32602);
    ASSERT_EQ(LSP_ERROR_INTERNAL_ERROR, -32603);
    ASSERT_EQ(LSP_ERROR_SERVER_NOT_INITIALIZED, -32002);
    ASSERT_EQ(LSP_ERROR_REQUEST_CANCELLED, -32800);
cleanup:
    TEST_FINI;
}

/* Test server state transitions */
TEST(server_state_lifecycle) {
    LSPServer *server = server_new(NULL, NULL);
    TEST_INIT;

    /* Initial state */
    ASSERT_EQ(server->state, SERVER_UNINITIALIZED);

    /* After initialize request */
    cJSON *init_params = cJSON_CreateObject();
    cJSON *caps = cJSON_CreateObject();
    cJSON_AddItemToObject(init_params, "capabilities", caps);

    cJSON *result = handle_initialize(server, init_params);
    ASSERT_EQ(server->state, SERVER_INITIALIZING);
    cJSON_Delete(result);
    cJSON_Delete(init_params);

    /* After initialized notification */
    handle_initialized(server, NULL);
    ASSERT_EQ(server->state, SERVER_RUNNING);

    /* After shutdown request */
    result = handle_shutdown(server, NULL);
    ASSERT_EQ(server->state, SERVER_SHUTDOWN);

cleanup:
    server_free(server);
    TEST_FINI;
}

/* Test initialize response structure matches LSP spec */
TEST(initialize_response_structure) {
    LSPServer *server = server_new(NULL, NULL);

    cJSON *params = cJSON_CreateObject();
    cJSON *caps = cJSON_CreateObject();
    cJSON_AddItemToObject(params, "capabilities", caps);

    cJSON *result = handle_initialize(server, params);
    TEST_INIT;

    /* Must have capabilities object */
    cJSON *serverCaps = cJSON_GetObjectItem(result, "capabilities");
    ASSERT_NOT_NULL(serverCaps);
    ASSERT(cJSON_IsObject(serverCaps));

    /* textDocumentSync must be present */
    cJSON *sync = cJSON_GetObjectItem(serverCaps, "textDocumentSync");
    ASSERT_NOT_NULL(sync);

    /* Completion provider if present must have proper structure */
    cJSON *completion = cJSON_GetObjectItem(serverCaps, "completionProvider");
    if (completion) {
        ASSERT(cJSON_IsObject(completion));
    }

    /* serverInfo is optional but if present must have name */
    cJSON *serverInfo = cJSON_GetObjectItem(result, "serverInfo");
    if (serverInfo) {
        ASSERT(cJSON_IsObject(serverInfo));
        ASSERT_NOT_NULL(cJSON_GetObjectItem(serverInfo, "name"));
    }

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

/* Test that completion items match LSP spec */
TEST(completion_item_structure) {
    LSPServer *server = server_new(NULL, NULL);
    server->state = SERVER_RUNNING;

    cJSON *params = cJSON_CreateObject();
    cJSON *result = handle_completion(server, params);
    TEST_INIT;

    ASSERT(cJSON_IsArray(result));

    /* Each item should have at least label and kind */
    for (int i = 0; i < cJSON_GetArraySize(result); i++) {
        cJSON *item = cJSON_GetArrayItem(result, i);
        ASSERT(cJSON_IsObject(item));

        cJSON *label = cJSON_GetObjectItem(item, "label");
        ASSERT_NOT_NULL(label);
        ASSERT(cJSON_IsString(label));

        cJSON *kind = cJSON_GetObjectItem(item, "kind");
        ASSERT_NOT_NULL(kind);
        ASSERT(cJSON_IsNumber(kind));
        /* Kind should be valid CompletionItemKind (1-25) */
        int k = (int)kind->valuedouble;
        ASSERT(k >= 1 && k <= 25);
    }

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

/* Test server new/free doesn't leak */
TEST(server_new_free) {
    TEST_INIT;
    for (int i = 0; i < 100; i++) {
        LSPServer *server = server_new(NULL, NULL);
        ASSERT_NOT_NULL(server);
        server_free(server);
    }
cleanup:
    TEST_FINI;
}

/* Test JSON-RPC message formatting */
TEST(jsonrpc_message_format) {
    cJSON *id = cJSON_CreateNumber(1);
    cJSON *result = cJSON_CreateString("ok");

    cJSON *response = jsonrpc_result(id, result);
    char *str = cJSON_PrintUnformatted(response);
    TEST_INIT;

    /* Must contain jsonrpc version */
    ASSERT(strstr(str, "\"jsonrpc\":\"2.0\"") != NULL);

    /* Must contain id */
    ASSERT(strstr(str, "\"id\":1") != NULL);

    /* Must contain result */
    ASSERT(strstr(str, "\"result\":\"ok\"") != NULL);

cleanup:
    free(str);
    cJSON_Delete(id);
    cJSON_Delete(response);
    TEST_FINI;
}

/* Test error response format */
TEST(jsonrpc_error_format) {
    cJSON *id = cJSON_CreateNumber(5);

    cJSON *response = jsonrpc_error(id, -32600, "Test error");
    char *str = cJSON_PrintUnformatted(response);
    TEST_INIT;

    /* Must contain jsonrpc version */
    ASSERT(strstr(str, "\"jsonrpc\":\"2.0\"") != NULL);

    /* Must contain error object with code and message */
    ASSERT(strstr(str, "\"error\":{") != NULL);
    ASSERT(strstr(str, "\"code\":-32600") != NULL);
    ASSERT(strstr(str, "\"message\":\"Test error\"") != NULL);

    /* Must NOT contain result */
    ASSERT(strstr(str, "\"result\"") == NULL);

cleanup:
    free(str);
    cJSON_Delete(id);
    cJSON_Delete(response);
    TEST_FINI;
}

TEST_SUITE(protocol) {
    RUN_TEST(error_codes_defined);
    RUN_TEST(server_state_lifecycle);
    RUN_TEST(initialize_response_structure);
    RUN_TEST(completion_item_structure);
    RUN_TEST(server_new_free);
    RUN_TEST(jsonrpc_message_format);
    RUN_TEST(jsonrpc_error_format);
}
