#ifndef TESTING_H
#define TESTING_H
#include <stdbool.h>
#include <stdio.h>

static const char* fail_msg = "\033[31mFAIL\033[0m"; // Colored red
static const char* pass_msg = "\033[32mPASS\033[0m"; // Colored green

// Each test calls this to print a color-coded pass/fail report, so I don't have
// to type out all the function names manually.
#define REPORT_RESULT(passed) printf("[%s] %s\n", (passed) ? pass_msg : fail_msg, __func__)
#endif // TESTING_H

