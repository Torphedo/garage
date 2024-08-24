#include <stdbool.h>

#include <common/int.h>
#include <common/logging.h>

bool test_stfs();

typedef bool (*testproc)(void);
testproc tests[] = {
    test_stfs,
};

int main() {
    enable_win_ansi();

    LOG_MSG(info, "Running %d tests\n", ARRAY_SIZE(tests));
    u32 passed_count = 0;
    for (u32 i = 0; i < ARRAY_SIZE(tests); i++) {
        passed_count += tests[i]();
    }

    float passed_ratio = ((float)passed_count / (float)ARRAY_SIZE(tests)) * 100;
    LOG_MSG(info, "%.1f%% passed.\n", passed_ratio);

    // Communicate overall pass/fail via process exit code
    if (passed_count != ARRAY_SIZE(tests)) {
        return 1;
    }
    return 0;
}
