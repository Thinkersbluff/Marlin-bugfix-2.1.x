/**
 * M1128 - Stable Z Home (port of Klipper Stable-Z-Home)
 *
 * Usage:
 *  - M1128         : Run stable-Z-home at current XY
 *  - M1128 Pnn Wnn Tnn.n Xnn.n Ynn.n : Override runtime values for
 *      P = max probes, W = window size, T = retry tolerance (mm),
 *      X,Y = probe XY location to use for the routine.
 *
 * If any of P/W/T are supplied they will update the runtime variables
 * and be saved to EEPROM immediately (M500 performed internally).
 */

#include "../../inc/MarlinConfig.h"
#include "../gcode.h"
#include "../../module/probe.h"
#include "../../module/motion.h"
#include "../../module/planner.h"
#include "../../module/settings.h"
#include "../../module/stable_z_home.h"

#if ENABLED(STABLE_Z_HOME) && HAS_BED_PROBE

void GcodeSuite::M1128() {
  // Read optional parameter overrides
  const bool hasP = parser.seenval('P');
  const bool hasW = parser.seenval('W');
  const bool hasT = parser.seenval('T');

  if (hasP) stable_z_home_max_probes = parser.value_ushort();
  if (hasW) stable_z_home_window_size = (uint8_t)parser.value_byte();
  if (hasT) stable_z_home_retry_tolerance = parser.value_float();

  if (hasP || hasW || hasT) {
    // Do NOT persist automatically. M1128 is commonly embedded in gcode files
    // and we must not overwrite user EEPROM settings unexpectedly. The user
    // must call M500 to save changes to EEPROM explicitly.
    SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: runtime parameters updated (use M500 to save to EEPROM)"); SERIAL_EOL();
  }

  // Probe location
  xy_pos_t pt = xy_pos_t({ current_position[X_AXIS], current_position[Y_AXIS] });
  if (parser.seenval('X')) pt.x = parser.value_float();
  if (parser.seenval('Y')) pt.y = parser.value_float();

  // Fallback to center if probe can't reach the requested point
  if (!probe.can_reach(pt.x, pt.y)) {
    pt.x = (probe.min_x() + probe.max_x()) / 2.0f;
    pt.y = (probe.min_y() + probe.max_y()) / 2.0f;
    SERIAL_ECHO_START(); SERIAL_ECHOLNPGM("M1128: requested point unreachable, using center");
  }

  const uint16_t max_probes = (stable_z_home_max_probes > 0) ? stable_z_home_max_probes : 20;
  const uint8_t window_size = (stable_z_home_window_size > 0) ? stable_z_home_window_size : 4;
  const float tol = stable_z_home_retry_tolerance;

  // Cap arrays to a reasonable stack size
  const uint16_t cap = _MIN(max_probes, (uint16_t)100);
  float samples[100] = { 0 };
  uint16_t idx = 0;
  uint16_t collected = 0;

  SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: probing at X"); SERIAL_ECHO(pt.x); SERIAL_ECHOPGM(" Y"); SERIAL_ECHOLN(pt.y);

  // Move to XY at a safe Z clearance before probing
  do_blocking_move_to(xy_pos_t({ pt.x, pt.y }), MMM_TO_MMS(3000.0f));

  bool success = false;
  for (uint16_t attempt = 0; attempt < max_probes && attempt < cap; ++attempt) {
    // Probe at point; leave probe raised after each sample
    const float z = probe.probe_at_point(pt, ProbePtRaise::PROBE_PT_RAISE, 0, true, true);
    samples[idx++] = z;
    if (idx >= cap) idx = 0;
    if (collected < cap) ++collected;

    // Only evaluate window when we have enough samples
    if (collected >= window_size) {
      float mean = 0.0f;
      if (stable_z_window_is_stable(samples, cap, idx, collected, window_size, tol, mean)) {
        // Stable sample found. Use the mean from the helper and declare success
        SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: stable Z = "); SERIAL_ECHOLN(mean);

        // Synchronize planner/steppers, set current Z from steppers and mark Z homed
        set_current_from_steppers_for_axis(Z_AXIS);
        // set current_position.z should already reflect the probe trigger via probe.probe_at_point
        set_axis_is_at_home(Z_AXIS);
        sync_plan_position();

        success = true;
        break;
      }
      else {
        // For debug visibility, compute the range of the latest window and report it
        float minv = 1e9f, maxv = -1e9f;
        uint16_t wstart = (idx + cap - window_size) % cap;
        for (uint8_t w = 0; w < window_size; ++w) {
          const float v = samples[(wstart + w) % cap];
          if (v < minv) minv = v;
          if (v > maxv) maxv = v;
        }
        const float range = maxv - minv;
        SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: attempt "); SERIAL_ECHO(attempt + 1);
        SERIAL_ECHOPGM(" window_range="); SERIAL_ECHOLN(range);
      }
    }
    safe_delay(50); // small gap between probes
  }

  if (!success) {
    SERIAL_ECHO_START(); SERIAL_ECHOLNPGM("M1128: failed to find stable Z within probe limit");
  }
}

#else
void GcodeSuite::M1128() {
  SERIAL_ECHOLNPGM("M1128: STABLE_Z_HOME not enabled or no probe available");
}
#endif
