#ifndef TIMING_TARGETS_H
#define TIMING_TARGETS_H
#include <stdbool.h>

/// @brief Assertion to check if we're meeting a performance target (Non-fatal)
///
/// Checks if the current time is less than [target_ms] milliseconds away from
/// the provided time [since]. If not, prints a debug message about the event.
/// @param since The time when the operation you're timing started (usually the start of a function)
/// @param target_ms The performance target time (in milliseconds)
/// @param caller The name of the function to log as the caller. Use the
/// DBG_ASSERT_PERF() macro to have this parameter filled automatically.
/// @return Returns whether you hit your target
bool dbg_assert_perf_func(double since, double target_ms, const char* caller);

/// @brief Wrapper around dbg_assert_perf_func(), automatically passes the caller's name
#define DBG_ASSERT_PERF(since, target_ms) dbg_assert_perf_func(since, target_ms, __func__)

#endif // TIMING_TARGETS_H
