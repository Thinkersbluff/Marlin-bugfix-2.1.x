/**
 * stable_z_home.h
 * Runtime variables for STABLE_Z_HOME feature
 */
#pragma once

#include "../inc/MarlinConfig.h"

#if ENABLED(STABLE_Z_HOME)
extern uint16_t stable_z_home_max_probes;
extern uint8_t  stable_z_home_window_size;
extern float    stable_z_home_range_tolerance;
extern float    stable_z_home_max_z_shift;

void stable_z_home_defaults();

// Testable helper: evaluate the most-recent window of samples for stability.
// - `samples`: circular buffer of length `cap` containing probe Z samples
// - `cap`: capacity of the circular buffer
// - `idx`: next write index in the circular buffer (i.e. one past the latest sample)
// - `collected`: number of samples collected so far (<= cap)
// - `window_size`: number of newest samples to evaluate
// - `tol`: acceptable range (max - min) for the window to be considered stable
// Returns true when the latest window is stable and writes the computed mean to `mean_out`.
bool stable_z_window_is_stable(const float *samples, uint16_t cap, uint16_t idx, uint16_t collected, uint8_t window_size, float tol, float &mean_out);
#endif
