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
 * Tests for JSON-RPC message handling
 */
#include "test.h"
#include "../lsp.h"
#include <stdlib.h>
#include <string.h>

TEST(jsonrpc_result_with_object) {
    cJSON *id = cJSON_CreateNumber(1);
    cJSON *result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "name", "test");

    cJSON *response = jsonrpc_result(id, result);
    TEST_INIT;

    ASSERT_NOT_NULL(response);
    ASSERT_STREQ(cJSON_GetObjectItem(response, "jsonrpc")->valuestring, "2.0");
    ASSERT_EQ(cJSON_GetObjectItem(response, "id")->valuedouble, 1);
    ASSERT_NOT_NULL(cJSON_GetObjectItem(response, "result"));
    ASSERT_STREQ(cJSON_GetObjectItem(cJSON_GetObjectItem(response, "result"), "name")->valuestring, "test");

cleanup:
    cJSON_Delete(id);
    cJSON_Delete(response);
    TEST_FINI;
}

TEST(jsonrpc_result_null) {
    cJSON *id = cJSON_CreateNumber(42);

    cJSON *response = jsonrpc_result(id, NULL);
    TEST_INIT;

    ASSERT_NOT_NULL(response);
    ASSERT_STREQ(cJSON_GetObjectItem(response, "jsonrpc")->valuestring, "2.0");
    ASSERT(cJSON_IsNull(cJSON_GetObjectItem(response, "result")));

cleanup:
    cJSON_Delete(id);
    cJSON_Delete(response);
    TEST_FINI;
}

TEST(jsonrpc_error_basic) {
    cJSON *id = cJSON_CreateNumber(1);

    cJSON *response = jsonrpc_error(id, -32600, "Invalid request");
    TEST_INIT;

    ASSERT_NOT_NULL(response);
    ASSERT_STREQ(cJSON_GetObjectItem(response, "jsonrpc")->valuestring, "2.0");

    cJSON *error = cJSON_GetObjectItem(response, "error");
    ASSERT_NOT_NULL(error);
    ASSERT_EQ(cJSON_GetObjectItem(error, "code")->valuedouble, -32600);
    ASSERT_STREQ(cJSON_GetObjectItem(error, "message")->valuestring, "Invalid request");

cleanup:
    cJSON_Delete(id);
    cJSON_Delete(response);
    TEST_FINI;
}

TEST(jsonrpc_error_null_id) {
    cJSON *response = jsonrpc_error(NULL, -32700, "Parse error");
    TEST_INIT;

    ASSERT_NOT_NULL(response);
    ASSERT(cJSON_IsNull(cJSON_GetObjectItem(response, "id")));

cleanup:
    cJSON_Delete(response);
    TEST_FINI;
}

TEST(jsonrpc_notification) {
    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "uri", "file:///test.src");

    cJSON *notif = jsonrpc_notification("textDocument/didOpen", params);
    TEST_INIT;

    ASSERT_NOT_NULL(notif);
    ASSERT_STREQ(cJSON_GetObjectItem(notif, "jsonrpc")->valuestring, "2.0");
    ASSERT_STREQ(cJSON_GetObjectItem(notif, "method")->valuestring, "textDocument/didOpen");
    ASSERT_NULL(cJSON_GetObjectItem(notif, "id"));

cleanup:
    cJSON_Delete(notif);
    TEST_FINI;
}

TEST(jsonrpc_string_id) {
    cJSON *id = cJSON_CreateString("req-123");
    cJSON *result = cJSON_CreateBool(true);

    cJSON *response = jsonrpc_result(id, result);
    TEST_INIT;

    ASSERT_NOT_NULL(response);
    ASSERT_STREQ(cJSON_GetObjectItem(response, "id")->valuestring, "req-123");

cleanup:
    cJSON_Delete(id);
    cJSON_Delete(response);
    TEST_FINI;
}

TEST_SUITE(jsonrpc) {
    RUN_TEST(jsonrpc_result_with_object);
    RUN_TEST(jsonrpc_result_null);
    RUN_TEST(jsonrpc_error_basic);
    RUN_TEST(jsonrpc_error_null_id);
    RUN_TEST(jsonrpc_notification);
    RUN_TEST(jsonrpc_string_id);
}
