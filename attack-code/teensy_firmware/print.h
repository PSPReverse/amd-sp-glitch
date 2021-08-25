// Copyright (C) 2021 Niklas Jacob
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef PRINT_H
#define PRINT_H

#include <core_pins.h>
#include <usb_serial.h>

inline void print_init() {
    Serial.begin(115200);
    delay(1500);
}

inline void print_byte(char c) {
    Serial.write(c);
}

inline void print_str(const char * str) {
    Serial.write(str);
}

inline void print_str(const char * str, int n) {
    Serial.write(str, n);
}

inline void println(const char * str) {
    Serial.println(str);
}

inline void println() {
    Serial.println();
}

inline void print_bool(bool b) {
    if (b) print_str("true");
    else print_str("false");
}

inline char get_hex_nibble(uint8_t b) {
    b &= 0xf;
    return b>9 ? (b-10)+'a' : b+'0';
}

inline void print_hex_nibble(uint8_t b) {
    char str[] = "0x0";
    str[2] = get_hex_nibble(b);
    print_str(str, 3);
}

inline void print_hex_byte(uint8_t b) {
    char str[] = "0x00";
    str[3] = get_hex_nibble(b);
    str[2] = get_hex_nibble(b>>4);
    print_str(str, 4);
}

inline void print_hex_short(short s) {
    char str[] = "0x0000";
    for (uint8_t i = 0; i < 4; i++) {
        str[5-i] = get_hex_nibble((uint8_t) s);
        s >>= 4;
    }
    print_str(str, 6);
}

inline void print_hex_int(int i) {
    char str[] = "0x00000000";
    for (uint8_t j = 0; j < 8; j++) {
        str[9-j] = get_hex_nibble((uint8_t) i);
        i >>= 4;
    }
    print_str(str, 10);
}

#define print_param(NAME, VALUE, TYPE) \
    print_str(NAME); print_str(" = "); \
    print_ ## TYPE (VALUE); \
    println()

#define print_hex_param(NAME, VALUE, TYPE) \
    print_param(NAME, VALUE, hex_ ## TYPE)

#define print_value(VALUE, TYPE) \
    print_param(#VALUE, VALUE, TYPE)

#define print_hex_value(VALUE, TYPE) \
    print_hex_param(#VALUE, VALUE, TYPE)

#define print_struct_begin(NAME) \
    print_str(NAME); println(" = {")

#define print_struct_member(OBJ, MEMBER, TYPE) \
    print_str("    "); \
    print_param(#MEMBER, OBJ . MEMBER, TYPE)

#define print_struct_hex_member(OBJ, MEMBER, TYPE) \
    print_str("    "); \
    print_hex_param(#MEMBER, OBJ . MEMBER, TYPE)

#define print_struct_end() \
    println("}")

#define print_enum_begin(VALUE) switch (VALUE) {
#define print_enum_member(MEMBER)   case MEMBER: \
                                        print_str(#MEMBER); \
                                        break;
#define print_enum_end()            default: \
                                        print_str("Unknown"); \
                                }

#endif /* PRINT_H */
