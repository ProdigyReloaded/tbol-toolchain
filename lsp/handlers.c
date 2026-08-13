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
 * LSP request handlers
 */
#include "lsp.h"
#include "../shared/ast.h"
#include "../shared/tbol_parse.h"
#include "../formatter/tbol_fmt.h"
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>  /* access, F_OK */
#include "../shared/util/strutil.h"  /* strcasestr fallback on Windows */

/*
 * URI/Path conversion utilities
 */
static char *uri_to_path(const char *uri) {
    if (!uri) return NULL;
    if (strncmp(uri, "file://", 7) != 0) {
        return strdup(uri);
    }
    const char *path = uri + 7;
    /* Handle Windows paths like file:///C:/... */
    if (path[0] == '/' && path[2] == ':') {
        path++;
    }
    /* URL decode */
    char *result = malloc(strlen(path) + 1);
    char *out = result;
    const char *in = path;
    while (*in) {
        if (*in == '%' && in[1] && in[2]) {
            char hex[3] = {in[1], in[2], 0};
            *out++ = (char)strtol(hex, NULL, 16);
            in += 3;
        } else {
            *out++ = *in++;
        }
    }
    *out = '\0';
    return result;
}

/* path_to_uri() is declared in lsp.h, implemented in document.c */

/*
 * Read entire file into memory
 */
static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    fseek(f, 0, SEEK_SET);

    char *content = malloc(size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(content, 1, size, f);
    content[read] = '\0';
    fclose(f);
    return content;
}

/*
 * Check if a file contains a COPY statement for a given file
 * Returns true if the file COPYs the target (case-insensitive)
 */
static bool file_copies(const char *filepath, const char *copy_basename) {
    char *content = read_file(filepath);
    if (!content) return false;

    bool found = false;
    char *p = content;

    /* Search for COPY statements */
    while ((p = strcasestr(p, "COPY")) != NULL) {
        /* Check it's a keyword (preceded by whitespace or start of line) */
        if (p > content && !isspace((unsigned char)p[-1]) && p[-1] != ';') {
            p += 4;
            continue;
        }
        p += 4;  /* Skip "COPY" */

        /* Skip whitespace */
        while (*p && isspace((unsigned char)*p)) p++;

        /* Extract the filename */
        char filename[256];
        int i = 0;
        while (*p && !isspace((unsigned char)*p) && *p != ';' && *p != ',' && i < 255) {
            filename[i++] = *p++;
        }
        filename[i] = '\0';

        /* Compare with target (case-insensitive) */
        if (strcasecmp(filename, copy_basename) == 0) {
            found = true;
            break;
        }
    }

    free(content);
    return found;
}

/*
 * Scan a directory recursively for .src files
 * Calls callback for each file found
 */
typedef void (*file_callback)(const char *path, void *userdata);

static void scan_directory(const char *dirpath, file_callback callback, void *userdata) {
    DIR *dir = opendir(dirpath);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;  /* Skip hidden files/dirs */

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);

        struct stat st;
        if (stat(path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            /* Skip directories that won't contain TBOL sources */
            if (strcmp(entry->d_name, "node_modules") == 0 ||
                strcmp(entry->d_name, "coverage") == 0) continue;
            /* Recurse into subdirectory */
            scan_directory(path, callback, userdata);
        } else if (S_ISREG(st.st_mode)) {
            /* Check if it's a .src file */
            size_t len = strlen(entry->d_name);
            if (len > 4 && strcasecmp(entry->d_name + len - 4, ".src") == 0) {
                callback(path, userdata);
            }
        }
    }
    closedir(dir);
}

/*
 * Data structure for COPY file scanning
 */
typedef struct {
    const char *copy_basename;  /* Name of the COPY file to search for */
    char **files;               /* Array of file paths that COPY it */
    int count;
    int capacity;
} CopyScanData;

static void copy_scan_callback(const char *path, void *userdata) {
    CopyScanData *data = (CopyScanData *)userdata;
    if (file_copies(path, data->copy_basename)) {
        if (data->count >= data->capacity) {
            data->capacity = data->capacity ? data->capacity * 2 : 16;
            data->files = realloc(data->files, data->capacity * sizeof(char *));
        }
        data->files[data->count++] = strdup(path);
    }
}

/*
 * Find all .src files in workspace that COPY a given file
 */
static char **find_files_that_copy(LSPServer *server, const char *copy_file, int *count) {
    CopyScanData data = {0};

    /* Extract basename from copy file path */
    const char *basename = strrchr(copy_file, '/');
    basename = basename ? basename + 1 : copy_file;
    data.copy_basename = basename;

    /* Scan workspace folders */
    if (server->workspace_folders && server->workspace_folder_count > 0) {
        for (int i = 0; i < server->workspace_folder_count; i++) {
            char *path = uri_to_path(server->workspace_folders[i]);
            if (path) {
                scan_directory(path, copy_scan_callback, &data);
                free(path);
            }
        }
    } else if (server->root_uri) {
        /* Fall back to root_uri only if no workspace folders are set.
         * rootUri is deprecated in LSP 3.x; workspaceFolders supersedes it.
         * Scanning both causes duplicate files and overlapping edits. */
        char *path = uri_to_path(server->root_uri);
        if (path) {
            scan_directory(path, copy_scan_callback, &data);
            free(path);
        }
    }

    *count = data.count;
    return data.files;
}

/* Forward declarations */
static void add_rename_edit(cJSON *changes, const char *uri, int start_line, int start_col,
                            int end_line, int end_col, const char *new_text);

/*
 * Detect if a URI refers to a COPY file (extensionless)
 */
static bool uri_is_copy_file(const char *uri) {
    char *path = uri_to_path(uri);
    if (!path) return false;
    const char *basename = strrchr(path, '/');
    basename = basename ? basename + 1 : path;
    bool is_copy = (strchr(basename, '.') == NULL);
    free(path);
    return is_copy;
}

/*
 * Context resolution for COPY files
 *
 * When operating on a COPY file, the AST uses "transparent" line numbers
 * (continuing from the parent .src file). This struct provides the offset
 * for converting between LSP (COPY-local) lines and AST (transparent) lines.
 */
typedef struct {
    Document *doc;
    const char *target_file;  /* Path for node filtering */
    int copy_start_line;      /* Transparent line of COPY insertion point (1-based) */
    bool is_copy;
} ResolvedContext;

static ResolvedContext resolve_context(LSPServer *server, const char *uri) {
    ResolvedContext ctx = {0};
    ctx.doc = docstore_get(server->documents, uri);
    if (!ctx.doc) return ctx;

    ctx.target_file = NULL;
    ctx.is_copy = ctx.doc->is_copy_file;
    ctx.copy_start_line = 0;

    if (ctx.is_copy && ctx.doc->preproc_events) {
        char *copy_path = uri_to_path(uri);
        if (copy_path) {
            /* Find the COPY event that matches this file */
            for (int i = 0; i < ctx.doc->preproc_event_count; i++) {
                PreprocEvent *evt = &ctx.doc->preproc_events[i];
                if (evt->kind == PREPROC_EVENT_COPY && evt->resolved_path &&
                    strcmp(evt->resolved_path, copy_path) == 0) {
                    ctx.copy_start_line = evt->line;
                    break;
                }
            }
            ctx.target_file = copy_path;  /* Note: caller must use this transiently */
            free(copy_path);
            /* target_file was freed, but we set it for the concept;
             * actual usage will re-derive it as needed */
            ctx.target_file = NULL;
        }
    }

    return ctx;
}

/* Convert LSP 0-based line to AST 1-based transparent line (for COPY files) */
static int lsp_to_ast_line(ResolvedContext *ctx, int lsp_line) {
    int ast_line = lsp_line + 1;  /* 0-based -> 1-based */
    if (ctx->is_copy && ctx->copy_start_line > 0) {
        ast_line = lsp_line + ctx->copy_start_line;  /* COPY-local -> transparent */
    }
    return ast_line;
}

/*
 * Check if a character is a TBOL identifier character
 */
static bool is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

/*
 * Extract the word under a cursor position from document content.
 * lsp_line and lsp_col are 0-based (LSP convention).
 * Returns a malloc'd string, or NULL if no word at position.
 * If out_col_1based is non-NULL, stores the 1-based column of the word start.
 */
static char *extract_word_at(const char *content, int lsp_line, int lsp_col, int *out_col_1based) {
    if (!content) return NULL;

    const char *p = content;
    for (int i = 0; i < lsp_line && *p; i++) {
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }
    const char *line_start = p;
    for (int i = 0; i < lsp_col && *p && *p != '\n'; i++) p++;

    if (!*p || *p == '\n' || !is_ident_char(*p)) return NULL;

    const char *word_start = p;
    while (word_start > line_start && is_ident_char(word_start[-1])) word_start--;
    const char *word_end = p;
    while (*word_end && is_ident_char(*word_end)) word_end++;

    int word_len = (int)(word_end - word_start);
    char *word = malloc(word_len + 1);
    memcpy(word, word_start, word_len);
    word[word_len] = '\0';

    if (out_col_1based) {
        *out_col_1based = (int)(word_start - line_start) + 1;
    }

    return word;
}

/*
 * Check if a 0-based cursor position is inside a comment or string literal.
 * TBOL comments use { ... } and strings use '...'.
 */
static bool is_in_comment_or_string(const char *content, int lsp_line, int lsp_col) {
    if (!content) return false;

    const char *p = content;
    int cur_line = 0, cur_col = 0;
    bool in_comment = false;
    bool in_string = false;

    while (*p) {
        if (cur_line == lsp_line && cur_col == lsp_col) {
            return in_comment || in_string;
        }

        if (in_comment) {
            if (*p == '}') in_comment = false;
        } else if (in_string) {
            if (*p == '\'' && p[1] == '\'') { cur_col++; p++; }
            else if (*p == '\\' && p[1]) { cur_col++; p++; }
            else if (*p == '\'') in_string = false;
        } else {
            if (*p == '{') in_comment = true;
            else if (*p == '\'') in_string = true;
        }

        if (*p == '\n') { cur_line++; cur_col = 0; } else { cur_col++; }
        p++;
    }

    return false;
}

/*
 * Collect rename edits from a file using text-based scanning.
 * Finds identifier occurrences bounded by non-identifier characters.
 * Skips comments { ... } and string literals '...' to avoid false matches.
 */
static void collect_text_rename_edits(const char *content, const char *old_name,
                                       const char *new_name, const char *uri,
                                       cJSON *changes) {
    if (!content || !old_name) return;
    size_t name_len = strlen(old_name);
    if (name_len == 0) return;

    int line = 1;
    int col = 1;
    const char *p = content;

    while (*p) {
        /* Skip comments { ... } */
        if (*p == '{') {
            col++; p++;
            while (*p && *p != '}') {
                if (*p == '\n') { line++; col = 1; } else { col++; }
                p++;
            }
            if (*p == '}') { col++; p++; }
            continue;
        }

        /* Skip string literals '...' */
        if (*p == '\'') {
            col++; p++;
            while (*p && *p != '\n') {
                if (*p == '\'' && p[1] == '\'') {
                    col += 2; p += 2;
                    continue;
                }
                if (*p == '\\' && p[1]) {
                    col += 2; p += 2;
                    continue;
                }
                if (*p == '\'') {
                    col++; p++;
                    break;
                }
                col++; p++;
            }
            continue;
        }

        /* Check for match at current position (case-insensitive) */
        if (strncasecmp(p, old_name, name_len) == 0) {
            /* Check word boundaries */
            bool left_bound = (p == content) || !is_ident_char(p[-1]);
            bool right_bound = !is_ident_char(p[name_len]);

            if (left_bound && right_bound) {
                /* Found a match - create edit (1-based line/col) */
                add_rename_edit(changes, uri,
                               line, col,
                               line, col + (int)name_len,
                               new_name);
            }
        }

        /* Advance position tracking */
        if (*p == '\n') {
            line++;
            col = 1;
        } else {
            col++;
        }
        p++;
    }
}

/*
 * Handle initialize request
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#initialize
 */
cJSON *handle_initialize(LSPServer *server, cJSON *params) {
    server->state = SERVER_INITIALIZING;

    /* Extract client capabilities */
    cJSON *capabilities = cJSON_GetObjectItem(params, "capabilities");
    if (capabilities) {
        cJSON *general = cJSON_GetObjectItem(capabilities, "general");
        if (general) {
            cJSON *markdown = cJSON_GetObjectItem(general, "markdown");
            if (markdown) {
                server->client_supports_markdown = true;
            }
        }

        cJSON *window = cJSON_GetObjectItem(capabilities, "window");
        if (window) {
            cJSON *workDoneProgress = cJSON_GetObjectItem(window, "workDoneProgress");
            if (cJSON_IsTrue(workDoneProgress)) {
                server->client_supports_work_done_progress = true;
            }
        }

        /* Check for workspace/configuration support */
        cJSON *workspace = cJSON_GetObjectItem(capabilities, "workspace");
        if (workspace) {
            cJSON *configuration = cJSON_GetObjectItem(workspace, "configuration");
            if (cJSON_IsTrue(configuration)) {
                server->client_supports_configuration = true;
                log_info("Client supports workspace/configuration\n");
            }
        }
    }

    /* Extract workspace info */
    cJSON *rootUri = cJSON_GetObjectItem(params, "rootUri");
    if (cJSON_IsString(rootUri)) {
        server->root_uri = strdup(rootUri->valuestring);
        log_info("Workspace root: %s\n", server->root_uri);
    }

    cJSON *workspaceFolders = cJSON_GetObjectItem(params, "workspaceFolders");
    if (cJSON_IsArray(workspaceFolders)) {
        int count = cJSON_GetArraySize(workspaceFolders);
        server->workspace_folders = calloc(count, sizeof(char *));
        server->workspace_folder_count = count;
        for (int i = 0; i < count; i++) {
            cJSON *folder = cJSON_GetArrayItem(workspaceFolders, i);
            cJSON *uri = cJSON_GetObjectItem(folder, "uri");
            if (cJSON_IsString(uri)) {
                server->workspace_folders[i] = strdup(uri->valuestring);
            }
        }
    }

    /* Build server capabilities response */
    cJSON *result = cJSON_CreateObject();

    cJSON *serverCapabilities = cJSON_CreateObject();

    /* Text document sync - full sync for now */
    cJSON *textDocumentSync = cJSON_CreateObject();
    cJSON_AddBoolToObject(textDocumentSync, "openClose", true);
    cJSON_AddNumberToObject(textDocumentSync, "change", 1);  /* 1 = Full sync */
    cJSON_AddBoolToObject(textDocumentSync, "save", true);
    cJSON_AddItemToObject(serverCapabilities, "textDocumentSync", textDocumentSync);

    /* Hover support */
    cJSON_AddBoolToObject(serverCapabilities, "hoverProvider", true);

    /* Completion support */
    cJSON *completionProvider = cJSON_CreateObject();
    cJSON *triggerChars = cJSON_CreateArray();
    cJSON_AddItemToArray(triggerChars, cJSON_CreateString("."));
    cJSON_AddItemToArray(triggerChars, cJSON_CreateString("("));
    cJSON_AddItemToObject(completionProvider, "triggerCharacters", triggerChars);
    cJSON_AddItemToObject(serverCapabilities, "completionProvider", completionProvider);

    /* Signature help support */
    cJSON *signatureHelpProvider = cJSON_CreateObject();
    cJSON *sigTriggerChars = cJSON_CreateArray();
    cJSON_AddItemToArray(sigTriggerChars, cJSON_CreateString(" "));
    cJSON_AddItemToArray(sigTriggerChars, cJSON_CreateString(","));
    cJSON_AddItemToObject(signatureHelpProvider, "triggerCharacters", sigTriggerChars);
    cJSON_AddItemToObject(serverCapabilities, "signatureHelpProvider", signatureHelpProvider);

    /* Definition support */
    cJSON_AddBoolToObject(serverCapabilities, "definitionProvider", true);

    /* References support */
    cJSON_AddBoolToObject(serverCapabilities, "referencesProvider", true);

    /* Document symbol support */
    cJSON_AddBoolToObject(serverCapabilities, "documentSymbolProvider", true);

    /* Workspace symbol support */
    cJSON_AddBoolToObject(serverCapabilities, "workspaceSymbolProvider", true);

    /* Rename support with prepare */
    cJSON *renameProvider = cJSON_CreateObject();
    cJSON_AddBoolToObject(renameProvider, "prepareProvider", true);
    cJSON_AddItemToObject(serverCapabilities, "renameProvider", renameProvider);

    /* Document formatting support */
    cJSON_AddBoolToObject(serverCapabilities, "documentFormattingProvider", true);

    /* Folding range support */
    cJSON_AddBoolToObject(serverCapabilities, "foldingRangeProvider", true);

    /* Selection range support */
    cJSON_AddBoolToObject(serverCapabilities, "selectionRangeProvider", true);

    /* Semantic tokens support */
    cJSON *semanticTokensProvider = cJSON_CreateObject();
    cJSON *stLegend = cJSON_CreateObject();
    cJSON *tokenTypes = cJSON_CreateArray();
    cJSON_AddItemToArray(tokenTypes, cJSON_CreateString("variable"));
    cJSON_AddItemToArray(tokenTypes, cJSON_CreateString("function"));
    cJSON_AddItemToArray(tokenTypes, cJSON_CreateString("macro"));
    cJSON_AddItemToArray(tokenTypes, cJSON_CreateString("label"));
    cJSON_AddItemToObject(stLegend, "tokenTypes", tokenTypes);
    cJSON *tokenModifiers = cJSON_CreateArray();
    cJSON_AddItemToArray(tokenModifiers, cJSON_CreateString("declaration"));
    cJSON_AddItemToObject(stLegend, "tokenModifiers", tokenModifiers);
    cJSON_AddItemToObject(semanticTokensProvider, "legend", stLegend);
    cJSON *stFull = cJSON_CreateObject();
    cJSON_AddBoolToObject(stFull, "delta", false);
    cJSON_AddItemToObject(semanticTokensProvider, "full", stFull);
    cJSON_AddItemToObject(serverCapabilities, "semanticTokensProvider", semanticTokensProvider);

    cJSON_AddItemToObject(result, "capabilities", serverCapabilities);

    /* Server info */
    cJSON *serverInfo = cJSON_CreateObject();
    cJSON_AddStringToObject(serverInfo, "name", "tbol-lsp");
    cJSON_AddStringToObject(serverInfo, "version", "0.1.0");
    cJSON_AddItemToObject(result, "serverInfo", serverInfo);

    return result;
}

/*
 * Handle initialized notification
 */
void handle_initialized(LSPServer *server, cJSON *params) {
    (void)params;
    server->state = SERVER_RUNNING;
    log_info("Server initialized\n");

    /* Request configuration from client if supported */
    if (server->client_supports_configuration) {
        cJSON *configParams = cJSON_CreateObject();
        cJSON *items = cJSON_CreateArray();

        /* Request tbol configuration section */
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "section", "tbol");
        cJSON_AddItemToArray(items, item);

        cJSON_AddItemToObject(configParams, "items", items);

        /* Send request with unique ID */
        server->pending_config_request_id = ++server->next_request_id;
        cJSON *request = jsonrpc_request(server->pending_config_request_id,
                                         "workspace/configuration", configParams);
        char *str = cJSON_PrintUnformatted(request);
        message_write(server->out, str);
        free(str);
        cJSON_Delete(request);

        log_info("Sent workspace/configuration request (id=%d)\n",
                 server->pending_config_request_id);
    }
}

/*
 * Handle workspace/configuration response
 */
void handle_configuration_response(LSPServer *server, cJSON *result) {
    if (!cJSON_IsArray(result)) {
        log_warn("workspace/configuration result is not an array\n");
        return;
    }

    /* Result is an array of configuration objects (one per item requested) */
    cJSON *tbolConfig = cJSON_GetArrayItem(result, 0);
    if (!tbolConfig || cJSON_IsNull(tbolConfig)) {
        log_info("No tbol configuration from client\n");
        return;
    }

    /* Extract unreferenced symbol style */
    cJSON *unreferencedStyle = cJSON_GetObjectItem(tbolConfig, "unreferencedStyle");
    if (cJSON_IsString(unreferencedStyle)) {
        const char *val = unreferencedStyle->valuestring;
        if (strcmp(val, "none") == 0)          server->unreferenced_style = 0;
        else if (strcmp(val, "dim") == 0)       server->unreferenced_style = 1;
        else if (strcmp(val, "strikethrough") == 0) server->unreferenced_style = 2;
        else if (strcmp(val, "both") == 0)      server->unreferenced_style = 3;
        log_info("Unreferenced style: %s (%d)\n", val, server->unreferenced_style);
    }

    /* Extract include paths */
    cJSON *includePaths = cJSON_GetObjectItem(tbolConfig, "includePaths");
    if (cJSON_IsArray(includePaths)) {
        /* Free existing paths */
        for (int i = 0; i < server->include_path_count; i++) {
            free(server->include_paths[i]);
        }
        free(server->include_paths);

        int count = cJSON_GetArraySize(includePaths);
        server->include_paths = calloc(count, sizeof(char *));
        server->include_path_count = 0;

        for (int i = 0; i < count; i++) {
            cJSON *path = cJSON_GetArrayItem(includePaths, i);
            if (cJSON_IsString(path)) {
                server->include_paths[server->include_path_count++] = strdup(path->valuestring);
                log_info("Include path: %s\n", path->valuestring);
            }
        }

        log_info("Configured %d include paths\n", server->include_path_count);

        /* Re-parse all open documents with new include paths.
         * Parse .src files first, then COPY files (which depend on .src parses). */
        if (server->documents && server->include_path_count > 0) {
            /* First pass: .src files */
            for (int i = 0; i < server->documents->count; i++) {
                Document *d = server->documents->documents[i];
                if (d && !d->is_copy_file) {
                    document_parse_with_overrides(d, server);
                    if (d->preproc_events) {
                        char *d_path = uri_to_path(d->uri);
                        if (d_path) {
                            copy_index_populate_from_events(server->copy_index, d_path,
                                                             d->preproc_events, d->preproc_event_count);
                            free(d_path);
                        }
                    }
                    document_publish_diagnostics(server, d);
                }
            }
            /* Second pass: COPY files */
            for (int i = 0; i < server->documents->count; i++) {
                Document *d = server->documents->documents[i];
                if (d && d->is_copy_file) {
                    document_parse_copy(d, server);
                    document_publish_diagnostics(server, d);
                }
            }
        }
    }
}

/*
 * Handle shutdown request
 */
cJSON *handle_shutdown(LSPServer *server, cJSON *params) {
    (void)params;
    server->state = SERVER_SHUTDOWN;
    log_info("Server shutting down\n");
    return NULL;  /* null result */
}

/*
 * Handle exit notification
 */
void handle_exit(LSPServer *server, cJSON *params) {
    (void)params;
    log_info("Server exiting\n");
    /* Exit with 0 if shutdown was called, 1 otherwise */
    exit(server->state == SERVER_SHUTDOWN ? 0 : 1);
}

/*
 * Handle textDocument/didOpen notification
 */
void handle_did_open(LSPServer *server, cJSON *params) {
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    if (!textDocument) return;

    cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
    cJSON *text = cJSON_GetObjectItem(textDocument, "text");
    cJSON *version = cJSON_GetObjectItem(textDocument, "version");

    if (!cJSON_IsString(uri) || !cJSON_IsString(text)) return;

    int ver = cJSON_IsNumber(version) ? (int)version->valuedouble : 0;

    /* Store document */
    Document *doc = docstore_open(server->documents, uri->valuestring, text->valuestring, ver);

    /* Detect COPY files (extensionless) */
    doc->is_copy_file = uri_is_copy_file(uri->valuestring);

    log_info("Document opened: %s (version %d, copy=%d)\n",
             uri->valuestring, ver, doc->is_copy_file);

    /* Parse and publish diagnostics */
    if (doc->is_copy_file) {
        document_parse_copy(doc, server);
    } else {
        document_parse_with_overrides(doc, server);
        /* Populate CopyIndex from parse results */
        if (doc->preproc_events) {
            char *doc_path = uri_to_path(uri->valuestring);
            if (doc_path) {
                copy_index_populate_from_events(server->copy_index, doc_path,
                                                 doc->preproc_events, doc->preproc_event_count);
                free(doc_path);
            }
        }
    }
    document_publish_diagnostics(server, doc);
}

/*
 * Handle textDocument/didChange notification
 */
void handle_did_change(LSPServer *server, cJSON *params) {
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    if (!textDocument) return;

    cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
    cJSON *version = cJSON_GetObjectItem(textDocument, "version");
    cJSON *contentChanges = cJSON_GetObjectItem(params, "contentChanges");

    if (!cJSON_IsString(uri) || !cJSON_IsArray(contentChanges)) return;

    int ver = cJSON_IsNumber(version) ? (int)version->valuedouble : 0;

    /* For full sync, take the first (and only) change */
    cJSON *change = cJSON_GetArrayItem(contentChanges, 0);
    cJSON *text = cJSON_GetObjectItem(change, "text");

    if (!cJSON_IsString(text)) return;

    log_debug("Document changed: %s (version %d)\n", uri->valuestring, ver);

    /* Update document content */
    docstore_update(server->documents, uri->valuestring, text->valuestring, ver);

    /* Reparse and publish diagnostics */
    Document *doc = docstore_get(server->documents, uri->valuestring);
    if (doc) {
        if (doc->is_copy_file) {
            /* Re-parse COPY file through parent */
            document_parse_copy(doc, server);
            document_publish_diagnostics(server, doc);

            /* Cascade: re-parse any open .src files that include this COPY file */
            char *copy_path = uri_to_path(uri->valuestring);
            if (copy_path) {
                int dep_count = 0;
                char **deps = copy_index_find_dependents(server->copy_index, copy_path, &dep_count);
                for (int i = 0; i < dep_count; i++) {
                    char *dep_uri = path_to_uri(deps[i]);
                    Document *dep_doc = docstore_get(server->documents, dep_uri);
                    if (dep_doc && !dep_doc->is_copy_file) {
                        document_parse_with_overrides(dep_doc, server);
                        document_publish_diagnostics(server, dep_doc);
                    }
                    free(dep_uri);
                    free(deps[i]);
                }
                free(deps);
                free(copy_path);
            }
        } else {
            document_parse_with_overrides(doc, server);
            /* Update CopyIndex */
            if (doc->preproc_events) {
                char *doc_path = uri_to_path(uri->valuestring);
                if (doc_path) {
                    copy_index_populate_from_events(server->copy_index, doc_path,
                                                     doc->preproc_events, doc->preproc_event_count);
                    free(doc_path);
                }
            }
            document_publish_diagnostics(server, doc);

            /* Cascade: re-parse any open COPY files that use this .src as parent */
            for (int i = 0; i < server->documents->count; i++) {
                Document *d = server->documents->documents[i];
                if (d && d->is_copy_file && d->parent_src_uri &&
                    strcmp(d->parent_src_uri, uri->valuestring) == 0) {
                    document_parse_copy(d, server);
                    document_publish_diagnostics(server, d);
                }
            }
        }
    }
}

/*
 * Handle textDocument/didClose notification
 */
void handle_did_close(LSPServer *server, cJSON *params) {
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    if (!textDocument) return;

    cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
    if (!cJSON_IsString(uri)) return;

    log_info("Document closed: %s\n", uri->valuestring);

    /* Clear diagnostics for the closed document */
    Document *doc = docstore_get(server->documents, uri->valuestring);
    if (doc) {
        if (doc->diagnostics) {
            cJSON_Delete(doc->diagnostics);
            doc->diagnostics = NULL;
        }
        /* Publish empty diagnostics to clear them in the editor */
        document_publish_diagnostics(server, doc);
    }

    /* Remove document from store */
    docstore_close(server->documents, uri->valuestring);
}

/*
 * Handle textDocument/didSave notification
 */
void handle_did_save(LSPServer *server, cJSON *params) {
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    if (!textDocument) return;

    cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
    if (!cJSON_IsString(uri)) return;

    log_info("Document saved: %s\n", uri->valuestring);

    /* Reparse and publish diagnostics */
    Document *doc = docstore_get(server->documents, uri->valuestring);
    if (doc) {
        if (doc->is_copy_file) {
            document_parse_copy(doc, server);
        } else {
            document_parse_with_overrides(doc, server);
            /* Update CopyIndex */
            if (doc->preproc_events) {
                char *doc_path = uri_to_path(uri->valuestring);
                if (doc_path) {
                    copy_index_populate_from_events(server->copy_index, doc_path,
                                                     doc->preproc_events, doc->preproc_event_count);
                    free(doc_path);
                }
            }
        }
        document_publish_diagnostics(server, doc);
    }
}

/*
 * Check if a position is within a range
 */
static bool position_in_range(int line, int col, SourceRange *range) {
    /* LSP is 0-based, AST is 1-based */
    int ast_line = line + 1;
    int ast_col = col + 1;

    if (ast_line < range->start.line) return false;
    if (ast_line > range->end.line) return false;
    if (ast_line == range->start.line && ast_col < range->start.column) return false;
    if (ast_line == range->end.line && ast_col > range->end.column) return false;
    return true;
}

/*
 * Find AST node at position (best match)
 * Walks entire tree because parent ranges may not fully encompass children.
 * When target_file is non-NULL, only nodes from that file are considered
 * as matches (but children are always recursed into, since a container
 * from a different file may hold children from the target file).
 */
static AstNode *find_node_at_position_impl(AstNode *node, int line, int col,
                                            const char *target_file, AstNode **best) {
    if (!node) return *best;

    /* Check if position is in this node's range */
    if (position_in_range(line, col, &node->range)) {
        /* Only consider as a match if from the target file (or no filter) */
        bool file_matches = true;
        if (target_file && node->range.start.filename) {
            file_matches = (strcmp(node->range.start.filename, target_file) == 0);
        }

        if (file_matches) {
            /* This node matches. It's "better" than current best if:
             * 1. No current best, or
             * 2. This node is more specific (smaller range or deeper in tree)
             */
            if (!*best) {
                *best = node;
            } else {
                /* Prefer nodes with smaller ranges (more specific) */
                int best_lines = (*best)->range.end.line - (*best)->range.start.line;
                int this_lines = node->range.end.line - node->range.start.line;
                if (this_lines < best_lines ||
                    (this_lines == best_lines &&
                     node->range.end.column - node->range.start.column <
                     (*best)->range.end.column - (*best)->range.start.column)) {
                    *best = node;
                }
            }
        }
    }

    /* Always check children - parent ranges may be incorrect */
    for (int i = 0; i < node->child_count; i++) {
        find_node_at_position_impl(node->children[i], line, col, target_file, best);
    }

    return *best;
}

static AstNode *find_node_at_position(AstNode *node, int line, int col, const char *target_file) {
    AstNode *best = NULL;
    return find_node_at_position_impl(node, line, col, target_file, &best);
}

/*
 * Get hover info for a node
 */
static cJSON *get_hover_info(AstNode *node) {
    if (!node) return NULL;

    char info[512];

    switch (node->kind) {
        case AST_PROGRAM:
            snprintf(info, sizeof(info), "**Program**: `%s`",
                    node->data.program.name ? node->data.program.name : "<unknown>");
            break;

        case AST_DATA_SECTION:
            snprintf(info, sizeof(info), "**DATA Section**: `%s`",
                    node->data.data_section.name ? node->data.data_section.name : "<unknown>");
            break;

        case AST_VAR_DECL: {
            const char *varname = node->data.var_decl.name ? node->data.var_decl.name : "<unknown>";
            const char *source = node->range.start.filename;
            const char *basename = source ? strrchr(source, '/') : NULL;
            basename = basename ? basename + 1 : source;
            if (node->data.var_decl.array_size > 0) {
                if (basename) {
                    snprintf(info, sizeof(info), "**Variable**: `%s(%d)`\n\nArray of %d elements\n\nDefined in: `%s`",
                            varname, node->data.var_decl.array_size, node->data.var_decl.array_size, basename);
                } else {
                    snprintf(info, sizeof(info), "**Variable**: `%s(%d)`\n\nArray of %d elements",
                            varname, node->data.var_decl.array_size, node->data.var_decl.array_size);
                }
            } else {
                if (basename) {
                    snprintf(info, sizeof(info), "**Variable**: `%s`\n\nDefined in: `%s`", varname, basename);
                } else {
                    snprintf(info, sizeof(info), "**Variable**: `%s`", varname);
                }
            }
            break;
        }

        case AST_DEFINE: {
            const char *defname = node->data.define.name ? node->data.define.name : "<unknown>";
            const char *defval = node->data.define.value ? node->data.define.value : "";
            const char *source = node->range.start.filename;
            const char *basename = source ? strrchr(source, '/') : NULL;
            basename = basename ? basename + 1 : source;
            if (basename) {
                snprintf(info, sizeof(info), "**DEFINE**: `%s`\n\nValue: `%s`\n\nDefined in: `%s`",
                        defname, defval, basename);
            } else {
                snprintf(info, sizeof(info), "**DEFINE**: `%s`\n\nValue: `%s`", defname, defval);
            }
            break;
        }

        case AST_PROC:
            snprintf(info, sizeof(info), "**Procedure**: `%s`",
                    node->data.proc.name ? node->data.proc.name : "<unknown>");
            break;

        case AST_LABEL:
            snprintf(info, sizeof(info), "**Label**: `%s:`",
                    node->data.label.name ? node->data.label.name : "<unknown>");
            break;

        case AST_VERB_STMT:
            snprintf(info, sizeof(info), "**Statement**: `%s`\n\nTBOL verb statement",
                    node->data.call.name ? node->data.call.name : "<unknown>");
            break;

        case AST_IDENT:
            snprintf(info, sizeof(info), "**Identifier**: `%s`",
                    node->data.ident.name ? node->data.ident.name : "<unknown>");
            break;

        case AST_REG_I:
            snprintf(info, sizeof(info), "**I Register**: `I%d`\n\nInteger register",
                    node->data.reg.number);
            break;

        case AST_REG_D:
            snprintf(info, sizeof(info), "**D Register**: `D%d`\n\nDecimal register",
                    node->data.reg.number);
            break;

        case AST_REG_P:
            snprintf(info, sizeof(info), "**P Register**: `P%d`\n\nPointer register",
                    node->data.reg.number);
            break;

        case AST_PEV:
            snprintf(info, sizeof(info), "**PEV**: `PEV%d`\n\nProgram Environment Variable",
                    node->data.ext_var.number);
            break;

        case AST_GEV:
            snprintf(info, sizeof(info), "**GEV**: `GEV%d`\n\nGlobal Environment Variable",
                    node->data.ext_var.number);
            break;

        default:
            return NULL;  /* No hover info for this node type */
    }

    cJSON *hover = cJSON_CreateObject();
    cJSON *contents = cJSON_CreateObject();
    cJSON_AddStringToObject(contents, "kind", "markdown");
    cJSON_AddStringToObject(contents, "value", info);
    cJSON_AddItemToObject(hover, "contents", contents);

    return hover;
}

/*
 * Handle textDocument/hover request
 */
cJSON *handle_hover(LSPServer *server, cJSON *params) {
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    cJSON *position = cJSON_GetObjectItem(params, "position");

    if (!textDocument || !position) return NULL;

    cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
    cJSON *line = cJSON_GetObjectItem(position, "line");
    cJSON *character = cJSON_GetObjectItem(position, "character");

    if (!cJSON_IsString(uri) || !cJSON_IsNumber(line) || !cJSON_IsNumber(character)) {
        return NULL;
    }

    /* Find the document */
    ResolvedContext ctx = resolve_context(server, uri->valuestring);
    Document *doc = ctx.doc;
    if (!doc || !doc->ast) return NULL;

    char *doc_path = uri_to_path(uri->valuestring);

    /* Compute AST line: for COPY files, translate to transparent line number */
    int lsp_line = (int)line->valuedouble;
    int ast_line = lsp_to_ast_line(&ctx, lsp_line);
    int ast_col = (int)character->valuedouble + 1;

    /* For COPY files, search the preproc events using the COPY file path
     * and transparent line number */
    if (doc->preproc_events && !ctx.is_copy) {
        PreprocEvent *evt = preproc_event_find_at(
            doc->preproc_events, doc->preproc_event_count,
            doc_path, ast_line, ast_col);
        if (evt) {
            char info[512];
            info[0] = '\0';
            if (evt->kind == PREPROC_EVENT_COPY) {
                if (evt->resolved_path) {
                    snprintf(info, sizeof(info), "**COPY**: `%s`\n\nFile: `%s`",
                            evt->name, evt->resolved_path);
                } else {
                    snprintf(info, sizeof(info), "**COPY**: `%s`\n\n*File not found*",
                            evt->name);
                }
            } else if (evt->kind == PREPROC_EVENT_DEFINE_REF) {
                if (evt->def_filename) {
                    const char *basename = strrchr(evt->def_filename, '/');
                    basename = basename ? basename + 1 : evt->def_filename;
                    snprintf(info, sizeof(info),
                            "**DEFINE**: `%s`\n\nValue: `%s`\n\nDefined in: `%s`",
                            evt->name, evt->define_value ? evt->define_value : "",
                            basename);
                } else {
                    snprintf(info, sizeof(info),
                            "**DEFINE**: `%s`\n\nValue: `%s`",
                            evt->name, evt->define_value ? evt->define_value : "");
                }
            }
            if (info[0] != '\0') {
                free(doc_path);
                cJSON *result = cJSON_CreateObject();
                cJSON *contents = cJSON_CreateObject();
                cJSON_AddStringToObject(contents, "kind", "markdown");
                cJSON_AddStringToObject(contents, "value", info);
                cJSON_AddItemToObject(result, "contents", contents);
                return result;
            }
        }
    }

    /* Find node at position - use AST line (0-based for position_in_range) */
    AstNode *ast = (AstNode *)doc->ast;
    const char *target_file = doc_path;
    AstNode *node = find_node_at_position(ast, ast_line - 1, (int)character->valuedouble, target_file);
    free(doc_path);

    if (!node) return NULL;

    return get_hover_info(node);
}

/*
 * LSP Completion Item Kinds
 */
#define COMPLETION_TEXT         1
#define COMPLETION_METHOD       2
#define COMPLETION_FUNCTION     3
#define COMPLETION_CONSTRUCTOR  4
#define COMPLETION_FIELD        5
#define COMPLETION_VARIABLE     6
#define COMPLETION_CLASS        7
#define COMPLETION_INTERFACE    8
#define COMPLETION_MODULE       9
#define COMPLETION_PROPERTY     10
#define COMPLETION_UNIT         11
#define COMPLETION_VALUE        12
#define COMPLETION_ENUM         13
#define COMPLETION_KEYWORD      14
#define COMPLETION_SNIPPET      15
#define COMPLETION_COLOR        16
#define COMPLETION_FILE         17
#define COMPLETION_REFERENCE    18
#define COMPLETION_FOLDER       19
#define COMPLETION_ENUMMEMBER   20
#define COMPLETION_CONSTANT     21
#define COMPLETION_STRUCT       22
#define COMPLETION_EVENT        23
#define COMPLETION_OPERATOR     24
#define COMPLETION_TYPEPARAM    25

/*
 * Collect names from AST of a given kind
 * main_filename: the main document's filename (to detect symbols from COPY files)
 */
static void collect_names_with_origin(AstNode *node, AstNodeKind kind, cJSON *items,
                                       int completion_kind, const char *main_filename) {
    if (!node) return;

    if (node->kind == kind) {
        const char *name = NULL;
        const char *original = NULL;
        const char *base_detail = NULL;
        switch (kind) {
            case AST_VAR_DECL:
                name = node->data.var_decl.name;
                original = node->data.var_decl.original_text;
                base_detail = node->data.var_decl.array_size > 0 ? "Array variable" : "Variable";
                break;
            case AST_PROC:
                name = node->data.proc.name;
                original = node->data.proc.original_text;
                base_detail = "Procedure";
                break;
            case AST_LABEL:
                name = node->data.label.name;
                original = node->data.label.original_text;
                base_detail = "Label";
                break;
            case AST_DEFINE:
                name = node->data.define.name;
                original = node->data.define.original_text;
                base_detail = node->data.define.value;
                break;
            default:
                break;
        }
        if (name) {
            const char *display = original ? original : name;
            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "label", display);
            cJSON_AddStringToObject(item, "filterText", name);
            cJSON_AddStringToObject(item, "insertText", display);
            cJSON_AddNumberToObject(item, "kind", completion_kind);

            /* Build detail string, including source file if from COPY */
            const char *source_file = node->range.start.filename;
            char detail_buf[256];

            /* Compare basenames to detect COPY file symbols */
            const char *source_base = source_file ? strrchr(source_file, '/') : NULL;
            source_base = source_base ? source_base + 1 : source_file;
            const char *main_base = main_filename ? strrchr(main_filename, '/') : NULL;
            main_base = main_base ? main_base + 1 : main_filename;

            bool from_copy = source_file && main_filename &&
                             source_base && main_base &&
                             strcasecmp(source_base, main_base) != 0;

            if (from_copy) {
                /* Symbol is from a COPY file */
                if (base_detail) {
                    snprintf(detail_buf, sizeof(detail_buf), "%s (from %s)", base_detail, source_base);
                } else {
                    snprintf(detail_buf, sizeof(detail_buf), "from %s", source_base);
                }
                cJSON_AddStringToObject(item, "detail", detail_buf);
            } else if (base_detail) {
                cJSON_AddStringToObject(item, "detail", base_detail);
            }
            cJSON_AddItemToArray(items, item);
        }
    }

    for (int i = 0; i < node->child_count; i++) {
        collect_names_with_origin(node->children[i], kind, items, completion_kind, main_filename);
    }
}

/*
 * Handle textDocument/completion request
 */
cJSON *handle_completion(LSPServer *server, cJSON *params) {
    cJSON *items = cJSON_CreateArray();

    /* Check if the cursor is in a name-definition context (after PROC, PROGRAM,
     * DEFINE, or DATA group name position). In these positions the user is typing
     * a NEW name, not referencing an existing symbol - return empty completions. */
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    cJSON *position = cJSON_GetObjectItem(params, "position");
    if (textDocument && position) {
        cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
        cJSON *pos_line = cJSON_GetObjectItem(position, "line");
        cJSON *pos_char = cJSON_GetObjectItem(position, "character");
        if (cJSON_IsString(uri) && cJSON_IsNumber(pos_line) && cJSON_IsNumber(pos_char)) {
            Document *doc = docstore_get(server->documents, uri->valuestring);
            if (doc && doc->content) {
                int line = (int)pos_line->valuedouble;
                int col = (int)pos_char->valuedouble;

                /* Extract the text of the current line */
                const char *p = doc->content;
                int cur_line = 0;
                while (cur_line < line && *p) {
                    if (*p == '\n') cur_line++;
                    p++;
                }
                /* p now points to the start of the cursor's line */
                char line_buf[1024];
                int len = 0;
                while (p[len] && p[len] != '\n' && len < (int)sizeof(line_buf) - 1) {
                    line_buf[len] = p[len];
                    len++;
                }
                line_buf[len] = '\0';

                /* Scan backwards from cursor to find the preceding keyword.
                 * Skip the word the user is currently typing, then find
                 * the previous whitespace-delimited token. */
                int i = col < len ? col : len;
                /* Skip current word (what user is typing) */
                while (i > 0 && line_buf[i - 1] != ' ' && line_buf[i - 1] != '\t') i--;
                /* Skip whitespace */
                while (i > 0 && (line_buf[i - 1] == ' ' || line_buf[i - 1] == '\t')) i--;
                /* Extract preceding token */
                int tok_end = i;
                while (i > 0 && line_buf[i - 1] != ' ' && line_buf[i - 1] != '\t') i--;
                int tok_len = tok_end - i;

                /* Check preceding token for name-defining keywords */
                if (tok_len > 0 && tok_len < 16) {
                    char prev_token[16];
                    memcpy(prev_token, line_buf + i, tok_len);
                    prev_token[tok_len] = '\0';

                    /* Suppress completions after keywords that introduce new names */
                    if (strcasecmp(prev_token, "PROC") == 0 ||
                        strcasecmp(prev_token, "PROGRAM") == 0 ||
                        strcasecmp(prev_token, "DEFINE") == 0 ||
                        strcasecmp(prev_token, "COPY") == 0 ||
                        strcasecmp(prev_token, "DATA") == 0) {
                        return items;  /* Empty array */
                    }
                }

                /* Check if we're in the DATA region of the program.
                 * Uses the last valid AST, which handles COPY files correctly.
                 * During mid-edit with a broken parse, the AST may be stale and
                 * completions may briefly appear - this resolves when the LSP
                 * reparses. See TODO.md for the proper fix (error-tolerant
                 * parsing or grammar-based cursor context). */
                if (doc->ast) {
                    AstNode *ast = (AstNode *)doc->ast;
                    if (ast->kind == AST_PROGRAM) {
                        int first_data_line = -1;
                        int first_proc_line = -1;
                        for (int ci = 0; ci < ast->child_count; ci++) {
                            AstNode *child = ast->children[ci];
                            if (child->kind == AST_DATA_SECTION && first_data_line < 0)
                                first_data_line = child->range.start.line;
                            if (child->kind == AST_PROC && first_proc_line < 0)
                                first_proc_line = child->range.start.line;
                        }
                        int cursor_line_1 = line + 1;
                        if (first_data_line >= 0 &&
                            cursor_line_1 >= first_data_line &&
                            (first_proc_line < 0 || cursor_line_1 < first_proc_line)) {
                            return items;  /* Empty array */
                        }
                    }
                }
            }
        }
    }

    /* TBOL keywords (excluding those offered as snippets below) */
    const char *keywords[] = {
        "PROGRAM", "END_PROC", "DATA", "COPY", "DEFINE",
        "THEN", "ELSE", "END", "GOTO", "EXIT", "RETURN",
        "MOVE", "ADD", "SUBTRACT", "MULTIPLY", "DIVIDE",
        "STRING", "SUBSTR", "INSTR", "LENGTH", "UPPERCASE",
        "CLEAR", "SAVE", "RESTORE", "RELEASE",
        "LINK", "TRANSFER", "NAVIGATE", "SEND", "FETCH",
        "SORT", "LOOKUP", "FILL", "SWAP", "PUSH", "POP",
        "NOTE", "POINT", "OPEN", "CLOSE", "READ", "WRITE",
        "SET_FUNCTION", "SET_ATTRIBUTE", "DEFINE_FIELD", "MAKE_FORMAT",
        "ERROR", "SOUND", "SET_SOUND",
        NULL
    };

    for (int i = 0; keywords[i]; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "label", keywords[i]);
        cJSON_AddNumberToObject(item, "kind", COMPLETION_KEYWORD);
        cJSON_AddItemToArray(items, item);
    }

    /* Snippet completions - these expand into structured templates.
     * insertTextFormat: 2 = Snippet (uses $1, $0 tab stops) */
    typedef struct { const char *label; const char *detail; const char *body; } SnippetDef;
    static const SnippetDef snippets[] = {
        {"PROC",    "Procedure definition",
         "PROC ${1:name} =\n\t$0\nEND_PROC"},
        {"IF",      "IF with single statement",
         "IF ${1:condition} THEN\n\t$0"},
        {"WHILE",   "WHILE loop",
         "WHILE ${1:condition} THEN\n\t$0"},
        {"DO",      "DO/END block (after THEN or ELSE)",
         "DO\n\t$0\nEND;"},
        {NULL, NULL, NULL}
    };

    for (int i = 0; snippets[i].label; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "label", snippets[i].label);
        cJSON_AddNumberToObject(item, "kind", COMPLETION_SNIPPET);
        cJSON_AddStringToObject(item, "detail", snippets[i].detail);
        cJSON_AddStringToObject(item, "insertText", snippets[i].body);
        cJSON_AddNumberToObject(item, "insertTextFormat", 2);  /* Snippet format */
        cJSON_AddNumberToObject(item, "insertTextMode", 2);    /* adjustIndentation */
        cJSON_AddItemToArray(items, item);
    }

    /* Add context-aware completions from the document's AST */
    if (textDocument) {
        cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
        if (cJSON_IsString(uri)) {
            Document *doc = docstore_get(server->documents, uri->valuestring);
            if (doc && doc->ast) {
                AstNode *ast = (AstNode *)doc->ast;

                /* Get main document's filename for COPY detection */
                char *main_path = uri_to_path(uri->valuestring);
                const char *main_filename = main_path;

                /* Add variables from AST (showing origin file for COPY symbols) */
                collect_names_with_origin(ast, AST_VAR_DECL, items, COMPLETION_VARIABLE, main_filename);

                /* Add procedures */
                collect_names_with_origin(ast, AST_PROC, items, COMPLETION_FUNCTION, main_filename);

                /* Add labels */
                collect_names_with_origin(ast, AST_LABEL, items, COMPLETION_REFERENCE, main_filename);

                /* Add DEFINEs */
                collect_names_with_origin(ast, AST_DEFINE, items, COMPLETION_CONSTANT, main_filename);

                free(main_path);
            }
        }
    }

    /* Register completions */
    for (int i = 1; i <= 8; i++) {
        char label[8];
        snprintf(label, sizeof(label), "I%d", i);
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "label", label);
        cJSON_AddNumberToObject(item, "kind", COMPLETION_VARIABLE);
        cJSON_AddStringToObject(item, "detail", "Integer register");
        cJSON_AddItemToArray(items, item);
    }
    for (int i = 1; i <= 8; i++) {
        char label[8];
        snprintf(label, sizeof(label), "D%d", i);
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "label", label);
        cJSON_AddNumberToObject(item, "kind", COMPLETION_VARIABLE);
        cJSON_AddStringToObject(item, "detail", "Decimal register");
        cJSON_AddItemToArray(items, item);
    }
    for (int i = 1; i <= 8; i++) {
        char label[8];
        snprintf(label, sizeof(label), "P%d", i);
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "label", label);
        cJSON_AddNumberToObject(item, "kind", COMPLETION_VARIABLE);
        cJSON_AddStringToObject(item, "detail", "Pointer register");
        cJSON_AddItemToArray(items, item);
    }

    /* Built-in constants */
    cJSON *rda_first = cJSON_CreateObject();
    cJSON_AddStringToObject(rda_first, "label", "RDA_FIRST");
    cJSON_AddNumberToObject(rda_first, "kind", COMPLETION_CONSTANT);
    cJSON_AddStringToObject(rda_first, "detail", "= 34");
    cJSON_AddItemToArray(items, rda_first);

    cJSON *rda_last = cJSON_CreateObject();
    cJSON_AddStringToObject(rda_last, "label", "RDA_LAST");
    cJSON_AddNumberToObject(rda_last, "kind", COMPLETION_CONSTANT);
    cJSON_AddStringToObject(rda_last, "detail", "= 255");
    cJSON_AddItemToArray(items, rda_last);

    return items;
}

/*
 * Create LSP Range from AST location (1-based to 0-based conversion)
 */
static cJSON *make_range(int start_line, int start_col, int end_line, int end_col) {
    cJSON *range = cJSON_CreateObject();
    cJSON *start = cJSON_CreateObject();
    cJSON *end = cJSON_CreateObject();

    /* LSP uses 0-based lines and columns */
    cJSON_AddNumberToObject(start, "line", start_line > 0 ? start_line - 1 : 0);
    cJSON_AddNumberToObject(start, "character", start_col > 0 ? start_col - 1 : 0);
    cJSON_AddNumberToObject(end, "line", end_line > 0 ? end_line - 1 : 0);
    cJSON_AddNumberToObject(end, "character", end_col > 0 ? end_col - 1 : 0);

    cJSON_AddItemToObject(range, "start", start);
    cJSON_AddItemToObject(range, "end", end);
    return range;
}

/*
 * Find a definition node in the AST by name and kind
 */
static AstNode *find_definition(AstNode *node, const char *name, AstNodeKind kind) {
    if (!node || !name) return NULL;

    /* Check if this node is the definition we're looking for */
    if (node->kind == kind) {
        const char *node_name = NULL;
        switch (kind) {
            case AST_VAR_DECL:
                node_name = node->data.var_decl.name;
                break;
            case AST_PROC:
                node_name = node->data.proc.name;
                break;
            case AST_LABEL:
                node_name = node->data.label.name;
                break;
            case AST_DEFINE:
                node_name = node->data.define.name;
                break;
            default:
                break;
        }
        if (node_name && strcasecmp(node_name, name) == 0) {
            return node;
        }
    }

    /* Search children */
    for (int i = 0; i < node->child_count; i++) {
        AstNode *found = find_definition(node->children[i], name, kind);
        if (found) return found;
    }

    return NULL;
}

/*
 * Get the identifier name from an AST node at a position
 */
static const char *get_identifier_at_node(AstNode *node) {
    if (!node) return NULL;

    switch (node->kind) {
        case AST_IDENT:
            return node->data.ident.name;
        case AST_VAR_DECL:
            return node->data.var_decl.name;
        case AST_PROC:
            return node->data.proc.name;
        case AST_PROC_CALL:
            return node->data.call.name;  /* Procedure call */
        case AST_LABEL:
            return node->data.label.name;
        case AST_DEFINE:
            return node->data.define.name;
        case AST_PROGRAM:
            return node->data.program.name;
        case AST_GOTO:
            return node->data.goto_stmt.label;
        default:
            return NULL;
    }
}

/*
 * Create an LSP Location object
 */
static cJSON *make_location(const char *uri, int start_line, int start_col, int end_line, int end_col) {
    cJSON *loc = cJSON_CreateObject();
    cJSON_AddStringToObject(loc, "uri", uri);
    cJSON_AddItemToObject(loc, "range", make_range(start_line, start_col, end_line, end_col));
    return loc;
}

/*
 * Create an LSP Location object from a file path (not URI)
 */
static cJSON *make_location_from_path(const char *path, int start_line, int start_col, int end_line, int end_col) {
    char *uri = path_to_uri(path);
    if (!uri) return NULL;

    cJSON *loc = make_location(uri, start_line, start_col, end_line, end_col);
    free(uri);
    return loc;
}

/*
 * Convert transparent AST line to COPY-local line using preproc events
 */
static int transparent_to_copy_local(int transparent_line, const char *copy_path,
                                      PreprocEvent *events, int event_count) {
    if (!copy_path || !events) return transparent_line;
    for (int i = 0; i < event_count; i++) {
        PreprocEvent *evt = &events[i];
        if (evt->kind == PREPROC_EVENT_COPY && evt->resolved_path &&
            strcmp(evt->resolved_path, copy_path) == 0) {
            int local = transparent_line - evt->line + 1;
            return local < 1 ? 1 : local;
        }
    }
    return transparent_line;
}

/*
 * Handle textDocument/definition request
 */
cJSON *handle_definition(LSPServer *server, cJSON *params) {
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    cJSON *position = cJSON_GetObjectItem(params, "position");

    if (!textDocument || !position) return NULL;

    cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
    cJSON *line = cJSON_GetObjectItem(position, "line");
    cJSON *character = cJSON_GetObjectItem(position, "character");

    if (!cJSON_IsString(uri) || !cJSON_IsNumber(line) || !cJSON_IsNumber(character)) {
        return NULL;
    }

    /* Find the document */
    ResolvedContext ctx = resolve_context(server, uri->valuestring);
    Document *doc = ctx.doc;
    if (!doc || !doc->ast) return NULL;

    char *doc_path = uri_to_path(uri->valuestring);
    int lsp_line = (int)line->valuedouble;
    int ast_line = lsp_to_ast_line(&ctx, lsp_line);
    int ast_col = (int)character->valuedouble + 1;

    /* Check for preprocessor events (COPY / DEFINE references) - only for .src files */
    if (doc->preproc_events && !ctx.is_copy) {
        PreprocEvent *evt = preproc_event_find_at(
            doc->preproc_events, doc->preproc_event_count,
            doc_path, ast_line, ast_col);
        if (evt) {
            if (evt->kind == PREPROC_EVENT_COPY && evt->resolved_path) {
                free(doc_path);
                return make_location_from_path(evt->resolved_path, 1, 1, 1, 1);
            }
            if (evt->kind == PREPROC_EVENT_DEFINE_REF && evt->def_filename) {
                /* If define is in a COPY file, convert transparent line to local */
                int def_line = evt->def_line;
                /* def_line from event is already COPY-local (stored that way by preproc) */
                free(doc_path);
                return make_location_from_path(evt->def_filename,
                    def_line, evt->def_column,
                    def_line, evt->def_column);
            }
        }
    }

    /* Find node at position - use 0-based line for position_in_range */
    AstNode *ast = (AstNode *)doc->ast;
    AstNode *node = find_node_at_position(ast, ast_line - 1, (int)character->valuedouble, doc_path);

    if (!node) { free(doc_path); return NULL; }

    /* Get the identifier name */
    const char *name = get_identifier_at_node(node);
    if (!name) { free(doc_path); return NULL; }

    /* Try to find the definition */
    AstNode *def = NULL;

    /* If we're on a reference, search for its definition */
    if (node->kind == AST_IDENT) {
        def = find_definition(ast, name, AST_VAR_DECL);
        if (!def) def = find_definition(ast, name, AST_PROC);
        if (!def) def = find_definition(ast, name, AST_LABEL);
        if (!def) def = find_definition(ast, name, AST_DEFINE);
    } else if (node->kind == AST_PROC_CALL) {
        def = find_definition(ast, name, AST_PROC);
    } else if (node->kind == AST_GOTO) {
        def = find_definition(ast, name, AST_LABEL);
    } else {
        def = node;
    }

    if (!def) { free(doc_path); return NULL; }

    /* Determine the correct URI for the definition */
    const char *def_filename = def->range.start.filename;

    if (def_filename && def_filename[0] != '\0') {
        int start_line = def->range.start.line;
        int end_line = def->range.end.line;

        /* If definition is in a different file (COPY file), convert transparent
         * line numbers to COPY-local lines */
        if (doc->preproc_events && strcmp(def_filename, doc_path) != 0) {
            start_line = transparent_to_copy_local(start_line, def_filename,
                                                    doc->preproc_events, doc->preproc_event_count);
            end_line = transparent_to_copy_local(end_line, def_filename,
                                                  doc->preproc_events, doc->preproc_event_count);
        }

        /* If we're in a COPY file and the definition is in the same COPY file,
         * also convert to local lines */
        if (ctx.is_copy && strcmp(def_filename, doc_path) == 0 && ctx.copy_start_line > 0) {
            start_line = start_line - ctx.copy_start_line + 1;
            end_line = end_line - ctx.copy_start_line + 1;
            if (start_line < 1) start_line = 1;
            if (end_line < 1) end_line = 1;
        }

        free(doc_path);
        return make_location_from_path(def_filename,
                                       start_line, def->range.start.column,
                                       end_line, def->range.end.column);
    } else {
        free(doc_path);
        return make_location(uri->valuestring,
                            def->range.start.line, def->range.start.column,
                            def->range.end.line, def->range.end.column);
    }
}

/*
 * Collect references to a name from an AST, with optional file filter.
 * - Emits LSP Locations with URI derived from node->range.start.filename
 * - Adjusts transparent line numbers for COPY file nodes using preproc events
 * - If only_from_file is non-NULL, only emits nodes whose filename matches it
 *   (used to avoid duplicates when searching multiple ASTs)
 */
static void collect_references_ex(AstNode *node, const char *name, cJSON *locations,
                                   PreprocEvent *events, int event_count,
                                   const char *only_from_file) {
    if (!node || !name) return;

    const char *node_name = NULL;
    switch (node->kind) {
        case AST_IDENT:     node_name = node->data.ident.name; break;
        case AST_VAR_DECL:  node_name = node->data.var_decl.name; break;
        case AST_PROC:      node_name = node->data.proc.name; break;
        case AST_LABEL:     node_name = node->data.label.name; break;
        case AST_DEFINE:    node_name = node->data.define.name; break;
        case AST_GOTO:      node_name = node->data.goto_stmt.label; break;
        case AST_PROC_CALL: node_name = node->data.call.name; break;
        default: break;
    }

    if (node_name && strcasecmp(node_name, name) == 0) {
        const char *node_file = node->range.start.filename;

        /* Apply file filter if set */
        if (only_from_file) {
            if (!node_file || strcmp(node_file, only_from_file) != 0) {
                goto recurse;
            }
        }

        int start_line = node->range.start.line;
        int end_line = node->range.end.line;

        /* Check if this node is from a COPY file and adjust lines */
        if (node_file && events) {
            for (int i = 0; i < event_count; i++) {
                PreprocEvent *evt = &events[i];
                if (evt->kind == PREPROC_EVENT_COPY && evt->resolved_path &&
                    strcmp(evt->resolved_path, node_file) == 0) {
                    /* Convert transparent -> COPY-local */
                    start_line = start_line - evt->line + 1;
                    end_line = end_line - evt->line + 1;
                    if (start_line < 1) start_line = 1;
                    if (end_line < 1) end_line = 1;
                    break;
                }
            }
        }

        cJSON *loc = make_location_from_path(node_file ? node_file : "",
                                              start_line, node->range.start.column,
                                              end_line, node->range.end.column);
        if (loc) cJSON_AddItemToArray(locations, loc);
    }

recurse:
    for (int i = 0; i < node->child_count; i++) {
        collect_references_ex(node->children[i], name, locations, events, event_count,
                               only_from_file);
    }
}

/*
 * Find the file where a name is defined (AST_VAR_DECL, AST_PROC, etc.)
 * Returns the filename from the definition node, or NULL if not found.
 */
static const char *find_definition_file(AstNode *node, const char *name) {
    if (!node || !name) return NULL;

    const char *node_name = NULL;
    switch (node->kind) {
        case AST_VAR_DECL: node_name = node->data.var_decl.name; break;
        case AST_PROC:     node_name = node->data.proc.name; break;
        case AST_LABEL:    node_name = node->data.label.name; break;
        case AST_DEFINE:   node_name = node->data.define.name; break;
        default: break;
    }
    if (node_name && strcasecmp(node_name, name) == 0) {
        return node->range.start.filename;
    }
    for (int i = 0; i < node->child_count; i++) {
        const char *result = find_definition_file(node->children[i], name);
        if (result) return result;
    }
    return NULL;
}

/*
 * Collect content overrides from all open COPY files (for temporary parses).
 * Caller must free override_paths entries and the arrays themselves.
 */
static void collect_copy_overrides(LSPServer *server,
                                    const char ***out_paths, const char ***out_contents,
                                    int *out_count) {
    int count = 0;
    if (server->documents) {
        for (int i = 0; i < server->documents->count; i++) {
            Document *d = server->documents->documents[i];
            if (d && d->is_copy_file && d->content) count++;
        }
    }
    if (count == 0) {
        *out_paths = NULL;
        *out_contents = NULL;
        *out_count = 0;
        return;
    }
    *out_paths = calloc(count, sizeof(char *));
    *out_contents = calloc(count, sizeof(char *));
    int idx = 0;
    for (int i = 0; i < server->documents->count; i++) {
        Document *d = server->documents->documents[i];
        if (d && d->is_copy_file && d->content) {
            (*out_paths)[idx] = uri_to_path(d->uri);
            (*out_contents)[idx] = d->content;  /* not owned */
            idx++;
        }
    }
    *out_count = idx;
}

static void free_copy_overrides(const char **paths, const char **contents, int count) {
    for (int i = 0; i < count; i++) free((char *)paths[i]);
    free(paths);
    free(contents);
}

/*
 * Search a .src file for references to `name`, collecting .src-local nodes only.
 * If the file is open, uses its existing AST. Otherwise, parses from disk.
 */
static void search_src_for_references(LSPServer *server, const char *src_path,
                                       const char *name, cJSON *locations,
                                       const char **ovr_paths, const char **ovr_contents,
                                       int ovr_count) {
    /* Check if the file is open in the editor */
    char *src_uri = path_to_uri(src_path);
    Document *d = docstore_get(server->documents, src_uri);
    free(src_uri);

    if (d && d->ast) {
        /* Use existing AST - collect .src-local references only */
        collect_references_ex((AstNode *)d->ast, name, locations,
                               d->preproc_events, d->preproc_event_count, src_path);
        return;
    }

    /* Parse from disk on demand */
    TbolParseOptions opts = {
        .include_paths = (const char **)server->include_paths,
        .include_path_count = server->include_path_count,
        .filename = src_path,
        .check_only = false,
        .collect_symbols = true,
        .override_paths = ovr_paths,
        .override_contents = ovr_contents,
        .override_count = ovr_count,
    };

    TbolParseResult *result = tbol_parse_file(src_path, &opts);
    if (!result || !result->ast) {
        if (result) tbol_parse_result_free(result);
        return;
    }

    /* Collect .src-local references only */
    collect_references_ex(result->ast, name, locations,
                           result->preproc_events, result->preproc_event_count, src_path);

    tbol_parse_result_free(result);
}

/*
 * Handle textDocument/references request
 *
 * Searches the entire workspace for references. For symbols defined in COPY
 * files, all .src files that include the COPY file are scanned (parsing from
 * disk if not open). For .src-local symbols, only the current file is searched.
 */
cJSON *handle_references(LSPServer *server, cJSON *params) {
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    cJSON *position = cJSON_GetObjectItem(params, "position");

    if (!textDocument || !position) return cJSON_CreateArray();

    cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
    cJSON *line = cJSON_GetObjectItem(position, "line");
    cJSON *character = cJSON_GetObjectItem(position, "character");

    if (!cJSON_IsString(uri) || !cJSON_IsNumber(line) || !cJSON_IsNumber(character)) {
        return cJSON_CreateArray();
    }

    /* Find the document */
    ResolvedContext ctx = resolve_context(server, uri->valuestring);
    Document *doc = ctx.doc;
    if (!doc || !doc->ast) return cJSON_CreateArray();

    /* Find node at position */
    AstNode *ast = (AstNode *)doc->ast;
    char *doc_path = uri_to_path(uri->valuestring);
    int ast_line_0based = lsp_to_ast_line(&ctx, (int)line->valuedouble) - 1;
    AstNode *node = find_node_at_position(ast, ast_line_0based, (int)character->valuedouble, doc_path);

    if (!node) { free(doc_path); return cJSON_CreateArray(); }

    /* Get the identifier name */
    const char *name = get_identifier_at_node(node);
    if (!name) { free(doc_path); return cJSON_CreateArray(); }

    cJSON *locations = cJSON_CreateArray();

    /* Collect content overrides for on-demand parses (dirty COPY buffers) */
    const char **ovr_paths = NULL, **ovr_contents = NULL;
    int ovr_count = 0;
    collect_copy_overrides(server, &ovr_paths, &ovr_contents, &ovr_count);

    if (ctx.is_copy) {
        /*
         * COPY file: collect COPY-local references from this doc's AST,
         * then scan the entire workspace for .src files that include this
         * COPY and collect .src-local references from each.
         */

        /* COPY-local references (definition + usages within the COPY file) */
        collect_references_ex(ast, name, locations,
                               doc->preproc_events, doc->preproc_event_count, doc_path);

        /* Parent .src references (from the AST we already have) */
        if (doc->parent_src_uri) {
            char *parent_path = uri_to_path(doc->parent_src_uri);
            collect_references_ex(ast, name, locations,
                                   doc->preproc_events, doc->preproc_event_count, parent_path);
            free(parent_path);
        }

        /* Workspace scan: all other .src files that COPY this file */
        const char *copy_base = strrchr(doc_path, '/');
        copy_base = copy_base ? copy_base + 1 : doc_path;

        int src_count = 0;
        char **src_files = workspace_find_src_copying(server, copy_base, &src_count);

        char *parent_path = doc->parent_src_uri ? uri_to_path(doc->parent_src_uri) : NULL;
        for (int i = 0; i < src_count; i++) {
            /* Skip the parent we already searched via the COPY doc's AST */
            if (parent_path && strcmp(src_files[i], parent_path) == 0) {
                free(src_files[i]);
                continue;
            }
            search_src_for_references(server, src_files[i], name, locations,
                                       ovr_paths, ovr_contents, ovr_count);
            free(src_files[i]);
        }
        free(src_files);
        free(parent_path);
    } else {
        /*
         * .src file: collect all references from this doc's AST. Then check
         * if the symbol is defined in a COPY file - if so, scan the workspace
         * for all other .src files that include it.
         */
        collect_references_ex(ast, name, locations,
                               doc->preproc_events, doc->preproc_event_count, NULL);

        /* Check if symbol is defined in a COPY file */
        const char *def_file = find_definition_file(ast, name);
        if (def_file && strcmp(def_file, doc_path) != 0) {
            /* Defined in a COPY file - find all .src files that include it */
            const char *copy_base = strrchr(def_file, '/');
            copy_base = copy_base ? copy_base + 1 : def_file;

            int src_count = 0;
            char **src_files = workspace_find_src_copying(server, copy_base, &src_count);

            for (int i = 0; i < src_count; i++) {
                /* Skip the current .src (already searched above) */
                if (strcmp(src_files[i], doc_path) == 0) {
                    free(src_files[i]);
                    continue;
                }
                search_src_for_references(server, src_files[i], name, locations,
                                           ovr_paths, ovr_contents, ovr_count);
                free(src_files[i]);
            }
            free(src_files);
        }
    }

    free_copy_overrides(ovr_paths, ovr_contents, ovr_count);
    free(doc_path);
    return locations;
}

/*
 * LSP Symbol Kinds
 */
#define SYMBOL_KIND_FILE        1
#define SYMBOL_KIND_MODULE      2
#define SYMBOL_KIND_NAMESPACE   3
#define SYMBOL_KIND_PACKAGE     4
#define SYMBOL_KIND_CLASS       5
#define SYMBOL_KIND_METHOD      6
#define SYMBOL_KIND_PROPERTY    7
#define SYMBOL_KIND_FIELD       8
#define SYMBOL_KIND_CONSTRUCTOR 9
#define SYMBOL_KIND_ENUM        10
#define SYMBOL_KIND_INTERFACE   11
#define SYMBOL_KIND_FUNCTION    12
#define SYMBOL_KIND_VARIABLE    13
#define SYMBOL_KIND_CONSTANT    14
#define SYMBOL_KIND_STRING      15
#define SYMBOL_KIND_NUMBER      16
#define SYMBOL_KIND_BOOLEAN     17
#define SYMBOL_KIND_ARRAY       18
#define SYMBOL_KIND_OBJECT      19
#define SYMBOL_KIND_KEY         20
#define SYMBOL_KIND_NULL        21
#define SYMBOL_KIND_ENUMMEMBER  22
#define SYMBOL_KIND_STRUCT      23
#define SYMBOL_KIND_EVENT       24
#define SYMBOL_KIND_OPERATOR    25
#define SYMBOL_KIND_TYPEPARAM   26

/*
 * Create a DocumentSymbol
 */
static cJSON *make_symbol(const char *name, int kind, int start_line, int start_col,
                          int end_line, int end_col) {
    cJSON *sym = cJSON_CreateObject();
    cJSON_AddStringToObject(sym, "name", name ? name : "<unknown>");
    cJSON_AddNumberToObject(sym, "kind", kind);
    cJSON_AddItemToObject(sym, "range", make_range(start_line, start_col, end_line, end_col));

    /* selectionRange must be contained within range.
     * Clamp the name highlight to not exceed the full range - this can happen
     * during incomplete edits (e.g., snippet expansion with no END_PROC yet). */
    int sel_end_line = start_line;
    int sel_end_col = start_col + (name ? (int)strlen(name) : 1);
    if (sel_end_line > end_line || (sel_end_line == end_line && sel_end_col > end_col)) {
        sel_end_line = end_line;
        sel_end_col = end_col;
    }
    cJSON_AddItemToObject(sym, "selectionRange", make_range(start_line, start_col, sel_end_line, sel_end_col));
    return sym;
}

/*
 * Find the COPY statement line for a node from a COPY file.
 * Returns the COPY statement's line, or -1 if not found.
 */
static int find_copy_line(const char *copy_path, PreprocEvent *events, int event_count) {
    if (!copy_path || !events) return -1;
    for (int i = 0; i < event_count; i++) {
        if (events[i].kind == PREPROC_EVENT_COPY && events[i].resolved_path &&
            strcmp(events[i].resolved_path, copy_path) == 0) {
            return events[i].line;
        }
    }
    return -1;
}

/*
 * Extract symbols from AST node recursively.
 * Symbols from COPY files have their positions mapped to the COPY statement's line,
 * so the outline shows all composite DATA variables and DEFINEs.
 */
static void extract_symbols(AstNode *node, cJSON *symbols, cJSON *parent_children,
                            const char *doc_path, PreprocEvent *events, int event_count) {
    if (!node) return;

    /* Determine if this node comes from a COPY file */
    int copy_line = -1;
    if (doc_path && node->range.start.filename &&
        strcmp(node->range.start.filename, doc_path) != 0) {
        copy_line = find_copy_line(node->range.start.filename, events, event_count);
        if (copy_line < 0) return;  /* Unknown origin - skip */
    }

    cJSON *target = parent_children ? parent_children : symbols;
    cJSON *children = NULL;
    cJSON *sym = NULL;

    /* For COPY file nodes, map position to the COPY statement line.
     * The full range must encompass the selectionRange (name), so for
     * COPY-mapped symbols we set end_col to cover at least the name. */
    int start_line = (copy_line >= 0) ? copy_line : node->range.start.line;
    int start_col = (copy_line >= 0) ? 1 : node->range.start.column;

    switch (node->kind) {
        case AST_PROGRAM:
            sym = make_symbol(node->data.program.name, SYMBOL_KIND_MODULE,
                             node->range.start.line, node->range.start.column,
                             node->range.end.line, node->range.end.column);
            children = cJSON_CreateArray();
            cJSON_AddItemToObject(sym, "children", children);
            cJSON_AddItemToArray(symbols, sym);
            for (int i = 0; i < node->child_count; i++) {
                extract_symbols(node->children[i], symbols, children, doc_path, events, event_count);
            }
            return;

        case AST_DATA_SECTION: {
            const char *sname = node->data.data_section.name ? node->data.data_section.name : "";
            int end_line, end_col;
            if (copy_line >= 0) {
                /* Use wide range so children (VAR_DECLs) fit within */
                end_line = copy_line;
                end_col = 999;
            } else {
                end_line = node->range.end.line > 0 ? node->range.end.line : node->range.start.line;
                end_col = node->range.end.column > 0 ? node->range.end.column : node->range.start.column + 10;
            }
            sym = make_symbol(sname, SYMBOL_KIND_STRUCT,
                             start_line, start_col, end_line, end_col);
            children = cJSON_CreateArray();
            cJSON_AddItemToObject(sym, "children", children);
            cJSON_AddItemToArray(target, sym);
            for (int i = 0; i < node->child_count; i++) {
                extract_symbols(node->children[i], symbols, children, doc_path, events, event_count);
            }
            return;
        }

        case AST_VAR_DECL: {
            const char *vname = node->data.var_decl.name ? node->data.var_decl.name : "";
            sym = make_symbol(vname, SYMBOL_KIND_VARIABLE,
                             start_line, start_col,
                             start_line, start_col + (int)strlen(vname));
            cJSON_AddItemToArray(target, sym);
            return;
        }

        case AST_PROC: {
            const char *pname = node->data.proc.name ? node->data.proc.name : "";
            int end_line, end_col;
            if (copy_line >= 0) {
                /* Use wide range so children (labels) fit within */
                end_line = copy_line;
                end_col = 999;
            } else {
                end_line = node->range.end.line > 0 ? node->range.end.line : node->range.start.line;
                end_col = node->range.end.column > 0 ? node->range.end.column : node->range.start.column + 10;
            }
            sym = make_symbol(pname, SYMBOL_KIND_FUNCTION,
                             start_line, start_col, end_line, end_col);
            children = cJSON_CreateArray();
            cJSON_AddItemToObject(sym, "children", children);
            cJSON_AddItemToArray(target, sym);
            for (int i = 0; i < node->child_count; i++) {
                extract_symbols(node->children[i], symbols, children, doc_path, events, event_count);
            }
            return;
        }

        case AST_LABEL: {
            const char *lname = node->data.label.name ? node->data.label.name : "";
            sym = make_symbol(lname, SYMBOL_KIND_KEY,
                             start_line, start_col,
                             start_line, start_col + (int)strlen(lname));
            cJSON_AddItemToArray(target, sym);
            return;
        }

        default:
            for (int i = 0; i < node->child_count; i++) {
                extract_symbols(node->children[i], symbols, parent_children, doc_path, events, event_count);
            }
            return;
    }
}

/*
 * Extract symbols from COPY file AST, filtering to only nodes from this COPY file
 * and adjusting line numbers from transparent to COPY-local
 */
static void extract_copy_symbols(AstNode *node, cJSON *symbols, cJSON *parent_children,
                                   const char *copy_path, int copy_start_line) {
    if (!node) return;

    /* Only include nodes from the COPY file */
    if (!node->range.start.filename ||
        strcmp(node->range.start.filename, copy_path) != 0) {
        /* Still recurse - children may be from the COPY file */
        for (int i = 0; i < node->child_count; i++) {
            extract_copy_symbols(node->children[i], symbols, parent_children,
                                  copy_path, copy_start_line);
        }
        return;
    }

    /* Convert transparent line to COPY-local */
    int local_line = node->range.start.line - copy_start_line + 1;
    if (local_line < 1) local_line = 1;

    cJSON *target = parent_children ? parent_children : symbols;
    cJSON *sym = NULL;
    cJSON *children = NULL;

    switch (node->kind) {
        case AST_DATA_SECTION: {
            const char *sname = node->data.data_section.name ? node->data.data_section.name : "";
            int end_local = node->range.end.line - copy_start_line + 1;
            if (end_local < local_line) end_local = local_line;
            sym = make_symbol(sname, SYMBOL_KIND_STRUCT,
                             local_line, node->range.start.column,
                             end_local, node->range.end.column > 0 ? node->range.end.column : 999);
            children = cJSON_CreateArray();
            cJSON_AddItemToObject(sym, "children", children);
            cJSON_AddItemToArray(target, sym);
            for (int i = 0; i < node->child_count; i++) {
                extract_copy_symbols(node->children[i], symbols, children,
                                      copy_path, copy_start_line);
            }
            return;
        }
        case AST_VAR_DECL: {
            const char *vname = node->data.var_decl.name ? node->data.var_decl.name : "";
            sym = make_symbol(vname, SYMBOL_KIND_VARIABLE,
                             local_line, node->range.start.column,
                             local_line, node->range.start.column + (int)strlen(vname));
            cJSON_AddItemToArray(target, sym);
            return;
        }
        case AST_PROC: {
            const char *pname = node->data.proc.name ? node->data.proc.name : "";
            int end_local = node->range.end.line - copy_start_line + 1;
            if (end_local < local_line) end_local = local_line;
            sym = make_symbol(pname, SYMBOL_KIND_FUNCTION,
                             local_line, node->range.start.column,
                             end_local, node->range.end.column > 0 ? node->range.end.column : 999);
            children = cJSON_CreateArray();
            cJSON_AddItemToObject(sym, "children", children);
            cJSON_AddItemToArray(target, sym);
            for (int i = 0; i < node->child_count; i++) {
                extract_copy_symbols(node->children[i], symbols, children,
                                      copy_path, copy_start_line);
            }
            return;
        }
        case AST_LABEL: {
            const char *lname = node->data.label.name ? node->data.label.name : "";
            sym = make_symbol(lname, SYMBOL_KIND_KEY,
                             local_line, node->range.start.column,
                             local_line, node->range.start.column + (int)strlen(lname));
            cJSON_AddItemToArray(target, sym);
            return;
        }
        case AST_DEFINE: {
            const char *dname = node->data.define.name ? node->data.define.name : "";
            sym = make_symbol(dname, SYMBOL_KIND_CONSTANT,
                             local_line, node->range.start.column,
                             local_line, node->range.start.column + (int)strlen(dname));
            cJSON_AddItemToArray(target, sym);
            return;
        }
        default:
            for (int i = 0; i < node->child_count; i++) {
                extract_copy_symbols(node->children[i], symbols, parent_children,
                                      copy_path, copy_start_line);
            }
            return;
    }
}

/*
 * Handle textDocument/documentSymbol request
 */
cJSON *handle_document_symbol(LSPServer *server, cJSON *params) {
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    if (!textDocument) return cJSON_CreateArray();

    cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
    if (!cJSON_IsString(uri)) return cJSON_CreateArray();

    /* Find the document */
    ResolvedContext ctx = resolve_context(server, uri->valuestring);
    Document *doc = ctx.doc;
    if (!doc || !doc->ast) return cJSON_CreateArray();

    char *doc_path = uri_to_path(uri->valuestring);
    cJSON *symbols = cJSON_CreateArray();
    AstNode *ast = (AstNode *)doc->ast;

    if (ctx.is_copy && ctx.copy_start_line > 0) {
        /* For COPY files, extract only symbols from this COPY file */
        extract_copy_symbols(ast, symbols, NULL, doc_path, ctx.copy_start_line);
    } else {
        /* For .src files, use existing logic */
        extract_symbols(ast, symbols, NULL, doc_path, doc->preproc_events, doc->preproc_event_count);
    }
    free(doc_path);

    return symbols;
}

/*
 * Create a SymbolInformation object (for workspace symbols)
 */
static cJSON *make_symbol_info(const char *name, int kind, const char *uri,
                                int start_line, int start_col, int end_line, int end_col,
                                const char *container_name) {
    cJSON *sym = cJSON_CreateObject();
    cJSON_AddStringToObject(sym, "name", name ? name : "<unknown>");
    cJSON_AddNumberToObject(sym, "kind", kind);
    cJSON_AddItemToObject(sym, "location", make_location(uri, start_line, start_col, end_line, end_col));
    if (container_name) {
        cJSON_AddStringToObject(sym, "containerName", container_name);
    }
    return sym;
}

/*
 * Collect workspace symbols from AST matching query
 */
static void collect_workspace_symbols(AstNode *node, const char *query, const char *uri,
                                       const char *container, cJSON *symbols) {
    if (!node) return;

    const char *name = NULL;
    int kind = 0;
    const char *new_container = container;

    switch (node->kind) {
        case AST_PROGRAM:
            name = node->data.program.name;
            kind = SYMBOL_KIND_MODULE;
            new_container = name;
            break;
        case AST_VAR_DECL:
            name = node->data.var_decl.name;
            kind = SYMBOL_KIND_VARIABLE;
            break;
        case AST_DEFINE:
            name = node->data.define.name;
            kind = SYMBOL_KIND_CONSTANT;
            break;
        case AST_PROC:
            name = node->data.proc.name;
            kind = SYMBOL_KIND_FUNCTION;
            new_container = name;
            break;
        case AST_LABEL:
            name = node->data.label.name;
            kind = SYMBOL_KIND_KEY;
            break;
        default:
            break;
    }

    /* Check if name matches query (case-insensitive substring match) */
    if (name && kind > 0) {
        bool matches = true;
        if (query && query[0] != '\0') {
            /* Simple case-insensitive substring search */
            matches = false;
            size_t query_len = strlen(query);
            size_t name_len = strlen(name);
            for (size_t i = 0; i + query_len <= name_len && !matches; i++) {
                if (strncasecmp(name + i, query, query_len) == 0) {
                    matches = true;
                }
            }
        }
        if (matches) {
            cJSON *sym = make_symbol_info(name, kind, uri,
                                          node->range.start.line, node->range.start.column,
                                          node->range.end.line, node->range.end.column,
                                          container);
            cJSON_AddItemToArray(symbols, sym);
        }
    }

    /* Recurse into children */
    for (int i = 0; i < node->child_count; i++) {
        collect_workspace_symbols(node->children[i], query, uri, new_container, symbols);
    }
}

/*
 * Handle workspace/symbol request
 */
cJSON *handle_workspace_symbol(LSPServer *server, cJSON *params) {
    cJSON *symbols = cJSON_CreateArray();

    /* Get query string */
    const char *query = "";
    cJSON *query_json = cJSON_GetObjectItem(params, "query");
    if (cJSON_IsString(query_json)) {
        query = query_json->valuestring;
    }

    /* Search through all open documents */
    if (server->documents) {
        for (int i = 0; i < server->documents->count; i++) {
            Document *doc = server->documents->documents[i];
            if (doc && doc->ast) {
                AstNode *ast = (AstNode *)doc->ast;
                collect_workspace_symbols(ast, query, doc->uri, NULL, symbols);
            }
        }
    }

    return symbols;
}

/*
 * Get the source file of a symbol definition
 */
static const char *get_definition_file(AstNode *node) {
    if (!node) return NULL;
    return node->range.start.filename;
}

/*
 * Collect all locations of a symbol for renaming
 * Stores results in a hash table keyed by URI
 */
static void add_rename_edit(cJSON *changes, const char *uri, int start_line, int start_col,
                            int end_line, int end_col, const char *new_text) {
    /* Find or create edits array for this URI */
    cJSON *edits = cJSON_GetObjectItem(changes, uri);
    if (!edits) {
        edits = cJSON_CreateArray();
        cJSON_AddItemToObject(changes, uri, edits);
    }

    /* Create TextEdit */
    cJSON *edit = cJSON_CreateObject();
    cJSON_AddItemToObject(edit, "range", make_range(start_line, start_col, end_line, end_col));
    cJSON_AddStringToObject(edit, "newText", new_text);
    cJSON_AddItemToArray(edits, edit);
}

/*
 * Handle textDocument/prepareRename request
 * Uses text-based word extraction so rename works from any cursor position
 * within an identifier, regardless of AST column accuracy.
 */
cJSON *handle_prepare_rename(LSPServer *server, cJSON *params) {
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    cJSON *position = cJSON_GetObjectItem(params, "position");

    if (!textDocument || !position) return NULL;

    cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
    cJSON *line = cJSON_GetObjectItem(position, "line");
    cJSON *character = cJSON_GetObjectItem(position, "character");

    if (!cJSON_IsString(uri) || !cJSON_IsNumber(line) || !cJSON_IsNumber(character)) {
        return NULL;
    }

    /* Find the document */
    Document *doc = docstore_get(server->documents, uri->valuestring);
    if (!doc || !doc->ast || !doc->content) return NULL;

    int lsp_line = (int)line->valuedouble;
    int lsp_col = (int)character->valuedouble;

    /* Don't allow rename inside comments or string literals */
    if (is_in_comment_or_string(doc->content, lsp_line, lsp_col)) return NULL;

    /* Extract word under cursor from source text */
    int word_col_1based = 0;
    char *word = extract_word_at(doc->content, lsp_line, lsp_col, &word_col_1based);
    if (!word) return NULL;

    /* Verify this word is a known symbol in the AST */
    AstNode *ast = (AstNode *)doc->ast;
    bool known = false;
    if (find_definition(ast, word, AST_VAR_DECL)) known = true;
    if (!known && find_definition(ast, word, AST_PROC)) known = true;
    if (!known && find_definition(ast, word, AST_LABEL)) known = true;
    if (!known && find_definition(ast, word, AST_DEFINE)) known = true;

    if (!known) {
        free(word);
        return NULL;
    }

    int line_1based = lsp_line + 1;
    int word_len = (int)strlen(word);

    cJSON *result = cJSON_CreateObject();
    cJSON_AddItemToObject(result, "range",
                          make_range(line_1based, word_col_1based,
                                    line_1based, word_col_1based + word_len));
    cJSON_AddStringToObject(result, "placeholder", word);
    free(word);

    return result;
}

/*
 * Helper: cascade rename to a COPY file and all .src files that include it.
 * copy_path: absolute path of the COPY file
 * old_name/new_name: the rename pair
 * skip_uri: URI to skip (the document that initiated the rename)
 */
static void rename_cascade_copy(LSPServer *server, const char *copy_path,
                                  const char *old_name, const char *new_name,
                                  const char *skip_uri, cJSON *changes) {
    char *copy_uri = path_to_uri(copy_path);

    /* Rename in the COPY file (unless it's the skip_uri) */
    if (!skip_uri || strcmp(copy_uri, skip_uri) != 0) {
        Document *copy_doc = docstore_get(server->documents, copy_uri);
        char *copy_content_alloc = NULL;
        const char *copy_content = NULL;
        if (copy_doc && copy_doc->content) {
            copy_content = copy_doc->content;
        } else {
            copy_content_alloc = read_file(copy_path);
            copy_content = copy_content_alloc;
        }
        if (copy_content) {
            collect_text_rename_edits(copy_content, old_name, new_name, copy_uri, changes);
        }
        free(copy_content_alloc);
    }

    /* Find and rename in all .src files that COPY this file */
    /* Try CopyIndex first, fall back to workspace scan */
    int dep_count = 0;
    char **dependent_files = copy_index_find_dependents(server->copy_index, copy_path, &dep_count);
    if (dep_count == 0) {
        free(dependent_files);
        dependent_files = find_files_that_copy(server, copy_path, &dep_count);
        /* Populate CopyIndex from the scan results */
        for (int i = 0; i < dep_count; i++) {
            copy_index_add(server->copy_index, copy_path, dependent_files[i]);
        }
    }

    for (int i = 0; i < dep_count; i++) {
        char *dep_uri = path_to_uri(dependent_files[i]);

        /* Skip the originating document and the COPY file itself */
        if ((skip_uri && strcmp(dep_uri, skip_uri) == 0) ||
            strcmp(dep_uri, copy_uri) == 0) {
            free(dep_uri);
            free(dependent_files[i]);
            continue;
        }

        Document *dep_doc = docstore_get(server->documents, dep_uri);
        char *dep_content_alloc = NULL;
        const char *dep_content = NULL;
        if (dep_doc && dep_doc->content) {
            dep_content = dep_doc->content;
        } else {
            dep_content_alloc = read_file(dependent_files[i]);
            dep_content = dep_content_alloc;
        }
        if (dep_content) {
            collect_text_rename_edits(dep_content, old_name, new_name, dep_uri, changes);
        }
        free(dep_content_alloc);
        free(dep_uri);
        free(dependent_files[i]);
    }
    free(dependent_files);
    free(copy_uri);
}

/*
 * Handle textDocument/rename request
 * Uses text-based scanning to find all identifier occurrences, avoiding
 * AST position inaccuracies from COPY transparency and DEFINE expansion.
 * Supports COPY file cascade - renames propagate to the COPY file and
 * all .src files in the workspace that include it.
 */
cJSON *handle_rename(LSPServer *server, cJSON *params) {
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    cJSON *position = cJSON_GetObjectItem(params, "position");
    cJSON *newNameJson = cJSON_GetObjectItem(params, "newName");

    if (!textDocument || !position || !cJSON_IsString(newNameJson)) {
        return NULL;
    }

    cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
    cJSON *line = cJSON_GetObjectItem(position, "line");
    cJSON *character = cJSON_GetObjectItem(position, "character");

    if (!cJSON_IsString(uri) || !cJSON_IsNumber(line) || !cJSON_IsNumber(character)) {
        return NULL;
    }

    const char *new_name = newNameJson->valuestring;

    /* Find the document */
    Document *doc = docstore_get(server->documents, uri->valuestring);
    if (!doc || !doc->ast || !doc->content) return NULL;

    /* Extract old_name from source text at cursor position */
    char *old_name = extract_word_at(doc->content,
                                      (int)line->valuedouble,
                                      (int)character->valuedouble, NULL);
    if (!old_name) {
        log_info("Rename: no word at position %d:%d\n",
                 (int)line->valuedouble, (int)character->valuedouble);
        return NULL;
    }

    log_info("Rename: old_name='%s' new_name='%s'\n", old_name, new_name);

    /* Build WorkspaceEdit */
    cJSON *workspace_edit = cJSON_CreateObject();
    cJSON *changes = cJSON_CreateObject();

    /* Rename in current document using text scanning */
    collect_text_rename_edits(doc->content, old_name, new_name, uri->valuestring, changes);

    if (doc->is_copy_file) {
        /* Renaming FROM a COPY file: cascade to all .src files that include it */
        char *copy_path = uri_to_path(uri->valuestring);
        if (copy_path) {
            rename_cascade_copy(server, copy_path, old_name, new_name,
                                 uri->valuestring, changes);
            free(copy_path);
        }
    } else {
        /* Renaming FROM a .src file: check if symbol is defined in a COPY file */
        AstNode *ast = (AstNode *)doc->ast;
        AstNode *def = find_definition(ast, old_name, AST_VAR_DECL);
        if (!def) def = find_definition(ast, old_name, AST_PROC);
        if (!def) def = find_definition(ast, old_name, AST_LABEL);
        if (!def) def = find_definition(ast, old_name, AST_DEFINE);

        char *current_path = uri_to_path(uri->valuestring);
        const char *def_file = def ? get_definition_file(def) : NULL;
        bool is_from_copy_file = def_file && current_path && strcmp(def_file, current_path) != 0;

        if (is_from_copy_file && def_file) {
            rename_cascade_copy(server, def_file, old_name, new_name,
                                 uri->valuestring, changes);
        }
        free(current_path);
    }

    free(old_name);
    cJSON_AddItemToObject(workspace_edit, "changes", changes);
    return workspace_edit;
}

/*
 * TBOL Verb Signature Database
 */
typedef struct {
    const char *name;
    const char *label;           /* Full signature display */
    const char *documentation;
    const char **params;         /* NULL-terminated array of parameter names */
} VerbSignature;

static const char *move_params[] = {"source", "destination", NULL};
static const char *add_params[] = {"value", "destination", NULL};
static const char *sub_params[] = {"value", "destination", NULL};
static const char *mul_params[] = {"value", "destination", NULL};
static const char *div_params[] = {"dividend", "divisor", "quotient", "remainder", NULL};
static const char *string_params[] = {"destination", "source...", NULL};
static const char *substr_params[] = {"source", "start", "length", "destination", NULL};
static const char *instr_params[] = {"haystack", "needle", "result", NULL};
static const char *length_params[] = {"source", "result", NULL};
static const char *uppercase_params[] = {"variable", NULL};
static const char *clear_params[] = {"structure", NULL};
static const char *save_params[] = {"structure", "slot", NULL};
static const char *restore_params[] = {"structure", "slot", NULL};
static const char *release_params[] = {"slot", NULL};
static const char *goto_params[] = {"label", NULL};
static const char *link_params[] = {"procedure", NULL};
static const char *transfer_params[] = {"program", NULL};
static const char *navigate_params[] = {"screen", "field", NULL};
static const char *send_params[] = {"destination", "message", NULL};
static const char *fetch_params[] = {"key", "result", NULL};
static const char *sort_params[] = {"array", "start", "count", NULL};
/* lookup_params - removed, using lookup_full_params */
static const char *fill_params[] = {"destination", "value", "count", NULL};
static const char *swap_params[] = {"var1", "var2", NULL};
static const char *push_params[] = {"value", NULL};
static const char *pop_params[] = {"destination", NULL};
static const char *note_params[] = {"file", "result", NULL};
static const char *point_params[] = {"file", "position", NULL};
static const char *open_params[] = {"file", "mode", NULL};
static const char *close_params[] = {"file", NULL};
static const char *read_params[] = {"file", "destination", "[length]", NULL};
static const char *write_params[] = {"file", "source", "[length]", NULL};
static const char *error_params[] = {"message", NULL};
static const char *sound_params[] = {"frequency", "duration", NULL};
/* set_function_params - removed, using set_function_full_params */
static const char *set_attribute_params[] = {"field", "attribute", "value", NULL};
/* define_field_params - removed, using define_field_full_params */
static const char *make_format_params[] = {"destination", "format_spec...", NULL};

/* Additional parameter arrays for missing verbs */
static const char *deletefile_params[] = {"filename", NULL};
static const char *xor_params[] = {"value", "destination", NULL};
static const char *test_params[] = {"value", "destination", NULL};
static const char *and_params[] = {"value", "destination", NULL};
static const char *or_params[] = {"value", "destination", NULL};
static const char *open_window_params[] = {"window", NULL};
static const char *close_window_params[] = {"window", NULL};
static const char *open_error_window_params[] = {"window", NULL};
static const char *kill_params[] = {"task", NULL};
static const char *connect_params[] = {"host", NULL};
static const char *receive_params[] = {"source", "destination", NULL};
static const char *cancel_params[] = {"request", NULL};
static const char *start_params[] = {"task", NULL};
static const char *stop_params[] = {"task", NULL};
static const char *erase_params[] = {"field", NULL};
static const char *set_cursor_params[] = {"field", NULL};
static const char *set_key_params[] = {"key", "row", "col", NULL};
static const char *format_params[] = {"source", "format", "destination", NULL};
static const char *trigger_function_params[] = {"function", NULL};
static const char *edit_params[] = {"destination", "format", "sources...", NULL};
static const char *goto_depending_params[] = {"value", "labels...", NULL};
static const char *lookup_full_params[] = {"table", "key", "start", "count", "result", NULL};
static const char *define_field_full_params[] = {"name", "row", "col", "length", "attribute", "format", "variable", NULL};
static const char *set_function_full_params[] = {"key", "action", "row", "col", NULL};

static const VerbSignature verb_signatures[] = {
    {"MOVE", "MOVE source, destination;", "Copy value from source to destination. When both operands are arrays of equal size, performs a block move.", move_params},
    {"ADD", "ADD value, destination;", "Add value to destination", add_params},
    {"SUBTRACT", "SUBTRACT value, destination;", "Subtract value from destination", sub_params},
    {"SUB", "SUB value, destination;", "Subtract value from destination (alias)", sub_params},
    {"MULTIPLY", "MULTIPLY value, destination;", "Multiply destination by value", mul_params},
    {"MUL", "MUL value, destination;", "Multiply destination by value (alias)", mul_params},
    {"DIVIDE", "DIVIDE dividend, divisor, quotient, remainder;", "Divide dividend by divisor", div_params},
    {"DIV", "DIV dividend, divisor, quotient, remainder;", "Divide (alias)", div_params},
    {"STRING", "STRING destination, source...;", "Concatenate sources into destination", string_params},
    {"SUBSTR", "SUBSTR source, start, length, destination;", "Extract substring", substr_params},
    {"INSTR", "INSTR haystack, needle, result;", "Find needle in haystack, store position in result", instr_params},
    {"LENGTH", "LENGTH source, result;", "Get length of source string", length_params},
    {"UPPERCASE", "UPPERCASE variable;", "Convert variable to uppercase in place", uppercase_params},
    {"CLEAR", "CLEAR structure;", "Clear all variables in structure to empty", clear_params},
    {"SAVE", "SAVE structure, slot;", "Save structure to RDA slot", save_params},
    {"RESTORE", "RESTORE structure, slot;", "Restore structure from RDA slot", restore_params},
    {"RELEASE", "RELEASE slot;", "Release RDA slot", release_params},
    {"GOTO", "GOTO label;", "Jump to label", goto_params},
    {"LINK", "LINK procedure;", "Call procedure (returns with RETURN)", link_params},
    {"TRANSFER", "TRANSFER program;", "Transfer control to another program", transfer_params},
    {"NAVIGATE", "NAVIGATE screen, field;", "Navigate to screen and field", navigate_params},
    {"SEND", "SEND destination, message;", "Send message to destination", send_params},
    {"FETCH", "FETCH key, result;", "Fetch value by key into result", fetch_params},
    {"SORT", "SORT array, start, count;", "Sort array elements", sort_params},
    {"LOOKUP", "LOOKUP table, key, start, count, result;", "Look up key in table", lookup_full_params},
    {"FILL", "FILL destination, value, count;", "Fill destination with value", fill_params},
    {"SWAP", "SWAP var1, var2;", "Swap values of two variables", swap_params},
    {"PUSH", "PUSH value;", "Push value onto stack", push_params},
    {"POP", "POP destination;", "Pop value from stack into destination", pop_params},
    {"NOTE", "NOTE file, result;", "Get current position in file", note_params},
    {"POINT", "POINT file, position;", "Set position in file", point_params},
    {"OPEN", "OPEN file, mode;", "Open file with mode", open_params},
    {"CLOSE", "CLOSE file;", "Close file", close_params},
    {"READ", "READ file, dest[, length];", "Read from file (2 args: line read; 3 args: record read)", read_params},
    {"WRITE", "WRITE file, source[, length];", "Write to file (2 args: line write; 3 args: record write)", write_params},
    {"ERROR", "ERROR message;", "Display error message", error_params},
    {"SOUND", "SOUND frequency, duration;", "Play sound", sound_params},
    {"SET_FUNCTION", "SET_FUNCTION key, action[, row, col];", "Set function key handler", set_function_full_params},
    {"SET_ATTRIBUTE", "SET_ATTRIBUTE field, attribute, value;", "Set field attribute", set_attribute_params},
    {"DEFINE_FIELD", "DEFINE_FIELD name, row, col, length, attr, format[, var];", "Define screen field", define_field_full_params},
    {"MAKE_FORMAT", "MAKE_FORMAT destination, format_spec...;", "Format data into destination", make_format_params},
    {"RETURN", "RETURN;", "Return from procedure", NULL},
    {"EXIT", "EXIT;", "Exit program", NULL},

    /* File operations */
    {"DELETE", "DELETE filename;", "Delete a file", deletefile_params},

    /* Bitwise operations */
    {"XOR", "XOR value, destination;", "Bitwise XOR value with destination", xor_params},
    {"TEST", "TEST value, destination;", "Test bits in destination", test_params},
    {"AND", "AND value, destination;", "Bitwise AND value with destination", and_params},
    {"OR", "OR value, destination;", "Bitwise OR value with destination", or_params},

    /* Window operations */
    {"OPEN_WINDOW", "OPEN_WINDOW window;", "Open a window", open_window_params},
    {"CLOSE_WINDOW", "CLOSE_WINDOW [window];", "Close window (optional window ID)", close_window_params},
    {"OPEN_ERROR_WINDOW", "OPEN_ERROR_WINDOW window;", "Open error window", open_error_window_params},

    /* Process control */
    {"KILL", "KILL [task];", "Kill task (optional task ID)", kill_params},
    {"START", "START [task];", "Start task", start_params},
    {"STOP", "STOP [task];", "Stop task", stop_params},
    {"CANCEL", "CANCEL [request];", "Cancel pending request", cancel_params},

    /* Network/messaging */
    {"CONNECT", "CONNECT host;", "Connect to host", connect_params},
    {"DISCONNECT", "DISCONNECT;", "Disconnect from host", NULL},
    {"RECEIVE", "RECEIVE source, destination;", "Receive message", receive_params},

    /* Persistence */
    {"PURGE_CACHE", "PURGE_CACHE;", "Purge RDA cache", NULL},

    /* Timer operations */
    {"WAIT", "WAIT;", "Wait for event", NULL},

    /* Display operations */
    {"REFRESH", "REFRESH;", "Refresh screen display", NULL},
    {"ERASE", "ERASE [field];", "Erase field (optional field)", erase_params},
    {"SET_CURSOR", "SET_CURSOR [field];", "Set cursor position", set_cursor_params},

    /* Additional field operations */
    {"SET_KEY", "SET_KEY key, row, col;", "Set key at screen position", set_key_params},
    {"FORMAT", "FORMAT source, format, destination;", "Format value", format_params},

    /* Other */
    {"TRIGGER_FUNCTION", "TRIGGER_FUNCTION function;", "Trigger a function key", trigger_function_params},

    /* String formatting */
    {"EDIT", "EDIT destination, format, sources...;", "Format multiple values into destination", edit_params},

    /* Control flow */
    {"GOTO_DEPENDING_ON", "GOTO_DEPENDING_ON value, label1, label2...;", "Branch to label based on value", goto_depending_params},
    {"NAVIGATE_FIRST", "NAVIGATE FIRST;", "Navigate to first record", NULL},
    {"NAVIGATE_NEXT", "NAVIGATE NEXT;", "Navigate to next record", NULL},
    {"NAVIGATE_BACK", "NAVIGATE BACK;", "Navigate to previous record", NULL},
    {"NAVIGATE_LAST", "NAVIGATE LAST;", "Navigate to last record", NULL},

    {NULL, NULL, NULL, NULL}
};

/*
 * Find verb signature by name
 */
static const VerbSignature *find_verb_signature(const char *name) {
    for (int i = 0; verb_signatures[i].name; i++) {
        if (strcasecmp(verb_signatures[i].name, name) == 0) {
            return &verb_signatures[i];
        }
    }
    return NULL;
}

/*
 * Count commas before position in a line to determine active parameter
 */
static int count_commas_before(const char *line, int col) {
    int count = 0;
    for (int i = 0; i < col && line[i]; i++) {
        if (line[i] == ',') count++;
        /* Handle quoted strings - don't count commas inside them */
        if (line[i] == '\'') {
            i++;
            while (i < col && line[i] && line[i] != '\'') i++;
        }
    }
    return count;
}

/*
 * Get the verb name from the current line
 */
static const char *extract_verb_from_line(const char *line, char *verb_buf, size_t buf_size) {
    /* Skip leading whitespace */
    while (*line && isspace((unsigned char)*line)) line++;

    /* Extract word */
    size_t i = 0;
    while (*line && isalnum((unsigned char)*line) && i < buf_size - 1) {
        verb_buf[i++] = *line++;
    }
    verb_buf[i] = '\0';

    return verb_buf;
}

/*
 * Handle textDocument/signatureHelp request
 */
cJSON *handle_signature_help(LSPServer *server, cJSON *params) {
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    cJSON *position = cJSON_GetObjectItem(params, "position");

    if (!textDocument || !position) return NULL;

    cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
    cJSON *line_num = cJSON_GetObjectItem(position, "line");
    cJSON *character = cJSON_GetObjectItem(position, "character");

    if (!cJSON_IsString(uri) || !cJSON_IsNumber(line_num) || !cJSON_IsNumber(character)) {
        return NULL;
    }

    /* Find the document */
    Document *doc = docstore_get(server->documents, uri->valuestring);
    if (!doc || !doc->content) return NULL;

    int target_line = (int)line_num->valuedouble;
    int target_col = (int)character->valuedouble;

    /* Find the line in the document content */
    const char *content = doc->content;
    const char *line_start = content;
    int current_line = 0;

    while (*line_start && current_line < target_line) {
        if (*line_start == '\n') current_line++;
        line_start++;
    }

    if (current_line != target_line) return NULL;

    /* Find end of line */
    const char *line_end = line_start;
    while (*line_end && *line_end != '\n') line_end++;

    /* Copy line for analysis */
    size_t line_len = line_end - line_start;
    char *line = malloc(line_len + 1);
    memcpy(line, line_start, line_len);
    line[line_len] = '\0';

    /* Extract verb name */
    char verb_buf[64];
    extract_verb_from_line(line, verb_buf, sizeof(verb_buf));

    /* Look up signature */
    const VerbSignature *sig = find_verb_signature(verb_buf);
    if (!sig) {
        free(line);
        return NULL;
    }

    /* Count commas to find active parameter */
    int active_param = count_commas_before(line, target_col);

    free(line);

    /* Build SignatureHelp response */
    cJSON *result = cJSON_CreateObject();
    cJSON *signatures = cJSON_CreateArray();

    cJSON *signature = cJSON_CreateObject();
    cJSON_AddStringToObject(signature, "label", sig->label);

    if (sig->documentation) {
        cJSON *doc_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(doc_obj, "kind", "markdown");
        cJSON_AddStringToObject(doc_obj, "value", sig->documentation);
        cJSON_AddItemToObject(signature, "documentation", doc_obj);
    }

    /* Add parameters */
    if (sig->params) {
        cJSON *parameters = cJSON_CreateArray();
        for (int i = 0; sig->params[i]; i++) {
            cJSON *param = cJSON_CreateObject();
            cJSON_AddStringToObject(param, "label", sig->params[i]);
            cJSON_AddItemToArray(parameters, param);
        }
        cJSON_AddItemToObject(signature, "parameters", parameters);
    }

    cJSON_AddItemToArray(signatures, signature);
    cJSON_AddItemToObject(result, "signatures", signatures);
    cJSON_AddNumberToObject(result, "activeSignature", 0);
    cJSON_AddNumberToObject(result, "activeParameter", active_param);

    return result;
}

/* ================================================================
 * Folding Ranges
 * ================================================================ */

static void emit_fold(cJSON *ranges, int start_line, int end_line) {
    if (end_line > start_line) {
        cJSON *fr = cJSON_CreateObject();
        cJSON_AddNumberToObject(fr, "startLine", start_line - 1);
        cJSON_AddNumberToObject(fr, "endLine", end_line - 1);
        cJSON_AddStringToObject(fr, "kind", "region");
        cJSON_AddItemToArray(ranges, fr);
    }
}

/*
 * Recursively collect folding ranges from the AST.
 * filter_path: only emit nodes from this file (always set for both .src and COPY).
 * copy_start_line: transparent line offset for COPY files (0 for .src files).
 */
static void collect_folding_ranges(AstNode *node, cJSON *ranges,
                                   const char *filter_path, int copy_start_line) {
    if (!node) return;

    bool foldable = false;
    switch (node->kind) {
        case AST_PROC:
        case AST_DO_BLOCK:
        case AST_IF_STMT:
        case AST_WHILE_STMT:
        case AST_DATA_SECTION:
            foldable = true;
            break;
        default:
            break;
    }

    if (foldable) {
        /* Filter: only include nodes from the target file */
        if (filter_path) {
            if (!node->range.start.filename ||
                strcmp(node->range.start.filename, filter_path) != 0) {
                goto recurse;
            }
        }

        int start_line = node->range.start.line;
        int end_line = node->range.end.line;

        /* For COPY files, adjust transparent -> COPY-local lines */
        if (copy_start_line > 0) {
            start_line = start_line - copy_start_line + 1;
            end_line = end_line - copy_start_line + 1;
            if (start_line < 1) start_line = 1;
            if (end_line < 1) end_line = 1;
        }

        emit_fold(ranges, start_line, end_line);
    }

recurse:
    for (int i = 0; i < node->child_count; i++) {
        collect_folding_ranges(node->children[i], ranges, filter_path, copy_start_line);
    }
}

cJSON *handle_folding_range(LSPServer *server, cJSON *params) {
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    if (!textDocument) return cJSON_CreateArray();

    cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
    if (!cJSON_IsString(uri)) return cJSON_CreateArray();

    ResolvedContext ctx = resolve_context(server, uri->valuestring);
    Document *doc = ctx.doc;
    if (!doc || !doc->ast) return cJSON_CreateArray();

    AstNode *ast = (AstNode *)doc->ast;
    cJSON *ranges = cJSON_CreateArray();

    char *file_path = uri_to_path(uri->valuestring);
    int copy_start = (ctx.is_copy && ctx.copy_start_line > 0) ? ctx.copy_start_line : 0;
    collect_folding_ranges(ast, ranges, file_path, copy_start);
    free(file_path);

    return ranges;
}

/* ================================================================
 * Semantic Tokens
 * ================================================================ */

/* Token type indices (must match legend registration order) */
#define ST_TYPE_VARIABLE  0
#define ST_TYPE_FUNCTION  1
#define ST_TYPE_MACRO     2
#define ST_TYPE_LABEL     3

/* Token modifier bitmask */
#define ST_MOD_DECLARATION 0x01

typedef struct {
    int line;       /* 1-based AST line */
    int col;        /* 1-based AST column */
    int length;
    int type;
    int modifiers;
} SemanticToken;

typedef struct {
    SemanticToken *tokens;
    int count;
    int capacity;
} SemanticTokenList;

static void st_list_add(SemanticTokenList *list, int line, int col, int length, int type, int modifiers) {
    if (length <= 0) return;
    if (list->count >= list->capacity) {
        list->capacity = list->capacity ? list->capacity * 2 : 128;
        list->tokens = realloc(list->tokens, list->capacity * sizeof(SemanticToken));
    }
    SemanticToken *t = &list->tokens[list->count++];
    t->line = line;
    t->col = col;
    t->length = length;
    t->type = type;
    t->modifiers = modifiers;
}

static int st_compare(const void *a, const void *b) {
    const SemanticToken *ta = a;
    const SemanticToken *tb = b;
    if (ta->line != tb->line) return ta->line - tb->line;
    return ta->col - tb->col;
}

/*
 * Walk the AST collecting semantic token entries.
 * filter_path: only include nodes from this file (always set).
 * copy_start_line: transparent line offset for COPY files (0 for .src).
 *
 * Only emit tokens that add value beyond TextMate grammar:
 * - AST_VAR_DECL: variable declarations (in DATA section)
 * - AST_PROC_CALL: procedure calls (TextMate can't distinguish from variables)
 * - AST_LABEL: label declarations
 *
 * AST_IDENT (variable references) is intentionally excluded - TextMate already
 * colors identifiers, and emitting semantic tokens for all references overrides
 * the TextMate coloring with potentially different colors, causing "weird" styling.
 */
static void collect_semantic_tokens(AstNode *node, SemanticTokenList *list,
                                    const char *filter_path, int copy_start_line) {
    if (!node) return;

    int line = node->range.start.line;
    int col = node->range.start.column;
    const char *name = NULL;
    int type = -1;
    int modifiers = 0;

    switch (node->kind) {
        case AST_VAR_DECL:
            name = node->data.var_decl.name;
            type = ST_TYPE_VARIABLE;
            modifiers = ST_MOD_DECLARATION;
            break;
        case AST_PROC_CALL:
            name = node->data.call.name;
            type = ST_TYPE_FUNCTION;
            break;
        case AST_LABEL:
            name = node->data.label.name;
            type = ST_TYPE_LABEL;
            modifiers = ST_MOD_DECLARATION;
            break;
        default:
            break;
    }

    if (type >= 0 && name && name[0] != '\0') {
        bool include = true;

        /* Filter: only include nodes from the target file */
        if (filter_path) {
            if (!node->range.start.filename ||
                strcmp(node->range.start.filename, filter_path) != 0) {
                include = false;
            }
        }

        /* For COPY files, adjust transparent -> COPY-local lines */
        if (include && copy_start_line > 0) {
            line = line - copy_start_line + 1;
            if (line < 1) include = false;
        }

        if (include) {
            st_list_add(list, line, col, (int)strlen(name), type, modifiers);
        }
    }

    for (int i = 0; i < node->child_count; i++) {
        collect_semantic_tokens(node->children[i], list, filter_path, copy_start_line);
    }
}

cJSON *handle_semantic_tokens_full(LSPServer *server, cJSON *params) {
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    if (!textDocument) return cJSON_CreateNull();

    cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
    if (!cJSON_IsString(uri)) return cJSON_CreateNull();

    ResolvedContext ctx = resolve_context(server, uri->valuestring);
    Document *doc = ctx.doc;
    if (!doc || !doc->ast) return cJSON_CreateNull();

    AstNode *ast = (AstNode *)doc->ast;
    SemanticTokenList list = {0};

    char *file_path = uri_to_path(uri->valuestring);
    int copy_start = (ctx.is_copy && ctx.copy_start_line > 0) ? ctx.copy_start_line : 0;
    collect_semantic_tokens(ast, &list, file_path, copy_start);
    free(file_path);

    /* Sort by (line, col) */
    if (list.count > 1) {
        qsort(list.tokens, list.count, sizeof(SemanticToken), st_compare);
    }

    /* Encode as delta array */
    cJSON *data = cJSON_CreateArray();
    int prev_line = 1;
    int prev_col = 1;

    for (int i = 0; i < list.count; i++) {
        SemanticToken *t = &list.tokens[i];
        int delta_line = t->line - prev_line;
        int delta_col = (delta_line == 0) ? (t->col - prev_col) : (t->col - 1);
        /* LSP delta encoding: deltaLine, deltaStartChar, length, tokenType, tokenModifiers */
        cJSON_AddItemToArray(data, cJSON_CreateNumber(delta_line));
        cJSON_AddItemToArray(data, cJSON_CreateNumber(delta_col));
        cJSON_AddItemToArray(data, cJSON_CreateNumber(t->length));
        cJSON_AddItemToArray(data, cJSON_CreateNumber(t->type));
        cJSON_AddItemToArray(data, cJSON_CreateNumber(t->modifiers));
        prev_line = t->line;
        prev_col = t->col;
    }

    free(list.tokens);

    cJSON *result = cJSON_CreateObject();
    cJSON_AddItemToObject(result, "data", data);
    return result;
}

/* ================================================================
 * Selection Ranges
 * ================================================================ */

/*
 * Build a SelectionRange JSON object from a range, with optional parent.
 */
static cJSON *make_selection_range(int start_line, int start_col,
                                   int end_line, int end_col, cJSON *parent) {
    cJSON *sr = cJSON_CreateObject();
    cJSON_AddItemToObject(sr, "range", make_range(start_line, start_col, end_line, end_col));
    if (parent) {
        cJSON_AddItemToObject(sr, "parent", parent);
    }
    return sr;
}

/* Ancestor range info for building selection chains */
typedef struct {
    int start_line, start_col, end_line, end_col;  /* 1-based, for make_range */
} SelectionAncestor;

#define MAX_SELECTION_DEPTH 64

/*
 * Lenient containment check for selection ranges.
 * ast_new initializes end = start, so nodes without explicit ast_set_end
 * have a "point range" (start == end). For those, match if cursor is on
 * the same line at or after the start column.
 */
static bool node_contains_for_selection(AstNode *node, int line_0, int col_0) {
    if (!node) return false;
    if (node->range.start.line == 0) return false;

    /* Check if end was explicitly set (different from start) */
    bool end_is_set = (node->range.end.line != node->range.start.line ||
                       node->range.end.column != node->range.start.column);

    if (end_is_set) {
        return position_in_range(line_0, col_0, &node->range);
    }

    /* Point range (end == start): treat as single-line from start column onward */
    int ast_line = line_0 + 1;
    int ast_col = col_0 + 1;
    return (ast_line == node->range.start.line && ast_col >= node->range.start.column);
}

/*
 * Compute effective end range for a node.
 * For nodes with proper end ranges, returns the node's end.
 * For point-range nodes (end == start), computes the bounding box from children.
 */
static void compute_effective_end(AstNode *node, int *out_end_line, int *out_end_col) {
    bool is_point_range = (node->range.end.line == node->range.start.line &&
                           node->range.end.column == node->range.start.column);

    if (!is_point_range) {
        *out_end_line = node->range.end.line;
        *out_end_col = node->range.end.column;
        return;
    }

    /* Point range: find the maximum end position among children */
    int max_line = node->range.start.line;
    int max_col = node->range.start.column;
    for (int i = 0; i < node->child_count; i++) {
        AstNode *child = node->children[i];
        if (!child || child->range.start.line == 0) continue;
        int c_end_line, c_end_col;
        compute_effective_end(child, &c_end_line, &c_end_col);
        if (c_end_line > max_line || (c_end_line == max_line && c_end_col > max_col)) {
            max_line = c_end_line;
            max_col = c_end_col;
        }
    }

    /* If still a point after checking children, extend to end of line */
    if (max_line == node->range.start.line && max_col == node->range.start.column) {
        max_col = 999;
    }

    *out_end_line = max_line;
    *out_end_col = max_col;
}

/*
 * Collect all ancestor nodes that contain the position, in root-to-leaf order.
 * position: 0-based line, 0-based col (for position_in_range).
 * filter_path: only nodes from this file contribute (always set).
 * copy_start_line: transparent line offset for COPY files (0 for .src).
 */
static void collect_selection_ancestors(AstNode *node, int line, int col,
                                        const char *filter_path, int copy_start_line,
                                        SelectionAncestor *ancestors, int *depth) {
    if (!node || *depth >= MAX_SELECTION_DEPTH) return;

    /* Check if this node contains the position */
    if (node_contains_for_selection(node, line, col)) {
        int s_line = node->range.start.line;
        int s_col = node->range.start.column;
        int e_line, e_col;
        compute_effective_end(node, &e_line, &e_col);
        bool contributes = true;

        /* Filter: only include nodes from the target file */
        if (filter_path) {
            if (!node->range.start.filename ||
                strcmp(node->range.start.filename, filter_path) != 0) {
                contributes = false;
            }
        }

        /* For COPY files, adjust transparent -> COPY-local lines */
        if (contributes && copy_start_line > 0) {
            s_line = s_line - copy_start_line + 1;
            e_line = e_line - copy_start_line + 1;
            if (s_line < 1) contributes = false;
        }

        if (contributes) {
            if (e_line <= 0) e_line = s_line;
            if (e_col <= 0) e_col = 999;
            SelectionAncestor *a = &ancestors[*depth];
            a->start_line = s_line;
            a->start_col = s_col;
            a->end_line = e_line;
            a->end_col = e_col;
            (*depth)++;
        }
    }

    /* Always recurse - child ranges may extend beyond imprecise parent ranges */
    for (int i = 0; i < node->child_count; i++) {
        collect_selection_ancestors(node->children[i], line, col,
                                    filter_path, copy_start_line, ancestors, depth);
    }
}

cJSON *handle_selection_range(LSPServer *server, cJSON *params) {
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    cJSON *positions = cJSON_GetObjectItem(params, "positions");

    if (!textDocument || !cJSON_IsArray(positions)) return cJSON_CreateArray();

    cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
    if (!cJSON_IsString(uri)) return cJSON_CreateArray();

    ResolvedContext ctx = resolve_context(server, uri->valuestring);
    Document *doc = ctx.doc;
    if (!doc || !doc->ast) return cJSON_CreateArray();

    AstNode *ast = (AstNode *)doc->ast;
    char *file_path = uri_to_path(uri->valuestring);
    int copy_start = (ctx.is_copy && ctx.copy_start_line > 0) ? ctx.copy_start_line : 0;

    cJSON *result = cJSON_CreateArray();
    int pos_count = cJSON_GetArraySize(positions);

    for (int i = 0; i < pos_count; i++) {
        cJSON *pos = cJSON_GetArrayItem(positions, i);
        cJSON *line_j = cJSON_GetObjectItem(pos, "line");
        cJSON *char_j = cJSON_GetObjectItem(pos, "character");

        if (!cJSON_IsNumber(line_j) || !cJSON_IsNumber(char_j)) {
            cJSON_AddItemToArray(result, cJSON_CreateNull());
            continue;
        }

        int lsp_line = (int)line_j->valuedouble;
        int lsp_col = (int)char_j->valuedouble;

        /* Convert to AST coordinates: 0-based line/col for position_in_range */
        int ast_line = lsp_to_ast_line(&ctx, lsp_line);
        int ast_line_0 = ast_line - 1;
        int ast_col_0 = lsp_col;

        /* Collect ancestors from root to leaf */
        SelectionAncestor ancestors[MAX_SELECTION_DEPTH];
        int depth = 0;
        collect_selection_ancestors(ast, ast_line_0, ast_col_0,
                                    file_path, copy_start, ancestors, &depth);

        if (depth > 0) {
            /* Build chain: iterate from outermost (index 0) to innermost.
             * Each iteration wraps the previous chain as parent, so the
             * final result has the innermost range at the top level. */
            cJSON *chain = NULL;
            for (int j = 0; j < depth; j++) {
                SelectionAncestor *a = &ancestors[j];
                chain = make_selection_range(a->start_line, a->start_col,
                                             a->end_line, a->end_col, chain);
            }
            cJSON_AddItemToArray(result, chain);
        } else {
            /* Fallback: single-character range */
            cJSON_AddItemToArray(result, make_selection_range(
                lsp_line + 1, lsp_col + 1, lsp_line + 1, lsp_col + 2, NULL));
        }
    }

    free(file_path);
    return result;
}

/* -- textDocument/formatting -------------------------------------------- */

cJSON *handle_formatting(LSPServer *server, cJSON *params) {
    cJSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    if (!textDocument) return cJSON_CreateArray();

    cJSON *uri = cJSON_GetObjectItem(textDocument, "uri");
    if (!cJSON_IsString(uri)) return cJSON_CreateArray();

    Document *doc = docstore_get(server->documents, uri->valuestring);
    if (!doc || !doc->content) return cJSON_CreateArray();

    /* Extract formatting options from params (tab size) */
    TbolFmtOptions opts = tbol_fmt_defaults();
    cJSON *fmtOpts = cJSON_GetObjectItem(params, "options");
    if (fmtOpts) {
        cJSON *tabSize = cJSON_GetObjectItem(fmtOpts, "tabSize");
        if (cJSON_IsNumber(tabSize))
            opts.indent_width = (int)tabSize->valuedouble;
    }

    /* Don't strip labels in user-authored source - that's a decompiler pass */
    opts.strip_labels = false;

    char *formatted = tbol_fmt(doc->content, &opts);
    if (!formatted) return cJSON_CreateArray();

    /* If nothing changed, return empty edit array */
    if (strcmp(formatted, doc->content) == 0) {
        free(formatted);
        return cJSON_CreateArray();
    }

    /* Return a single TextEdit replacing the entire document */
    cJSON *edits = cJSON_CreateArray();
    cJSON *edit = cJSON_CreateObject();

    /* Count lines in original content for the range end */
    int line_count = 0;
    const char *p = doc->content;
    while (*p) { if (*p == '\n') line_count++; p++; }

    cJSON *range = cJSON_CreateObject();
    cJSON *start = cJSON_CreateObject();
    cJSON_AddNumberToObject(start, "line", 0);
    cJSON_AddNumberToObject(start, "character", 0);
    cJSON *end = cJSON_CreateObject();
    cJSON_AddNumberToObject(end, "line", line_count);
    cJSON_AddNumberToObject(end, "character", 0);
    cJSON_AddItemToObject(range, "start", start);
    cJSON_AddItemToObject(range, "end", end);

    cJSON_AddItemToObject(edit, "range", range);
    cJSON_AddStringToObject(edit, "newText", formatted);
    cJSON_AddItemToArray(edits, edit);

    free(formatted);
    return edits;
}
