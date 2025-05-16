/**
 * Copyright © 2024 Austin Berrio
 *
 * @file include/core/unit_test.h
 * @brief Minimal customizable unit testing framework for C.
 *
 * Provides structures and functions to define, run, and manage unit tests
 * with flexible test logic and callback hooks.
 */

#ifndef UNIT_TEST_H
#define UNIT_TEST_H

#include <stddef.h>
#include <stdint.h>

/**
 * @name Macros
 * @{
 */

/**
 * @brief Asserts a condition in a test and logs an error if false.
 *
 * If the condition is false, logs the formatted error message and
 * returns 1 to indicate test failure.
 *
 * @param condition Condition to assert.
 * @param format printf-style format string for the error message.
 * @param ... Arguments for the format string.
 */
#define ASSERT(condition, format, ...) \
    if (!(condition)) { \
        LOG_ERROR(format, ##__VA_ARGS__); \
        return 1; \
    }

/** @} */

/**
 * @name Structures
 * @{
 */

/**
 * @brief Represents a single test case.
 */
typedef struct TestCase {
    int8_t result; /**< Result of the test case (0 = success, 1 = failure). */
    size_t index; /**< Index number of the test case (1-based). */
    const void* unit; /**< Pointer to user-defined test data or state. */
} TestCase;

/**
 * @brief Context for running a group of tests.
 */
typedef struct TestContext {
    size_t total_tests; /**< Total number of test cases in the context. */
    const char* test_name; /**< Name or description of the test suite. */
    TestCase* test_cases; /**< Array of test cases to run. */
} TestContext;

/**
 * @brief Represents a named test suite.
 */
typedef struct TestRegister {
    const char* name; /**< Name of the test suite. */
    int (*test_suite)(void); /**< Pointer to the function that runs the suite. */
} TestRegister;

/** @} */

/**
 * @name Function Pointer Types
 * @{
 */

/**
 * @brief Pointer type for test logic functions.
 *
 * Functions implementing the test logic should take a pointer to a
 * TestCase and return 0 for success, non-zero for failure.
 */
typedef int (*TestLogic)(TestCase* test);

/**
 * @brief Pointer type for optional test callbacks.
 *
 * Called after each test case runs; useful for cleanup or logging.
 */
typedef void (*TestCallback)(TestCase* test);

/**
 * @brief Pointer type for test suite functions.
 *
 * Test suites run a series of tests and return 0 on success, non-zero otherwise.
 */
typedef int (*TestSuite)(void);

/** @} */

/**
 * @name Unit Test Functions
 * @{
 */

/**
 * @brief Runs a set of unit tests within a TestContext.
 *
 * Executes the provided TestLogic function on each TestCase.
 * Optionally invokes a callback after each test.
 * Logs results and returns 0 if all tests pass, 1 otherwise.
 *
 * @param context Pointer to the TestContext describing the tests.
 * @param logic Function pointer to test logic implementation.
 * @param callback Optional callback function called after each test (may be NULL).
 * @return 0 if all tests pass, 1 if any fail, -1 on invalid input.
 */
int run_unit_tests(TestContext* context, TestLogic logic, TestCallback callback);

/**
 * @brief Runs a named test suite function.
 *
 * Logs start and completion status of the suite.
 *
 * @param suite_name Name of the test suite.
 * @param suite Function pointer to the test suite to run.
 * @return 0 on success, non-zero on failure.
 */
int run_test_suite(const char* suite_name, TestSuite suite);

/** @} */

#endif // UNIT_TEST_H
