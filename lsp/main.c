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
 * TBOL Language Server - Entry Point
 */
#include "lsp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "\n");
    fprintf(stderr, "TBOL Language Server\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h, --help      Show this help\n");
    fprintf(stderr, "  -v, --version   Show version\n");
    fprintf(stderr, "  --stdio         Use stdio for communication (default)\n");
    fprintf(stderr, "  --debug         Enable debug logging\n");
    fprintf(stderr, "  -I <path>       Add include path for COPY files\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "The server communicates via stdin/stdout using the\n");
    fprintf(stderr, "Language Server Protocol (LSP) over JSON-RPC.\n");
}

static void print_version(void) {
    printf("tbol-lsp 0.1.0\n");
}

int main(int argc, char *argv[]) {
    char **include_paths = NULL;
    int include_count = 0;
    int include_cap = 0;

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        }
        if (strcmp(argv[i], "--stdio") == 0) {
            /* Default mode, nothing to do */
            continue;
        }
        if (strcmp(argv[i], "--debug") == 0) {
            g_log_level = LOG_DEBUG;
            continue;
        }
        if (strcmp(argv[i], "-I") == 0 && i + 1 < argc) {
            if (include_count >= include_cap) {
                include_cap = include_cap ? include_cap * 2 : 4;
                include_paths = realloc(include_paths, include_cap * sizeof(char *));
            }
            include_paths[include_count++] = strdup(argv[++i]);
            continue;
        }
        fprintf(stderr, "Unknown option: %s\n", argv[i]);
        print_usage(argv[0]);
        return 1;
    }

#ifdef _WIN32
    /* Set stdin/stdout to binary mode on Windows */
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    /* Create and run server */
    LSPServer *server = server_new(stdin, stdout);

    /* Transfer include paths to server */
    server->include_paths = include_paths;
    server->include_path_count = include_count;

    int result = server_run(server);

    server_free(server);

    return result;
}
