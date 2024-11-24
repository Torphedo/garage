#include <GLFW/glfw3.h>

#include <common/logging.h>
#include "timing_targets.h"

bool dbg_assert_perf_func(double since, double target_ms, const char* caller) {
    const double now = glfwGetTime();
    const double elapsed = now - since;
    if (elapsed > target_ms / 1000) {
        // We need the underlying logging_print() to print the caller's name
        logging_print(debug, caller, "Finished in %.3lfms [target %.0fms]\n", elapsed * 1000, target_ms);
        return false; // Failed target.
    }

    // We finished in time :)
    return true;
}
