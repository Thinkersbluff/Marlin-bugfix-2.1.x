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
#pragma once

#include <stdint.h>

//
// Utility functions to create and print hex strings as nybble, byte, and word.
//

constexpr char hex_nybble(const uint8_t n) {
  return (n & 0xF) + ((n & 0xF) < 10 ? '0' : 'A' - 10);
}
// Declarations and (optional) fallback implementations for hex helpers.
#if defined(NEED_HEX_PRINT) && NEED_HEX_PRINT
  char* _hex_word(const uint16_t w);
  char* _hex_long(const uint32_t l);

  char* hex_byte(const uint8_t b);
  template<typename T> char* hex_word(T w) { return _hex_word((uint16_t)w); }
  template<typename T> char* hex_long(T w) { return _hex_long((uint32_t)w); }

  char* hex_address(const void * const w);

  void print_hex_nybble(const uint8_t n);
  void print_hex_byte(const uint8_t b);
  void print_hex_word(const uint16_t w);
  void print_hex_address(const void * const w);
  void print_hex_long(const uint32_t w, const char delimiter='\0', const bool prefix=false);

#else
  // Provide lightweight inline implementations when NEED_HEX_PRINT is not set
  // to avoid link-time undefined references.
  static inline void __hex_byte_inline(char *buf, const uint8_t b, const uint8_t o=0) {
    buf[o + 0] = hex_nybble(b >> 4);
    buf[o + 1] = hex_nybble(b);
  }
  static inline void __hex_word_inline(char *buf, const uint16_t w, const uint8_t o=0) {
    __hex_byte_inline(buf, (uint8_t)(w >> 8), o + 0);
    __hex_byte_inline(buf, (uint8_t)w, o + 2);
  }
  static inline void __hex_long_inline(char *buf, const uint32_t l) {
    __hex_word_inline(buf, (uint16_t)(l >> 16), 0);
    __hex_word_inline(buf, (uint16_t)l, 4);
  }

  static inline char* _hex_word(const uint16_t w) {
    static char _hexbuf[9] = "00000000"; // no 0x prefix here
    __hex_word_inline(_hexbuf, w, 0);
    _hexbuf[8] = '\0';
    return _hexbuf;
  }
  static inline char* _hex_long(const uint32_t l) {
    static char _hexbuf[9] = "00000000";
    __hex_long_inline(_hexbuf, l);
    _hexbuf[8] = '\0';
    return _hexbuf;
  }

  static inline char* hex_byte(const uint8_t b) {
    static char _hexbuf[3] = "00";
    __hex_byte_inline(_hexbuf, b, 0);
    _hexbuf[2] = '\0';
    return _hexbuf;
  }

  template<typename T> static inline char* hex_word(T w) { return _hex_word((uint16_t)w); }
  template<typename T> static inline char* hex_long(T w) { return _hex_long((uint32_t)w); }

  static inline char* hex_address(const void * const a) {
    #ifdef CPU_32_BIT
      return _hex_long((uintptr_t)a);
    #else
      return _hex_word((uint16_t)(uintptr_t)a);
    #endif
  }

  static inline void print_hex_nybble(const uint8_t n)       { SERIAL_CHAR(hex_nybble(n));  }
  static inline void print_hex_byte(const uint8_t b)         { SERIAL_ECHO(hex_byte(b));    }
  static inline void print_hex_word(const uint16_t w)        { SERIAL_ECHO(_hex_word(w));   }
  static inline void print_hex_address(const void * const a) { SERIAL_ECHO(hex_address(a)); }
  static inline void print_hex_long(const uint32_t w, const char delimiter/*='\0'*/, const bool prefix/*=false*/) { SERIAL_ECHO(_hex_long(w)); }

#endif
