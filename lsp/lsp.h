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
 * TBOL Language Server
 */
#ifndef TBOL_LSP_H
#define TBOL_LSP_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cJSON.h"
#include "../compiler/lexer/preproc_event.h"

/*
 * Logging (to stderr, not stdout which is for LSP messages)
 */
#define LOG_DEBUG 0
#define LOG_INFO  1
#define LOG_WARN  2
#define LOG_ERROR 3

extern int g_log_level;

#define log_debug(...) do { if (g_log_level <= LOG_DEBUG) fprintf(stderr, "[DEBUG] " __VA_ARGS__); } while(0)
#define log_info(...)  do { if (g_log_level <= LOG_INFO)  fprintf(stderr, "[INFO] " __VA_ARGS__); } while(0)
#define log_warn(...)  do { if (g_log_level <= LOG_WARN)  fprintf(stderr, "[WARN] " __VA_ARGS__); } while(0)
#define log_error(...) do { if (g_log_level <= LOG_ERROR) fprintf(stderr, "[ERROR] " __VA_ARGS__); } while(0)

/*
 * JSON-RPC message handling
 */
typedef struct {
    char *content;
    size_t length;
} Message;

/* Read a JSON-RPC message from stdin */
Message *message_read(FILE *in);
void message_free(Message *msg);

/* Write a JSON-RPC message to stdout */
void message_write(FILE *out, const char *content);

/* Build JSON-RPC response */
cJSON *jsonrpc_result(cJSON *id, cJSON *result);
cJSON *jsonrpc_error(cJSON *id, int code, const char *message);
cJSON *jsonrpc_notification(const char *method, cJSON *params);
cJSON *jsonrpc_request(int id, const char *method, cJSON *params);

/*
 * LSP Error codes
 */
#define LSP_ERROR_PARSE_ERROR      -32700
#define LSP_ERROR_INVALID_REQUEST  -32600
#define LSP_ERROR_METHOD_NOT_FOUND -32601
#define LSP_ERROR_INVALID_PARAMS   -32602
#define LSP_ERROR_INTERNAL_ERROR   -32603
#define LSP_ERROR_SERVER_NOT_INITIALIZED -32002
#define LSP_ERROR_REQUEST_CANCELLED -32800

/*
 * LSP Server state
 */
typedef enum {
    SERVER_UNINITIALIZED,
    SERVER_INITIALIZING,
    SERVER_RUNNING,
    SERVER_SHUTDOWN
} ServerState;

/* Forward declarations */
typedef struct DocumentStore DocumentStore;
typedef struct CopyIndex CopyIndex;

typedef struct {
    ServerState state;
    FILE *in;
    FILE *out;

    /* Client capabilities */
    bool client_supports_work_done_progress;
    bool client_supports_markdown;
    bool client_supports_configuration;

    /* Workspace info */
    char *root_uri;
    char **workspace_folders;
    int workspace_folder_count;

    /* Include paths for COPY resolution */
    char **include_paths;
    int include_path_count;

    /* Unreferenced symbol style: 0=none, 1=dim, 2=strikethrough, 3=both */
    int unreferenced_style;

    /* Pending server->client requests */
    int next_request_id;
    int pending_config_request_id;  /* 0 = none pending */

    /* Open documents */
    DocumentStore *documents;

    /* COPY file index: maps COPY file paths to parent .src paths */
    CopyIndex *copy_index;
} LSPServer;

/* Server lifecycle */
LSPServer *server_new(FILE *in, FILE *out);
void server_free(LSPServer *server);
int server_run(LSPServer *server);

/*
 * Request handlers
 */
cJSON *handle_initialize(LSPServer *server, cJSON *params);
cJSON *handle_shutdown(LSPServer *server, cJSON *params);
void handle_initialized(LSPServer *server, cJSON *params);
void handle_exit(LSPServer *server, cJSON *params);

/* Text document handlers */
void handle_did_open(LSPServer *server, cJSON *params);
void handle_did_change(LSPServer *server, cJSON *params);
void handle_did_close(LSPServer *server, cJSON *params);
void handle_did_save(LSPServer *server, cJSON *params);

/* Language features */
cJSON *handle_hover(LSPServer *server, cJSON *params);
cJSON *handle_completion(LSPServer *server, cJSON *params);
cJSON *handle_definition(LSPServer *server, cJSON *params);
cJSON *handle_references(LSPServer *server, cJSON *params);
cJSON *handle_document_symbol(LSPServer *server, cJSON *params);
cJSON *handle_workspace_symbol(LSPServer *server, cJSON *params);
cJSON *handle_prepare_rename(LSPServer *server, cJSON *params);
cJSON *handle_rename(LSPServer *server, cJSON *params);
cJSON *handle_signature_help(LSPServer *server, cJSON *params);
cJSON *handle_folding_range(LSPServer *server, cJSON *params);
cJSON *handle_semantic_tokens_full(LSPServer *server, cJSON *params);
cJSON *handle_selection_range(LSPServer *server, cJSON *params);
cJSON *handle_formatting(LSPServer *server, cJSON *params);

/* Response handlers (for server->client requests) */
void handle_configuration_response(LSPServer *server, cJSON *result);

/*
 * COPY file index - maps COPY files to their parent .src files
 */
typedef struct {
    char *copy_path;         /* Absolute path to COPY file */
    char **parent_paths;     /* Absolute paths of .src files that include it */
    int parent_count;
    int parent_capacity;
} CopyIndexEntry;

struct CopyIndex {
    CopyIndexEntry *entries;
    int count;
    int capacity;
};

CopyIndex *copy_index_new(void);
void copy_index_free(CopyIndex *index);
void copy_index_add(CopyIndex *index, const char *copy_path, const char *src_path);
const char *copy_index_find_parent(CopyIndex *index, const char *copy_path, DocumentStore *store);
char **copy_index_find_dependents(CopyIndex *index, const char *copy_path, int *count);
void copy_index_populate_from_events(CopyIndex *index, const char *src_path,
                                      PreprocEvent *events, int event_count);

/*
 * Document management
 */
typedef struct {
    char *uri;
    char *content;
    int version;
    /* Parsed data - populated after parsing */
    void *ast;           /* AstNode* from compiler */
    void *symbols;       /* SymbolTable* from compiler */
    /* Preprocessor events (COPY/DEFINE references) */
    PreprocEvent *preproc_events;
    int preproc_event_count;
    /* Diagnostics */
    cJSON *diagnostics;
    /* COPY file support */
    bool is_copy_file;       /* true for extensionless COPY files */
    char *parent_src_uri;    /* URI of parent .src used for parse (COPY files only) */
} Document;

struct DocumentStore {
    Document **documents;
    int count;
    int capacity;
};

DocumentStore *docstore_new(void);
void docstore_free(DocumentStore *store);
Document *docstore_open(DocumentStore *store, const char *uri, const char *content, int version);
Document *docstore_get(DocumentStore *store, const char *uri);
void docstore_update(DocumentStore *store, const char *uri, const char *content, int version);
void docstore_close(DocumentStore *store, const char *uri);

/*
 * Parsing integration
 * These will call into the compiler infrastructure
 */
void document_parse(Document *doc, const char **include_paths, int include_count, int unreferenced_style);
void document_parse_with_overrides(Document *doc, LSPServer *server);
void document_parse_copy(Document *copy_doc, LSPServer *server);
void document_publish_diagnostics(LSPServer *server, Document *doc);

/* URI/Path utilities */
char *path_to_uri(const char *path);

/* Workspace scanning */
char **workspace_find_src_copying(LSPServer *server, const char *copy_basename, int *out_count);

#endif /* TBOL_LSP_H */
