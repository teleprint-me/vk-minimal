/*
 * Copyright © 2024 Austin Berrio
 *
 * tests/test_logger.c
 *
 * Build:
 *   gcc -o test_logger source/logger.c tests/test_logger.c -lpthread
 * Run:
 *   ./test_logger
 * Expected output:
 *   [WARN] Global logger warning
 *   [ERROR] Global logger error
 *   [DEBUG] Lazy logger debug
 *   [ERROR] Lazy logger error
 *   [INFO] Should log info
 *   [ERROR] Should log error
 *   Finished all tests!
 * Run: cat test.log
 * Expected output:
 *   [DEBUG] Logging to a file: 1, 2, 3... Done!
 */

#include "core/unit_test.h"
#include "core/logger.h"

#include <stdio.h>

typedef struct LoggerFileTest {
    LogLevel set_level;
    LogLevel message_level;
    const char* message;
    bool should_log; // Should this message appear in the file?
} LoggerFileTest;

int test_logger_file(TestCase* test) {
    LoggerFileTest* unit = (LoggerFileTest*) test->unit;
    const char* temp_file = "test_tmp.log";
    remove(temp_file); // Remove file if exists before the test

    // Log a message
    Logger* logger = logger_create(unit->set_level, LOG_TYPE_FILE, temp_file);
    logger_message(logger, unit->message_level, "%s", unit->message);
    logger_free(logger);

    // Read back file and check if the message appears
    FILE* fp = fopen(temp_file, "r");
    bool found = false;
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, unit->message)) {
                found = true;
                break;
            }
        }
        fclose(fp);
    }
    remove(temp_file);

    ASSERT(
        found == unit->should_log,
        "Logger file test failed: set_level=%d message_level=%d expected='%s' got='%s'",
        unit->set_level,
        unit->message_level,
        unit->should_log ? "present" : "absent",
        found ? "present" : "absent"
    );
    return 0;
}

int logger_file_test_suite(void) {
    static LoggerFileTest cases[] = {
        {LOG_LEVEL_INFO, LOG_LEVEL_DEBUG, "Debug should not log", false},
        {LOG_LEVEL_INFO, LOG_LEVEL_INFO, "Info should log", true},
        {LOG_LEVEL_INFO, LOG_LEVEL_ERROR, "Error should log", true},
    };

    size_t total_tests = sizeof(cases) / sizeof(LoggerFileTest);

    TestCase test_cases[total_tests];
    for (size_t i = 0; i < total_tests; i++) {
        test_cases[i].unit = &cases[i];
        test_cases[i].index = i + 1;
        test_cases[i].result = -1;
    }

    TestContext ctx = {
        .total_tests = total_tests,
        .test_name = "Logger File Output",
        .test_cases = test_cases,
    };

    return run_unit_tests(&ctx, test_logger_file, NULL);
}

// Test the explicit initialization of the global logger
void test_global_logger_initialization() {
    initialize_global_logger(LOG_LEVEL_WARN, LOG_TYPE_STREAM, "stream", stderr, NULL);
    LOG(&global_logger, LOG_LEVEL_INFO, "This message should not appear\n");
    LOG(&global_logger, LOG_LEVEL_WARN, "Global logger warning\n");
    LOG(&global_logger, LOG_LEVEL_ERROR, "Global logger error\n");
}

// Test lazy initialization and logging behavior
void test_lazy_initialization_and_logging() {
    Logger* lazy_logger = logger_create(LOG_LEVEL_DEBUG, LOG_TYPE_UNKNOWN, NULL);
    LOG(lazy_logger, LOG_LEVEL_DEBUG, "Lazy logger debug\n");
    LOG(lazy_logger, LOG_LEVEL_ERROR, "Lazy logger error\n");
    logger_free(lazy_logger);
}

// Test logging at different levels
void test_logging_at_different_levels() {
    Logger* level_logger = logger_create(LOG_LEVEL_INFO, LOG_TYPE_STREAM, NULL);
    LOG(level_logger, LOG_LEVEL_DEBUG, "Should not log debug\n");
    LOG(level_logger, LOG_LEVEL_INFO, "Should log info\n");
    LOG(level_logger, LOG_LEVEL_ERROR, "Should log error\n");
    logger_free(level_logger);
}

// Test logging to a file
void test_logging_to_file() {
    const char* file_path = "test.log";
    Logger* file_logger = logger_create(LOG_LEVEL_DEBUG, LOG_TYPE_FILE, file_path);
    LOG(file_logger, LOG_LEVEL_DEBUG, "Logging to a file: 1, 2, %d... Done!\n", 3);

    // Clean up
    logger_free(file_logger);
}

int main(void) {
    static TestRegister suites[] = {
        {"Logger File Output", logger_file_test_suite},
    };

    int result = 0;
    size_t total = sizeof(suites) / sizeof(TestRegister);
    for (size_t i = 0; i < total; i++) {
        result |= run_test_suite(suites[i].name, suites[i].test_suite);
    }

    return result;
}
