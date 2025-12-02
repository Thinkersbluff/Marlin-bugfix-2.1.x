/**
 * M526 - EEPROM Offsets mapping (debug)
 *
 * Prints EEPROM_OFFSETOF(...) for key SettingsData fields so
 * on-device index/address can be mapped to the source field.
 */

#include "../gcode.h"
#include "../../module/settings.h"

#if ENABLED(EEPROM_SETTINGS)

void GcodeSuite::M526() {
  MarlinSettings::print_offsets();
}

#endif // EEPROM_SETTINGS
