/**
 * M525 - EEPROM raw dump (temporary debug command)
 *
 *  M525 S<length>    ; dump <length> bytes starting at EEPROM base (default 256)
 *  M525              ; dump 256 bytes
 */

#include "../gcode.h"
#include "../../libs/hex_print.h"
#include "../../module/settings.h"

#if ENABLED(EEPROM_SETTINGS)

/**
 * M525: Raw EEPROM dump (temporary debug command)
 */
void GcodeSuite::M525() {
  // Default length to dump
  // Optional: P<offset> to start at offset (relative to EEPROM_OFFSET)
  uint32_t start = 0;
  if (parser.seen('P')) start = parser.value_ulong();

  // Length S<length> (default 256). Check explicitly for S to avoid
  // accidentally reading the last-seen parameter value (parser.value_*()
  // reads whatever value_ptr currently points to).
  uint16_t len = 256;
  if (parser.seen('S')) len = parser.value_ushort();

  // Ensure we don't read past capacity
  const uint32_t cap = persistentStore.capacity();
  uint32_t base = MarlinSettings::eeprom_offset();
  uint32_t addr = base + start;
  if (addr >= cap) {
    SERIAL_ECHOLNPGM("Start address is beyond EEPROM capacity");
    return;
  }
  if (base >= cap) {
    SERIAL_ECHOLNPGM("EEPROM base offset is beyond capacity");
    return;
  }

  if (addr + len > cap) len = cap - addr;

  SERIAL_ECHOPGM("EEPROM dump from 0x"); print_hex_long(addr, '\0', false); SERIAL_ECHOPGM(" length="); SERIAL_ECHOLN((int)len);

  uint8_t val;
  for (uint32_t i = 0; i < len; ++i) {
    const uint32_t a = addr + i;
    if (persistentStore.read_data(a, &val)) {
      SERIAL_ECHOPGM("EEPROM read error at address 0x"); print_hex_long(a, '\0', false); SERIAL_EOL();
      SERIAL_ECHOPGM("  index="); SERIAL_ECHOLN((int)i);
      SERIAL_ECHOPGM("  requested end address=0x"); print_hex_long(addr + len - 1, '\0', false); SERIAL_EOL();
      SERIAL_ECHOLNPGM("  EEPROM capacity="); SERIAL_ECHOLN((int)cap);
      return;
    }
    if ((i & 0x0F) == 0) {
      if (i) SERIAL_EOL();
      SERIAL_ECHOPGM("0x"); SERIAL_ECHOPGM(hex_word(addr + i)); SERIAL_ECHOPGM(":");
    }
    SERIAL_ECHOPGM(" "); SERIAL_ECHOPGM(hex_byte(val));
  }
  SERIAL_EOL();
}

#endif // EEPROM_SETTINGS
