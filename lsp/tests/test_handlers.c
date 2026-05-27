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
 * Tests for LSP request handlers
 */
#include "test.h"
#include "../lsp.h"
#include <stdlib.h>
#include <string.h>

/* Helper to create a mock server */
static LSPServer *create_test_server(void) {
    LSPServer *server = calloc(1, sizeof(LSPServer));
    server->state = SERVER_UNINITIALIZED;
    return server;
}

static void free_test_server(LSPServer *server) {
    free(server->root_uri);
    for (int i = 0; i < server->workspace_folder_count; i++) {
        free(server->workspace_folders[i]);
    }
    free(server->workspace_folders);
    free(server);
}

TEST(initialize_basic) {
    LSPServer *server = create_test_server();

    cJSON *params = cJSON_CreateObject();
    cJSON *capabilities = cJSON_CreateObject();
    cJSON_AddItemToObject(params, "capabilities", capabilities);

    cJSON *result = handle_initialize(server, params);
    TEST_INIT;

    ASSERT_NOT_NULL(result);
    ASSERT_EQ(server->state, SERVER_INITIALIZING);

    /* Check server capabilities in response */
    cJSON *caps = cJSON_GetObjectItem(result, "capabilities");
    ASSERT_NOT_NULL(caps);
    ASSERT(cJSON_IsTrue(cJSON_GetObjectItem(caps, "hoverProvider")));
    ASSERT(cJSON_IsTrue(cJSON_GetObjectItem(caps, "definitionProvider")));
    ASSERT(cJSON_IsTrue(cJSON_GetObjectItem(caps, "referencesProvider")));
    ASSERT(cJSON_IsTrue(cJSON_GetObjectItem(caps, "documentSymbolProvider")));

    /* Check text document sync */
    cJSON *sync = cJSON_GetObjectItem(caps, "textDocumentSync");
    ASSERT_NOT_NULL(sync);
    ASSERT(cJSON_IsTrue(cJSON_GetObjectItem(sync, "openClose")));

    /* Check server info */
    cJSON *serverInfo = cJSON_GetObjectItem(result, "serverInfo");
    ASSERT_NOT_NULL(serverInfo);
    ASSERT_STREQ(cJSON_GetObjectItem(serverInfo, "name")->valuestring, "tbol-lsp");

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    free_test_server(server);
    TEST_FINI;
}

TEST(initialize_with_root_uri) {
    LSPServer *server = create_test_server();

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "rootUri", "file:///home/user/project");
    cJSON *capabilities = cJSON_CreateObject();
    cJSON_AddItemToObject(params, "capabilities", capabilities);

    cJSON *result = handle_initialize(server, params);
    TEST_INIT;

    ASSERT_NOT_NULL(server->root_uri);
    ASSERT_STREQ(server->root_uri, "file:///home/user/project");

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    free_test_server(server);
    TEST_FINI;
}

TEST(initialize_with_workspace_folders) {
    LSPServer *server = create_test_server();

    cJSON *params = cJSON_CreateObject();
    cJSON *capabilities = cJSON_CreateObject();
    cJSON_AddItemToObject(params, "capabilities", capabilities);

    cJSON *folders = cJSON_CreateArray();
    cJSON *folder1 = cJSON_CreateObject();
    cJSON_AddStringToObject(folder1, "uri", "file:///folder1");
    cJSON_AddStringToObject(folder1, "name", "folder1");
    cJSON_AddItemToArray(folders, folder1);
    cJSON *folder2 = cJSON_CreateObject();
    cJSON_AddStringToObject(folder2, "uri", "file:///folder2");
    cJSON_AddStringToObject(folder2, "name", "folder2");
    cJSON_AddItemToArray(folders, folder2);
    cJSON_AddItemToObject(params, "workspaceFolders", folders);

    cJSON *result = handle_initialize(server, params);
    TEST_INIT;

    ASSERT_EQ(server->workspace_folder_count, 2);
    ASSERT_STREQ(server->workspace_folders[0], "file:///folder1");
    ASSERT_STREQ(server->workspace_folders[1], "file:///folder2");

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    free_test_server(server);
    TEST_FINI;
}

TEST(initialized_transitions_to_running) {
    LSPServer *server = create_test_server();
    server->state = SERVER_INITIALIZING;
    TEST_INIT;

    handle_initialized(server, NULL);

    ASSERT_EQ(server->state, SERVER_RUNNING);

cleanup:
    free_test_server(server);
    TEST_FINI;
}

TEST(shutdown_transitions_to_shutdown) {
    LSPServer *server = create_test_server();
    server->state = SERVER_RUNNING;

    cJSON *result = handle_shutdown(server, NULL);
    TEST_INIT;

    ASSERT_NULL(result);  /* shutdown returns null */
    ASSERT_EQ(server->state, SERVER_SHUTDOWN);

cleanup:
    free_test_server(server);
    TEST_FINI;
}

TEST(completion_returns_keywords) {
    LSPServer *server = create_test_server();
    server->state = SERVER_RUNNING;

    cJSON *params = cJSON_CreateObject();
    cJSON *result = handle_completion(server, params);
    TEST_INIT;

    ASSERT_NOT_NULL(result);
    ASSERT(cJSON_IsArray(result));

    /* Check that we have some completions */
    int count = cJSON_GetArraySize(result);
    ASSERT(count > 0);

    /* Check first item has label and kind */
    cJSON *first = cJSON_GetArrayItem(result, 0);
    ASSERT_NOT_NULL(cJSON_GetObjectItem(first, "label"));
    ASSERT_NOT_NULL(cJSON_GetObjectItem(first, "kind"));

    /* Verify some expected keywords are present */
    bool found_move = false, found_if = false, found_program = false;
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(result, i);
        const char *label = cJSON_GetObjectItem(item, "label")->valuestring;
        if (strcmp(label, "MOVE") == 0) found_move = true;
        if (strcmp(label, "IF") == 0) found_if = true;
        if (strcmp(label, "PROGRAM") == 0) found_program = true;
    }
    ASSERT(found_move);
    ASSERT(found_if);
    ASSERT(found_program);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    free_test_server(server);
    TEST_FINI;
}

TEST(hover_returns_null_when_no_info) {
    LSPServer *server = create_test_server();
    server->state = SERVER_RUNNING;

    cJSON *params = cJSON_CreateObject();
    cJSON *result = handle_hover(server, params);
    TEST_INIT;

    /* For now, hover returns null (no implementation) */
    ASSERT_NULL(result);

cleanup:
    cJSON_Delete(params);
    free_test_server(server);
    TEST_FINI;
}

TEST(definition_returns_null_when_not_found) {
    LSPServer *server = create_test_server();
    server->state = SERVER_RUNNING;

    cJSON *params = cJSON_CreateObject();
    cJSON *result = handle_definition(server, params);
    TEST_INIT;

    ASSERT_NULL(result);

cleanup:
    cJSON_Delete(params);
    free_test_server(server);
    TEST_FINI;
}

TEST(references_returns_empty_array) {
    LSPServer *server = create_test_server();
    server->state = SERVER_RUNNING;

    cJSON *params = cJSON_CreateObject();
    cJSON *result = handle_references(server, params);
    TEST_INIT;

    ASSERT_NOT_NULL(result);
    ASSERT(cJSON_IsArray(result));
    ASSERT_EQ(cJSON_GetArraySize(result), 0);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    free_test_server(server);
    TEST_FINI;
}

TEST(document_symbol_returns_empty_array) {
    LSPServer *server = create_test_server();
    server->state = SERVER_RUNNING;

    cJSON *params = cJSON_CreateObject();
    cJSON *result = handle_document_symbol(server, params);
    TEST_INIT;

    ASSERT_NOT_NULL(result);
    ASSERT(cJSON_IsArray(result));
    ASSERT_EQ(cJSON_GetArraySize(result), 0);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    free_test_server(server);
    TEST_FINI;
}

TEST_SUITE(handlers) {
    RUN_TEST(initialize_basic);
    RUN_TEST(initialize_with_root_uri);
    RUN_TEST(initialize_with_workspace_folders);
    RUN_TEST(initialized_transitions_to_running);
    RUN_TEST(shutdown_transitions_to_shutdown);
    RUN_TEST(completion_returns_keywords);
    RUN_TEST(hover_returns_null_when_no_info);
    RUN_TEST(definition_returns_null_when_not_found);
    RUN_TEST(references_returns_empty_array);
    RUN_TEST(document_symbol_returns_empty_array);
}
