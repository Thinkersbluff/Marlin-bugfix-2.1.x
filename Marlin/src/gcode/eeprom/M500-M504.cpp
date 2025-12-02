/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../gcode.h"
#include "../../module/settings.h"
#include "../../inc/MarlinConfig.h"
#include "../../module/probe.h"

#if ENABLED(CONFIGURATION_EMBEDDING)
  #include "../../sd/cardreader.h"
  #include "../../mczip.h"
#endif

/**
 * M500: Store settings in EEPROM
 */
void GcodeSuite::M500() {
  const bool ok = settings.save();
  SERIAL_ECHO_START(); SERIAL_ECHOPGM("M500: "); SERIAL_ECHOLN(ok ? "EEPROM saved" : "EEPROM save FAILED");

  // Report which EEPROM backend is compiled in
  SERIAL_ECHO_START(); SERIAL_ECHOPGM("EEPROM backend: ");
  #if ENABLED(FLASH_EEPROM_EMULATION)
    SERIAL_ECHOLNPGM("FLASH_EEPROM_EMULATION");
  #elif ENABLED(SDCARD_EEPROM_EMULATION)
    SERIAL_ECHOLNPGM("SDCARD_EEPROM_EMULATION");
  #elif ENABLED(SRAM_EEPROM_EMULATION)
    SERIAL_ECHOLNPGM("SRAM_EEPROM_EMULATION");
  #elif ENABLED(IIC_BL24CXX_EEPROM)
    SERIAL_ECHOLNPGM("IIC_BL24CXX_EEPROM (I2C)");
  #elif ENABLED(I2C_EEPROM)
    SERIAL_ECHOLNPGM("I2C_EEPROM (generic)");
  #elif ENABLED(SPI_EEPROM)
    SERIAL_ECHOLNPGM("SPI_EEPROM");
  #elif ENABLED(QSPI_EEPROM)
    SERIAL_ECHOLNPGM("QSPI_EEPROM");
  #elif ENABLED(USE_FALLBACK_EEPROM)
    SERIAL_ECHOLNPGM("FALLBACK EEPROM (none)");
  #else
    SERIAL_ECHOLNPGM("(unknown)");
  #endif
}

/**
 * M501: Read settings from EEPROM
 */
void GcodeSuite::M501() {
  const bool ok = settings.load();
  SERIAL_ECHO_START(); SERIAL_ECHOPGM("M501: "); SERIAL_ECHOLN(ok ? "EEPROM loaded" : "EEPROM load FAILED");
}

/**
 * M502: Revert to default settings
 */
void GcodeSuite::M502() {
  (void)settings.reset();
}

#if DISABLED(DISABLE_M503)

  /**
   * M503: print settings currently in memory
   *
   *   S<bool> : Include / exclude header comments in the output. (Default: S1)
   *
   * With CONFIGURATION_EMBEDDING:
   *   C<flag> : Save the full Marlin configuration to SD Card as "mc.zip"
   */
  void GcodeSuite::M503() {
    (void)settings.report(!parser.boolval('S', true));
    
    #if ENABLED(PROBE_ACTIVATION_SWITCH)
      // Report M905 parameters with a clear labeled header
      SERIAL_ECHO_START(); SERIAL_ECHOLNPGM("; Probe Activation Calibration;");
      // Simplified, single-line M905 entry with space after the colon
      SERIAL_ECHO_START(); SERIAL_ECHOLNPGM(
        " M905 Z", p_float_t(probe.probe_en_off_height, 2),
        " M", p_float_t(MarlinSettings::get_probe_en_off_margin(), 0),
        " S", MarlinSettings::get_m905_step_settle_ms()
      );
    #endif

    #if ENABLED(CONFIGURATION_EMBEDDING)
      if (parser.seen_test('C')) {
        MediaFile file;
        // Need to create the config size on the SD card
        MediaFile root = card.getroot();
        if (file.open(&root, "mc.zip", O_WRITE|O_CREAT)) {
          bool success = true;
          for (uint16_t i = 0; success && i < sizeof(mc_zip); ++i) {
            const uint8_t c = pgm_read_byte(&mc_zip[i]);
            success = (file.write(c) == 1);
          }
          success = file.close() && success;

          if (success) SERIAL_ECHO_MSG("Configuration saved as 'mc.zip'");
        }
      }
    #endif
  }

#endif // !DISABLE_M503

#if ENABLED(EEPROM_SETTINGS)

  #if ENABLED(MARLIN_DEV_MODE)
    #include "../../libs/hex_print.h"
  #endif

  /**
   * M504: Validate EEPROM Contents
   */
  void GcodeSuite::M504() {
    #if ENABLED(MARLIN_DEV_MODE)
      const bool dowrite = parser.seenval('W');
      if (dowrite || parser.seenval('R')) {
        uint8_t val = 0;
        int addr = parser.value_ushort();
        if (dowrite) {
          val = parser.byteval('V');
          persistentStore.write_data(addr, &val);
          SERIAL_ECHOLNPGM("Wrote address ", addr, " with ", val);
        }
        else {
          if (parser.seenval('T')) {
            const int endaddr = parser.value_ushort();
            while (addr <= endaddr) {
              persistentStore.read_data(addr, &val);
              SERIAL_ECHOLNPGM("0x", hex_word(addr), ":", hex_byte(val));
              addr++;
              safe_delay(10);
            }
            SERIAL_EOL();
          }
          else {
            persistentStore.read_data(addr, &val);
            SERIAL_ECHOLNPGM("Read address ", addr, " and got ", val);
          }
        }
        return;
      }
    #endif

    if (settings.validate())
      SERIAL_ECHO_MSG("EEPROM OK");
  }

#endif
