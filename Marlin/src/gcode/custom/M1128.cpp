/**
 * M1128 - Stable Z Home (port of Klipper Stable-Z-Home, from
 * https://www.klipper3d.org/Stable_Z_Home.html)
 * 
 * Usage:
 *  - M1128         : Run stable-Z-home at current XY
 *  - M1128 Pnn Wnn Tnn.n Xnn.n Ynn.n : Override runtime values for
 *      P = max # of probes, W = sliding window size, T = max range tolerance (mm),
 *      X,Y = probe XY location to use for the routine.
 *
 * If any of P/W/T are supplied they will update the runtime variables.
 * Must use M500 to save changes to EEPROM explicitly.
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
  // Read optional parameter overrides (capture values for summary)
  const bool hasP = parser.seenval('P');
  const bool hasW = parser.seenval('W');
  const bool hasT = parser.seenval('T');
  int cmd_P = -1;
  int cmd_W = -1;
  float cmd_T = NAN;
  if (hasP) { cmd_P = parser.value_ushort(); stable_z_home_max_probes = cmd_P; }
  if (hasW) { cmd_W = parser.value_byte(); stable_z_home_window_size = (uint8_t)cmd_W; }
  if (hasT) { cmd_T = parser.value_float(); stable_z_home_range_tolerance = cmd_T; }

  if (hasP || hasW || hasT) {
    // Do NOT persist automatically. M1128 is commonly embedded in gcode files
    // and we must not overwrite user EEPROM settings unexpectedly. The user
    // must call M500 to save changes to EEPROM explicitly.
    SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: runtime parameters updated (use M500 to save to EEPROM)"); SERIAL_EOL();
  }

  // Probe location
  xy_pos_t pt = xy_pos_t({ current_position[X_AXIS], current_position[Y_AXIS] });
  float cmd_X = NAN, cmd_Y = NAN;
  if (parser.seenval('X')) { cmd_X = parser.value_float(); pt.x = cmd_X; }
  if (parser.seenval('Y')) { cmd_Y = parser.value_float(); pt.y = cmd_Y; }

  // Fallback to center if probe can't reach the requested point
  if (!probe.can_reach(pt.x, pt.y)) {
    pt.x = (probe.min_x() + probe.max_x()) / 2.0f;
    pt.y = (probe.min_y() + probe.max_y()) / 2.0f;
    SERIAL_ECHO_START(); SERIAL_ECHOLNPGM("M1128: requested point unreachable, using center");
  }

  const uint16_t max_probes = (stable_z_home_max_probes > 0) ? stable_z_home_max_probes : 20;
  const uint8_t window_size = (stable_z_home_window_size > 0) ? stable_z_home_window_size : 4;
  const float tol = stable_z_home_range_tolerance;
  // Capture the current homing Z (expected to be the active homed Z before probing)
  const float initial_homing_z = current_position[Z_AXIS];

  // Cap arrays to a reasonable stack size
  const uint16_t cap = _MIN(max_probes, (uint16_t)100);
  float samples[100] = { 0 };
  float pair_ranges[100]; for (uint16_t _i=0; _i<cap; ++_i) pair_ranges[_i] = NAN;
  uint8_t trig_counts[100]; for (uint16_t _ic=0; _ic<cap; ++_ic) trig_counts[_ic] = 0;
  // store up to 16 triggers per attempt
  float trig_values[100][16]; for (uint16_t _ir=0; _ir<cap; ++_ir) for (uint8_t _j=0; _j<16; ++_j) trig_values[_ir][_j] = NAN;
  uint16_t idx = 0;
  uint16_t collected = 0;

  SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: probing at X"); SERIAL_ECHO(pt.x); SERIAL_ECHOPGM(" Y"); SERIAL_ECHOLN(pt.y);

  // Move to XY at a safe Z clearance before probing
  do_blocking_move_to(xy_pos_t({ pt.x, pt.y }), MMM_TO_MMS(3000.0f));

  bool success = false;
  // Robustness: require N consecutive stable windows and keep a small history of window ranges
  const uint8_t consecutive_required = 2;
  const uint8_t range_history_size = 3;
  float range_history[range_history_size] = { 0.0f };
  uint8_t range_hist_idx = 0, range_hist_collected = 0;
  uint8_t consecutive_passes = 0;
  for (uint16_t attempt = 0; attempt < max_probes && attempt < cap; ++attempt) {
    // Probe at point; leave probe raised after each sample
    float trig_buf[16] = { 0.0f };
    uint8_t trig_count = 0;
    const float z = probe.probe_at_point(
      pt,
      ProbePtRaise::PROBE_PT_RAISE,
      0,
      true,
      true,
      /* z_min_point */ Z_PROBE_LOW_POINT,
      /* z_clearance */ Z_TWEEN_SAFE_CLEARANCE,
      /* raise_after_is_rel */ false,
      /* out_triggers */ trig_buf,
      /* out_trig_cap */ COUNT(trig_buf),
      /* out_trig_count */ &trig_count
    );
    // store this sample and associated pair-range / trigger heights in parallel arrays
    const uint16_t this_index = idx;
    samples[this_index] = z;
    idx = (idx + 1) % cap;
    if (collected < cap) ++collected;

    // Report per-call trigger range (if available)
    float this_pair_range = NAN;
    if (trig_count > 0) {
      float tmin = 1e9f, tmax = -1e9f;
      for (uint8_t ti = 0; ti < trig_count; ++ti) {
        const float tv = trig_buf[ti];
        tmin = _MIN(tmin, tv);
        tmax = _MAX(tmax, tv);
      }
      this_pair_range = tmax - tmin;
      pair_ranges[this_index] = this_pair_range;
      // store all trigger heights for this attempt
      trig_counts[this_index] = trig_count;
      for (uint8_t ti = 0; ti < 16; ++ti) {
        trig_values[this_index][ti] = (ti < trig_count) ? trig_buf[ti] : NAN;
      }

      // Print per-trigger heights and computed range for this multiprobe attempt
      SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: attempt "); SERIAL_ECHO(attempt + 1);
      SERIAL_ECHOPGM(" trigger_count="); SERIAL_ECHO(trig_count);
      SERIAL_ECHOPGM(" heights=");
      for (uint8_t ti = 0; ti < trig_count; ++ti) {
        if (ti) SERIAL_ECHOPGM(","); SERIAL_ECHO(p_float_t(trig_buf[ti], 3));
      }
      SERIAL_ECHOPGM(" range="); SERIAL_ECHO(p_float_t(this_pair_range, 3)); SERIAL_EOL();
    }

    // Early MAX_Z_SHIFT check after the first probe pair: abort immediately if configured and out-of-tolerance
    if (attempt == 0 && trig_count >= 2 && stable_z_home_max_z_shift > 0.0f) {
      // Use the maximum of the first two trigger heights and compare to the configured probe offset
      const float max_first_h = _MAX(trig_buf[0], trig_buf[1]);
      // Early tolerance: 3x the M1128 T factor per your request
      const float early_tol = 3.0f * tol;
      const float trigger_delta = fabsf(max_first_h - probe.offset.z);

      SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: early-check max_first_h="); SERIAL_ECHO(p_float_t(max_first_h, 3));
      SERIAL_ECHOPGM(" probe.offset.z="); SERIAL_ECHO(p_float_t(probe.offset.z, 3));
      SERIAL_ECHOPGM(" delta="); SERIAL_ECHO(p_float_t(trigger_delta, 3));
      SERIAL_ECHOPGM(" early_tol="); SERIAL_ECHO(p_float_t(early_tol, 3)); SERIAL_EOL();

      if (trigger_delta > early_tol) {
        SERIAL_ECHO_START(); SERIAL_ECHOLNPGM("M1128: aborting - Check the probe and Z Offset; first trigger height is outside tolerance.");
        // Return to homing height and abort
        do_blocking_move_to(xyz_pos_t({ current_position[X_AXIS], current_position[Y_AXIS], initial_homing_z }), MMM_TO_MMS(3000.0f));
        success = false;
        break;
      }
    }

    // Early derivative test: detect a downward trend in the measured trigger heights
    // that indicates drilling into a contamination blob. Use two small windows
    // (early and late) and compare their means. If the mean falls by more than
    // the allowed shift, abort early.
    if (stable_z_home_max_z_shift > 0.0f) {
      const uint8_t deriv_window = 3; // samples per side to compute means
      if (collected >= deriv_window * 2) {
        const uint16_t oldest = (idx + cap - collected) % cap;
        float mean_early = 0.0f, mean_late = 0.0f;
        for (uint8_t di = 0; di < deriv_window; ++di) {
          mean_early += samples[(oldest + di) % cap];
          mean_late  += samples[(idx + cap - deriv_window + di) % cap];
        }
        mean_early /= float(deriv_window);
        mean_late  /= float(deriv_window);
        const float delta = mean_late - mean_early; // negative -> trending downwards (closer to bed)
        const float allowed_shift = _MAX(3.0f * tol, stable_z_home_max_z_shift);

        SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: deriv-test mean_early="); SERIAL_ECHO(p_float_t(mean_early, 3));
        SERIAL_ECHOPGM(" mean_late="); SERIAL_ECHO(p_float_t(mean_late, 3));
        SERIAL_ECHOPGM(" delta="); SERIAL_ECHO(p_float_t(delta, 3));
        SERIAL_ECHOPGM(" allowed="); SERIAL_ECHO(p_float_t(allowed_shift, 3)); SERIAL_EOL();

        if (delta < -allowed_shift) {
          SERIAL_ECHO_START(); SERIAL_ECHOLNPGM("M1128: aborting - downward trend detected (possible nozzle/drip drilling). Check probe and Z Offset.");
          do_blocking_move_to(xyz_pos_t({ current_position[X_AXIS], current_position[Y_AXIS], initial_homing_z }), MMM_TO_MMS(3000.0f));
          success = false;
          break;
        }
      }
    }

    // Only evaluate window when we have enough samples
    if (collected >= window_size) {
      // Compute the current window min/max/mean
      float minv = 1e9f, maxv = -1e9f, sum = 0.0f;
      uint16_t wstart = (idx + cap - window_size) % cap;
      for (uint8_t w = 0; w < window_size; ++w) {
        const float v = samples[(wstart + w) % cap];
        if (v < minv) minv = v;
        if (v > maxv) maxv = v;
        sum += v;
      }
      const float range = maxv - minv;
      const float mean = sum / float(window_size);

      // Compute the median of the per-attempt pair ranges inside this window
      float wranges[16]; uint8_t wr_cnt = 0;
      for (uint8_t w = 0; w < window_size; ++w) {
        const float pr = pair_ranges[(wstart + w) % cap];
        if (!isnan(pr)) wranges[wr_cnt++] = pr;
      }
      float window_pair_median = NAN;
      if (wr_cnt > 0) {
        // simple sort (wr_cnt is small)
        for (uint8_t a = 0; a < wr_cnt; ++a) for (uint8_t b = a + 1; b < wr_cnt; ++b) if (wranges[b] < wranges[a]) { float t = wranges[a]; wranges[a] = wranges[b]; wranges[b] = t; }
        if (wr_cnt & 1) window_pair_median = wranges[wr_cnt/2];
        else window_pair_median = 0.5f * (wranges[wr_cnt/2 - 1] + wranges[wr_cnt/2]);
      }

      // Push the window-pair-median into small history (used for final stability check)
      range_history[range_hist_idx] = window_pair_median;
      range_hist_idx = (range_hist_idx + 1) % range_history_size;
      if (range_hist_collected < range_history_size) ++range_hist_collected;

      // DEBUG: print comparisons and history for diagnosis
      SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: compare window_z_range="); SERIAL_ECHO(p_float_t(range, 3)); SERIAL_EOL();
      SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: window_pair_median="); SERIAL_ECHO(p_float_t(window_pair_median, 3)); SERIAL_ECHOPGM(" tol="); SERIAL_ECHO(p_float_t(tol, 3)); SERIAL_EOL();
      SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: consecutive_passes(before)="); SERIAL_ECHOLN(consecutive_passes);
      SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: range_history: ");
      for (uint8_t rh = 0; rh < range_hist_collected; ++rh) {
        SERIAL_ECHOPGM(" "); SERIAL_ECHO(p_float_t(range_history[rh], 3));
      }
      SERIAL_EOL();

      if (range <= tol) {
        // Window is individually stable
        ++consecutive_passes;
        SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: stable window mean="); SERIAL_ECHO(p_float_t(mean, 3)); SERIAL_EOL();

        // If we have enough consecutive stable windows, check range-history stability
        if (consecutive_passes >= consecutive_required) {
          // Check variation of recent window medians
          float rh_min = 1e9f, rh_max = -1e9f;
          for (uint8_t r = 0; r < range_hist_collected; ++r) {
            if (isnan(range_history[r])) continue;
            rh_min = _MIN(rh_min, range_history[r]);
            rh_max = _MAX(rh_max, range_history[r]);
          }
          const float range_var = (rh_max < rh_min) ? NAN : (rh_max - rh_min);
            // DEBUG: report range-history min/max/var
            SERIAL_ECHOPGM("M1128: range_history min="); SERIAL_ECHO(p_float_t(rh_min, 3));
            SERIAL_ECHOPGM(" max="); SERIAL_ECHO(p_float_t(rh_max, 3));
            SERIAL_ECHOPGM(" var="); SERIAL_ECHO(p_float_t(range_var, 3)); SERIAL_EOL();
          if (!isnan(range_var) && range_var <= tol) {
            // All checks passed for range stability — now optionally enforce MAX_Z_SHIFT
            bool shift_ok = true;
            if (stable_z_home_max_z_shift > 0.0f) {
              // Allowed shift: be conservative and require at least 3*tolerance or the configured max shift
              const float allowed_shift = _MAX(3.0f * tol, stable_z_home_max_z_shift);
              // The probe mean is the average trigger height for the current window
              const float expect_trigger_z = initial_homing_z + probe.offset.z;
              const float trigger_delta = fabsf(mean - expect_trigger_z);

              // Diagnostic output
              SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: initial_homing_z="); SERIAL_ECHO(p_float_t(initial_homing_z, 3));
              SERIAL_ECHOPGM(" probe.offset.z="); SERIAL_ECHO(p_float_t(probe.offset.z, 3));
              SERIAL_ECHOPGM(" expect_trigger_z="); SERIAL_ECHO(p_float_t(expect_trigger_z, 3));
              SERIAL_ECHOPGM(" mean="); SERIAL_ECHO(p_float_t(mean, 3));
              SERIAL_ECHOPGM(" delta="); SERIAL_ECHO(p_float_t(trigger_delta, 3));
              SERIAL_ECHOPGM(" allowed="); SERIAL_ECHO(p_float_t(allowed_shift, 3)); SERIAL_EOL();

              if (trigger_delta > allowed_shift) {
                // Failure: abort probing, return to homing height, and alert operator
                SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: Z shift too large (delta>"); SERIAL_ECHO(p_float_t(allowed_shift, 3)); SERIAL_ECHOLNPGM(") - aborting. Check Z Offset and nozzle cleanliness.");
                // Move Z back to initial homing height
                do_blocking_move_to(xyz_pos_t({ current_position[X_AXIS], current_position[Y_AXIS], initial_homing_z }), MMM_TO_MMS(3000.0f));
                shift_ok = false;
                success = false;
                // Break out of outer loop by setting attempt to max_probes
                attempt = max_probes;
                break;
              }
            }

            if (shift_ok) {
              // All checks passed. Declare success.
              set_current_from_steppers_for_axis(Z_AXIS);
              // Keep existing homing behavior (use the stepper trigger position)
              set_axis_is_at_home(Z_AXIS);
              sync_plan_position();
              success = true;
              break;
            }
          }
          else {
            SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: range history unstable (var="); SERIAL_ECHO(range_var); SERIAL_ECHOLNPGM(") - continuing");
          }
        }
      }
      else {
        // Not stable: reset consecutive counter and report current window range
        consecutive_passes = 0;
        SERIAL_ECHO_START(); SERIAL_ECHOPGM("M1128: attempt "); SERIAL_ECHO(attempt + 1);
        SERIAL_ECHOPGM(" window_range="); SERIAL_ECHOLN(range);
      }
    }
    safe_delay(50); // small gap between probes
  }

  // Summary report: Command received, probe results, measured range, PASS/FAIL
  SERIAL_ECHO_START(); SERIAL_ECHOLNPGM("M1128 Summary:");

  // Command received line
  SERIAL_ECHO_START(); SERIAL_ECHOPGM("Command received: M1128 ");
  if (cmd_P >= 0) { SERIAL_ECHOPGM("P"); SERIAL_ECHO(cmd_P); } else SERIAL_ECHOPGM("P-");
  SERIAL_ECHOPGM(" ");
  if (cmd_W >= 0) { SERIAL_ECHOPGM("W"); SERIAL_ECHO(cmd_W); } else SERIAL_ECHOPGM("W-");
  SERIAL_ECHOPGM(" ");
  if (!isnan(cmd_T)) { SERIAL_ECHOPGM("T"); SERIAL_ECHOLN(cmd_T); } else SERIAL_ECHOLNPGM("T-");
  SERIAL_ECHO_START(); SERIAL_ECHOPGM("              X"); if (!isnan(cmd_X)) SERIAL_ECHOLN(cmd_X); else SERIAL_ECHOLNPGM("-");
  SERIAL_ECHO_START(); SERIAL_ECHOPGM("              Y"); if (!isnan(cmd_Y)) SERIAL_ECHOLN(cmd_Y); else SERIAL_ECHOLNPGM("-");

  // Probe results (print per-attempt details: sample z or per-trigger heights + range)
  SERIAL_ECHO_START(); SERIAL_ECHOPGM("Probe results:");
  if (collected == 0) {
    SERIAL_ECHOLNPGM(" <no samples collected>");
  }
  else {
    const uint16_t display_count = collected;
    const uint16_t first = (idx + cap - display_count) % cap; // oldest sample index
    for (uint16_t i = 0; i < display_count; ++i) {
      const uint16_t si = (first + i) % cap;
      const float v = samples[si];
      SERIAL_ECHO_START(); SERIAL_ECHOPGM(" "); SERIAL_ECHO(i + 1); SERIAL_ECHOPGM(": ");
      if (trig_counts[si] > 0) {
        // print all stored trigger heights and pair range
        for (uint8_t tt = 0; tt < trig_counts[si]; ++tt) {
          if (tt) SERIAL_ECHOPGM(","); SERIAL_ECHO(p_float_t(trig_values[si][tt], 3));
        }
        SERIAL_ECHOPGM(" range="); SERIAL_ECHO(p_float_t(pair_ranges[si], 3)); SERIAL_EOL();
      }
      else {
        SERIAL_ECHOLN(v);
      }
    }

    // Compute Measured Range as the median of per-attempt pair ranges inside the final sliding window (if available)
    float measured_range = NAN;
    if (collected >= window_size) {
      float wr[16]; uint8_t wrc = 0;
      const uint16_t fstart = (idx + cap - window_size) % cap;
      for (uint8_t w = 0; w < window_size; ++w) {
        const float pr = pair_ranges[(fstart + w) % cap];
        if (!isnan(pr)) wr[wrc++] = pr;
      }
      if (wrc > 0) {
        for (uint8_t a = 0; a < wrc; ++a) for (uint8_t b = a + 1; b < wrc; ++b) if (wr[b] < wr[a]) { float t = wr[a]; wr[a] = wr[b]; wr[b] = t; }
        if (wrc & 1) measured_range = wr[wrc/2];
        else measured_range = 0.5f * (wr[wrc/2 - 1] + wr[wrc/2]);
      }
    }
    // Fallback to min/max of samples if no per-pair ranges available
    if (isnan(measured_range)) {
      float minv = 1e9f, maxv = -1e9f;
      for (uint16_t i = 0; i < display_count; ++i) {
        const float v = samples[(first + i) % cap];
        if (v < minv) minv = v;
        if (v > maxv) maxv = v;
      }
      measured_range = maxv - minv;
    }

    SERIAL_ECHO_START(); SERIAL_ECHOPGM("              Measured Range = "); SERIAL_ECHO(p_float_t(measured_range, 3)); SERIAL_EOL();
  }

  // PASS / FAIL
  SERIAL_ECHO_START(); SERIAL_ECHOPGM("              "); SERIAL_ECHOLNPGM(success ? "PASS" : "FAIL");

  if (!success) {
    SERIAL_ECHO_START(); SERIAL_ECHOLNPGM("M1128: failed to find stable Z within probe limit");
  }
}

#else
void GcodeSuite::M1128() {
  SERIAL_ECHOLNPGM("M1128: STABLE_Z_HOME not enabled or no probe available");
}
#endif
