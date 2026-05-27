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
 * Test runner for TBOL LSP
 */
#include "test.h"
#include "../lsp.h"
#include <stdio.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

int test_pass_count = 0;
int test_fail_count = 0;

/* Override log level from server.c - set before tests run */
extern int g_log_level;

int main(void) {
    /* Unbuffer stdout so test names reach the log before any crash;
     * Windows in particular discards buffered output on abnormal exit. */
    setvbuf(stdout, NULL, _IONBF, 0);

    /* Ensure /tmp/ exists; tests use it for mkdtemp scratch dirs.  On
     * Linux/macOS it is always present; on Windows native (UCRT64)
     * /tmp resolves to <cwd-drive>:\tmp\ which is not preconfigured. */
#ifdef _WIN32
    _mkdir("/tmp");
#else
    mkdir("/tmp", 0755);
#endif

    /* Suppress log output during tests */
    g_log_level = LOG_ERROR + 1;

    printf("TBOL Language Server Tests\n");
    printf("==========================\n");

    RUN_SUITE(jsonrpc);
    RUN_SUITE(handlers);
    RUN_SUITE(protocol);
    RUN_SUITE(document);

    printf("\n==========================\n");
    printf("Results: %d passed, %d failed\n",
           test_pass_count, test_fail_count);

    return test_fail_count > 0 ? 1 : 0;
}
