/**
 * Copyright © 2024 Austin Berrio
 *
 * @file src/core/unit_test.c
 */

#include "core/logger.h"
#include "core/unit_test.h"

int run_unit_tests(TestContext* context, TestLogic logic, TestCallback callback) {
    if (!context || !context->test_cases || !logic) {
        LOG_ERROR("Invalid parameters.");
        return -1;
    }

    LOG_INFO("[RUN] %s: Number of tests: %zu", context->test_name, context->total_tests);

    size_t failures = 0;

    for (size_t i = 0; i < context->total_tests; i++) {
        TestCase* test_case = &context->test_cases[i];
        test_case->index = i + 1;

        int result = logic(test_case);

        if (result != 0) {
            failures++;
            LOG_ERROR("[FAIL] %s: Test case %zu failed.", context->test_name, test_case->index);
        }

        if (callback) {
            callback(test_case);
        }
    }

    size_t passed = context->total_tests - failures;
    LOG_INFO(
        "[RESULT] %s: %zu/%zu tests passed", context->test_name, passed, context->total_tests
    );

    return failures > 0 ? 1 : 0;
}

int run_test_suite(const char* suite_name, TestSuite suite) {
    LOG_INFO("[RUN] %s", suite_name);
    int result = suite();
    if (result == 0) {
        LOG_INFO("[PASS] %s", suite_name);
    } else {
        LOG_ERROR("[FAIL] %s", suite_name);
    }
    return result;
}
