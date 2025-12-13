#include "../test/unit_tests.h"
#include <src/module/stable_z_home.h>

#if ENABLED(STABLE_Z_HOME)

MARLIN_TEST(stable_z_home, window_detects_stable_mean) {
  const uint16_t cap = 8;
  float samples[8] = {0};
  // Fill buffer with values that are stable within tol=0.05
  samples[0] = 1.000f; samples[1] = 0.995f; samples[2] = 1.002f; samples[3] = 0.998f;
  const uint16_t idx = 4; // next write index
  const uint16_t collected = 4;
  const uint8_t window_size = 4;
  const float tol = 0.01f; // small tolerance

  float mean = 0.0f;
  const bool ok = stable_z_window_is_stable(samples, cap, idx, collected, window_size, tol, mean);
  TEST_ASSERT_EQUAL(true, ok);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, (samples[0]+samples[1]+samples[2]+samples[3]) / 4.0f, mean);
}

MARLIN_TEST(stable_z_home, window_detects_unstable) {
  const uint16_t cap = 6;
  float samples[6] = {0};
  samples[0] = 1.0f; samples[1] = 1.2f; samples[2] = 0.8f; samples[3] = 1.5f;
  const uint16_t idx = 4;
  const uint16_t collected = 4;
  const uint8_t window_size = 4;
  const float tol = 0.1f; // tight tolerance

  float mean = 0.0f;
  const bool ok = stable_z_window_is_stable(samples, cap, idx, collected, window_size, tol, mean);
  TEST_ASSERT_EQUAL(false, ok);
}

MARLIN_TEST(stable_z_home, window_not_enough_samples) {
  const uint16_t cap = 8;
  float samples[8] = {0};
  samples[0] = 1.0f; samples[1] = 1.0f;
  const uint16_t idx = 2;
  const uint16_t collected = 2;
  const uint8_t window_size = 4; // need more than collected
  const float tol = 0.1f;

  float mean = 0.0f;
  const bool ok = stable_z_window_is_stable(samples, cap, idx, collected, window_size, tol, mean);
  TEST_ASSERT_EQUAL(false, ok);
}

#else

// If the feature isn't enabled we still want the test suite to build and pass.
MARLIN_TEST(stable_z_home, stub_when_disabled) {
  TEST_ASSERT_EQUAL(true, true);
}

#endif
