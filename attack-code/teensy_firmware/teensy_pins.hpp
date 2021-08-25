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

#ifndef TEENSY_PINS_HPP
#define TEENSY_PINS_HPP

#include <imxrt.h>

/*
  [1]:  i.MX RT1060 Processor ReferenceManual
        https://www.pjrc.com/teensy/IMXRT1060RM_rev2.pdf
*/

namespace Teensy {

namespace Pad {

struct Registers {
    volatile uint32_t mux;  // offset 0x000
    uint32_t _unused[0x1ec/4];// offset 0x004
    volatile uint32_t ctl;  // offset 0x1f0
};

enum on_off : uint8_t { off, on, };

enum slew_rate : uint8_t { slow, fast, };

enum drive_strength : uint8_t {
    /*off,*/ ohm_150 = 1, ohm_75, ohm_50, ohm_38, ohm_30, ohm_25, ohm_21,
};

enum speed : uint8_t {
    low_50mhz, medium_100mhz, fast_150mhz, max_200mhz,
};

enum passive : uint8_t {
    //off             = 0b0000; same as in on_off
    keeper          = 0b0001,
    pulldown_100k   = 0b0011,
    pullup_22k      = 0b1111,
    pullup_47k      = 0b0111,
    pullup_100k     = 0b1011,
};

enum mux_mode : uint8_t {
    alt0, alt1, alt2, alt3, alt4, gpio, alt5, alt6, alt7, alt8, alt9,
};

struct Config {
    unsigned int slew_rate : 1;
    unsigned int _resv0 : 2;
    unsigned int drive_strength : 3;
    unsigned int speed : 2;
    unsigned int _resv1 : 3;
    unsigned int open_drain : 1;
    unsigned int passive : 4;
    unsigned int hysteresis : 1;
    unsigned int _resv2 : 7;
    unsigned int mux_mode : 4;
    unsigned int input : 1;

    static constexpr Config from_u32(uint32_t v) {
        //union { uint32_t v; Config c; } u = { .v = v };
        //return u.c;
        return {
            .slew_rate = v,
            ._resv0 = 0,
            .drive_strength = (v >> 3),
            .speed = (v >> 6),
            ._resv1 = 0,
            .open_drain = (v >> 11),
            .passive = (v >> 12),
            .hysteresis = (v >> 16),
            ._resv2 = 0,
            .mux_mode = (v >> 24),
            .input = (v >> 28),
        };
    }
    constexpr uint32_t to_u32() const {
        //union { uint32_t v; Config c; } u = { .c = *this };
        //return u.v;
        return slew_rate
            | (drive_strength << 3)
            | (speed << 6)
            | (open_drain << 11)
            | (passive << 12)
            | (hysteresis << 16)
            | (mux_mode << 24)
            | (input << 28);
    }

    constexpr Config Fastest() const {
        auto r = *this; r.speed = max_200mhz; r.slew_rate = fast; return r; }
    constexpr Config Input() const {
        auto r = *this; r.input = on; return r; }
    constexpr Config PullUp() const {
        auto r = *this; r.passive = pullup_22k; return r; }
    constexpr Config Output() const {
        auto r = *this; r.drive_strength = ohm_21; return r; }
    constexpr Config OpenDrain() const {
        auto r = *this; r.open_drain = on; return r; }
    constexpr Config Mux(uint8_t mux) const {
        auto r = *this; r.mux_mode = mux; return r; }
    constexpr Config Gpio() const {
        auto r = *this; r.mux_mode = gpio; return r; }
};

static_assert(sizeof(Config) == 4, "We want only one word for this config type!");

static_assert(Config::from_u32(0x0501b028).slew_rate == off, "Quick bit-field test!");
static_assert(Config::from_u32(0x0501b028).drive_strength == ohm_30, "Quick bit-field test!");
static_assert(Config::from_u32(0x0501b028).speed == low_50mhz, "Quick bit-field test!");
static_assert(Config::from_u32(0x0501b028).open_drain == off, "Quick bit-field test!");
static_assert(Config::from_u32(0x0501b028).passive == pullup_100k, "Quick bit-field test!");
static_assert(Config::from_u32(0x0501b028).hysteresis == on, "Quick bit-field test!");
static_assert(Config::from_u32(0x0501b028).mux_mode == gpio, "Quick bit-field test!");
static_assert(Config::from_u32(0x0501b028).input == off, "Quick bit-field test!");
static_assert(Config::from_u32(0xffffffff).to_u32() == 0x1f01f8f9, "Quick bit-field test!");

struct Hardware {
    Registers *regs;

    Config read() const { return Config::from_u32((regs->mux << 24) | regs->ctl); }
    Config write(Config c) const {
        uint32_t u = c.to_u32();
        c = read();
        regs->ctl = 0x1f8f9 & u;
        regs->mux = u >> 24;
        return c;
    }
};

struct SetupScopeGuard;

struct Setup {
    Hardware hw;
    Config cfg;

    Setup apply() const { return { hw, hw.write(cfg) }; }
    SetupScopeGuard apply_temporarily() const;
};

struct SetupScopeGuard {
private:
    friend Setup;
    const Setup original_setup;
    SetupScopeGuard(Setup to_apply) : original_setup(to_apply.apply()) {}
public:
    ~SetupScopeGuard() { original_setup.apply(); }

    SetupScopeGuard(const SetupScopeGuard&) = delete;
    SetupScopeGuard& operator=(const SetupScopeGuard&) = delete;

    constexpr SetupScopeGuard(SetupScopeGuard&&) = default;
    SetupScopeGuard& operator=(SetupScopeGuard&&) = default;
};

inline SetupScopeGuard Setup::apply_temporarily() const {
    return *this;
}

inline SetupScopeGuard TemporarySetup(Setup setup) {
    return setup.apply_temporarily();
}

} /* namespace Pad */

namespace Gpio {

// [1] pages 961-962
struct Registers {
    volatile uint32_t DR;           // offset 0x00
    volatile uint32_t GDIR;         // offset 0x04
    volatile uint32_t PSR;          // offset 0x08
    volatile uint32_t ICR1;         // offset 0x0c
    volatile uint32_t ICR2;         // offset 0x10
    volatile uint32_t IMR;          // offset 0xr4
    volatile uint32_t ISR;          // offset 0x18
    volatile uint32_t EDGE_SEL;     // offset 0x1c
    uint32_t          _unused[0x64/4];// offset 0x20
    volatile uint32_t DR_SET;       // offset 0x84
    volatile uint32_t DR_CLEAR;     // offset 0x88
    volatile uint32_t DR_TOGGLE;    // offset 0x8c

    // [1] pages 961-962
    static constexpr
    Registers*  from_bank(uint8_t bank) {
        if (bank > 4) return (Registers*) 0;
        return (Registers*) (0x42000000 + 0x4000 * bank);
    }
};

// TODO: config only contains the direction, no other state
struct Config {
    bool output = false;
    Pad::Config pad = Pad::Config().Gpio();

    static constexpr Config MakeInput(Pad::Config c = Pad::Config().Fastest()) {
        return { false, c.Gpio().Input() };
    }
    static constexpr Config MakeOutput(Pad::Config c = Pad::Config().Fastest()) {
        return { true, c.Gpio().Output() };
    }

    constexpr Config Input() const { return { false, pad.Input() }; }
    constexpr Config Output() const { return { true, pad.Output() }; }

    constexpr Config Fastest() const { return { output, pad.Fastest() }; }
    constexpr Config PullUp() const { return { output, pad.PullUp() }; }
    constexpr Config OpenDrain() const { return { output, pad.OpenDrain() }; }
};

struct Setup;

struct Hardware {
    Pad::Hardware       pad;
    Registers           *regs;
    uint32_t            mask;

    constexpr Hardware(Pad::Hardware pad, uint8_t bank, uint8_t bit)
      : pad(pad), regs(Registers::from_bank(bank)), mask(1<<bit) {}

    constexpr Setup Input(Pad::Config c = Pad::Config().Fastest()) const;
    constexpr Setup Output(Pad::Config c = Pad::Config().Fastest()) const;

    Config read() const {
        return { get_dir(), pad.read() };
    }

    Config write(Config c) const {
        bool o_dir = get_dir();
        if (c.output) regs->GDIR |= mask;
        else regs->GDIR &= ~mask;
        return  { o_dir, pad.write(c.pad.Gpio()) };
    }

    void set() { regs->DR_SET = mask; }
    void clear() { regs->DR_CLEAR = mask; }
    void toggle() { regs->DR_TOGGLE = mask; }

    void set_high() { set(); }
    void set_low() { clear(); }

    bool get() const { return (regs->PSR & mask) != 0; }
    bool get_output() const { return (regs->DR & mask) != 0; }
    bool get_dir() const { return (regs->GDIR & mask) != 0; }

    bool is_high() const { return get(); }
    bool is_low() const { return !get(); }
};

struct SetupScopeGuard;

struct Setup {
    Hardware hw;
    Config cfg;

    Setup apply() const { return { hw, hw.write(cfg) }; }
    SetupScopeGuard apply_temporarily() const;
};

constexpr Setup Hardware::Input(Pad::Config c) const {
    return { *this, Config::MakeInput(c) };
}
constexpr Setup Hardware::Output(Pad::Config c) const {
    return { *this, Config::MakeOutput(c) };
}

struct SetupScopeGuard {
private:
    friend Setup;
    const Setup original_setup;
    SetupScopeGuard(Setup to_apply) : original_setup(to_apply.apply()) {}
public:
    ~SetupScopeGuard() { original_setup.apply(); }

    SetupScopeGuard(const SetupScopeGuard&) = delete;
    SetupScopeGuard& operator=(const SetupScopeGuard&) = delete;

    constexpr SetupScopeGuard(SetupScopeGuard&&) = default;
    SetupScopeGuard& operator=(SetupScopeGuard&&) = default;
};

inline SetupScopeGuard Setup::apply_temporarily() const {
    return *this;
}

inline SetupScopeGuard TemporarySetup(Setup setup) {
    return setup.apply_temporarily();
}

} /* namespace Gpio */

#define MAKE_PAD(NR, NAME, BANK, BIT) \
constexpr Pad::Hardware Pad ## NR  = { (Pad::Registers*) &IOMUXC_SW_MUX_CTL_PAD_GPIO_ ## NAME }; \
constexpr Gpio::Hardware Gpio ## NR () { return Gpio::Hardware(Pad ## NR, BANK, BIT); }

MAKE_PAD(   0,  AD_B0_03,   0,  3   );
MAKE_PAD(   1,  AD_B0_02,   0,  2   );
MAKE_PAD(   2,  EMC_04,     3,  4   );
MAKE_PAD(   3,  EMC_05,     3,  5   );
MAKE_PAD(   4,  EMC_06,     3,  6   );
MAKE_PAD(   5,  EMC_08,     3,  8   );
MAKE_PAD(   6,  B0_10,      1,  10  );
MAKE_PAD(   7,  B1_01,      1,  17  );
MAKE_PAD(   8,  B1_00,      1,  16  );
MAKE_PAD(   9,  B0_11,      1,  11  );
MAKE_PAD(   10, B0_00,      1,  0   );
MAKE_PAD(   11, B0_02,      1,  2   );
MAKE_PAD(   12, B0_01,      1,  1   );
MAKE_PAD(   13, B0_03,      1,  3   );
MAKE_PAD(   14, AD_B1_02,   0,  18  );
MAKE_PAD(   15, AD_B1_03,   0,  19  );
MAKE_PAD(   16, AD_B1_07,   0,  23  );
MAKE_PAD(   17, AD_B1_06,   0,  22  );
MAKE_PAD(   18, AD_B1_01,   0,  17  );
MAKE_PAD(   19, AD_B1_00,   0,  16  );
MAKE_PAD(   20, AD_B1_10,   0,  26  );
MAKE_PAD(   21, AD_B1_11,   0,  27  );
MAKE_PAD(   22, AD_B1_08,   0,  24  );
MAKE_PAD(   23, AD_B1_09,   0,  25  );
MAKE_PAD(   24, AD_B1_12,   0,  28  );
MAKE_PAD(   25, AD_B1_13,   0,  29  );
MAKE_PAD(   26, AD_B1_14,   0,  30  );
MAKE_PAD(   27, AD_B1_15,   0,  31  );
MAKE_PAD(   28, EMC_32,     2,  18  );
MAKE_PAD(   29, EMC_31,     3,  31  );
MAKE_PAD(   30, EMC_37,     2,  23  );
MAKE_PAD(   31, EMC_36,     2,  22  );
MAKE_PAD(   32, B0_12,      1,  12  );
MAKE_PAD(   33, EMC_07,     3,  7   );
MAKE_PAD(   34, SD_B0_03,   3,  15  );
MAKE_PAD(   35, SD_B0_02,   3,  14  );
MAKE_PAD(   36, SD_B0_01,   3,  13  );
MAKE_PAD(   37, SD_B0_00,   3,  12  );
MAKE_PAD(   38, SD_B0_05,   3,  17  );
MAKE_PAD(   39, SD_B0_04,   3,  16  );

namespace ModuleInput {

struct Config {
    Pad::Config pad;
    uint32_t    daisy = 0;

    constexpr Config Daisy(uint32_t daisy) const { return { pad, daisy }; }
    constexpr Config Fastest() const { return { pad.Fastest(), daisy }; }
    constexpr Config Input() const { return { pad.Input(), daisy }; }
    constexpr Config PullUp() const { return { pad.PullUp(), daisy }; }
    constexpr Config Output() const { return { pad.Output(), daisy }; }
    constexpr Config OpenDrain() const { return { pad.OpenDrain(), daisy }; }
    constexpr Config Mux(uint8_t mux) const { return { pad.Mux(mux), daisy }; }
};

struct Hardware {
    Pad::Hardware       pad;
    volatile uint32_t   *input_select;

    Config read() const {
        return { pad.read(), *input_select };
    }

    Config write(Config c) const {
        bool o_daisy = *input_select;
        *input_select = c.daisy;
        return  { pad.write(c.pad), o_daisy };
    }
};

struct SetupScopeGuard;

struct Setup {
    Hardware hw;
    Config cfg;

    Setup apply() const { return { hw, hw.write(cfg) }; }
    SetupScopeGuard apply_temporarily() const;
};

struct SetupScopeGuard {
private:
    friend Setup;
    const Setup original_setup;
    SetupScopeGuard(Setup to_apply) : original_setup(to_apply.apply()) {}
public:
    ~SetupScopeGuard() { original_setup.apply(); }

    SetupScopeGuard(const SetupScopeGuard&) = delete;
    SetupScopeGuard& operator=(const SetupScopeGuard&) = delete;

    constexpr SetupScopeGuard(SetupScopeGuard&&) = default;
    SetupScopeGuard& operator=(SetupScopeGuard&&) = default;
};

inline SetupScopeGuard Setup::apply_temporarily() const {
    return *this;
}

inline SetupScopeGuard TemporarySetup(Setup setup) {
    return setup.apply_temporarily();
}

} /* namespace ModuleInput */

} /* namespace Teensy */

#endif /* TEENSY_PINS_HPP */
