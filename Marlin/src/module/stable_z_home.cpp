/**
 * stable_z_home.cpp
 * Define runtime variables for STABLE_Z_HOME and provide defaults
 */
#include "stable_z_home.h"

 #if ENABLED(STABLE_Z_HOME)
uint16_t stable_z_home_max_probes       = STABLE_Z_HOME_MAX_PROBES;
uint8_t  stable_z_home_window_size      = STABLE_Z_HOME_WINDOW_SIZE;
// Some build configurations may not define STABLE_Z_HOME_RANGE_TOLERANCE
#ifndef STABLE_Z_HOME_RANGE_TOLERANCE
#define STABLE_Z_HOME_RANGE_TOLERANCE 0.01f
#endif
float    stable_z_home_range_tolerance  = STABLE_Z_HOME_RANGE_TOLERANCE;
// Some build configurations may not define STABLE_Z_HOME_MAX_Z_SHIFT
#ifndef STABLE_Z_HOME_MAX_Z_SHIFT
#define STABLE_Z_HOME_MAX_Z_SHIFT 0.06f
#endif
float    stable_z_home_max_z_shift     = STABLE_Z_HOME_MAX_Z_SHIFT;

void stable_z_home_defaults() {
  stable_z_home_max_probes      = STABLE_Z_HOME_MAX_PROBES;
  stable_z_home_window_size     = STABLE_Z_HOME_WINDOW_SIZE;
  stable_z_home_range_tolerance = STABLE_Z_HOME_RANGE_TOLERANCE;
  stable_z_home_max_z_shift     = STABLE_Z_HOME_MAX_Z_SHIFT;
}

bool stable_z_window_is_stable(const float *samples, uint16_t cap, uint16_t idx, uint16_t collected, uint8_t window_size, float tol, float &mean_out) {
  // Not enough samples collected to form a window
  if (collected < window_size) return false;

  // Compute start index of the most recent window (idx is next-write index)
  const uint16_t wstart = (idx + cap - window_size) % cap;
  float minv = 1e9f, maxv = -1e9f, sum = 0.0f;
  for (uint8_t w = 0; w < window_size; ++w) {
    const float v = samples[(wstart + w) % cap];
    if (v < minv) minv = v;
    if (v > maxv) maxv = v;
    sum += v;
  }
  const float range = maxv - minv;
  if (range <= tol) {
    mean_out = sum / window_size;
    return true;
  }
  return false;
}
#endif
