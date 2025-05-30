/**
 * Copyright © 2024 Austin Berrio
 *
 * @file tests/test_logger.c
 * @brief Automated unit tests for the thread-safe logger implementation.
 *
 * Build:
 *   gcc -o test_logger source/logger.c tests/test_logger.c -lpthread
 *
 * Manual Run:
 *   ./build/tests/test_logger
 *   # Displays detailed pass/fail status for each logger test suite.
 *
 * Automated Run (CTest):
 *   ctest --output-on-failure --test-dir build/
 *   # Integrates with CMake/CTest; works in CI and locally.
 *
 * Output:
 *   [INFO] ... [RUN] <suite>
 *   [INFO] ... [RESULT] <suite>: <N/N> tests passed
 *   [PASS]/[FAIL] suite status
 *   # See test_logger output for detailed logs and any assertion failures.
 *
 * Temp Files:
 *   Each test writes logs to 'temp.log' in the working directory for verification.
 *   Files are created and removed automatically; no manual cleanup required.
 *
 * Notes:
 *   - Tests cover: log level filtering, file logging, global logger, and lazy logger initialization.
 *   - No manual inspection required; all assertions are automated.
 *   - To check actual log file output, inspect 'temp.log' after manual test run.
 */

#include "core/logger.h"
#include "core/unit_test.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

/**
 * @section File Utilities
 */

static const char* file_temp = "temp.log";

static int file_capture(FILE* pipe, const char* path) {
    remove(path); // Clean up BEFORE reading
    fflush(stderr);
    int state = dup(fileno(pipe));
    int file = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    dup2(file, fileno(pipe));
    close(file);
    return state;
}

static void file_restore(FILE* pipe, const char* path, int state) {
    fflush(pipe);
    dup2(state, fileno(pipe));
    close(state);
    remove(path); // Clean up AFTER reading
}

static bool file_match(const char* path, const char* message) {
    // Read back file and check if the message appears
    FILE* fp = fopen(path, "r");
    bool match = false;
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, message)) {
                match = true;
                break;
            }
        }
        fclose(fp);
    }
    return match;
}

/**
 * @section Test Log Levels
 */

typedef struct LoggerTestLevel {
    const LogLevel level;
    const char* message;
    const bool expected;
} LoggerTestLevel;

int test_logger_level(TestCase* test) {
    LoggerTestLevel* unit = (LoggerTestLevel*) test->unit;

    int state = file_capture(stderr, file_temp);

    Logger* logger = logger_create(LOG_LEVEL_WARN, LOG_TYPE_STREAM, NULL);
    if (!logger) {
        LOG_ERROR("[LoggerTestLevel] Failed to create a logger instance!");
        return 1;
    }
    const bool result = logger_message(logger, unit->level, "%s\n", unit->message);
    logger_free(logger);

    file_restore(stderr, file_temp, state);

    ASSERT(
        result == unit->expected,
        "[LoggerTestLevel] level=%d, message=%d expected='%s' got='%s'",
        unit->level,
        unit->message,
        unit->expected ? "true" : "false",
        result ? "true" : "false"
    );

    return 0;
}

int test_logger_level_suite(void) {
    static LoggerTestLevel cases[] = {
        {LOG_LEVEL_INFO, "This message should not appear", false},
        {LOG_LEVEL_WARN, "Global logger warning", true},
        {LOG_LEVEL_ERROR, "Global logger error", true},
    };

    size_t total_tests = sizeof(cases) / sizeof(LoggerTestLevel);

    TestCase test_cases[total_tests];
    for (size_t i = 0; i < total_tests; i++) {
        test_cases[i].unit = &cases[i];
    }

    TestContext context = {
        .total_tests = total_tests,
        .test_name = "Logger Test Level",
        .test_cases = test_cases,
    };

    // Return test suite results
    return run_unit_tests(&context, test_logger_level, NULL);
    ;
}

/**
 * @section Test Log Files
 */

typedef struct LoggerTestFile {
    LogLevel logger_level;
    LogLevel message_level;
    const char* message;
    bool should_log; // Should this message appear in the file?
} LoggerTestFile;

int test_logger_file(TestCase* test) {
    LoggerTestFile* unit = (LoggerTestFile*) test->unit;

    int state = file_capture(stderr, file_temp);

    Logger* logger = logger_create(unit->logger_level, LOG_TYPE_FILE, file_temp);
    if (!logger) {
        LOG_ERROR("[LoggerTestFile] Failed to create a logger instance!");
        return 1;
    }
    logger_message(logger, unit->message_level, "%s", unit->message);
    logger_free(logger);

    const bool match = file_match(file_temp, unit->message);
    file_restore(stderr, file_temp, state);

    ASSERT(
        match == unit->should_log,
        "[LoggerTestFile] logger_level=%d, message_level=%d, expected='%s', got='%s'",
        unit->logger_level,
        unit->message_level,
        unit->should_log ? "present" : "absent",
        match ? "present" : "absent"
    );

    return 0;
}

int test_logger_file_suite(void) {
    static LoggerTestFile cases[] = {
        {LOG_LEVEL_INFO, LOG_LEVEL_DEBUG, "Debug should not log", false},
        {LOG_LEVEL_INFO, LOG_LEVEL_INFO, "Info should log", true},
        {LOG_LEVEL_INFO, LOG_LEVEL_ERROR, "Error should log", true},
    };

    size_t total_tests = sizeof(cases) / sizeof(LoggerTestFile);

    TestCase test_cases[total_tests];
    for (size_t i = 0; i < total_tests; i++) {
        test_cases[i].unit = &cases[i];
    }

    TestContext context = {
        .total_tests = total_tests,
        .test_name = "Logger Test File",
        .test_cases = test_cases,
    };

    return run_unit_tests(&context, test_logger_file, NULL);
}

/**
 * @section Test Global Logger
 */

typedef struct LoggerTestGlobal {
    LogLevel logger_level; // What level is the global logger set to?
    LogLevel message_level; // What level is the log message?
    const char* message;
    bool should_log; // Should the message be logged, given the logger_level?
} LoggerTestGlobal;

int test_logger_global(TestCase* test) {
    LoggerTestGlobal* unit = (LoggerTestGlobal*) test->unit;

    int state = file_capture(stderr, file_temp);

    // Set up global logger to desired level
    logger_set_global(unit->logger_level, LOG_TYPE_STREAM, "stream", stderr, NULL);

    // Use LOG_* macro (or direct logger_message for even tighter focus)
    LOG(&logger_global, unit->message_level, "%s", unit->message);

    const bool match = file_match(file_temp, unit->message);
    file_restore(stderr, file_temp, state);

    logger_set_global(LOG_LEVEL_DEBUG, LOG_TYPE_STREAM, "stream", NULL, NULL); // Reset

    ASSERT(
        match == unit->should_log,
        "[LoggerTestGlobal] logger_level=%d, message_level=%d, expected='%s', got='%s'",
        unit->logger_level,
        unit->message_level,
        unit->should_log ? "present" : "absent",
        match ? "present" : "absent"
    );

    return 0;
}

int test_logger_global_suite(void) {
    static LoggerTestGlobal cases[] = {
        {LOG_LEVEL_WARN, LOG_LEVEL_INFO, "This message should not appear", false},
        {LOG_LEVEL_WARN, LOG_LEVEL_WARN, "Global logger warning", true},
        {LOG_LEVEL_WARN, LOG_LEVEL_ERROR, "Global logger error", true},
        {LOG_LEVEL_ERROR, LOG_LEVEL_WARN, "Warn should not log at error", false},
        {LOG_LEVEL_DEBUG, LOG_LEVEL_INFO, "Debug logger: info logs", true},
    };

    size_t total_tests = sizeof(cases) / sizeof(LoggerTestGlobal);

    TestCase test_cases[total_tests];
    for (size_t i = 0; i < total_tests; i++) {
        test_cases[i].unit = &cases[i];
    }

    TestContext context = {
        .total_tests = total_tests,
        .test_name = "Logger Global",
        .test_cases = test_cases,
    };

    return run_unit_tests(&context, test_logger_global, NULL);
}

/**
 * @section Test Lazy Initialization
 */

typedef struct LoggerTestLazy {
    LogLevel log_level; // Level to create the logger with
    LogLevel message_level;
    const char* message;
    bool should_log;
} LoggerTestLazy;

int test_logger_lazy(TestCase* test) {
    LoggerTestLazy* unit = (LoggerTestLazy*) test->unit;

    int state = file_capture(stderr, file_temp);

    Logger* lazy_logger = logger_create(unit->log_level, LOG_TYPE_UNKNOWN, NULL);
    if (!lazy_logger) {
        LOG_ERROR("[LoggerTestLazy] Failed to create a logger instance!");
        file_restore(stderr, file_temp, state);
        return 1;
    }
    LOG(lazy_logger, unit->message_level, "%s", unit->message);
    logger_free(lazy_logger);

    const bool match = file_match(file_temp, unit->message);
    file_restore(stderr, file_temp, state);

    ASSERT(
        match == unit->should_log,
        "[LoggerTestLazy] log_level=%d, message_level=%d, expected='%s', got='%s'",
        unit->log_level,
        unit->message_level,
        unit->should_log ? "present" : "absent",
        match ? "present" : "absent"
    );

    return 0;
}

int test_logger_lazy_suite(void) {
    static LoggerTestLazy cases[] = {
        {LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG, "Lazy logger debug", true},
        {LOG_LEVEL_DEBUG, LOG_LEVEL_ERROR, "Lazy logger error", true},
        {LOG_LEVEL_INFO, LOG_LEVEL_DEBUG, "This should not log", false},
    };

    size_t total_tests = sizeof(cases) / sizeof(LoggerTestLazy);

    TestCase test_cases[total_tests];
    for (size_t i = 0; i < total_tests; i++) {
        test_cases[i].unit = &cases[i];
    }

    TestContext context = {
        .total_tests = total_tests,
        .test_name = "Logger Lazy Initialization",
        .test_cases = test_cases,
    };

    return run_unit_tests(&context, test_logger_lazy, NULL);
}

int main(void) {
    static TestRegister suites[] = {
        {"Log Level", test_logger_level_suite},
        {"Log File", test_logger_file_suite},
        {"Logger Global", test_logger_global_suite},
        {"Logger Lazy Initialization", test_logger_lazy_suite},
    };

    int result = 0;
    size_t total = sizeof(suites) / sizeof(TestRegister);
    for (size_t i = 0; i < total; i++) {
        result |= run_test_suite(suites[i].name, suites[i].test_suite);
    }

    return result;
}
