/**
 * Copyright © 2024 Austin Berrio
 *
 * @file include/interface/unit_test.h
 */

#ifndef UNIT_TEST_H
#define UNIT_TEST_H

#include <stddef.h>
#include <stdint.h>

// ---------------------- Macros ----------------------

#define ASSERT(condition, format, ...) \
    if (!(condition)) { \
        LOG_ERROR(format, ##__VA_ARGS__); \
        return 1; \
    }

// ---------------------- Structures ----------------------

// Abstract I/O operations and allow the user to define them
typedef struct TestCase {
    int8_t result; // Test result (0 for success, 1 for failure)
    size_t index; // Index of the current test case
    const void* unit; // Arbitrary input/output structure (user-defined)
} TestCase;

typedef struct TestContext {
    size_t total_tests; // Total number of test cases
    const char* test_name; // Name of the test
    TestCase* test_cases; // Array of test cases
} TestContext;

typedef struct TestRegister {
    const char* name; // Test suite name
    int (*test_suite)(void); // Pointer to the test suite function
} TestRegister;

// ---------------------- Function Pointers ----------------------

typedef int (*TestLogic)(TestCase* test); // Test logic implementation
typedef void (*TestCallback)(TestCase* test); // Cleanup or logging callback
typedef int (*TestSuite)(void); // Run unit tests

// ---------------------- Prototypes ----------------------

int run_unit_tests(TestContext* context, TestLogic logic, TestCallback callback);
int run_test_suite(const char* suite_name, TestSuite suite);

#endif // UNIT_TEST_H
