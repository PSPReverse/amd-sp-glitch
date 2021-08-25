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

#ifndef AMD_SVI2_HPP
#define AMD_SVI2_HPP

#include "io.h"

#include "teensy_pins.hpp"
#include "teensy_twi.hpp"

using namespace Teensy;

namespace AmdSvi2 {

struct Command;

struct CommandRaw {
    uint16_t data;
    uint8_t address;

    // returns zero on success
    int receive(Twi::Slave &slave, uint32_t timeout) {
        address = 0;
        data = 0;

        int rc = slave.recv(address, (uint8_t*) &data, 2, timeout);

        address >>= 1;

        if (rc != 2) return rc;
        return 0;
    }

    int send(Twi::Master &master, uint32_t timeout) {
        return master.send_u16(address, data, timeout);
    }

    void print(const char* name = "CommandRaw") const {
        print_struct_begin(name);
        print_struct_member((*this), address, hex_byte);
        print_struct_member((*this), data, hex_short);
        print_struct_end();
    }

    Command to_cmd() const;
    static CommandRaw from_cmd(Command cmd);
};

#define MAKE_ENUM_PRINT_MEMBER(VAR, STR) \
        case VAR: \
            print_str(#STR); \
            break;
#define MAKE_ENUM_PRINT_FN_DECL(NAME) \
void NAME (uint8_t v);
#define MAKE_ENUM_PRINT_FN(NAME, VALUES) \
void NAME (uint8_t v) { \
    switch (v) { \
        VALUES(MAKE_ENUM_PRINT_MEMBER) \
        default: \
            print_str("Unknown"); \
    } \
}

#define MAKE_ENUM_PARSE_MEMBER(VAR, STR) \
    if (str_cmp(s, n, #STR, sizeof(#STR)) == 0) { \
        v = VAR; \
        return true; \
    }

#define MAKE_ENUM_PARSE_FN_DECL(NAME) \
bool NAME (uint8_t &v, const char* s, unsigned n);
#define MAKE_ENUM_PARSE_FN(NAME, VALUES) \
bool NAME (uint8_t &v, const char* s, unsigned n) { \
    VALUES(MAKE_ENUM_PARSE_MEMBER) \
    return false; \
}

#define MAKE_ENUM_PRINT_OPTION(VAR, STR) \
    println("    " #STR);
#define MAKE_ENUM_PRINT_OPTIONS_FN_DECL(NAME) \
void NAME ();
#define MAKE_ENUM_PRINT_OPTIONS_FN(NAME, VALUES) \
void NAME () { \
    VALUES(MAKE_ENUM_PRINT_OPTION) \
}

#define MAKE_ENUM_FN_DECLS(NAME) \
MAKE_ENUM_PRINT_FN_DECL(Print ## NAME) \
MAKE_ENUM_PRINT_OPTIONS_FN_DECL(Print ## NAME ## Options) \
MAKE_ENUM_PARSE_FN_DECL(Parse ## NAME)

#ifdef MAKE_ENUM_DEF_FNS

#define MAKE_ENUM_FN_DEFS(NAME, VALUES) \
MAKE_ENUM_PRINT_FN(Print ## NAME, VALUES) \
MAKE_ENUM_PRINT_OPTIONS_FN(Print ## NAME ## Options, VALUES) \
MAKE_ENUM_PARSE_FN(Parse ## NAME, VALUES)

#else

#define MAKE_ENUM_FN_DEFS(NAME, VALUES)

#endif

#define MAKE_ENUM_MEMBER(VAR, STR) \
    VAR,
#define MAKE_ENUM(NAME, VALUES) \
enum NAME : uint8_t { \
    VALUES(MAKE_ENUM_MEMBER) \
}; \
MAKE_ENUM_FN_DECLS(NAME) \
MAKE_ENUM_FN_DEFS(NAME, VALUES)

#define OFFSETS(X) \
    X(OffsetOff, off) \
    X(OffsetSub25mV, -25mV) \
    X(OffsetNoChange, no_change) \
    X(OffsetAdd25mV, +25mV)

MAKE_ENUM(OffsetTrim, OFFSETS)

#define LOADLINES(X) \
    X(LoadLineOff,      off) \
    X(LoadLineSub40P,   -40%) \
    X(LoadLineSub20P,   -20%) \
    X(LoadLineNoChange, no_change) \
    X(LoadLineAdd20P,   +20%) \
    X(LoadLineAdd40P,   +40%) \
    X(LoadLineAdd60P,   +60%) \
    X(LoadLineAdd80P,   +80%)

MAKE_ENUM(LoadLineSlopeTrim, LOADLINES)

#define POWERLEVELS(X) \
    X(PowerLow,     low) \
    X(PowerMid,     mid) \
    X(PowerFullAlt, full_alt) \
    X(PowerFull,    full)

MAKE_ENUM(PowerLevel, POWERLEVELS)

struct Command {
    unsigned int offset_trim : 2;
    unsigned int ll_slope_trim : 3;
    unsigned int tfn : 1;
    unsigned int psi1_l : 1;
    unsigned int vid_code : 8;
    unsigned int psi0_l : 1;

    unsigned int soc : 1;
    unsigned int core : 1;
    unsigned int constant : 5;

    constexpr Command() :
        offset_trim(OffsetNoChange),
        ll_slope_trim(LoadLineNoChange),
        tfn(0),         // don't change telemetry setting
        psi1_l(1),      // full power mode
        vid_code(0xff), // turn off voltage
        psi0_l(1),      // full power mode
        soc(0), core(0),// set neither voltage
        constant(0b11000)
    {}

    constexpr Command Core(bool c = true) const { auto r = *this; r.core = c; return r; }
    constexpr Command Soc(bool s = true) const { auto r = *this; r.soc = s; return r; }
    constexpr Command Vid(uint8_t v) const { auto r = *this; r.vid_code = v; return r; }

    constexpr Command Telemetry(bool t = true) const { auto r = *this; r.tfn = t; return r; }
    constexpr Command PowerLevel(uint8_t p) const {
        auto r = *this; r.psi0_l = p; r.psi1_l = p>>1; return r;
    }
    constexpr Command LoadLine(uint8_t l) const {
        auto r = *this; r.ll_slope_trim = l; return r;
    }
    constexpr Command Offset(uint8_t o) const {
        auto r = *this; r.offset_trim = o; return r;
    }

    //void print() const;

    static Command from_raw(CommandRaw raw) {
        union { CommandRaw raw; Command cmd; } u = { .raw = raw };
        u.raw.data = (u.raw.data << 8) | (u.raw.data >> 8);
        return u.cmd;
    }

    CommandRaw to_raw() {
        constant = 0b11000;
        union { CommandRaw raw; Command cmd; } u = { .cmd = *this };
        u.raw.data = (u.raw.data << 8) | (u.raw.data >> 8);
        return u.raw;
    }

    int send(Twi::Master &master, uint32_t timeout) { return to_raw().send(master, timeout); }
    int receive(Twi::Slave &slave, uint32_t timeout) {
        CommandRaw raw;
        int rc = raw.receive(slave, timeout);
        *this = raw.to_cmd();
        return rc;
    }
};

inline Command CommandRaw::to_cmd() const { return Command::from_raw(*this); }
inline CommandRaw CommandRaw::from_cmd(Command cmd) { return cmd.to_raw(); }

} /* namespace AmdSvi2 */

#endif /* AMD_SVI2_HPP */
