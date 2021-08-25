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

#include "io.h"

#include <core_pins.h>
#include <usb_serial.h>

void io_init() {
    Serial.begin(115200);
    delay(1500);
}



// INPUT

bool has_available() { return Serial.available(); }
char get_char() { return Serial.read(); }



// OUTPUT

void print_char(char c) { Serial.write(c); }

void print_str(const char * str) { Serial.write(str); }

void print_str(const char * str, int n) { Serial.write(str, n); }

void println(const char * str) { Serial.println(str); }

void println() { Serial.println(); }

void print_with_indent(unsigned indent, const char *s) {
    while (*s) {
        print_char(*s);
        if (*s == '\n')
            for (unsigned i = 0; i < indent; i++)
                print_char(' ');
        s++;
    }
}


// STRINGS

bool is_printable_ascii(char c) {
    return ' ' <= c && c <= '~';
}

unsigned str_len(const char * s) {
    unsigned i = 0;
    while (s[i] != 0) i++;
    return i;
}

unsigned str_len(const char * s, unsigned n) {
    for (unsigned i = 0; i < n; i++)
        if (s[i] == 0)
            return i;
    return n;
}

char str_cmp(const char * a, const char * b, unsigned n) {
    for (unsigned i = 0; i < n; i++) {
        auto c = a[i] - b[i];
        if (c) return c;
        if (a[i] == 0) return 0;
    }
    return 0;
}

char str_cmp(const char * a, unsigned n_a, const char * b, unsigned n_b) {
    char res = str_cmp(a, b, n_a <= n_b ? n_a : n_b);
    if (res == 0) {
        if (n_a < n_b) return -b[n_a];
        if (n_a > n_b) return a[n_b];
    }
    return res;
}

bool is_whitespace(char c) {
    switch (c) {
        case ' ':
        case '\t':
        case '\v':
        case '\r':
        case '\n':
            return true;
        default:
            return false;
    }
}

unsigned get_word(char * &w, char * &s, unsigned &n, char split) {
    w = s;
    unsigned len = 0;
    while (n > 0) {
        char c = *s;
        if (c == split)
            *s = 0;
        s++;
        n--;
        len++;
        if (c == split)
            break;
    }
    strip_start(s, n, split);
    return len;
}

void strip_start(char * &s, unsigned &n, char to_strip) {
    while (n && *s == to_strip) {
        s++;
        n--;
    }
}

void strip_end(char * s, unsigned &n, char to_strip) {
    n = str_len(s, n);
    while (n && s[n-1] == to_strip) {
        n--;
        s[n] = 0;
    }
}

unsigned skip_ws(const char * &s, unsigned &n) {
    unsigned count = 0;
    while (count < n && is_whitespace(s[count]))
        count++;
    s += count;
    n -= count;
    return count;
}

bool add_digit_bin(unsigned &v, char c) {
    if (c == '0') { v <<= 1; return true; }
    if (c == '1') { v <<= 1; v |= 1; return true; }
    return false;
}

bool add_digit_dec(unsigned &v, char c) {
    if ('0' <= c && c <= '9') {
        v *= 10;
        v += c - '0';
        return true;
    }
    return false;
}

bool add_digit_hex(unsigned &v, char c) {
    if ('0' <= c && c <= '9') {
        v <<= 4;
        v |= c - '0';
        return true;
    }
    if ('a' <= c && c <= 'f') {
        v <<= 4;
        v |= 10 + c - 'a';
        return true;
    }
    if ('A' <= c && c <= 'F') {
        v <<= 4;
        v |= 10 + c - 'A';
        return true;
    }
    return false;
}

unsigned stou_loop(bool (&add_digit) (unsigned&, char), unsigned &v, const char *s, unsigned n) {
    unsigned count;
    for (count = 0; count < n; count++) {
        if (!add_digit(v, s[count]))
            break;
    }
    return count;
}

unsigned stou(unsigned &v, const char * &s, unsigned &n) {
    v = 0;
    unsigned count = 0;
    if (n > 2 && str_cmp(s, "0x", 2) == 0)
        count = stou_loop(add_digit_hex, v, s+2, n-2) + 2;
    else if (n > 2 && str_cmp(s, "0b", 2) == 0)
        count = stou_loop(add_digit_bin, v, s+2, n-2) + 2;
    else
        count = stou_loop(add_digit_dec, v, s, n);
    if (count == n || s[count] == 0 || is_whitespace(s[count])) {
        s += count;
        n -= count;
        return count;
    }
    v = 0;
    return 0;
}
