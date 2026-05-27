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
 * Document management for LSP
 *
 * Tracks open documents, handles parsing, and manages diagnostics.
 */
#include "lsp.h"
#include "../shared/tbol_parse.h"
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include "../shared/util/strutil.h"  /* strcasestr fallback on Windows */

/*
 * Document store implementation
 */
DocumentStore *docstore_new(void) {
    DocumentStore *store = calloc(1, sizeof(DocumentStore));
    store->capacity = 8;
    store->documents = calloc(store->capacity, sizeof(Document *));
    return store;
}

void docstore_free(DocumentStore *store) {
    if (!store) return;

    for (int i = 0; i < store->count; i++) {
        Document *doc = store->documents[i];
        if (doc) {
            free(doc->uri);
            free(doc->content);
            if (doc->diagnostics) {
                cJSON_Delete(doc->diagnostics);
            }
            if (doc->preproc_events) {
                PreprocEventList tmp = {
                    .events = doc->preproc_events,
                    .count = doc->preproc_event_count,
                    .capacity = doc->preproc_event_count
                };
                preproc_event_cleanup(&tmp);
            }
            free(doc->parent_src_uri);
            if (doc->ast) {
                ast_free((AstNode *)doc->ast);
            }
            free(doc);
        }
    }
    free(store->documents);
    free(store);
}

static Document *document_new(const char *uri, const char *content, int version) {
    Document *doc = calloc(1, sizeof(Document));
    doc->uri = strdup(uri);
    doc->content = strdup(content);
    doc->version = version;
    return doc;
}

Document *docstore_open(DocumentStore *store, const char *uri, const char *content, int version) {
    /* Check if already open */
    Document *existing = docstore_get(store, uri);
    if (existing) {
        /* Update content */
        free(existing->content);
        existing->content = strdup(content);
        existing->version = version;
        return existing;
    }

    /* Create new document */
    Document *doc = document_new(uri, content, version);

    /* Add to store */
    if (store->count >= store->capacity) {
        store->capacity *= 2;
        store->documents = realloc(store->documents, store->capacity * sizeof(Document *));
    }
    store->documents[store->count++] = doc;

    return doc;
}

Document *docstore_get(DocumentStore *store, const char *uri) {
    for (int i = 0; i < store->count; i++) {
        if (strcmp(store->documents[i]->uri, uri) == 0) {
            return store->documents[i];
        }
    }
    return NULL;
}

void docstore_update(DocumentStore *store, const char *uri, const char *content, int version) {
    Document *doc = docstore_get(store, uri);
    if (doc) {
        free(doc->content);
        doc->content = strdup(content);
        doc->version = version;
    }
}

void docstore_close(DocumentStore *store, const char *uri) {
    for (int i = 0; i < store->count; i++) {
        if (strcmp(store->documents[i]->uri, uri) == 0) {
            Document *doc = store->documents[i];
            free(doc->uri);
            free(doc->content);
            if (doc->diagnostics) {
                cJSON_Delete(doc->diagnostics);
            }
            if (doc->preproc_events) {
                PreprocEventList tmp = {
                    .events = doc->preproc_events,
                    .count = doc->preproc_event_count,
                    .capacity = doc->preproc_event_count
                };
                preproc_event_cleanup(&tmp);
            }
            free(doc->parent_src_uri);
            free(doc);

            /* Shift remaining documents */
            for (int j = i; j < store->count - 1; j++) {
                store->documents[j] = store->documents[j + 1];
            }
            store->count--;
            return;
        }
    }
}

/*
 * Convert file:// URI to local path
 */
static char *uri_to_path(const char *uri) {
    if (strncmp(uri, "file://", 7) != 0) {
        return strdup(uri);
    }

    const char *path = uri + 7;

    /* Handle Windows paths like file:///C:/... */
    if (path[0] == '/' && path[2] == ':') {
        path++;  /* Skip leading / before drive letter */
    }

    /* URL decode (simple version - just handles %20 for space) */
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

/*
 * Convert a filesystem path to a file:// URI.
 * Percent-encodes characters that are not unreserved per RFC 3986,
 * except '/' which is kept as-is for path separators.
 */
char *path_to_uri(const char *path) {
    if (!path) return NULL;
    /* Worst case: every byte becomes %XX (3x) plus "file://" prefix + NUL */
    size_t len = strlen(path);
    char *uri = malloc(7 + len * 3 + 1);
    if (!uri) return NULL;
    char *out = uri;
    memcpy(out, "file://", 7);
    out += 7;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)path[i];
        /* RFC 3986 unreserved: ALPHA / DIGIT / "-" / "." / "_" / "~" plus "/" */
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '.' ||
            c == '_' || c == '~' || c == '/') {
            *out++ = c;
        } else {
            snprintf(out, 4, "%%%02X", c);
            out += 3;
        }
    }
    *out = '\0';
    return uri;
}

static char *read_file_content(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0) { fclose(f); return NULL; }
    fseek(f, 0, SEEK_SET);
    char *content = malloc(size + 1);
    if (!content) { fclose(f); return NULL; }
    size_t nread = fread(content, 1, size, f);
    content[nread] = '\0';
    fclose(f);
    return content;
}

/*
 * CopyIndex implementation
 */
CopyIndex *copy_index_new(void) {
    CopyIndex *index = calloc(1, sizeof(CopyIndex));
    index->capacity = 16;
    index->entries = calloc(index->capacity, sizeof(CopyIndexEntry));
    return index;
}

void copy_index_free(CopyIndex *index) {
    if (!index) return;
    for (int i = 0; i < index->count; i++) {
        free(index->entries[i].copy_path);
        for (int j = 0; j < index->entries[i].parent_count; j++) {
            free(index->entries[i].parent_paths[j]);
        }
        free(index->entries[i].parent_paths);
    }
    free(index->entries);
    free(index);
}

void copy_index_add(CopyIndex *index, const char *copy_path, const char *src_path) {
    if (!index || !copy_path || !src_path) return;

    /* Find existing entry for this COPY file */
    CopyIndexEntry *entry = NULL;
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].copy_path, copy_path) == 0) {
            entry = &index->entries[i];
            break;
        }
    }

    /* Create new entry if needed */
    if (!entry) {
        if (index->count >= index->capacity) {
            index->capacity *= 2;
            index->entries = realloc(index->entries, index->capacity * sizeof(CopyIndexEntry));
        }
        entry = &index->entries[index->count++];
        entry->copy_path = strdup(copy_path);
        entry->parent_paths = NULL;
        entry->parent_count = 0;
        entry->parent_capacity = 0;
    }

    /* Check for duplicate parent */
    for (int i = 0; i < entry->parent_count; i++) {
        if (strcmp(entry->parent_paths[i], src_path) == 0) {
            return;  /* Already recorded */
        }
    }

    /* Add parent */
    if (entry->parent_count >= entry->parent_capacity) {
        entry->parent_capacity = entry->parent_capacity ? entry->parent_capacity * 2 : 4;
        entry->parent_paths = realloc(entry->parent_paths, entry->parent_capacity * sizeof(char *));
    }
    entry->parent_paths[entry->parent_count++] = strdup(src_path);
}

const char *copy_index_find_parent(CopyIndex *index, const char *copy_path, DocumentStore *store) {
    if (!index || !copy_path) return NULL;

    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].copy_path, copy_path) == 0) {
            CopyIndexEntry *entry = &index->entries[i];
            /* Prefer a parent that is currently open in the editor */
            if (store) {
                for (int j = 0; j < entry->parent_count; j++) {
                    char *uri = path_to_uri(entry->parent_paths[j]);
                    Document *doc = docstore_get(store, uri);
                    free(uri);
                    if (doc && !doc->is_copy_file) {
                        return entry->parent_paths[j];
                    }
                }
            }
            /* Fall back to first parent */
            if (entry->parent_count > 0) {
                return entry->parent_paths[0];
            }
            return NULL;
        }
    }
    return NULL;
}

char **copy_index_find_dependents(CopyIndex *index, const char *copy_path, int *count) {
    *count = 0;
    if (!index || !copy_path) return NULL;

    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].copy_path, copy_path) == 0) {
            CopyIndexEntry *entry = &index->entries[i];
            char **result = calloc(entry->parent_count, sizeof(char *));
            for (int j = 0; j < entry->parent_count; j++) {
                result[j] = strdup(entry->parent_paths[j]);
            }
            *count = entry->parent_count;
            return result;
        }
    }
    return NULL;
}

void copy_index_populate_from_events(CopyIndex *index, const char *src_path,
                                      PreprocEvent *events, int event_count) {
    if (!index || !src_path || !events) return;
    for (int i = 0; i < event_count; i++) {
        if (events[i].kind == PREPROC_EVENT_COPY && events[i].resolved_path) {
            copy_index_add(index, events[i].resolved_path, src_path);
        }
    }
}

/*
 * Scan workspace to find a .src file that COPYs a given file
 * Fallback for when CopyIndex doesn't have the mapping yet
 */
static bool file_copies_name(const char *filepath, const char *copy_basename) {
    char *content = read_file_content(filepath);
    if (!content) return false;

    bool found = false;
    char *p = content;
    while ((p = strcasestr(p, "COPY")) != NULL) {
        if (p > content && !isspace((unsigned char)p[-1]) && p[-1] != ';') {
            p += 4;
            continue;
        }
        p += 4;
        while (*p && isspace((unsigned char)*p)) p++;
        char filename[256];
        int i = 0;
        while (*p && !isspace((unsigned char)*p) && *p != ';' && *p != ',' && i < 255) {
            filename[i++] = *p++;
        }
        filename[i] = '\0';
        if (strcasecmp(filename, copy_basename) == 0) {
            found = true;
            break;
        }
    }
    free(content);
    return found;
}

static char *find_first_src_that_copies(const char *dirpath, const char *copy_basename) {
    DIR *dir = opendir(dirpath);
    if (!dir) return NULL;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);
        struct stat st;
        if (stat(path, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            if (strcmp(entry->d_name, "node_modules") == 0) continue;
            char *result = find_first_src_that_copies(path, copy_basename);
            if (result) { closedir(dir); return result; }
        } else if (S_ISREG(st.st_mode)) {
            size_t len = strlen(entry->d_name);
            if (len > 4 && strcasecmp(entry->d_name + len - 4, ".src") == 0) {
                if (file_copies_name(path, copy_basename)) {
                    closedir(dir);
                    return strdup(path);
                }
            }
        }
    }
    closedir(dir);
    return NULL;
}

/*
 * Scan workspace to find ALL .src files that COPY a given file.
 * Returns a dynamically allocated array of paths; caller frees each path and the array.
 */
static void collect_src_files_copying(const char *dirpath, const char *copy_basename,
                                       char ***results, int *count, int *capacity) {
    DIR *dir = opendir(dirpath);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);
        struct stat st;
        if (stat(path, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            if (strcmp(entry->d_name, "node_modules") == 0) continue;
            collect_src_files_copying(path, copy_basename, results, count, capacity);
        } else if (S_ISREG(st.st_mode)) {
            size_t len = strlen(entry->d_name);
            if (len > 4 && strcasecmp(entry->d_name + len - 4, ".src") == 0) {
                if (file_copies_name(path, copy_basename)) {
                    if (*count >= *capacity) {
                        *capacity = *capacity ? *capacity * 2 : 16;
                        *results = realloc(*results, *capacity * sizeof(char *));
                    }
                    (*results)[(*count)++] = strdup(path);
                }
            }
        }
    }
    closedir(dir);
}

char **workspace_find_src_copying(LSPServer *server, const char *copy_basename, int *out_count) {
    char **results = NULL;
    int count = 0, capacity = 0;

    if (server->workspace_folders && server->workspace_folder_count > 0) {
        for (int i = 0; i < server->workspace_folder_count; i++) {
            char *folder = uri_to_path(server->workspace_folders[i]);
            if (folder) {
                collect_src_files_copying(folder, copy_basename, &results, &count, &capacity);
                free(folder);
            }
        }
    } else if (server->root_uri) {
        char *root = uri_to_path(server->root_uri);
        if (root) {
            collect_src_files_copying(root, copy_basename, &results, &count, &capacity);
            free(root);
        }
    }

    *out_count = count;
    return results;
}

/*
 * Check if any node in the tree references `name` via the given ref_kind.
 * ref_kind: AST_IDENT (vars/defines), AST_PROC_CALL (procs), AST_GOTO (labels).
 */
static bool has_reference(AstNode *node, const char *name, AstNodeKind ref_kind) {
    if (!node || !name) return false;

    if (node->kind == ref_kind) {
        const char *ref_name = NULL;
        if (ref_kind == AST_IDENT)
            ref_name = node->data.ident.name;
        else if (ref_kind == AST_PROC_CALL)
            ref_name = node->data.call.name;
        else if (ref_kind == AST_GOTO)
            ref_name = node->data.goto_stmt.label;
        if (ref_name && strcasecmp(ref_name, name) == 0)
            return true;
    }

    /* GOTO_DEPENDING_ON stores label targets as AST_IDENT children */
    if (ref_kind == AST_GOTO && node->kind == AST_GOTO_DEPENDING_ON) {
        for (int i = 1; i < node->child_count; i++) {  /* skip child 0 (selector) */
            AstNode *child = node->children[i];
            if (child && child->kind == AST_IDENT &&
                child->data.ident.name &&
                strcasecmp(child->data.ident.name, name) == 0)
                return true;
        }
    }

    for (int i = 0; i < node->child_count; i++) {
        if (has_reference(node->children[i], name, ref_kind))
            return true;
    }
    return false;
}

/*
 * Append "unreferenced" diagnostics for unused variables, DEFINEs, PROCs, and labels.
 * style: 0=none, 1=dim (Unnecessary), 2=strikethrough (Deprecated), 3=both.
 * `root` is always the program root (for reference searches).
 * `subtree` is the current node whose children we scan for declarations.
 * `main_file` is the main document's filename — skip declarations from COPY files.
 */
/* Check if a DEFINE name was referenced via preprocessor expansion */
static bool define_was_expanded(const char *name, PreprocEvent *events, int event_count) {
    if (!events || !name) return false;
    for (int i = 0; i < event_count; i++) {
        if (events[i].kind == PREPROC_EVENT_DEFINE_REF &&
            events[i].name && strcasecmp(events[i].name, name) == 0)
            return true;
    }
    return false;
}

static void append_unused_diagnostics(cJSON *diags, AstNode *root, AstNode *subtree,
                                       int style, const char *main_file,
                                       PreprocEvent *preproc_events, int preproc_event_count) {
    if (!subtree || !diags || style == 0) return;

    for (int i = 0; i < subtree->child_count; i++) {
        AstNode *node = subtree->children[i];
        if (!node) continue;

        const char *name = NULL;
        const char *kind_label = NULL;
        AstNodeKind ref_kind = AST_IDENT;

        switch (node->kind) {
            case AST_VAR_DECL:
                name = node->data.var_decl.name;
                kind_label = "variable";
                break;
            case AST_DEFINE:
                name = node->data.define.name;
                kind_label = "define";
                break;
            case AST_LABEL:
                name = node->data.label.name;
                kind_label = "label";
                ref_kind = AST_GOTO;
                break;
            case AST_PROC:
                name = node->data.proc.name;
                kind_label = "procedure";
                ref_kind = AST_PROC_CALL;
                /* main is always the entry point */
                if (name && strcasecmp(name, "main") == 0)
                    name = NULL;
                /* Also recurse into proc body to find labels */
                append_unused_diagnostics(diags, root, node, style, main_file, preproc_events, preproc_event_count);
                break;
            default:
                /* Recurse into any container (DATA_SECTION, DO_BLOCK,
                 * IF_STMT, WHILE_STMT, etc.) to find nested declarations */
                append_unused_diagnostics(diags, root, node, style, main_file, preproc_events, preproc_event_count);
                continue;
        }

        if (!name) continue;

        /* Skip declarations from COPY files — their symbols are used by includers */
        if (main_file && node->range.start.filename &&
            strcmp(node->range.start.filename, main_file) != 0)
            continue;

        if (has_reference(root, name, ref_kind)) continue;

        /* DEFINEs are expanded by the preprocessor — no AST_IDENT remains.
         * Check preprocessor events for expansion references. */
        if (node->kind == AST_DEFINE &&
            define_was_expanded(name, preproc_events, preproc_event_count))
            continue;

        /* Build diagnostic JSON */
        cJSON *diag = cJSON_CreateObject();

        cJSON *range = cJSON_CreateObject();
        cJSON *start = cJSON_CreateObject();
        cJSON *end = cJSON_CreateObject();
        int sl = node->range.start.line > 0 ? node->range.start.line - 1 : 0;
        int sc = node->range.start.column > 0 ? node->range.start.column - 1 : 0;
        int el = node->range.end.line > 0 ? node->range.end.line - 1 : 0;
        int ec = node->range.end.column > 0 ? node->range.end.column - 1 : 0;
        cJSON_AddNumberToObject(start, "line", sl);
        cJSON_AddNumberToObject(start, "character", sc);
        cJSON_AddNumberToObject(end, "line", el);
        cJSON_AddNumberToObject(end, "character", ec);
        cJSON_AddItemToObject(range, "start", start);
        cJSON_AddItemToObject(range, "end", end);
        cJSON_AddItemToObject(diag, "range", range);

        cJSON_AddNumberToObject(diag, "severity", 4);  /* Hint */
        cJSON_AddStringToObject(diag, "source", "tbol");

        char msg[256];
        snprintf(msg, sizeof(msg), "Unreferenced %s '%s'", kind_label, name);
        cJSON_AddStringToObject(diag, "message", msg);

        /* DiagnosticTag: Unnecessary=1 (dim), Deprecated=2 (strikethrough) */
        cJSON *tags = cJSON_CreateArray();
        if (style & 1) cJSON_AddItemToArray(tags, cJSON_CreateNumber(1));
        if (style & 2) cJSON_AddItemToArray(tags, cJSON_CreateNumber(2));
        cJSON_AddItemToObject(diag, "tags", tags);

        cJSON_AddItemToArray(diags, diag);
    }
}

/*
 * Parse a document and collect diagnostics
 */
void document_parse(Document *doc, const char **include_paths, int include_count, int unreferenced_style) {
    if (!doc || !doc->content) return;

    /* Convert URI to filename for error messages */
    char *filename = uri_to_path(doc->uri);

    /* Set up parse options */
    TbolParseOptions options = {
        .include_paths = include_paths,
        .include_path_count = include_count,
        .filename = filename,
        .check_only = false,
        .collect_symbols = true,
    };

    /* Parse */
    TbolParseResult *result = tbol_parse_string(doc->content, &options);

    /* Convert diagnostics to JSON.
     * Diagnostics from COPY files have transparent line numbers that are
     * meaningless in the parent .src context. Remap them to the COPY
     * directive's line with a message prefix showing the actual location. */
    cJSON *diags = cJSON_CreateArray();

    for (int i = 0; i < result->diagnostic_count; i++) {
        TbolDiagnostic *d = &result->diagnostics[i];

        /* Check if this diagnostic is from a COPY file */
        int display_line = d->line;
        char *prefixed_msg = NULL;

        if (d->filename && result->preproc_events) {
            for (int j = 0; j < result->preproc_event_count; j++) {
                PreprocEvent *evt = &result->preproc_events[j];
                if (evt->kind == PREPROC_EVENT_COPY && evt->resolved_path &&
                    strcmp(evt->resolved_path, d->filename) == 0) {
                    /* This diagnostic is from a COPY file.
                     * Remap to the COPY directive line in the parent. */
                    int local_line = d->line - evt->line + 1;
                    if (local_line < 1) local_line = 1;
                    const char *copy_base = strrchr(evt->resolved_path, '/');
                    copy_base = copy_base ? copy_base + 1 : evt->resolved_path;
                    display_line = evt->line;
                    size_t msg_len = strlen(copy_base) + 32 +
                                     strlen(d->message ? d->message : "Unknown error");
                    prefixed_msg = malloc(msg_len);
                    snprintf(prefixed_msg, msg_len, "[%s:%d] %s",
                             copy_base, local_line,
                             d->message ? d->message : "Unknown error");
                    break;
                }
            }
        }

        cJSON *diag = cJSON_CreateObject();

        /* Range */
        cJSON *range = cJSON_CreateObject();
        cJSON *start = cJSON_CreateObject();
        cJSON_AddNumberToObject(start, "line", display_line > 0 ? display_line - 1 : 0);
        cJSON_AddNumberToObject(start, "character", d->column > 0 ? d->column - 1 : 0);
        cJSON *end = cJSON_CreateObject();
        cJSON_AddNumberToObject(end, "line", display_line > 0 ? display_line - 1 : 0);
        cJSON_AddNumberToObject(end, "character", d->end_column > 0 ? d->end_column - 1 : d->column);
        cJSON_AddItemToObject(range, "start", start);
        cJSON_AddItemToObject(range, "end", end);
        cJSON_AddItemToObject(diag, "range", range);

        /* Severity: 1=Error, 2=Warning, 3=Info, 4=Hint */
        int severity = 1;
        switch (d->level) {
            case TBOL_DIAG_ERROR:   severity = 1; break;
            case TBOL_DIAG_WARNING: severity = 2; break;
            case TBOL_DIAG_NOTE:    severity = 3; break;
        }
        cJSON_AddNumberToObject(diag, "severity", severity);

        /* Source */
        cJSON_AddStringToObject(diag, "source", "tbol");

        /* Message */
        cJSON_AddStringToObject(diag, "message",
                                prefixed_msg ? prefixed_msg :
                                (d->message ? d->message : "Unknown error"));

        free(prefixed_msg);
        cJSON_AddItemToArray(diags, diag);
    }

    /* Append diagnostics for unreferenced symbols (main file only, not COPY) */
    if (result->ast) {
        append_unused_diagnostics(diags, result->ast, result->ast, unreferenced_style, filename,
                                  result->preproc_events, result->preproc_event_count);
    }

    /* Store diagnostics */
    if (doc->diagnostics) {
        cJSON_Delete(doc->diagnostics);
    }
    doc->diagnostics = diags;

    /* Store AST — only replace if the new parse produced one.
     * On parse failure (result->ast is NULL), keep the stale AST from the
     * last successful parse so the LSP can still provide completions,
     * hover, and go-to-definition from known symbols while the user is
     * mid-edit with syntax errors. */
    if (result->ast) {
        if (doc->ast) ast_free((AstNode *)doc->ast);
        doc->ast = result->ast;
        result->ast = NULL;
    }

    /* Store preprocessor events (transfer ownership) */
    if (doc->preproc_events) {
        PreprocEventList tmp = {
            .events = doc->preproc_events,
            .count = doc->preproc_event_count,
            .capacity = doc->preproc_event_count
        };
        preproc_event_cleanup(&tmp);
    }
    doc->preproc_events = result->preproc_events;
    doc->preproc_event_count = result->preproc_event_count;
    result->preproc_events = NULL;  /* Prevent double-free */
    result->preproc_event_count = 0;

    /* Cleanup */
    tbol_parse_result_free(result);
    free(filename);
}

/*
 * Parse a .src document with content overrides for open COPY files.
 * Used when cascading changes from a dirty COPY file to dependent .src files.
 * This ensures the .src file sees the editor's buffer content, not stale disk.
 */
void document_parse_with_overrides(Document *doc, LSPServer *server) {
    if (!doc || !doc->content || !server) return;

    int unreferenced_style = server->unreferenced_style;
    char *filename = uri_to_path(doc->uri);

    /* Collect content overrides from all open COPY files */
    int override_count = 0;
    const char **override_paths = NULL;
    const char **override_contents = NULL;

    if (server->documents) {
        for (int i = 0; i < server->documents->count; i++) {
            Document *d = server->documents->documents[i];
            if (d && d->is_copy_file && d->content) {
                override_count++;
            }
        }
        if (override_count > 0) {
            override_paths = calloc(override_count, sizeof(char *));
            override_contents = calloc(override_count, sizeof(char *));
            int idx = 0;
            for (int i = 0; i < server->documents->count; i++) {
                Document *d = server->documents->documents[i];
                if (d && d->is_copy_file && d->content) {
                    override_paths[idx] = uri_to_path(d->uri);
                    override_contents[idx] = d->content;
                    idx++;
                }
            }
        }
    }

    TbolParseOptions options = {
        .include_paths = (const char **)server->include_paths,
        .include_path_count = server->include_path_count,
        .filename = filename,
        .check_only = false,
        .collect_symbols = true,
        .override_paths = override_paths,
        .override_contents = override_contents,
        .override_count = override_count,
    };

    TbolParseResult *result = tbol_parse_string(doc->content, &options);

    /* Convert diagnostics to JSON (same logic as document_parse) */
    cJSON *diags = cJSON_CreateArray();

    for (int i = 0; i < result->diagnostic_count; i++) {
        TbolDiagnostic *d = &result->diagnostics[i];

        int display_line = d->line;
        char *prefixed_msg = NULL;

        if (d->filename && result->preproc_events) {
            for (int j = 0; j < result->preproc_event_count; j++) {
                PreprocEvent *evt = &result->preproc_events[j];
                if (evt->kind == PREPROC_EVENT_COPY && evt->resolved_path &&
                    strcmp(evt->resolved_path, d->filename) == 0) {
                    int local_line = d->line - evt->line + 1;
                    if (local_line < 1) local_line = 1;
                    const char *copy_base = strrchr(evt->resolved_path, '/');
                    copy_base = copy_base ? copy_base + 1 : evt->resolved_path;
                    display_line = evt->line;
                    size_t msg_len = strlen(copy_base) + 32 +
                                     strlen(d->message ? d->message : "Unknown error");
                    prefixed_msg = malloc(msg_len);
                    snprintf(prefixed_msg, msg_len, "[%s:%d] %s",
                             copy_base, local_line,
                             d->message ? d->message : "Unknown error");
                    break;
                }
            }
        }

        cJSON *diag = cJSON_CreateObject();
        cJSON *range = cJSON_CreateObject();
        cJSON *start = cJSON_CreateObject();
        cJSON_AddNumberToObject(start, "line", display_line > 0 ? display_line - 1 : 0);
        cJSON_AddNumberToObject(start, "character", d->column > 0 ? d->column - 1 : 0);
        cJSON *end = cJSON_CreateObject();
        cJSON_AddNumberToObject(end, "line", display_line > 0 ? display_line - 1 : 0);
        cJSON_AddNumberToObject(end, "character", d->end_column > 0 ? d->end_column - 1 : d->column);
        cJSON_AddItemToObject(range, "start", start);
        cJSON_AddItemToObject(range, "end", end);
        cJSON_AddItemToObject(diag, "range", range);

        int severity = 1;
        switch (d->level) {
            case TBOL_DIAG_ERROR:   severity = 1; break;
            case TBOL_DIAG_WARNING: severity = 2; break;
            case TBOL_DIAG_NOTE:    severity = 3; break;
        }
        cJSON_AddNumberToObject(diag, "severity", severity);
        cJSON_AddStringToObject(diag, "source", "tbol");
        cJSON_AddStringToObject(diag, "message",
                                prefixed_msg ? prefixed_msg :
                                (d->message ? d->message : "Unknown error"));
        free(prefixed_msg);
        cJSON_AddItemToArray(diags, diag);
    }

    /* Append diagnostics for unreferenced symbols (main file only, not COPY) */
    if (result->ast) {
        append_unused_diagnostics(diags, result->ast, result->ast, unreferenced_style, filename,
                                  result->preproc_events, result->preproc_event_count);
    }

    if (doc->diagnostics) cJSON_Delete(doc->diagnostics);
    doc->diagnostics = diags;

    /* Keep stale AST on failed parse for mid-edit completions */
    if (result->ast) {
        if (doc->ast) ast_free((AstNode *)doc->ast);
        doc->ast = result->ast;
        result->ast = NULL;
    }

    if (doc->preproc_events) {
        PreprocEventList tmp = {
            .events = doc->preproc_events,
            .count = doc->preproc_event_count,
            .capacity = doc->preproc_event_count
        };
        preproc_event_cleanup(&tmp);
    }
    doc->preproc_events = result->preproc_events;
    doc->preproc_event_count = result->preproc_event_count;
    result->preproc_events = NULL;
    result->preproc_event_count = 0;

    tbol_parse_result_free(result);

    /* Free override paths (contents are not owned) */
    for (int i = 0; i < override_count; i++) {
        free((char *)override_paths[i]);
    }
    free(override_paths);
    free(override_contents);
    free(filename);
}

/*
 * Parse a COPY file by parsing through its parent .src file
 *
 * Finds a parent .src that includes this COPY file, parses the parent
 * (injecting dirty editor buffers as content overrides), then transfers
 * the AST and diagnostics to the COPY document.
 */
void document_parse_copy(Document *copy_doc, LSPServer *server) {
    if (!copy_doc || !server) return;

    char *copy_path = uri_to_path(copy_doc->uri);
    if (!copy_path) return;

    const char *copy_basename = strrchr(copy_path, '/');
    copy_basename = copy_basename ? copy_basename + 1 : copy_path;

    /* Find a parent .src file via CopyIndex */
    const char *parent_path = copy_index_find_parent(server->copy_index, copy_path,
                                                      server->documents);

    /* Fallback: scan workspace for a .src that includes this COPY file */
    char *parent_path_alloc = NULL;
    if (!parent_path) {
        if (server->workspace_folders && server->workspace_folder_count > 0) {
            for (int i = 0; i < server->workspace_folder_count && !parent_path_alloc; i++) {
                char *folder = uri_to_path(server->workspace_folders[i]);
                if (folder) {
                    parent_path_alloc = find_first_src_that_copies(folder, copy_basename);
                    free(folder);
                }
            }
        } else if (server->root_uri) {
            char *root = uri_to_path(server->root_uri);
            if (root) {
                parent_path_alloc = find_first_src_that_copies(root, copy_basename);
                free(root);
            }
        }
        parent_path = parent_path_alloc;
    }

    if (!parent_path) {
        log_info("COPY parse: no parent .src found for '%s'\n", copy_basename);
        free(copy_path);
        return;
    }

    log_info("COPY parse: parsing '%s' through parent '%s'\n", copy_basename, parent_path);

    /* Get parent content: prefer open editor buffer, fall back to disk */
    char *parent_uri = path_to_uri(parent_path);
    Document *parent_doc = docstore_get(server->documents, parent_uri);
    char *parent_content_alloc = NULL;
    const char *parent_content = NULL;
    if (parent_doc && parent_doc->content) {
        parent_content = parent_doc->content;
    } else {
        parent_content_alloc = read_file_content(parent_path);
        parent_content = parent_content_alloc;
    }

    if (!parent_content) {
        log_warn("COPY parse: could not read parent '%s'\n", parent_path);
        free(parent_uri);
        free(copy_path);
        free(parent_path_alloc);
        return;
    }

    /* Collect content overrides from all open COPY files */
    int override_count = 0;
    const char **override_paths = NULL;
    const char **override_contents = NULL;

    if (server->documents) {
        /* Count open COPY files with content */
        for (int i = 0; i < server->documents->count; i++) {
            Document *d = server->documents->documents[i];
            if (d && d->is_copy_file && d->content) {
                override_count++;
            }
        }
        if (override_count > 0) {
            override_paths = calloc(override_count, sizeof(char *));
            override_contents = calloc(override_count, sizeof(char *));
            int idx = 0;
            for (int i = 0; i < server->documents->count; i++) {
                Document *d = server->documents->documents[i];
                if (d && d->is_copy_file && d->content) {
                    override_paths[idx] = uri_to_path(d->uri);
                    override_contents[idx] = d->content;
                    idx++;
                }
            }
        }
    }

    /* Parse the parent with overrides */
    TbolParseOptions options = {
        .include_paths = (const char **)server->include_paths,
        .include_path_count = server->include_path_count,
        .filename = parent_path,
        .check_only = false,
        .collect_symbols = true,
        .override_paths = override_paths,
        .override_contents = override_contents,
        .override_count = override_count,
    };

    TbolParseResult *result = tbol_parse_string(parent_content, &options);

    /* Update CopyIndex from parse results */
    if (result->preproc_events) {
        copy_index_populate_from_events(server->copy_index, parent_path,
                                         result->preproc_events, result->preproc_event_count);
        /* Populate index with the parent_path_alloc if it was a fallback discovery */
        if (parent_path_alloc) {
            copy_index_add(server->copy_index, copy_path, parent_path);
        }
    }

    /* Filter diagnostics: only keep ones from this COPY file */
    cJSON *diags = cJSON_CreateArray();
    for (int i = 0; i < result->diagnostic_count; i++) {
        TbolDiagnostic *d = &result->diagnostics[i];

        /* Check if this diagnostic is from our COPY file */
        bool from_copy = false;
        if (d->filename && strcmp(d->filename, copy_path) == 0) {
            from_copy = true;
        }

        if (from_copy) {
            /* Find the COPY event to compute line offset */
            int copy_start_line = 0;
            if (result->preproc_events) {
                for (int j = 0; j < result->preproc_event_count; j++) {
                    PreprocEvent *evt = &result->preproc_events[j];
                    if (evt->kind == PREPROC_EVENT_COPY && evt->resolved_path &&
                        strcmp(evt->resolved_path, copy_path) == 0) {
                        copy_start_line = evt->line;
                        break;
                    }
                }
            }

            /* Convert transparent line to COPY-local line */
            int local_line = d->line;
            if (copy_start_line > 0) {
                local_line = d->line - copy_start_line + 1;
                if (local_line < 1) local_line = 1;
            }

            cJSON *diag = cJSON_CreateObject();
            cJSON *range = cJSON_CreateObject();
            cJSON *start = cJSON_CreateObject();
            cJSON_AddNumberToObject(start, "line", local_line > 0 ? local_line - 1 : 0);
            cJSON_AddNumberToObject(start, "character", d->column > 0 ? d->column - 1 : 0);
            cJSON *end = cJSON_CreateObject();
            cJSON_AddNumberToObject(end, "line", local_line > 0 ? local_line - 1 : 0);
            cJSON_AddNumberToObject(end, "character", d->end_column > 0 ? d->end_column - 1 : d->column);
            cJSON_AddItemToObject(range, "start", start);
            cJSON_AddItemToObject(range, "end", end);
            cJSON_AddItemToObject(diag, "range", range);

            int severity = 1;
            switch (d->level) {
                case TBOL_DIAG_ERROR:   severity = 1; break;
                case TBOL_DIAG_WARNING: severity = 2; break;
                case TBOL_DIAG_NOTE:    severity = 3; break;
            }
            cJSON_AddNumberToObject(diag, "severity", severity);
            cJSON_AddStringToObject(diag, "source", "tbol");
            cJSON_AddStringToObject(diag, "message", d->message ? d->message : "Unknown error");
            cJSON_AddItemToArray(diags, diag);
        }
    }

    /* Store diagnostics */
    if (copy_doc->diagnostics) {
        cJSON_Delete(copy_doc->diagnostics);
    }
    copy_doc->diagnostics = diags;

    /* Keep stale AST on failed parse for mid-edit completions */
    if (result->ast) {
        if (copy_doc->ast) ast_free((AstNode *)copy_doc->ast);
        copy_doc->ast = result->ast;
        result->ast = NULL;
    }

    /* Store preprocessor events (transfer ownership) */
    if (copy_doc->preproc_events) {
        PreprocEventList tmp = {
            .events = copy_doc->preproc_events,
            .count = copy_doc->preproc_event_count,
            .capacity = copy_doc->preproc_event_count
        };
        preproc_event_cleanup(&tmp);
    }
    copy_doc->preproc_events = result->preproc_events;
    copy_doc->preproc_event_count = result->preproc_event_count;
    result->preproc_events = NULL;
    result->preproc_event_count = 0;

    /* Store parent URI */
    free(copy_doc->parent_src_uri);
    copy_doc->parent_src_uri = strdup(parent_uri);

    /* Cleanup */
    tbol_parse_result_free(result);
    free(parent_uri);
    free(parent_content_alloc);
    free(parent_path_alloc);
    free(copy_path);

    /* Free override paths (we strdup'd them) */
    for (int i = 0; i < override_count; i++) {
        free((char *)override_paths[i]);
    }
    free(override_paths);
    free(override_contents);
}

/*
 * Publish diagnostics to client
 */
void document_publish_diagnostics(LSPServer *server, Document *doc) {
    if (!server || !doc) return;

    /* Build notification */
    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "uri", doc->uri);

    if (doc->diagnostics) {
        cJSON_AddItemToObject(params, "diagnostics", cJSON_Duplicate(doc->diagnostics, true));
    } else {
        cJSON_AddItemToObject(params, "diagnostics", cJSON_CreateArray());
    }

    cJSON *notif = jsonrpc_notification("textDocument/publishDiagnostics", params);
    char *str = cJSON_PrintUnformatted(notif);
    message_write(server->out, str);
    free(str);
    cJSON_Delete(notif);
}
