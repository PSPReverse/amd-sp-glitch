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

#ifndef IO_H
#define IO_H

#include <stdint.h>

void io_init();



// INPUT

bool has_available();
char get_char();


// OUTPUT

void print_char(char c);

void print_str(const char * str);
void print_str(const char * str, int n);

void println(const char * str);
void println();

void print_with_indent(unsigned indent, const char *s);


inline void clear_screen() {
    print_str("\x1b" "[2J");
}

inline void print_bool(bool b) { print_str(b ? "true" : "false"); }

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



// STRINGS


// length of a string (until a null byte or n)
unsigned str_len(const char * s);
unsigned str_len(const char * s, unsigned n);

// compare two strings of length at most n
// if they are equal up to n they are considered equal
char str_cmp(const char * a, const char * b, unsigned n);
char str_cmp(const char * a, unsigned n_a, const char * b, unsigned n_b);

// space, tabs, linebreaks, etc.
bool is_whitespace(char c);

//
bool is_printable_ascii(char c);

// splits string into two pieces at the specified character
// replaces the split character with a null byte
// s points to the rest of the string, which has n bytes left
// and whose start was striped of any split characters
// w points to the word split of of the original string
// the string pointed to by w is exactly as long as the returned number (including null byte)
// if the end of the string is reached n will be zero, s point to the next byte
// after the string and the original value of n iwll be returned
unsigned get_word(char * &w, char * &s, unsigned &n, char split = ' ');

void strip_start(char * &s, unsigned &n, char to_strip = ' ');
void strip_end(char * s, unsigned &n, char to_strip = ' ');

// skip whitespaces -> count
unsigned skip_ws(const char * &s, unsigned &n);

// parse an unsigned int from string -> count
unsigned stou(unsigned &v, const char * &s, unsigned &n);

#endif /* IO_H */
