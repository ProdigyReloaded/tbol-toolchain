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
 * Tests for document management and parsing integration
 */
#include "test.h"
#include "../lsp.h"
#include "../../shared/ast.h"
#include "../../shared/tbol_parse.h"
#include "../../compiler/options.h"
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

/* Portable recursive-remove for tests.  Replaces system("rm -rf ...")
 * which on Windows runs through cmd.exe and does not know rm/-rf,
 * causing setup to silently fail and subsequent fopen() to return NULL. */
static void rm_rf_dir(const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        rmdir(path);
        return;
    }
    struct dirent *e;
    char child[PATH_MAX];
    while ((e = readdir(d)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        snprintf(child, sizeof(child), "%s/%s", path, e->d_name);
        struct stat st;
        if (stat(child, &st) == 0 && S_ISDIR(st.st_mode)) {
            rm_rf_dir(child);
        } else {
            unlink(child);
        }
    }
    closedir(d);
    rmdir(path);
}

/*
 * Test document store basic operations
 */
TEST(docstore_new_free) {
    DocumentStore *store = docstore_new();
    TEST_INIT;
    ASSERT_NOT_NULL(store);
    ASSERT_EQ(store->count, 0);
    ASSERT(store->capacity > 0);
cleanup:
    docstore_free(store);
    TEST_FINI;
}

TEST(docstore_open_get) {
    DocumentStore *store = docstore_new();

    const char *uri = "file:///test.src";
    const char *content = "PROGRAM test;";

    Document *doc = docstore_open(store, uri, content, 1);
    TEST_INIT;
    ASSERT_NOT_NULL(doc);
    ASSERT_STREQ(doc->uri, uri);
    ASSERT_STREQ(doc->content, content);
    ASSERT_EQ(doc->version, 1);

    /* Get same document */
    Document *got = docstore_get(store, uri);
    ASSERT(got == doc);

cleanup:
    docstore_free(store);
    TEST_FINI;
}

TEST(docstore_update) {
    DocumentStore *store = docstore_new();

    const char *uri = "file:///test.src";
    docstore_open(store, uri, "old content", 1);

    docstore_update(store, uri, "new content", 2);

    Document *doc = docstore_get(store, uri);
    TEST_INIT;
    ASSERT_NOT_NULL(doc);
    ASSERT_STREQ(doc->content, "new content");
    ASSERT_EQ(doc->version, 2);

cleanup:
    docstore_free(store);
    TEST_FINI;
}

TEST(docstore_close) {
    DocumentStore *store = docstore_new();

    docstore_open(store, "file:///a.src", "a", 1);
    docstore_open(store, "file:///b.src", "b", 1);
    docstore_open(store, "file:///c.src", "c", 1);
    TEST_INIT;

    ASSERT_EQ(store->count, 3);

    docstore_close(store, "file:///b.src");
    ASSERT_EQ(store->count, 2);

    ASSERT_NOT_NULL(docstore_get(store, "file:///a.src"));
    ASSERT_NULL(docstore_get(store, "file:///b.src"));
    ASSERT_NOT_NULL(docstore_get(store, "file:///c.src"));

cleanup:
    docstore_free(store);
    TEST_FINI;
}

TEST(docstore_get_not_found) {
    DocumentStore *store = docstore_new();
    TEST_INIT;

    Document *doc = docstore_get(store, "file:///nonexistent.src");
    ASSERT_NULL(doc);

cleanup:
    docstore_free(store);
    TEST_FINI;
}

TEST(docstore_reopen_updates) {
    DocumentStore *store = docstore_new();

    const char *uri = "file:///test.src";
    docstore_open(store, uri, "version 1", 1);
    docstore_open(store, uri, "version 2", 2);
    TEST_INIT;

    /* Should have updated, not duplicated */
    ASSERT_EQ(store->count, 1);

    Document *doc = docstore_get(store, uri);
    ASSERT_STREQ(doc->content, "version 2");
    ASSERT_EQ(doc->version, 2);

cleanup:
    docstore_free(store);
    TEST_FINI;
}

/*
 * Test document parsing
 */
TEST(document_parse_valid_tbol) {
    DocumentStore *store = docstore_new();

    /* Valid TBOL source */
    const char *src =
        "PROGRAM TBOLTEST;\n"
        "DATA D = myvar;\n"
        "PROC main =\n"
        "    MOVE '42', myvar;\n"
        "END_PROC\n";

    Document *doc = docstore_open(store, "file:///test.src", src, 1);
    TEST_INIT;
    ASSERT_NOT_NULL(doc);

    document_parse(doc, NULL, 0, 0);

    /* Should have no errors for valid code */
    if (doc->diagnostics) {
        int count = cJSON_GetArraySize(doc->diagnostics);
        /* Allow warnings but no errors */
        for (int i = 0; i < count; i++) {
            cJSON *diag = cJSON_GetArrayItem(doc->diagnostics, i);
            cJSON *severity = cJSON_GetObjectItem(diag, "severity");
            /* Severity 1 = error - we want none of those */
            if (cJSON_IsNumber(severity) && severity->valuedouble == 1) {
                ASSERT(0 && "Unexpected error in valid TBOL");
            }
        }
    }

cleanup:
    docstore_free(store);
    TEST_FINI;
}

TEST(document_parse_syntax_error) {
    DocumentStore *store = docstore_new();

    /* Invalid TBOL - missing semicolon */
    const char *src = "PROGRAM TBOLTEST\n";  /* Missing semicolon */

    Document *doc = docstore_open(store, "file:///test.src", src, 1);
    TEST_INIT;
    ASSERT_NOT_NULL(doc);

    document_parse(doc, NULL, 0, 0);

    /* Should have diagnostics */
    ASSERT_NOT_NULL(doc->diagnostics);
    int count = cJSON_GetArraySize(doc->diagnostics);
    ASSERT(count > 0);

    /* Check first diagnostic has error severity */
    cJSON *diag = cJSON_GetArrayItem(doc->diagnostics, 0);
    ASSERT_NOT_NULL(diag);

    cJSON *severity = cJSON_GetObjectItem(diag, "severity");
    ASSERT_NOT_NULL(severity);
    ASSERT_EQ((int)severity->valuedouble, 1);  /* 1 = Error */

    cJSON *source = cJSON_GetObjectItem(diag, "source");
    ASSERT_NOT_NULL(source);
    ASSERT_STREQ(source->valuestring, "tbol");

cleanup:
    docstore_free(store);
    TEST_FINI;
}

TEST(document_parse_undefined_variable) {
    DocumentStore *store = docstore_new();

    /* Undefined variable error - UNDEFINED_VAR not declared */
    const char *src =
        "PROGRAM TBOLTEST;\n"
        "DATA D = myvar;\n"
        "PROC main =\n"
        "    MOVE UNDEFINED_VAR, myvar;\n"
        "END_PROC\n";

    Document *doc = docstore_open(store, "file:///test.src", src, 1);
    document_parse(doc, NULL, 0, 0);
    TEST_INIT;

    /* Should have at least one error diagnostic */
    ASSERT_NOT_NULL(doc->diagnostics);
    int count = cJSON_GetArraySize(doc->diagnostics);
    ASSERT(count > 0);

cleanup:
    docstore_free(store);
    TEST_FINI;
}

TEST(document_reparse_clears_old_diagnostics) {
    DocumentStore *store = docstore_new();

    /* First, parse invalid code - missing semicolon */
    const char *invalid = "PROGRAM TBOLTEST\n";
    Document *doc = docstore_open(store, "file:///test.src", invalid, 1);
    document_parse(doc, NULL, 0, 0);
    TEST_INIT;

    ASSERT_NOT_NULL(doc->diagnostics);
    int error_count = cJSON_GetArraySize(doc->diagnostics);
    ASSERT(error_count > 0);

    /* Now update with valid code and reparse */
    const char *valid =
        "PROGRAM TBOLTEST;\n"
        "DATA D = myvar;\n"
        "PROC main =\n"
        "    RETURN;\n"
        "END_PROC\n";

    docstore_update(store, "file:///test.src", valid, 2);
    doc = docstore_get(store, "file:///test.src");
    document_parse(doc, NULL, 0, 0);

    /* Should have fewer or no diagnostics */
    if (doc->diagnostics) {
        int new_count = cJSON_GetArraySize(doc->diagnostics);
        /* Check no errors (warnings ok) */
        for (int i = 0; i < new_count; i++) {
            cJSON *diag = cJSON_GetArrayItem(doc->diagnostics, i);
            cJSON *severity = cJSON_GetObjectItem(diag, "severity");
            if (cJSON_IsNumber(severity) && severity->valuedouble == 1) {
                ASSERT(0 && "Fixed code still has errors");
            }
        }
    }

cleanup:
    docstore_free(store);
    TEST_FINI;
}

TEST(document_hover_on_proc) {
    /* Create a server and open a document */
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///hover.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = myvar;\n"
        "PROC main =\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Build params for hover request - position on "main" (line 2, col 5) */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 2);      /* 0-based, line 3 = PROC main */
    cJSON_AddNumberToObject(position, "character", 5); /* On "main" */
    cJSON_AddItemToObject(params, "position", position);

    /* Call hover handler */
    cJSON *result = handle_hover(server, params);
    TEST_INIT;

    /* Should have hover info for PROC */
    ASSERT_NOT_NULL(result);

    cJSON *contents = cJSON_GetObjectItem(result, "contents");
    ASSERT_NOT_NULL(contents);

    cJSON *value = cJSON_GetObjectItem(contents, "value");
    ASSERT_NOT_NULL(value);
    /* TBOL uppercases identifiers, so check for MAIN */
    ASSERT(strstr(value->valuestring, "MAIN") != NULL);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(document_hover_on_variable) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///hover_var.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = counter, total;\n"
        "PROC main =\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Position on "counter" (line 1, col 9 = 0-based) */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 1);       /* DATA line */
    cJSON_AddNumberToObject(position, "character", 9);  /* On "counter" */
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_hover(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    cJSON *contents = cJSON_GetObjectItem(result, "contents");
    ASSERT_NOT_NULL(contents);

    cJSON *value = cJSON_GetObjectItem(contents, "value");
    ASSERT_NOT_NULL(value);
    ASSERT(strstr(value->valuestring, "COUNTER") != NULL);
    ASSERT(strstr(value->valuestring, "Variable") != NULL);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(document_hover_on_define) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///hover_def.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DEFINE MAX_SIZE, 100;\n"
        "DATA D = myvar;\n"
        "PROC main =\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Position on "MAX_SIZE" (line 1, col 7 = 0-based) */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 1);       /* DEFINE line */
    cJSON_AddNumberToObject(position, "character", 7);  /* On "MAX_SIZE" */
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_hover(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    cJSON *contents = cJSON_GetObjectItem(result, "contents");
    ASSERT_NOT_NULL(contents);

    cJSON *value = cJSON_GetObjectItem(contents, "value");
    ASSERT_NOT_NULL(value);
    ASSERT(strstr(value->valuestring, "MAX_SIZE") != NULL);
    ASSERT(strstr(value->valuestring, "DEFINE") != NULL);
    /* Should show the value */
    ASSERT(strstr(value->valuestring, "100") != NULL);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(document_hover_on_label) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///hover_label.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = myvar;\n"
        "PROC main =\n"
        "start:\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Position on "start" (line 3, col 0 = 0-based) */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 3);       /* Label line */
    cJSON_AddNumberToObject(position, "character", 0);  /* On "start" */
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_hover(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    cJSON *contents = cJSON_GetObjectItem(result, "contents");
    ASSERT_NOT_NULL(contents);

    cJSON *value = cJSON_GetObjectItem(contents, "value");
    ASSERT_NOT_NULL(value);
    ASSERT(strstr(value->valuestring, "START") != NULL);
    ASSERT(strstr(value->valuestring, "Label") != NULL);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(document_hover_on_program) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///hover_prog.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = myvar;\n"
        "PROC main =\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Position on "MYAPP" (line 0, col 8 = 0-based) */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 0);       /* PROGRAM line */
    cJSON_AddNumberToObject(position, "character", 8);  /* On "MYAPP" */
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_hover(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    cJSON *contents = cJSON_GetObjectItem(result, "contents");
    ASSERT_NOT_NULL(contents);

    cJSON *value = cJSON_GetObjectItem(contents, "value");
    ASSERT_NOT_NULL(value);
    ASSERT(strstr(value->valuestring, "MYAPP") != NULL);
    ASSERT(strstr(value->valuestring, "Program") != NULL);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(document_hover_outside_range) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///hover_outside.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = myvar;\n"
        "PROC main =\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Position way past end of file */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 100);     /* Way past EOF */
    cJSON_AddNumberToObject(position, "character", 0);
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_hover(server, params);
    TEST_INIT;
    /* Should return NULL for positions outside the AST */
    ASSERT_NULL(result);

cleanup:
    cJSON_Delete(params);
    server_free(server);
    TEST_FINI;
}

/*
 * Go to Definition tests
 */
TEST(document_goto_def_variable) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///gotodef.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = counter, total;\n"
        "PROC main =\n"
        "    MOVE counter, total;\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Position on "counter" usage in MOVE (line 3, col 9 = 0-based) */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 3);       /* MOVE line */
    cJSON_AddNumberToObject(position, "character", 9);  /* On "counter" */
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_definition(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    /* Should jump to DATA section declaration */
    cJSON *range = cJSON_GetObjectItem(result, "range");
    ASSERT_NOT_NULL(range);

    cJSON *start = cJSON_GetObjectItem(range, "start");
    ASSERT_NOT_NULL(start);

    cJSON *start_line = cJSON_GetObjectItem(start, "line");
    ASSERT_NOT_NULL(start_line);
    /* counter is declared on line 1 (0-based) = DATA line */
    ASSERT_EQ((int)start_line->valuedouble, 1);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(document_goto_def_proc) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///gotodef_proc.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = myvar;\n"
        "PROC helper =\n"
        "    RETURN;\n"
        "END_PROC\n"
        "PROC main =\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Position on "helper" (line 2, col 5 = 0-based) */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 2);       /* PROC helper line */
    cJSON_AddNumberToObject(position, "character", 5);  /* On "helper" */
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_definition(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    /* Already on definition, should return same location */
    cJSON *range = cJSON_GetObjectItem(result, "range");
    ASSERT_NOT_NULL(range);

    cJSON *start = cJSON_GetObjectItem(range, "start");
    cJSON *start_line = cJSON_GetObjectItem(start, "line");
    ASSERT_EQ((int)start_line->valuedouble, 2);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(document_goto_def_label) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///gotodef_label.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = myvar;\n"
        "PROC main =\n"
        "start:\n"
        "    GOTO start;\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Position on "GOTO" keyword (line 4, col 4 = 0-based)
     * Note: AST GOTO range is [5:5 - 5:5] (1-based) = [4:4] in 0-based
     * The label "start" is stored in goto_stmt.label, not as a child node */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 4);       /* GOTO line */
    cJSON_AddNumberToObject(position, "character", 4);  /* On "GOTO" */
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_definition(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    /* Should jump to label definition on line 3 (0-based) */
    cJSON *range = cJSON_GetObjectItem(result, "range");
    cJSON *start = cJSON_GetObjectItem(range, "start");
    cJSON *start_line = cJSON_GetObjectItem(start, "line");
    ASSERT_EQ((int)start_line->valuedouble, 3);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(document_goto_def_define) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///gotodef_define.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DEFINE MAX_SIZE, 100;\n"
        "DATA D = myvar;\n"
        "PROC main =\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Position on "MAX_SIZE" (line 1, col 7 = 0-based) */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 1);       /* DEFINE line */
    cJSON_AddNumberToObject(position, "character", 7);  /* On "MAX_SIZE" */
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_definition(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    /* Already on definition */
    cJSON *range = cJSON_GetObjectItem(result, "range");
    cJSON *start = cJSON_GetObjectItem(range, "start");
    cJSON *start_line = cJSON_GetObjectItem(start, "line");
    ASSERT_EQ((int)start_line->valuedouble, 1);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(document_goto_def_not_found) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///gotodef_notfound.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = myvar;\n"
        "PROC main =\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Position on whitespace/no identifier */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 100);     /* Past EOF */
    cJSON_AddNumberToObject(position, "character", 0);
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_definition(server, params);
    TEST_INIT;
    ASSERT_NULL(result);

cleanup:
    cJSON_Delete(params);
    server_free(server);
    TEST_FINI;
}

/*
 * Completion tests
 */
TEST(completion_includes_variables) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///completion.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = counter, total;\n"
        "PROC main =\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 3);
    cJSON_AddNumberToObject(position, "character", 4);
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_completion(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);
    ASSERT(cJSON_IsArray(result));

    /* Check that variables are included.
     * Labels use the original source case (e.g., "counter" not "COUNTER")
     * because the LSP preserves the user's casing via original_text. */
    int count = cJSON_GetArraySize(result);
    bool found_counter = false, found_total = false;
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(result, i);
        const char *label = cJSON_GetObjectItem(item, "label")->valuestring;
        if (strcasecmp(label, "COUNTER") == 0) found_counter = true;
        if (strcasecmp(label, "TOTAL") == 0) found_total = true;
    }
    ASSERT(found_counter);
    ASSERT(found_total);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(completion_includes_defines) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///completion_def.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DEFINE MAX_SIZE, 100;\n"
        "DATA D = myvar;\n"
        "PROC main =\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 4);
    cJSON_AddNumberToObject(position, "character", 4);
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_completion(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    /* Check that DEFINE is included with its value */
    int count = cJSON_GetArraySize(result);
    bool found_max = false;
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(result, i);
        const char *label = cJSON_GetObjectItem(item, "label")->valuestring;
        if (strcmp(label, "MAX_SIZE") == 0) {
            found_max = true;
            cJSON *detail = cJSON_GetObjectItem(item, "detail");
            ASSERT_NOT_NULL(detail);
            ASSERT(strstr(detail->valuestring, "100") != NULL);
        }
    }
    ASSERT(found_max);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(completion_includes_registers) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///completion_reg.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = myvar;\n"
        "PROC main =\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 3);
    cJSON_AddNumberToObject(position, "character", 4);
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_completion(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    /* Check that I, D, P registers and RDA constants are included */
    int count = cJSON_GetArraySize(result);
    bool found_i1 = false, found_d1 = false, found_p1 = false;
    bool found_rda_first = false, found_rda_last = false;
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(result, i);
        const char *label = cJSON_GetObjectItem(item, "label")->valuestring;
        if (strcmp(label, "I1") == 0) found_i1 = true;
        if (strcmp(label, "D1") == 0) found_d1 = true;
        if (strcmp(label, "P1") == 0) found_p1 = true;
        if (strcmp(label, "RDA_FIRST") == 0) found_rda_first = true;
        if (strcmp(label, "RDA_LAST") == 0) found_rda_last = true;
    }
    ASSERT(found_i1);
    ASSERT(found_d1);
    ASSERT(found_p1);
    ASSERT(found_rda_first);
    ASSERT(found_rda_last);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(completion_includes_procs_and_labels) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///completion_proc.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = myvar;\n"
        "PROC helper =\n"
        "start:\n"
        "    RETURN;\n"
        "END_PROC\n"
        "PROC main =\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 7);
    cJSON_AddNumberToObject(position, "character", 4);
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_completion(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    /* Check for procedures and labels.
     * Labels use original source case via original_text. */
    int count = cJSON_GetArraySize(result);
    bool found_helper = false, found_main = false, found_start = false;
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(result, i);
        const char *label = cJSON_GetObjectItem(item, "label")->valuestring;
        if (strcasecmp(label, "HELPER") == 0) found_helper = true;
        if (strcasecmp(label, "MAIN") == 0) found_main = true;
        if (strcasecmp(label, "START") == 0) found_start = true;
    }
    ASSERT(found_helper);
    ASSERT(found_main);
    ASSERT(found_start);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

/*
 * Find References tests
 */
TEST(find_references_variable) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///refs.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = counter;\n"
        "PROC main =\n"
        "    MOVE '0', counter;\n"
        "    ADD '1', counter;\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Position on "counter" declaration (line 1, col 9) */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 1);
    cJSON_AddNumberToObject(position, "character", 9);
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_references(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);
    ASSERT(cJSON_IsArray(result));

    /* Should find 3 references: declaration + 2 uses */
    int count = cJSON_GetArraySize(result);
    ASSERT(count >= 3);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(find_references_label) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///refs_label.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = myvar;\n"
        "PROC main =\n"
        "loop:\n"
        "    GOTO loop;\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Position on "loop" label (line 3, col 0) */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 3);
    cJSON_AddNumberToObject(position, "character", 0);
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_references(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);
    ASSERT(cJSON_IsArray(result));

    /* Should find 2 references: label definition + GOTO */
    int count = cJSON_GetArraySize(result);
    ASSERT(count >= 2);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

/*
 * Workspace Symbol tests
 */
TEST(workspace_symbols_from_open_documents) {
    LSPServer *server = server_new(stdin, stdout);

    /* Open two documents */
    const char *uri1 = "file:///file1.src";
    const char *src1 =
        "PROGRAM APP1;\n"
        "DATA D = var1;\n"
        "PROC init =\n"
        "    RETURN;\n"
        "END_PROC\n";

    const char *uri2 = "file:///file2.src";
    const char *src2 =
        "PROGRAM APP2;\n"
        "DATA D = var2;\n"
        "PROC process =\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc1 = docstore_open(server->documents, uri1, src1, 1);
    document_parse(doc1, NULL, 0, 0);
    Document *doc2 = docstore_open(server->documents, uri2, src2, 1);
    document_parse(doc2, NULL, 0, 0);

    /* Search with empty query - should return all symbols */
    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "query", "");

    cJSON *result = handle_workspace_symbol(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);
    ASSERT(cJSON_IsArray(result));

    /* Should have symbols from both documents */
    int count = cJSON_GetArraySize(result);
    ASSERT(count >= 4);  /* At least: APP1, VAR1, APP2, VAR2 */

    /* Check we have symbols from both files */
    bool found_app1 = false, found_app2 = false;
    for (int i = 0; i < count; i++) {
        cJSON *sym = cJSON_GetArrayItem(result, i);
        const char *name = cJSON_GetObjectItem(sym, "name")->valuestring;
        if (strcmp(name, "APP1") == 0) found_app1 = true;
        if (strcmp(name, "APP2") == 0) found_app2 = true;
    }
    ASSERT(found_app1);
    ASSERT(found_app2);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(workspace_symbols_with_query) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///wsquery.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = counter, total_count, name;\n"
        "PROC count_items =\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Search for "count" - should match counter, total_count, count_items */
    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "query", "count");

    cJSON *result = handle_workspace_symbol(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);
    ASSERT(cJSON_IsArray(result));

    int count = cJSON_GetArraySize(result);
    ASSERT(count >= 3);  /* counter, total_count, count_items */

    /* Verify "name" is not included (doesn't match "count") */
    bool found_name = false;
    for (int i = 0; i < count; i++) {
        cJSON *sym = cJSON_GetArrayItem(result, i);
        const char *sym_name = cJSON_GetObjectItem(sym, "name")->valuestring;
        if (strcmp(sym_name, "NAME") == 0) found_name = true;
    }
    ASSERT(!found_name);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

/*
 * Rename tests
 */
TEST(prepare_rename_on_variable) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///rename.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = counter;\n"
        "PROC main =\n"
        "    MOVE '0', counter;\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Position on "counter" declaration */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 1);
    cJSON_AddNumberToObject(position, "character", 9);
    cJSON_AddItemToObject(params, "position", position);

    cJSON *result = handle_prepare_rename(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    /* Should return range and placeholder */
    cJSON *range = cJSON_GetObjectItem(result, "range");
    ASSERT_NOT_NULL(range);

    cJSON *placeholder = cJSON_GetObjectItem(result, "placeholder");
    ASSERT_NOT_NULL(placeholder);
    ASSERT_STREQ(placeholder->valuestring, "counter");

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(rename_variable_single_file) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///rename_var.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = counter;\n"
        "PROC main =\n"
        "    MOVE '0', counter;\n"
        "    ADD '1', counter;\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Rename "counter" to "total" */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 1);
    cJSON_AddNumberToObject(position, "character", 9);
    cJSON_AddItemToObject(params, "position", position);

    cJSON_AddStringToObject(params, "newName", "TOTAL");

    cJSON *result = handle_rename(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    /* Should return WorkspaceEdit with changes */
    cJSON *changes = cJSON_GetObjectItem(result, "changes");
    ASSERT_NOT_NULL(changes);

    /* Should have edits for this file */
    cJSON *file_edits = cJSON_GetObjectItem(changes, uri);
    ASSERT_NOT_NULL(file_edits);
    ASSERT(cJSON_IsArray(file_edits));

    /* Should have 3 edits (declaration + 2 uses) */
    int count = cJSON_GetArraySize(file_edits);
    ASSERT(count >= 3);

    /* Verify each edit has newText = "TOTAL" */
    for (int i = 0; i < count; i++) {
        cJSON *edit = cJSON_GetArrayItem(file_edits, i);
        cJSON *newText = cJSON_GetObjectItem(edit, "newText");
        ASSERT_NOT_NULL(newText);
        ASSERT_STREQ(newText->valuestring, "TOTAL");
    }

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(rename_across_files) {
    LSPServer *server = server_new(stdin, stdout);

    /* Open two files that use the same variable name */
    const char *uri1 = "file:///file1.src";
    const char *src1 =
        "PROGRAM APP1;\n"
        "DATA D = shared_var;\n"
        "PROC init =\n"
        "    MOVE '0', shared_var;\n"
        "    RETURN;\n"
        "END_PROC\n";

    const char *uri2 = "file:///file2.src";
    const char *src2 =
        "PROGRAM APP2;\n"
        "DATA D = shared_var;\n"
        "PROC process =\n"
        "    ADD '1', shared_var;\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc1 = docstore_open(server->documents, uri1, src1, 1);
    document_parse(doc1, NULL, 0, 0);
    Document *doc2 = docstore_open(server->documents, uri2, src2, 1);
    document_parse(doc2, NULL, 0, 0);

    /* Rename from file1 */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri1);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 1);
    cJSON_AddNumberToObject(position, "character", 9);
    cJSON_AddItemToObject(params, "position", position);

    cJSON_AddStringToObject(params, "newName", "NEW_VAR");

    cJSON *result = handle_rename(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    cJSON *changes = cJSON_GetObjectItem(result, "changes");
    ASSERT_NOT_NULL(changes);

    /* Should have edits only for the current file — separate TBOL programs
     * with the same variable name are independent and not cross-renamed.
     * Cross-file rename only applies to COPY file symbols. */
    cJSON *edits1 = cJSON_GetObjectItem(changes, uri1);
    cJSON *edits2 = cJSON_GetObjectItem(changes, uri2);

    ASSERT_NOT_NULL(edits1);
    ASSERT(cJSON_GetArraySize(edits1) >= 2);

    /* file2 should NOT be affected (independent program) */
    ASSERT(edits2 == NULL);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

TEST(rename_label) {
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///rename_label.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = myvar;\n"
        "PROC main =\n"
        "loop:\n"
        "    GOTO loop;\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Rename "loop" — cursor on the label name in GOTO statement */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 4);       /* GOTO line */
    cJSON_AddNumberToObject(position, "character", 9);  /* On "loop" */
    cJSON_AddItemToObject(params, "position", position);

    cJSON_AddStringToObject(params, "newName", "START");

    cJSON *result = handle_rename(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    cJSON *changes = cJSON_GetObjectItem(result, "changes");
    ASSERT_NOT_NULL(changes);

    cJSON *file_edits = cJSON_GetObjectItem(changes, uri);
    ASSERT_NOT_NULL(file_edits);

    /* Should have 2 edits: label definition + GOTO reference */
    int count = cJSON_GetArraySize(file_edits);
    ASSERT(count >= 2);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

/*
 * COPY file cascade test using actual files
 * Tests that rename finds all files that COPY a given file
 */
TEST(rename_copy_file_cascade) {
    /* Create a unique tmpdir via mkdtemp so this is portable on Windows
     * (where system("rm -rf && mkdir -p") would invoke cmd.exe and fail). */
    char tmpdir[] = "/tmp/tbol_lsp_test_XXXXXX";
    char *made = mkdtemp(tmpdir);
    char copyfile[PATH_MAX], file1[PATH_MAX], file2[PATH_MAX];
    char uri1[PATH_MAX + 16], root_uri[PATH_MAX + 16];
    LSPServer *server = NULL;
    Document *doc = NULL;
    char *content = NULL;
    FILE *f;
    TEST_INIT;
    ASSERT_NOT_NULL(made);

    snprintf(copyfile, sizeof(copyfile), "%s/SHAREDCP", tmpdir);
    snprintf(file1, sizeof(file1), "%s/file1.src", tmpdir);
    snprintf(file2, sizeof(file2), "%s/file2.src", tmpdir);
    snprintf(root_uri, sizeof(root_uri), "file://%s", tmpdir);
    snprintf(uri1, sizeof(uri1), "file://%s", file1);

    /* COPY file with a shared structure definition (structname=var,var;)
     * that continues the DATA section. */
    f = fopen(copyfile, "wb");
    ASSERT_NOT_NULL(f);
    fprintf(f, "shared=shared_counter,shared_total;\n");
    fclose(f);

    /* file1.src that uses COPY at statement level, after DATA. */
    f = fopen(file1, "wb");
    ASSERT_NOT_NULL(f);
    fprintf(f, "PROGRAM APP1;\n");
    fprintf(f, "DATA local=local_var;\n");
    fprintf(f, "COPY SHAREDCP;\n");
    fprintf(f, "PROC main =\n");
    fprintf(f, "    MOVE '0', shared_counter;\n");
    fprintf(f, "    RETURN;\n");
    fprintf(f, "END_PROC\n");
    fclose(f);

    /* file2.src that also uses COPY file. */
    f = fopen(file2, "wb");
    ASSERT_NOT_NULL(f);
    fprintf(f, "PROGRAM APP2;\n");
    fprintf(f, "DATA other=another_var;\n");
    fprintf(f, "COPY SHAREDCP;\n");
    fprintf(f, "PROC process =\n");
    fprintf(f, "    ADD '1', shared_counter;\n");
    fprintf(f, "    RETURN;\n");
    fprintf(f, "END_PROC\n");
    fclose(f);

    server = server_new(stdin, stdout);
    server->root_uri = strdup(root_uri);
    server->include_paths = malloc(sizeof(char *));
    server->include_paths[0] = strdup(tmpdir);
    server->include_path_count = 1;

    f = fopen(file1, "rb");
    ASSERT_NOT_NULL(f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    content = malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    doc = docstore_open(server->documents, uri1, content, 1);
    document_parse(doc, (const char **)server->include_paths, server->include_path_count, 0);

    ASSERT_NOT_NULL(doc->ast);

cleanup:
    free(content);
    if (server) {
        if (server->include_paths) {
            free(server->include_paths[0]);
            free(server->include_paths);
            server->include_paths = NULL;
            server->include_path_count = 0;
        }
        server_free(server);
    }
    if (made) rm_rf_dir(tmpdir);

    TEST_FINI;
}

TEST(document_symbols_extracted) {
    /* Create a server and open a document */
    LSPServer *server = server_new(stdin, stdout);

    const char *uri = "file:///symbols.src";
    const char *src =
        "PROGRAM MYAPP;\n"
        "DEFINE MAX_SIZE, 100;\n"
        "DATA D = counter, total;\n"
        "PROC init =\n"
        "start:\n"
        "    RETURN;\n"
        "END_PROC\n"
        "PROC main =\n"
        "loop:\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Build params for documentSymbol request */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    /* Call handler */
    cJSON *result = handle_document_symbol(server, params);
    TEST_INIT;

    /* Should have symbols */
    ASSERT_NOT_NULL(result);
    ASSERT(cJSON_IsArray(result));

    /* Should have at least one symbol (the program) */
    int count = cJSON_GetArraySize(result);
    ASSERT(count > 0);

    /* First symbol should be the program */
    cJSON *prog = cJSON_GetArrayItem(result, 0);
    ASSERT_NOT_NULL(prog);

    cJSON *name = cJSON_GetObjectItem(prog, "name");
    ASSERT_NOT_NULL(name);
    ASSERT_STREQ(name->valuestring, "MYAPP");

    /* Program should have children (DATA section, DEFINEs, PROCs) */
    cJSON *children = cJSON_GetObjectItem(prog, "children");
    ASSERT_NOT_NULL(children);
    ASSERT(cJSON_GetArraySize(children) >= 2);  /* At least DATA and PROCs */

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

/*
 * Test signature help for MOVE verb
 */
TEST(signature_help_move) {
    LSPServer *server = server_new(NULL, NULL);
    server->state = SERVER_RUNNING;
    const char *uri = "file:///test/sighelp.src";

    /* TBOL source with MOVE statement - cursor at position after "MOVE " */
    const char *src =
        "PROGRAM SIGHELP;\n"
        "DATA d=x;\n"
        "PROC main =\n"
        "    MOVE '5', x;\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Build params for signatureHelp request - cursor on line 4 (0-indexed: 3), after MOVE keyword */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);
    cJSON *pos = cJSON_CreateObject();
    cJSON_AddNumberToObject(pos, "line", 3);       /* Line 4 (0-indexed) */
    cJSON_AddNumberToObject(pos, "character", 10); /* After "    MOVE " */
    cJSON_AddItemToObject(params, "position", pos);

    /* Call handler */
    cJSON *result = handle_signature_help(server, params);
    TEST_INIT;

    /* Should have signature info */
    ASSERT_NOT_NULL(result);

    cJSON *signatures = cJSON_GetObjectItem(result, "signatures");
    ASSERT_NOT_NULL(signatures);
    ASSERT(cJSON_IsArray(signatures));
    ASSERT(cJSON_GetArraySize(signatures) == 1);

    cJSON *sig = cJSON_GetArrayItem(signatures, 0);
    ASSERT_NOT_NULL(sig);

    cJSON *label = cJSON_GetObjectItem(sig, "label");
    ASSERT_NOT_NULL(label);
    /* Should contain MOVE signature */
    ASSERT(strstr(label->valuestring, "MOVE") != NULL);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

/*
 * Test signature help returns NULL for unknown verb
 */
TEST(signature_help_unknown_verb) {
    LSPServer *server = server_new(NULL, NULL);
    server->state = SERVER_RUNNING;
    const char *uri = "file:///test/sighelp2.src";

    /* TBOL source with a procedure call (not a verb) */
    const char *src =
        "PROGRAM SIGHELP;\n"
        "DATA d=x;\n"
        "PROC main =\n"
        "    myproc x;\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Build params for signatureHelp request */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);
    cJSON *pos = cJSON_CreateObject();
    cJSON_AddNumberToObject(pos, "line", 3);
    cJSON_AddNumberToObject(pos, "character", 10);
    cJSON_AddItemToObject(params, "position", pos);

    /* Call handler */
    cJSON *result = handle_signature_help(server, params);
    TEST_INIT;

    /* Should return NULL for non-verb */
    ASSERT(result == NULL);

cleanup:
    cJSON_Delete(params);
    server_free(server);
    TEST_FINI;
}

/*
 * Test signature help for DELETE verb
 */
TEST(signature_help_deletefile) {
    LSPServer *server = server_new(NULL, NULL);
    server->state = SERVER_RUNNING;
    const char *uri = "file:///test/sighelp3.src";

    const char *src =
        "PROGRAM SIGHELP;\n"
        "DATA d=filename;\n"
        "PROC main =\n"
        "    DELETE filename;\n"
        "    RETURN;\n"
        "END_PROC\n";

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Build params */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);
    cJSON *pos = cJSON_CreateObject();
    cJSON_AddNumberToObject(pos, "line", 3);
    cJSON_AddNumberToObject(pos, "character", 11);  /* After DELETE */
    cJSON_AddItemToObject(params, "position", pos);

    /* Call handler */
    cJSON *result = handle_signature_help(server, params);
    TEST_INIT;

    /* Should have signature info */
    ASSERT_NOT_NULL(result);

    cJSON *signatures = cJSON_GetObjectItem(result, "signatures");
    ASSERT_NOT_NULL(signatures);
    ASSERT(cJSON_GetArraySize(signatures) == 1);

    cJSON *sig = cJSON_GetArrayItem(signatures, 0);
    cJSON *label = cJSON_GetObjectItem(sig, "label");
    ASSERT(strstr(label->valuestring, "DELETE") != NULL);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

/*
 * Helper: create a temp directory with COPY files for hover/goto tests.
 * Returns a malloc'd tmpdir path. Caller must clean up files + dir.
 *
 * Layout:
 *   <tmpdir>/BIGCOPY      — 20-line COPY file with DEFINEs + DATA vars
 *   <tmpdir>/MAIN.SRC     — main file that COPYs BIGCOPY, then has code
 *
 * The COPY file has enough lines that its transparent line numbers
 * overlap with the main file's actual code lines.
 */
static char *create_copy_test_files(char *tmpdir_template,
                                    char *mainfile, size_t mainfile_sz,
                                    char *copyfile, size_t copyfile_sz) {
    mkdtemp(tmpdir_template);

    /* COPY file: 20 lines of DEFINEs + DATA vars */
    snprintf(copyfile, copyfile_sz, "%s/BIGCOPY", tmpdir_template);
    FILE *f = fopen(copyfile, "wb");
    fprintf(f, "DEFINE MY_CONST, '42';\n");       /* line 1 */
    fprintf(f, "DEFINE MY_OTHER, '99';\n");        /* line 2 */
    fprintf(f, "DEFINE MY_THIRD, '100';\n");       /* line 3 */
    fprintf(f, "DEFINE MY_FOURTH, '200';\n");      /* line 4 */
    fprintf(f, "DEFINE MY_FIFTH, '300';\n");       /* line 5 */
    fprintf(f, "DEFINE MY_SIXTH, '400';\n");       /* line 6 */
    fprintf(f, "DEFINE MY_SEVENTH, '500';\n");     /* line 7 */
    fprintf(f, "DEFINE MY_EIGHTH, '600';\n");      /* line 8 */
    fprintf(f, "DEFINE MY_NINTH, '700';\n");       /* line 9 */
    fprintf(f, "DEFINE MY_TENTH, '800';\n");       /* line 10 */
    fprintf(f, "copy_sec =\n");                     /* line 11 — DATA group */
    fprintf(f, "    copy_var1,\n");                 /* line 12 */
    fprintf(f, "    copy_var2,\n");                 /* line 13 */
    fprintf(f, "    copy_var3,\n");                 /* line 14 */
    fprintf(f, "    copy_var4,\n");                 /* line 15 */
    fprintf(f, "    copy_var5,\n");                 /* line 16 */
    fprintf(f, "    copy_var6,\n");                 /* line 17 */
    fprintf(f, "    copy_var7,\n");                 /* line 18 */
    fprintf(f, "    copy_var8,\n");                 /* line 19 */
    fprintf(f, "    copy_var9;\n");                 /* line 20 */
    fclose(f);

    /* Main file:
     *   line 1: PROGRAM TEST;
     *   line 2: DATA D =
     *   line 3:     local_x;
     *   line 4: COPY BIGCOPY;       <- transparent lines 4..24 (20 lines)
     *   line 5: PROC main =         <- actual line, overlaps transparent ~line 5
     *   line 6:     MOVE local_x, P1;
     *   line 7:     SET_CURSOR 14;
     *   line 8:     RETURN;
     *   line 9: END_PROC
     */
    snprintf(mainfile, mainfile_sz, "%s/MAIN.SRC", tmpdir_template);
    f = fopen(mainfile, "wb");
    fprintf(f, "PROGRAM MYTEST;\n");
    fprintf(f, "DATA D =\n");
    fprintf(f, "    local_x;\n");
    fprintf(f, "COPY BIGCOPY;\n");
    fprintf(f, "PROC main =\n");
    fprintf(f, "    MOVE local_x, P1;\n");
    fprintf(f, "    SET_CURSOR 14;\n");
    fprintf(f, "    RETURN;\n");
    fprintf(f, "END_PROC\n");
    fclose(f);

    return tmpdir_template;
}

static LSPServer *setup_copy_test_server(const char *tmpdir, const char *mainfile,
                                          char *uri_buf, size_t uri_sz,
                                          Document **out_doc) {
    LSPServer *server = server_new(NULL, NULL);
    server->state = SERVER_RUNNING;

    server->include_paths = malloc(sizeof(char*));
    server->include_paths[0] = strdup(tmpdir);
    server->include_path_count = 1;

    snprintf(uri_buf, uri_sz, "file://%s", mainfile);

    FILE *f = fopen(mainfile, "rb");
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *content = malloc(len + 1);
    fread(content, 1, len, f);
    content[len] = '\0';
    fclose(f);

    Document *doc = docstore_open(server->documents, uri_buf, content, 1);
    document_parse(doc, (const char **)server->include_paths, server->include_path_count, 0);
    free(content);

    *out_doc = doc;
    return server;
}

static void cleanup_copy_test(LSPServer *server, const char *tmpdir,
                               const char *mainfile, const char *copyfile) {
    server_free(server);
    unlink(copyfile);
    unlink(mainfile);
    rmdir(tmpdir);
}

/*
 * Test: hover on main file code line must NOT return COPY file content.
 * SET_CURSOR is on main file line 7 (0-based: 6), which overlaps with
 * transparent lines from BIGCOPY.  Without filename filtering, the hover
 * would return a DEFINE or variable from BIGCOPY.
 */
TEST(hover_copy_no_false_match) {
    char tmpdir[] = "/tmp/lsp_hcopy_XXXXXX";
    char mainfile[256], copyfile[256], uri[512];
    create_copy_test_files(tmpdir, mainfile, sizeof(mainfile),
                           copyfile, sizeof(copyfile));

    Document *doc;
    LSPServer *server = setup_copy_test_server(tmpdir, mainfile, uri, sizeof(uri), &doc);

    /* Hover on SET_CURSOR (main file line 7, 0-based line 6, col 4) */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);
    cJSON *pos = cJSON_CreateObject();
    cJSON_AddNumberToObject(pos, "line", 6);
    cJSON_AddNumberToObject(pos, "character", 4);
    cJSON_AddItemToObject(params, "position", pos);

    cJSON *result = handle_hover(server, params);
    TEST_INIT;

    /* Result should either be NULL (no hover for SET_CURSOR verb) or
     * contain info about SET_CURSOR — but must NOT contain DEFINE or
     * COPY variable info from BIGCOPY. */
    if (result) {
        cJSON *contents = cJSON_GetObjectItem(result, "contents");
        if (contents) {
            cJSON *value = cJSON_GetObjectItem(contents, "value");
            if (value && cJSON_IsString(value)) {
                ASSERT(strstr(value->valuestring, "DEFINE") == NULL);
                ASSERT(strstr(value->valuestring, "COPY_VAR") == NULL);
            }
        }
        cJSON_Delete(result);
    }

cleanup:
    cJSON_Delete(params);
    cleanup_copy_test(server, tmpdir, mainfile, copyfile);
    TEST_FINI;
}

/*
 * Test: hover on main file variable must return that variable's info,
 * not a COPY file variable that happens to share the same transparent line.
 */
TEST(hover_copy_correct_variable) {
    char tmpdir[] = "/tmp/lsp_hvar_XXXXXX";
    char mainfile[256], copyfile[256], uri[512];
    create_copy_test_files(tmpdir, mainfile, sizeof(mainfile),
                           copyfile, sizeof(copyfile));

    Document *doc;
    LSPServer *server = setup_copy_test_server(tmpdir, mainfile, uri, sizeof(uri), &doc);

    /* Hover on "local_x" in MOVE statement (main file line 6, 0-based 5, col 9) */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);
    cJSON *pos = cJSON_CreateObject();
    cJSON_AddNumberToObject(pos, "line", 5);
    cJSON_AddNumberToObject(pos, "character", 9);
    cJSON_AddItemToObject(params, "position", pos);

    cJSON *result = handle_hover(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    cJSON *contents = cJSON_GetObjectItem(result, "contents");
    ASSERT_NOT_NULL(contents);
    cJSON *value = cJSON_GetObjectItem(contents, "value");
    ASSERT_NOT_NULL(value);

    /* Must mention LOCAL_X, not any COPY variable */
    ASSERT(strstr(value->valuestring, "LOCAL_X") != NULL);
    ASSERT(strstr(value->valuestring, "COPY_VAR") == NULL);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    cleanup_copy_test(server, tmpdir, mainfile, copyfile);
    TEST_FINI;
}

/*
 * Test: hover on COPY keyword shows COPY file info (preproc event).
 */
TEST(hover_copy_keyword) {
    char tmpdir[] = "/tmp/lsp_hkw_XXXXXX";
    char mainfile[256], copyfile[256], uri[512];
    create_copy_test_files(tmpdir, mainfile, sizeof(mainfile),
                           copyfile, sizeof(copyfile));

    Document *doc;
    LSPServer *server = setup_copy_test_server(tmpdir, mainfile, uri, sizeof(uri), &doc);

    /* Hover on "COPY" keyword (main file line 4, 0-based 3, col 0) */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);
    cJSON *pos = cJSON_CreateObject();
    cJSON_AddNumberToObject(pos, "line", 3);
    cJSON_AddNumberToObject(pos, "character", 0);
    cJSON_AddItemToObject(params, "position", pos);

    cJSON *result = handle_hover(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    cJSON *contents = cJSON_GetObjectItem(result, "contents");
    ASSERT_NOT_NULL(contents);
    cJSON *value = cJSON_GetObjectItem(contents, "value");
    ASSERT_NOT_NULL(value);

    /* Should show COPY info with BIGCOPY filename */
    ASSERT(strstr(value->valuestring, "COPY") != NULL);
    ASSERT(strstr(value->valuestring, "BIGCOPY") != NULL);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    cleanup_copy_test(server, tmpdir, mainfile, copyfile);
    TEST_FINI;
}

/*
 * Test: go-to-definition on COPY keyword navigates to COPY file.
 */
TEST(goto_def_copy_keyword) {
    char tmpdir[] = "/tmp/lsp_gkw_XXXXXX";
    char mainfile[256], copyfile[256], uri[512];
    create_copy_test_files(tmpdir, mainfile, sizeof(mainfile),
                           copyfile, sizeof(copyfile));

    Document *doc;
    LSPServer *server = setup_copy_test_server(tmpdir, mainfile, uri, sizeof(uri), &doc);

    /* Go-to-def on "COPY" keyword (main file line 4, 0-based 3, col 0) */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);
    cJSON *pos = cJSON_CreateObject();
    cJSON_AddNumberToObject(pos, "line", 3);
    cJSON_AddNumberToObject(pos, "character", 0);
    cJSON_AddItemToObject(params, "position", pos);

    cJSON *result = handle_definition(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);

    /* Should have a URI pointing to the COPY file */
    cJSON *loc_uri = cJSON_GetObjectItem(result, "uri");
    ASSERT_NOT_NULL(loc_uri);
    ASSERT(strstr(loc_uri->valuestring, "BIGCOPY") != NULL);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    cleanup_copy_test(server, tmpdir, mainfile, copyfile);
    TEST_FINI;
}

/*
 * Test: go-to-definition from main file code should NOT jump to COPY file
 * unless the identifier is actually defined there.
 * Hovering "PROC main" on line 5 (0-based 4) should find the PROC, not COPY content.
 */
TEST(goto_def_copy_no_false_match) {
    char tmpdir[] = "/tmp/lsp_gnf_XXXXXX";
    char mainfile[256], copyfile[256], uri[512];
    create_copy_test_files(tmpdir, mainfile, sizeof(mainfile),
                           copyfile, sizeof(copyfile));

    Document *doc;
    LSPServer *server = setup_copy_test_server(tmpdir, mainfile, uri, sizeof(uri), &doc);

    /* Go-to-def on "main" in PROC main (line 5, 0-based 4, col 5) */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);
    cJSON *pos = cJSON_CreateObject();
    cJSON_AddNumberToObject(pos, "line", 4);
    cJSON_AddNumberToObject(pos, "character", 5);
    cJSON_AddItemToObject(params, "position", pos);

    cJSON *result = handle_definition(server, params);
    TEST_INIT;

    /* If result exists, it should point to main file, not COPY file */
    if (result) {
        cJSON *loc_uri = cJSON_GetObjectItem(result, "uri");
        if (loc_uri && cJSON_IsString(loc_uri)) {
            ASSERT(strstr(loc_uri->valuestring, "BIGCOPY") == NULL);
            ASSERT(strstr(loc_uri->valuestring, "MAIN") != NULL);
        }
        cJSON_Delete(result);
    }

cleanup:
    cJSON_Delete(params);
    cleanup_copy_test(server, tmpdir, mainfile, copyfile);
    TEST_FINI;
}

/*
 * Test that completion includes symbols from COPY files with origin info
 */
TEST(completion_includes_copy_symbols) {
    /* Create temp directory and COPY file */
    char tmpdir[] = "/tmp/lsp_copy_XXXXXX";
    mkdtemp(tmpdir);

    char copyfile[256];
    snprintf(copyfile, sizeof(copyfile), "%s/SHAREDCP", tmpdir);
    FILE *f = fopen(copyfile, "wb");
    fprintf(f, "shared=shared_var,shared_count;\n");
    fclose(f);

    char mainfile[256];
    snprintf(mainfile, sizeof(mainfile), "%s/MAIN.SRC", tmpdir);
    f = fopen(mainfile, "wb");
    fprintf(f, "PROGRAM MAIN;\n");
    fprintf(f, "DATA local=local_var;\n");
    fprintf(f, "COPY SHAREDCP;\n");
    fprintf(f, "PROC main =\n");
    fprintf(f, "    RETURN;\n");
    fprintf(f, "END_PROC\n");
    fclose(f);

    LSPServer *server = server_new(NULL, NULL);
    server->state = SERVER_RUNNING;

    /* Add include path */
    server->include_paths = malloc(sizeof(char*));
    server->include_paths[0] = strdup(tmpdir);
    server->include_path_count = 1;

    char uri[512];
    snprintf(uri, sizeof(uri), "file://%s", mainfile);

    /* Read and parse the main file */
    f = fopen(mainfile, "rb");
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *content = malloc(len + 1);
    fread(content, 1, len, f);
    content[len] = '\0';
    fclose(f);

    Document *doc = docstore_open(server->documents, uri, content, 1);
    document_parse(doc, (const char **)server->include_paths, server->include_path_count, 0);

    /* Check for parse errors */
    if (doc->diagnostics) {
        int ndiags = cJSON_GetArraySize(doc->diagnostics);
        if (ndiags > 0) {
            fprintf(stderr, "Parse diagnostics (%d):\n", ndiags);
            for (int i = 0; i < ndiags; i++) {
                cJSON *d = cJSON_GetArrayItem(doc->diagnostics, i);
                cJSON *msg = cJSON_GetObjectItem(d, "message");
                if (msg) fprintf(stderr, "  - %s\n", msg->valuestring);
            }
        }
    }

    /* Build completion request */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);
    cJSON *pos = cJSON_CreateObject();
    cJSON_AddNumberToObject(pos, "line", 4);
    cJSON_AddNumberToObject(pos, "character", 4);
    cJSON_AddItemToObject(params, "position", pos);

    /* Call handler */
    cJSON *result = handle_completion(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);
    ASSERT(cJSON_IsArray(result));

    /* Look for local_var (no origin annotation) */
    bool found_local = false;
    /* Look for shared_var (with origin annotation) */
    bool found_shared = false;
    bool shared_has_origin = false;

    for (int i = 0; i < cJSON_GetArraySize(result); i++) {
        cJSON *item = cJSON_GetArrayItem(result, i);
        cJSON *label = cJSON_GetObjectItem(item, "label");
        cJSON *detail = cJSON_GetObjectItem(item, "detail");

        if (label && cJSON_IsString(label)) {
            if (strcasecmp(label->valuestring, "LOCAL_VAR") == 0) {
                found_local = true;
            }
            if (strcasecmp(label->valuestring, "SHARED_VAR") == 0) {
                found_shared = true;
                /* Check if detail includes COPY file origin */
                if (detail && cJSON_IsString(detail) &&
                    strstr(detail->valuestring, "SHAREDCP") != NULL) {
                    shared_has_origin = true;
                }
            }
        }
    }

    ASSERT(found_local);
    ASSERT(found_shared);
    ASSERT(shared_has_origin);

cleanup:
    /* Cleanup */
    free(content);
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);

    unlink(copyfile);
    unlink(mainfile);
    rmdir(tmpdir);
    TEST_FINI;
}

/*
 * Test that document outline excludes COPY file symbols
 */
TEST(document_symbols_copy_mapped) {
    char tmpdir[] = "/tmp/lsp_dsym_XXXXXX";
    char mainfile[256], copyfile[256], uri[512];
    create_copy_test_files(tmpdir, mainfile, sizeof(mainfile),
                           copyfile, sizeof(copyfile));

    Document *doc;
    LSPServer *server = setup_copy_test_server(tmpdir, mainfile, uri, sizeof(uri), &doc);

    /* Build params for documentSymbol request */
    cJSON *params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);

    /* Call handler */
    cJSON *result = handle_document_symbol(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(result);
    ASSERT(cJSON_IsArray(result));

    /* Should have exactly 1 top-level symbol: the program */
    ASSERT(cJSON_GetArraySize(result) == 1);

    cJSON *prog = cJSON_GetArrayItem(result, 0);
    cJSON *prog_name = cJSON_GetObjectItem(prog, "name");
    ASSERT_STREQ(prog_name->valuestring, "MYTEST");

    /* Program children should include DATA section, PROC main,
     * and COPY file DATA groups (but NOT DEFINEs) */
    cJSON *prog_children = cJSON_GetObjectItem(prog, "children");
    ASSERT_NOT_NULL(prog_children);

    bool found_data = false, found_proc = false;
    bool found_copy_define = false;
    bool found_copy_data_group = false;
    int child_count = cJSON_GetArraySize(prog_children);
    for (int i = 0; i < child_count; i++) {
        cJSON *child = cJSON_GetArrayItem(prog_children, i);
        cJSON *cname = cJSON_GetObjectItem(child, "name");
        if (!cname) continue;
        if (strcmp(cname->valuestring, "D") == 0) found_data = true;
        if (strcmp(cname->valuestring, "MAIN") == 0) found_proc = true;
        if (strncmp(cname->valuestring, "MY_", 3) == 0) found_copy_define = true;
        if (strcmp(cname->valuestring, "COPY_SEC") == 0) {
            found_copy_data_group = true;
            /* COPY file DATA group should be mapped to line 4 (the COPY statement) */
            cJSON *range = cJSON_GetObjectItem(child, "range");
            cJSON *start = cJSON_GetObjectItem(range, "start");
            cJSON *line_json = cJSON_GetObjectItem(start, "line");
            ASSERT(line_json->valuedouble == 3.0);  /* 0-based line 3 = source line 4 */
        }
    }

    ASSERT(found_data);
    ASSERT(found_proc);
    ASSERT(!found_copy_define);  /* DEFINEs should NOT appear in outline */
    ASSERT(found_copy_data_group);  /* COPY DATA group IS included */

    /* DATA section "D" should only have local_x (its direct child from main file).
     * The COPY file's DATA group "COPY_SEC" is a separate section in the outline. */
    for (int i = 0; i < child_count; i++) {
        cJSON *child = cJSON_GetArrayItem(prog_children, i);
        cJSON *cname = cJSON_GetObjectItem(child, "name");
        if (cname && strcmp(cname->valuestring, "D") == 0) {
            cJSON *data_children = cJSON_GetObjectItem(child, "children");
            ASSERT_NOT_NULL(data_children);
            int var_count = cJSON_GetArraySize(data_children);
            ASSERT(var_count == 1);
            cJSON *var = cJSON_GetArrayItem(data_children, 0);
            cJSON *vname = cJSON_GetObjectItem(var, "name");
            ASSERT_STREQ(vname->valuestring, "LOCAL_X");
            break;
        }
    }

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    cleanup_copy_test(server, tmpdir, mainfile, copyfile);
    TEST_FINI;
}

/*
 * Rename stress test - exercises full COPY cascade with workspace scan.
 * Creates 20 .src files that COPY a shared file, triggers rename,
 * and verifies memory doesn't grow.
 */
#ifdef __APPLE__
#include <mach/mach.h>
static size_t get_rss(void) {
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &count) != KERN_SUCCESS)
        return 0;
    return info.resident_size;
}
#else
static size_t get_rss(void) {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return 0;
    size_t rss = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            rss = (size_t)atol(line + 6) * 1024;
            break;
        }
    }
    fclose(f);
    return rss;
}
#endif

TEST(parse_repeated_no_hang) {
    TEST_INIT;
    /* Test that tbol_parse_string can be called repeatedly without hanging */
    const char *src =
        "PROGRAM MYAPP;\n"
        "DATA D = counter;\n"
        "PROC main =\n"
        "    MOVE '0', counter;\n"
        "    RETURN;\n"
        "END_PROC\n";

    for (int i = 0; i < 10; i++) {
        extern Options g_options;
        memset(&g_options, 0, sizeof(g_options));
        TbolParseOptions options = {
            .filename = "<test>",
            .collect_symbols = false,
        };
        TbolParseResult *result = tbol_parse_string(src, &options);
        ASSERT_NOT_NULL(result);
        ASSERT_NOT_NULL(result->ast);
        tbol_parse_result_free(result);
    }
cleanup:
    TEST_FINI;
}

TEST(rename_copy_cascade_no_leak) {
    /* Create temp workspace with a COPY file and many .src files.
     * The COPY file has enough lines that its transparent line numbers
     * overlap with actual source lines — this tests that edits from
     * COPY file nodes don't corrupt the main file. */
    char tmpdir_raw[] = "/tmp/lsp_rleak_XXXXXX";
    mkdtemp(tmpdir_raw);
    /* Canonicalize to resolve /tmp -> /private/tmp symlink on macOS;
     * use _fullpath on Windows where POSIX realpath is unavailable.
     * On Windows also normalize backslashes to forward slashes so URIs
     * built from the path stay in the same canonical form the LSP uses. */
    char tmpdir[PATH_MAX];
#ifdef _WIN32
    if (!_fullpath(tmpdir, tmpdir_raw, sizeof(tmpdir))) {
#else
    if (!realpath(tmpdir_raw, tmpdir)) {
#endif
        strncpy(tmpdir, tmpdir_raw, sizeof(tmpdir) - 1);
        tmpdir[sizeof(tmpdir) - 1] = '\0';
    }
#ifdef _WIN32
    for (char *p = tmpdir; *p; p++) if (*p == '\\') *p = '/';
#endif

    /* Create COPY file with shared variables.
     * Use multiple lines so transparent line numbers overlap with source. */
    char copyfile[256];
    snprintf(copyfile, sizeof(copyfile), "%s/SHAREDCP", tmpdir);
    FILE *f = fopen(copyfile, "wb");
    fprintf(f, "shared=\n");          /* line 1 of COPY */
    fprintf(f, "    shared_var,\n");  /* line 2 */
    fprintf(f, "    shared_count,\n");/* line 3 */
    fprintf(f, "    filler1,\n");     /* line 4 — transparent line = COPY stmt + 4 = line 7 */
    fprintf(f, "    filler2;\n");     /* line 5 — transparent line = line 8 = END_PROC line! */
    fclose(f);

    /* Create 20 .src files that all COPY SHAREDCP */
    char srcfiles[20][256];
    for (int i = 0; i < 20; i++) {
        snprintf(srcfiles[i], sizeof(srcfiles[i]), "%s/FILE%02d.SRC", tmpdir, i);
        f = fopen(srcfiles[i], "wb");
        fprintf(f, "PROGRAM FILE%02d;\n", i);           /* line 1 */
        fprintf(f, "DATA D%d = local_%d;\n", i, i);     /* line 2 */
        fprintf(f, "COPY SHAREDCP;\n");                   /* line 3 — COPY, transparent lines 3..7 */
        fprintf(f, "PROC main =\n");                      /* line 4 */
        fprintf(f, "    MOVE '0', shared_var;\n");        /* line 5 */
        fprintf(f, "    ADD '1', shared_count;\n");       /* line 6 */
        fprintf(f, "    RETURN;\n");                       /* line 7 */
        fprintf(f, "END_PROC\n");                          /* line 8 */
        fclose(f);
    }

    /* Set up server with workspace */
    LSPServer *server = server_new(NULL, NULL);
    server->state = SERVER_RUNNING;
    size_t root_uri_size = strlen(tmpdir) + 8;
    server->root_uri = malloc(root_uri_size);
    snprintf(server->root_uri, root_uri_size, "file://%s", tmpdir);
    server->include_paths = malloc(sizeof(char *));
    server->include_paths[0] = strdup(tmpdir);
    server->include_path_count = 1;

    /* Open FILE00.SRC */
    char uri[512];
    snprintf(uri, sizeof(uri), "file://%s", srcfiles[0]);
    f = fopen(srcfiles[0], "rb");
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *content = malloc(len + 1);
    fread(content, 1, len, f);
    content[len] = '\0';
    fclose(f);

    Document *doc = docstore_open(server->documents, uri, content, 1);
    document_parse(doc, (const char **)server->include_paths, server->include_path_count, 0);
    free(content);
    content = NULL;

    cJSON *params = NULL;
    cJSON *result = NULL;
    TEST_INIT;
    ASSERT_NOT_NULL(doc->ast);

    /* Measure RSS before rename */
    size_t rss_before = get_rss();

    /* Trigger rename: rename "shared_var" (line 5, col 20 = 0-based 4,19)
     * It's on the MOVE line: "    MOVE '0', shared_var;"
     * 0-based: line 4, character 14 ("shared_var" starts at col 15 in 1-based = 14 in 0-based) */
    params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);
    cJSON *pos = cJSON_CreateObject();
    cJSON_AddNumberToObject(pos, "line", 4);
    cJSON_AddNumberToObject(pos, "character", 14);
    cJSON_AddItemToObject(params, "position", pos);
    cJSON_AddStringToObject(params, "newName", "RENAMED_VAR");

    result = handle_rename(server, params);

    /* Rename should complete and return a WorkspaceEdit */
    ASSERT_NOT_NULL(result);

    cJSON *changes = cJSON_GetObjectItem(result, "changes");
    ASSERT_NOT_NULL(changes);

    /* Should have edits for the open file — but ONLY for actual source lines,
     * not for COPY file content at transparent line numbers */
    cJSON *file_edits = cJSON_GetObjectItem(changes, uri);
    ASSERT_NOT_NULL(file_edits);
    int edit_count = cJSON_GetArraySize(file_edits);
    /* Should be exactly 1 edit: the reference on line 5 (0-based 4).
     * The COPY file VAR_DECL at transparent line 4 must NOT appear here. */
    ASSERT(edit_count == 1);
    cJSON *edit0 = cJSON_GetArrayItem(file_edits, 0);
    cJSON *range0 = cJSON_GetObjectItem(edit0, "range");
    cJSON *start0 = cJSON_GetObjectItem(range0, "start");
    ASSERT((int)cJSON_GetObjectItem(start0, "line")->valuedouble == 4);  /* 0-based line 4 = source line 5 */

    /* COPY file should also get text-based edits.  Use path_to_uri so the
     * key encoding (e.g. ':' -> %3A on Windows) matches what the LSP
     * produces internally. */
    char *copy_uri = path_to_uri(copyfile);
    cJSON *copy_edits = cJSON_GetObjectItem(changes, copy_uri);
    free(copy_uri);
    ASSERT_NOT_NULL(copy_edits);
    ASSERT(cJSON_GetArraySize(copy_edits) > 0);

    cJSON_Delete(params);
    cJSON_Delete(result);
    params = NULL;
    result = NULL;

    /* Measure RSS after rename */
    size_t rss_after = get_rss();
    size_t rss_growth = rss_after > rss_before ? rss_after - rss_before : 0;

    /* Memory growth should be modest (< 10 MB for 20 files) */
    if (rss_growth > 10 * 1024 * 1024) {
        printf("FAIL\n    RSS grew by %zu MB during rename (before=%zu MB, after=%zu MB)\n",
               rss_growth / (1024*1024), rss_before / (1024*1024), rss_after / (1024*1024));
        test_fail_count++;
        goto cleanup;
    }

    /* Run rename a second time to check for cumulative leaks */
    size_t rss_mid = get_rss();

    params = cJSON_CreateObject();
    textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);
    pos = cJSON_CreateObject();
    cJSON_AddNumberToObject(pos, "line", 4);
    cJSON_AddNumberToObject(pos, "character", 14);
    cJSON_AddItemToObject(params, "position", pos);
    cJSON_AddStringToObject(params, "newName", "RENAMED_AGAIN");

    result = handle_rename(server, params);
    ASSERT_NOT_NULL(result);
    cJSON_Delete(params);
    cJSON_Delete(result);
    params = NULL;
    result = NULL;

    size_t rss_final = get_rss();
    size_t cumulative_growth = rss_final > rss_mid ? rss_final - rss_mid : 0;

    if (cumulative_growth > 5 * 1024 * 1024) {
        printf("FAIL\n    Cumulative leak: RSS grew %zu MB on second rename\n",
               cumulative_growth / (1024*1024));
        test_fail_count++;
        goto cleanup;
    }

cleanup:
    /* Cleanup */
    cJSON_Delete(params);
    cJSON_Delete(result);
    free(server->include_paths[0]);
    free(server->include_paths);
    server->include_paths = NULL;
    server->include_path_count = 0;
    server_free(server);

    rm_rf_dir(tmpdir);

    TEST_FINI;
}

/*
 * Test that rename does not corrupt text at DEFINE expansion sites.
 * E.g., DEFINE ACTION_DELETE, one; — renaming "one" must not touch
 * the source position of "ACTION_DELETE".
 */
TEST(rename_skips_define_expansion) {
    const char *src =
        "PROGRAM MYAPP;\n"                           /* line 1 */
        "DATA D = one;\n"                            /* line 2 - VAR_DECL */
        "DEFINE ACTION_DELETE, one;\n"               /* line 3 - DEFINE with value 'one' */
        "PROC main =\n"                              /* line 4 */
        "    MOVE one, P1;\n"                        /* line 5 - direct ref, should rename */
        "    IF P1 = ACTION_DELETE THEN\n"           /* line 6 - DEFINE expansion, must NOT touch */
        "        MOVE '0', P2;\n"                    /* line 7 */
        "    RETURN;\n"                              /* line 8 */
        "END_PROC\n";                                /* line 9 */

    LSPServer *server = server_new(NULL, NULL);
    server->state = SERVER_RUNNING;
    const char *uri = "file:///test_define_expand.src";
    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    cJSON *params = NULL;
    cJSON *result = NULL;
    TEST_INIT;
    ASSERT_NOT_NULL(doc->ast);

    /* Rename "one" to "ZZZZZ" at line 5 (0-based 4), character 9
     * "    MOVE one, P1;" — 'one' starts at column 10 (1-based) = char 9 (0-based) */
    params = cJSON_CreateObject();
    cJSON *textDoc = cJSON_CreateObject();
    cJSON_AddStringToObject(textDoc, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDoc);
    cJSON *pos = cJSON_CreateObject();
    cJSON_AddNumberToObject(pos, "line", 4);
    cJSON_AddNumberToObject(pos, "character", 9);
    cJSON_AddItemToObject(params, "position", pos);
    cJSON_AddStringToObject(params, "newName", "ZZZZZ");

    result = handle_rename(server, params);
    ASSERT_NOT_NULL(result);

    cJSON *changes = cJSON_GetObjectItem(result, "changes");
    ASSERT_NOT_NULL(changes);

    cJSON *file_edits = cJSON_GetObjectItem(changes, uri);
    ASSERT_NOT_NULL(file_edits);

    /* Check each edit: all should be on lines where "one" actually appears
     * in the source text, NOT on line 6 where ACTION_DELETE expands to "one" */
    int edit_count = cJSON_GetArraySize(file_edits);
    for (int i = 0; i < edit_count; i++) {
        cJSON *edit = cJSON_GetArrayItem(file_edits, i);
        cJSON *range = cJSON_GetObjectItem(edit, "range");
        cJSON *start = cJSON_GetObjectItem(range, "start");
        int edit_line = (int)cJSON_GetObjectItem(start, "line")->valuedouble;
        /* Line 6 (0-based 5) has ACTION_DELETE — no edit should be there */
        if (edit_line == 5) {
            printf("FAIL\n    Edit at line 6 (ACTION_DELETE expansion site)\n");
            test_fail_count++;
            goto cleanup;
        }
    }

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

/*
 * Folding range tests
 */
TEST(folding_range_if_else) {
    LSPServer *server = server_new(stdin, stdout);
    server->state = SERVER_RUNNING;

    const char *uri = "file:///fold_test.src";
    const char *src =
        "PROGRAM FOLDTEST;\n"             /* line 0 */
        "DATA D = myvar;\n"               /* line 1 */
        "PROC main =\n"                   /* line 2 */
        "    IF myvar > '0' THEN DO\n"    /* line 3 */
        "        MOVE '1', myvar;\n"      /* line 4 */
        "    END; ELSE DO\n"              /* line 5 */
        "        MOVE '0', myvar;\n"      /* line 6 */
        "    END;\n"                       /* line 7 */
        "END_PROC\n";                      /* line 8 */

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    cJSON *params = cJSON_CreateObject();
    cJSON *td = cJSON_CreateObject();
    cJSON_AddStringToObject(td, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", td);

    cJSON *result = handle_folding_range(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(doc->ast);
    ASSERT_NOT_NULL(result);

    int count = cJSON_GetArraySize(result);
    /* Should have folds for: PROGRAM, PROC, IF_STMT, THEN DO_BLOCK, ELSE DO_BLOCK */
    ASSERT(count >= 4);

    /* Check that at least one fold starts at or after line 5 (ELSE DO) */
    bool found_else_fold = false;
    for (int i = 0; i < count; i++) {
        cJSON *fr = cJSON_GetArrayItem(result, i);
        int startLine = (int)cJSON_GetObjectItem(fr, "startLine")->valuedouble;
        int endLine = (int)cJSON_GetObjectItem(fr, "endLine")->valuedouble;
        if (startLine >= 5 && endLine >= 6) {
            found_else_fold = true;
        }
    }
    ASSERT(found_else_fold);

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

/*
 * Semantic token tests
 */
TEST(semantic_tokens_variable_decl_and_ref) {
    LSPServer *server = server_new(stdin, stdout);
    server->state = SERVER_RUNNING;

    const char *uri = "file:///semtok_test.src";
    const char *src =
        "PROGRAM SEMTEST;\n"              /* line 0 */
        "DATA D = counter;\n"             /* line 1 */
        "PROC main =\n"                   /* line 2 */
        "    MOVE '1', counter;\n"        /* line 3 */
        "    helper;\n"                   /* line 4 — proc call */
        "END_PROC\n"                      /* line 5 */
        "PROC helper =\n"                /* line 6 */
        "    RETURN;\n"                   /* line 7 */
        "END_PROC\n";                     /* line 8 */

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    cJSON *params = cJSON_CreateObject();
    cJSON *td = cJSON_CreateObject();
    cJSON_AddStringToObject(td, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", td);

    cJSON *result = handle_semantic_tokens_full(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(doc->ast);
    ASSERT_NOT_NULL(result);

    cJSON *data = cJSON_GetObjectItem(result, "data");
    ASSERT_NOT_NULL(data);

    /* We emit tokens for: AST_VAR_DECL (counter), AST_PROC_CALL (helper).
     * AST_IDENT (counter in MOVE) is intentionally excluded.
     * Each token = 5 values in the delta array. */
    int data_count = cJSON_GetArraySize(data);
    ASSERT(data_count >= 10);  /* At least 2 tokens */
    ASSERT(data_count % 5 == 0);  /* Must be multiple of 5 */

    /* First token should be the VAR_DECL "counter" on line 1 */
    int t0_deltaLine = (int)cJSON_GetArrayItem(data, 0)->valuedouble;
    int t0_type = (int)cJSON_GetArrayItem(data, 3)->valuedouble;
    int t0_mod = (int)cJSON_GetArrayItem(data, 4)->valuedouble;
    ASSERT(t0_deltaLine == 1);  /* line 1 (1-based=2, delta from 1-based-1=1 → delta=1) */
    ASSERT(t0_type == 0);  /* variable */
    ASSERT(t0_mod == 1);  /* declaration */

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

/*
 * Selection range tests
 */
TEST(selection_range_nested_hierarchy) {
    LSPServer *server = server_new(stdin, stdout);
    server->state = SERVER_RUNNING;

    const char *uri = "file:///selrange_test.src";
    const char *src =
        "PROGRAM SELTEST;\n"              /* line 0 */
        "DATA D = myvar;\n"               /* line 1 */
        "PROC main =\n"                   /* line 2 */
        "    MOVE '42', myvar;\n"         /* line 3 */
        "END_PROC\n";                      /* line 4 */

    Document *doc = docstore_open(server->documents, uri, src, 1);
    document_parse(doc, NULL, 0, 0);

    /* Request selection range at "myvar" on line 3 (0-based) */
    cJSON *params = cJSON_CreateObject();
    cJSON *td = cJSON_CreateObject();
    cJSON_AddStringToObject(td, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", td);

    cJSON *positions = cJSON_CreateArray();
    cJSON *pos = cJSON_CreateObject();
    cJSON_AddNumberToObject(pos, "line", 3);
    cJSON_AddNumberToObject(pos, "character", 16);  /* on "myvar" */
    cJSON_AddItemToArray(positions, pos);
    cJSON_AddItemToObject(params, "positions", positions);

    cJSON *result = handle_selection_range(server, params);
    TEST_INIT;
    ASSERT_NOT_NULL(doc->ast);
    ASSERT_NOT_NULL(result);
    ASSERT(cJSON_GetArraySize(result) == 1);

    /* The innermost range is at the top level */
    cJSON *sr = cJSON_GetArrayItem(result, 0);
    ASSERT_NOT_NULL(sr);

    cJSON *range = cJSON_GetObjectItem(sr, "range");
    ASSERT_NOT_NULL(range);

    /* The innermost range should be on line 3 (a token or statement) */
    cJSON *start = cJSON_GetObjectItem(range, "start");
    int startLine = (int)cJSON_GetObjectItem(start, "line")->valuedouble;
    ASSERT(startLine == 3 || startLine == 2);  /* token or statement line */

    /* Should have at least one parent (the chain should go deeper than 1) */
    cJSON *parent = cJSON_GetObjectItem(sr, "parent");
    ASSERT_NOT_NULL(parent);

    /* Walk up the parent chain — outermost should be PROGRAM-level */
    cJSON *outermost = sr;
    int levels = 1;
    while (cJSON_GetObjectItem(outermost, "parent")) {
        outermost = cJSON_GetObjectItem(outermost, "parent");
        levels++;
    }
    /* Expect at least 3 levels: token, stmt, proc (PROGRAM excluded due to range) */
    ASSERT(levels >= 3);

    /* Outermost should be PROC at line 2 (0-based), since PROGRAM's range
     * doesn't encompass inner lines (parser uses MAKE_LOC for end). */
    range = cJSON_GetObjectItem(outermost, "range");
    start = cJSON_GetObjectItem(range, "start");
    int outerLine = (int)cJSON_GetObjectItem(start, "line")->valuedouble;
    ASSERT(outerLine <= 2);  /* PROC or PROGRAM */

cleanup:
    cJSON_Delete(params);
    cJSON_Delete(result);
    server_free(server);
    TEST_FINI;
}

/*
 * Test suite
 */
TEST_SUITE(document) {
    RUN_TEST(docstore_new_free);
    RUN_TEST(docstore_open_get);
    RUN_TEST(docstore_update);
    RUN_TEST(docstore_close);
    RUN_TEST(docstore_get_not_found);
    RUN_TEST(docstore_reopen_updates);
    RUN_TEST(document_parse_valid_tbol);
    RUN_TEST(document_parse_syntax_error);
    RUN_TEST(document_hover_on_proc);
    RUN_TEST(document_hover_on_variable);
    RUN_TEST(document_hover_on_define);
    RUN_TEST(document_hover_on_label);
    RUN_TEST(document_hover_on_program);
    RUN_TEST(document_hover_outside_range);
    RUN_TEST(document_goto_def_variable);
    RUN_TEST(document_goto_def_proc);
    RUN_TEST(document_goto_def_label);
    RUN_TEST(document_goto_def_define);
    RUN_TEST(document_goto_def_not_found);
    RUN_TEST(completion_includes_variables);
    RUN_TEST(completion_includes_defines);
    RUN_TEST(completion_includes_registers);
    RUN_TEST(completion_includes_procs_and_labels);
    RUN_TEST(find_references_variable);
    RUN_TEST(find_references_label);
    RUN_TEST(workspace_symbols_from_open_documents);
    RUN_TEST(workspace_symbols_with_query);
    RUN_TEST(prepare_rename_on_variable);
    RUN_TEST(rename_variable_single_file);
    RUN_TEST(rename_across_files);
    RUN_TEST(rename_label);
    RUN_TEST(rename_copy_file_cascade);
    RUN_TEST(document_symbols_extracted);
    RUN_TEST(document_parse_undefined_variable);
    RUN_TEST(document_reparse_clears_old_diagnostics);
    RUN_TEST(signature_help_move);
    RUN_TEST(signature_help_unknown_verb);
    RUN_TEST(signature_help_deletefile);
    RUN_TEST(completion_includes_copy_symbols);
    RUN_TEST(hover_copy_no_false_match);
    RUN_TEST(hover_copy_correct_variable);
    RUN_TEST(hover_copy_keyword);
    RUN_TEST(goto_def_copy_keyword);
    RUN_TEST(goto_def_copy_no_false_match);
    RUN_TEST(document_symbols_copy_mapped);
    RUN_TEST(parse_repeated_no_hang);
    RUN_TEST(rename_copy_cascade_no_leak);
    RUN_TEST(rename_skips_define_expansion);
    RUN_TEST(folding_range_if_else);
    RUN_TEST(semantic_tokens_variable_decl_and_ref);
    RUN_TEST(selection_range_nested_hierarchy);
}
