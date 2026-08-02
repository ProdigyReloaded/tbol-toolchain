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
 * Simple test framework for TBOL LSP
 *
 * Tests use goto-cleanup to ensure resources are freed even on assertion failure.
 * Each TEST function must:
 *   1. Declare TEST_INIT at the top
 *   2. Have a cleanup: label before resource freeing
 *   3. End with TEST_FINI
 */
#ifndef TBOL_LSP_TEST_H
#define TBOL_LSP_TEST_H

#include <stdio.h>
#include <string.h>

/* Test counters */
extern int test_pass_count;
extern int test_fail_count;

/* Test macros */
#define TEST(name) static void test_##name(void)

#define RUN_TEST(name) do { \
    int _before = test_fail_count; \
    printf("  %s... ", #name); \
    test_##name(); \
    if (test_fail_count == _before) { \
        printf("OK\n"); \
    } \
} while(0)

/* Place at the start of each test function */
#define TEST_INIT int _test_failed = 0; (void)_test_failed

/* Place at the end of each test function (after cleanup: label and resource freeing) */
#define TEST_FINI do { if (!_test_failed) test_pass_count++; } while(0)

/* Assertion macros - goto cleanup on failure */
#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL\n    Assertion failed: %s\n    At %s:%d\n", \
               #cond, __FILE__, __LINE__); \
        test_fail_count++; \
        _test_failed = 1; \
        goto cleanup; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("FAIL\n    Expected %s == %s\n    At %s:%d\n", \
               #a, #b, __FILE__, __LINE__); \
        test_fail_count++; \
        _test_failed = 1; \
        goto cleanup; \
    } \
} while(0)

#define ASSERT_STREQ(a, b) do { \
    if (strcmp((a), (b)) != 0) { \
        printf("FAIL\n    Expected \"%s\" == \"%s\"\n    Got \"%s\" == \"%s\"\n    At %s:%d\n", \
               #a, #b, (a), (b), __FILE__, __LINE__); \
        test_fail_count++; \
        _test_failed = 1; \
        goto cleanup; \
    } \
} while(0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        printf("FAIL\n    Expected %s != NULL\n    At %s:%d\n", \
               #ptr, __FILE__, __LINE__); \
        test_fail_count++; \
        _test_failed = 1; \
        goto cleanup; \
    } \
} while(0)

#define ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        printf("FAIL\n    Expected %s == NULL\n    At %s:%d\n", \
               #ptr, __FILE__, __LINE__); \
        test_fail_count++; \
        _test_failed = 1; \
        goto cleanup; \
    } \
} while(0)

#define TEST_SUITE(name) void test_suite_##name(void)

#define RUN_SUITE(name) do { \
    printf("\n%s:\n", #name); \
    test_suite_##name(); \
} while(0)

/* Test suite declarations */
void test_suite_jsonrpc(void);
void test_suite_handlers(void);
void test_suite_protocol(void);
void test_suite_document(void);

#endif /* TBOL_LSP_TEST_H */
