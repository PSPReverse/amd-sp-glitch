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

#ifndef TEENSY_TWI_HPP
#define TEENSY_TWI_HPP

#include <imxrt.h>
#include "teensy_pins.hpp"

/*
  [1]:  i.MX RT1060 Processor ReferenceManual
        https://www.pjrc.com/teensy/IMXRT1060RM_rev2.pdf
*/

namespace Teensy {
namespace Twi {

struct HardwareSetup;

struct Hardware {
    IMXRT_LPI2C_t           *regs;
    ModuleInput::Hardware   scl;
    ModuleInput::Hardware   sda;
    ModuleInput::Config     pin_cfg;

    HardwareSetup      input_only() const;
    HardwareSetup      open_drain() const;
    HardwareSetup      open_drain_output_only() const;
    HardwareSetup      push_pull() const;

    // Pin configurations working on the teensy
    static const Hardware Pins_19_18;
    static const Hardware Pins_37_36;
    static const Hardware Pins_16_17;
    static const Hardware Pins_24_25;

    // Base clock
    // Use 24 Mhz as the default.
    #ifndef I2C_BASE_CLK
    #  warning I2C_BASE_CLK not specified, using 24000000 (24 Mhz).
    #  define I2C_BASE_CLK 24000000
    #endif 

    static bool base_clock_initialized;
    static void setup_base_clock();
};

struct HardwareSetup {
    Hardware    hw;
    uint8_t     pincfg;

    bool verify() const;
    void reset() const;
    void setup() const;
};

// Use 400Kbps as a default baudrate
#ifndef I2C_BAUDRATE
#  warning I2C_BAUDRATE not specified, using 400000 (400 Kbps).
#  define I2C_BAUDRATE 400000
#endif 

inline HardwareSetup Hardware::input_only() const {
    HardwareSetup cfg = { .hw = *this, .pincfg = 0 };
    cfg.hw.pin_cfg.pad = cfg.hw.pin_cfg.pad.Input();
    return cfg;
}

inline HardwareSetup Hardware::open_drain() const {
    HardwareSetup cfg = { .hw = *this, .pincfg = 0 };
    cfg.hw.pin_cfg.pad = cfg.hw.pin_cfg.pad.OpenDrain().PullUp().Output();
    return cfg;
}

inline HardwareSetup Hardware::open_drain_output_only() const {
    HardwareSetup cfg = { .hw = *this, .pincfg = 1 };
    //cfg.hw.pin_cfg.pad = cfg.hw.pin_cfg.pad.OpenDrain().PullUp().Output();
    cfg.hw.pin_cfg.pad = cfg.hw.pin_cfg.pad.OpenDrain().Output();
    return cfg;
}

inline HardwareSetup Hardware::push_pull() const {
    HardwareSetup cfg = { .hw = *this, .pincfg = 2 };
    cfg.hw.pin_cfg.pad = cfg.hw.pin_cfg.pad.Output();
    return cfg;
}

inline void Hardware::setup_base_clock() {
    if (base_clock_initialized) return;

    // disable module clocks (see [1] pages 1024, 1086-1087 and 1091-1092)
    // TODO: is this necessary because of glitching?
    CCM_CCGR2 &= ~(CCM_CCGR2_LPI2C1(3) | CCM_CCGR2_LPI2C2(3) | CCM_CCGR2_LPI2C3(3));
    CCM_CCGR6 &= ~(CCM_CCGR6_LPI2C4_SERIAL(3));

    #if I2C_BASE_CLK == 24000000

    // Use the oscillator clock (24 MHz) as the LPI2C_CLK_ROOT clock
    // (see [1] pages 1017-1018 and 1067-1068)
    CCM_CSCDR2 |= CCM_CSCDR2_LPI2C_CLK_SEL;
    // No clock division (divisor = 1 + 0);
    CCM_CSCDR2 &= ~(CCM_CSCDR2_LPI2C_CLK_PODF(0x1f));

    #elif I2C_BASE_CLK == 60000000

    // Use the scaled PLL3 clock (60 MHz) as the LPI2C_CLK_ROOT clock
    // (see [1] pages 1017-1018 and 1067-1068)
    CCM_CSCDR2 &= ~CCM_CSCDR2_LPI2C_CLK_SEL;
    // No clock division (divisor = 1 + 0);
    CCM_CSCDR2 &= ~(CCM_CSCDR2_LPI2C_CLK_PODF(0x1f));

    #else
    #  error Unknown I2C_BASE_CLK value, use 24000000 or 60000000!
    #endif

    // enable module clocks modules (also in WAIT mode)
    // (see [1] pages 1024, 1086-1087 and 1091-1092)
    CCM_CCGR2 |= CCM_CCGR2_LPI2C1(3) | CCM_CCGR2_LPI2C2(3) | CCM_CCGR2_LPI2C3(3);
    CCM_CCGR6 |= CCM_CCGR6_LPI2C4_SERIAL(3);

    base_clock_initialized = true;
}

inline bool HardwareSetup::verify() const {
    // All LPI2Cs need to have version 1.0 and support Master and Slave modes.
    // (see [1] pages 2759-2760)
    uint32_t verid = hw.regs->VERID;
    return ((verid >> 16) == 0x0100) && ((verid & 3) == 3);
}

inline void HardwareSetup::reset() const {
    // Reset Master FIFOs and Registers
    // (see [1] pages 2761-2762)
    hw.regs->MCR = LPI2C_MCR_RRF | LPI2C_MCR_RTF | LPI2C_MCR_RST;

    // Reset Slave FIFOs and Registers
    // (see [1] pages 2761-2762)
    hw.regs->SCR = LPI2C_SCR_RRF | LPI2C_SCR_RTF | LPI2C_SCR_RST;
}

inline void HardwareSetup::setup() const {

    hw.setup_base_clock();

    // Setup the used pins
    hw.scl.write(hw.pin_cfg);
    hw.sda.write(hw.pin_cfg);

    // Set pinconfig (see [1] pages 2748-2749 and 2768-2770)
    hw.regs->MCFGR1 =
        (hw.regs->MCFGR1 & ~LPI2C_MCFGR1_PINCFG(0xffffffff))
        | LPI2C_MCFGR1_PINCFG(pincfg);

    // Set timing parameters (see [1] pages 2749-2751, 2770-2772 and 2774-2775)
    // These are:
    //      - DATAVD
    //      - SETHOLD
    //      - CLKHI
    //      - CLKLO
    //      - FILTSDA
    //      - FILTSCL
    //      - PRESCALE
    // The others will be left as their defaults
    //      - BUSIDLE = 0 (no timeout)
    //
    // In this case the base clock cycles it takes to transmit on bit are:
    //  (2 + floor((2 + FILTSCL)/2^PRESCALE) + CLKHI + CLKLO) * 2^PRESCALE
    //
    // with PRESCALE = 0:
    //  4 + FILTSCL + CLKHI + CLKLO
    //
    // with PRESCALE = 1:
    //  4 + 2*(floor(1 + FILTSCL/2) + CLKHI + CLKLO )
    //
    // with PRESCALE = 2:
    //  8 + 4*(floor(1/2 + FILTSCL/4) + CLKHI + CLKLO )
    //
    // with PRESCALE = 3:
    //  16 + 8*(floor(1/4 + FILTSCL/8) + CLKHI + CLKLO )
    //
    // So we will set PRESCALE, FILTSCL, CLKLO and CLKHI them accordingly for each baudrate.
    // For simplicity we set FILTSDA equal to FILTSCL.

    #if I2C_BASE_CLK == 24000000 // 24 MHz
    // | Baudrate | PRESCALE | CLKLO | CLKHI | FILTSCL | Total Cycles |
    // |----------|----------|-------|-------|---------|--------------|
    // | 100 Kbps |        2 |    31 |    23 |      14 |          240 |
    // | 400 Kbps |        1 |    14 |    10 |       6 |           60 |
    // |   1 Mbps |        0 |    11 |     7 |       2 |           24 |
    // |   3 Mbps |        0 |     3 |     1 |       0 |            8 |
    //
    // Note: CLKLO is usually higher than CLKHI (see manual [1] page 2751).
    // TODO: check the HI-LO differences (against I2C spec. or by experiments)
    //
    // DATAVD must be between 1 and CLKLO - 2
    // SETHOLD must be above 2
    //
    // | Baudrate | DATAVD | SETHOLD |
    // |----------|--------|---------|
    // | 100 Kbps |     15 |      54 |
    // | 400 Kbps |      7 |      48 |
    // |   1 Mbps |      5 |      18 |
    // |   3 Mbps |      1 |       4 |
    
    #if I2C_BAUDRATE == 100000
    hw.regs->MCFGR1 |= LPI2C_MCFGR1_PRESCALE(2);
    hw.regs->MCCR0 |= LPI2C_MCCR0_DATAVD(15)
                    | LPI2C_MCCR0_SETHOLD(54)
                    | LPI2C_MCCR0_CLKHI(23)
                    | LPI2C_MCCR0_CLKLO(31);
    hw.regs->MCFGR2 |= LPI2C_MCFGR2_FILTSDA(14) | LPI2C_MCFGR2_FILTSCL(14);
    hw.regs->SCFGR2 |= LPI2C_SCFGR2_FILTSDA(14) | LPI2C_SCFGR2_FILTSCL(14)
                    | LPI2C_SCFGR2_DATAVD(30);
    #elif I2C_BAUDRATE == 400000
    hw.regs->MCFGR1 |= LPI2C_MCFGR1_PRESCALE(1);
    hw.regs->MCCR0 |= LPI2C_MCCR0_DATAVD(7)
                    | LPI2C_MCCR0_SETHOLD(48)
                    | LPI2C_MCCR0_CLKHI(10)
                    | LPI2C_MCCR0_CLKLO(14);
    hw.regs->MCFGR2 |= LPI2C_MCFGR2_FILTSDA(6) | LPI2C_MCFGR2_FILTSCL(6);
    hw.regs->SCFGR2 |= LPI2C_SCFGR2_FILTSDA(6) | LPI2C_SCFGR2_FILTSCL(6)
                    | LPI2C_SCFGR2_DATAVD(12);
    #elif I2C_BAUDRATE == 1000000
    hw.regs->MCCR0 |= LPI2C_MCCR0_DATAVD(5)
                    | LPI2C_MCCR0_SETHOLD(18)
                    | LPI2C_MCCR0_CLKHI(7);
                    | LPI2C_MCCR0_CLKLO(11);
    hw.regs->MCFGR2 |= LPI2C_MCFGR2_FILTSDA(2) | LPI2C_MCFGR2_FILTSCL(2);
    hw.regs->SCFGR2 |= LPI2C_SCFGR2_FILTSDA(2) | LPI2C_SCFGR2_FILTSCL(2)
                    | LPI2C_SCFGR2_DATAVD(4);
    #elif I2C_BAUDRATE == 3000000
    #  warning "A 3 Mbps baud rate is unstable with a 24 MHz baseclock!"
    hw.regs->MCCR0 |= LPI2C_MCCR0_DATAVD(1)
                    | LPI2C_MCCR0_SETHOLD(4)
                    | LPI2C_MCCR0_CLKHI(1)
                    | LPI2C_MCCR0_CLKLO(3);
    hw.regs->SCFGR2 |= LPI2C_SCFGR2_DATAVD(1);
    #else
    #  error Unknown I2C_BAUDRATE value, use 100000, 400000, 1000000 or 3000000!
    #endif

    #elif I2C_BASE_CLK == 60000000
    // | Baudrate | PRESCALE |FILTSCL |  CLKLO | CLKHI | Total Cycles |
    // |----------|----------|--------|--------|-------|--------------|
    // | 100 Kbps |        3 |     15 |     41 |    30 |          600 |
    // | 400 Kbps |        2 |     13 |     19 |    13 |          152 | (c.a. 405 Kbps)
    // |   1 Mbps |        0 |      6 |     30 |    20 |           60 |
    // | 3.3 Mbps |        0 |      3 |      7 |     4 |           18 |
    //
    // Note: CLKLO is usually higher than CLKHI (see manual [1] page 2751).
    // TODO: check the HI-LO differences (against I2C spec. or by experiments)
    //
    // DATAVD must be between 1 and CLKLO - 2
    // SETHOLD must be above 2
    //
    // | Baudrate | DATAVD | SETHOLD |
    // |----------|--------|---------|
    // | 100 Kbps |     20 |      63 |
    // | 400 Kbps |      9 |      32 |
    // |   1 Mbps |     15 |      50 |
    // | 3.3 Mbps |      3 |      11 |
    
    #if I2C_BAUDRATE == 100000
    hw.regs->MCFGR1 |= LPI2C_MCFGR1_PRESCALE(3);
    hw.regs->MCCR0 |= LPI2C_MCCR0_DATAVD(20)
                    | LPI2C_MCCR0_SETHOLD(63)
                    | LPI2C_MCCR0_CLKHI(30)
                    | LPI2C_MCCR0_CLKLO(41);
    hw.regs->MCFGR2 |= LPI2C_MCFGR2_FILTSDA(15) | LPI2C_MCFGR2_FILTSCL(15);
    hw.regs->SCFGR2 |= LPI2C_SCFGR2_FILTSDA(15) | LPI2C_SCFGR2_FILTSCL(15)
                    | LPI2C_SCFGR2_DATAVD(30);
    #elif I2C_BAUDRATE == 400000
    hw.regs->MCFGR1 |= LPI2C_MCFGR1_PRESCALE(2);
    hw.regs->MCCR0 |= LPI2C_MCCR0_DATAVD(9)
                    | LPI2C_MCCR0_SETHOLD(32)
                    | LPI2C_MCCR0_CLKHI(13)
                    | LPI2C_MCCR0_CLKLO(19);
    hw.regs->MCFGR2 |= LPI2C_MCFGR2_FILTSDA(13) | LPI2C_MCFGR2_FILTSCL(13);
    hw.regs->SCFGR2 |= LPI2C_SCFGR2_FILTSDA(13) | LPI2C_SCFGR2_FILTSCL(13)
                    | LPI2C_SCFGR2_DATAVD(26);
    #elif I2C_BAUDRATE == 1000000
    hw.regs->MCCR0 |= LPI2C_MCCR0_DATAVD(15)
                    | LPI2C_MCCR0_SETHOLD(50)
                    | LPI2C_MCCR0_CLKHI(20)
                    | LPI2C_MCCR0_CLKLO(30);
    hw.regs->MCFGR2 |= LPI2C_MCFGR2_FILTSDA(6) | LPI2C_MCFGR2_FILTSCL(6);
    hw.regs->SCFGR2 |= LPI2C_SCFGR2_FILTSDA(6) | LPI2C_SCFGR2_FILTSCL(6)
                    | LPI2C_SCFGR2_DATAVD(15);
    #elif I2C_BAUDRATE == 3300000
    hw.regs->MCCR0 |= LPI2C_MCCR0_DATAVD(3)
                    | LPI2C_MCCR0_SETHOLD(21)
                    | LPI2C_MCCR0_CLKHI(4)
                    | LPI2C_MCCR0_CLKLO(7);
    hw.regs->MCFGR2 |= LPI2C_MCFGR2_FILTSDA(3) | LPI2C_MCFGR2_FILTSCL(3);
    hw.regs->SCFGR2 |= LPI2C_SCFGR2_FILTSDA(3) | LPI2C_SCFGR2_FILTSCL(3)
                    | LPI2C_SCFGR2_DATAVD(3);
    #elif I2C_BAUDRATE == 4670000
    hw.regs->MCCR0 |= LPI2C_MCCR0_DATAVD(2)
                    | LPI2C_MCCR0_SETHOLD(10)
                    | LPI2C_MCCR0_CLKHI(2)
                    | LPI2C_MCCR0_CLKLO(6);
    hw.regs->MCFGR2 |= LPI2C_MCFGR2_FILTSDA(1) | LPI2C_MCFGR2_FILTSCL(1);
    hw.regs->SCFGR2 |= LPI2C_SCFGR2_FILTSDA(1) | LPI2C_SCFGR2_FILTSCL(1)
                    | LPI2C_SCFGR2_DATAVD(2);
    #else
    #  error Unknown I2C_BAUDRATE value, use 100000, 400000, 1000000, 3300000 or 4670000!
    #endif

    #else
    #  error Unknown I2C_BASE_CLK value, use 24000000 or 60000000!
    #endif
}

struct MasterConfig {
    unsigned int ignore_nacks : 1;
};

struct Master {
    HardwareSetup  hw;
    MasterConfig    cfg;

    inline IMXRT_LPI2C_t&  regs() const { return *hw.hw.regs; }

    Master() : hw(Hardware::Pins_19_18.open_drain()) {}
    Master(MasterConfig cfg, HardwareSetup hw) : hw(hw), cfg(cfg) {}

    inline void setup() const {
        disable();

        hw.setup();

        // Ignore NACKs (see [1] pages 2768-2770)
        if (cfg.ignore_nacks)
            regs().MCFGR1 |= LPI2C_MCFGR1_IGNACK;
        else
            regs().MCFGR1 &= ~LPI2C_MCFGR1_IGNACK;
    }

    inline void enable() const {
        // Enable the master (see [1] pages 2761-2762)
        regs().MCR |= LPI2C_MCR_MEN;
    }

    inline void disable() const {
        // Disable the master (see [1] pages 2761-2762)
        regs().MCR &= ~LPI2C_MCR_MEN;
    }

    // Returns MCR on success, negative error codes on error
    int send_u16(uint8_t address, uint16_t message, uint32_t timeout);
};

enum addrcfg : uint8_t {
    addr0_7bit                  = 0,
    addr0_10bit                 = 1,
    addr0_7bit_or_addr1_7bit    = 2,
    addr0_10bit_or_addr1_10bit  = 3,
    addr0_7bit_or_addr1_10bit   = 4,
    addr0_10bit_or_addr1_7bit   = 5,
    addr0_to_addr1_7bit         = 6,
    addr0_to_addr1_10bit        = 7,
};

struct SlaveConfig {
    unsigned int    use_filter : 1;
    unsigned int    ignore_nacks : 1;
    unsigned int    addrcfg : 3;
    unsigned int    addr0 : 10;
    unsigned int    addr1 : 10;
};

struct Slave {
    HardwareSetup  hw;
    SlaveConfig    cfg;

    inline IMXRT_LPI2C_t&  regs() const { return *hw.hw.regs; }

    Slave() {}
    Slave(SlaveConfig cfg, HardwareSetup hw) : hw(hw), cfg(cfg) {}

    inline void setup() const {
        // Slave Configuration Register (see [1] pages 2781-2782)
        regs().SCR =
            LPI2C_SCR_RRF | LPI2C_SCR_RTF   // clear fifos
            | (cfg.use_filter ? LPI2C_SCR_FILTEN : 0)
        ; // disables slave

        // Sets Slave configuration register 2
        hw.setup();

        // Slave Interrupt Enable Register (see [1] pages 2785-2786)
        regs().SIER = 0;

        // Slave DMA Enable Register (see [1] pages 2786-2787)
        regs().SDER = 0;

        // Slave configuration register 1 (see [1] pages 2787-2790)
        regs().SCFGR1 =
              LPI2C_SCFGR1_ADDRCFG(cfg.addrcfg)
            | (cfg.ignore_nacks ? LPI2C_SCFGR1_IGNACK : 0)
        ;

        // Slave Address Match Register (see [1] pages 2791-2792)
        regs().SAMR = LPI2C_SAMR_ADDR0(cfg.addr0) | LPI2C_SAMR_ADDR1(cfg.addr1);
    }

    inline void clear_errors_and_fifos() const {
        regs().SCR |= LPI2C_SCR_RRF | LPI2C_SCR_RTF;
        regs().SSR = LPI2C_SSR_FEF | LPI2C_SSR_BEF | LPI2C_SSR_SDF;
    }

    inline void disable() const {
        // Enable the slave (see [1] pages 2761-2762)
        regs().SCR &= ~LPI2C_SCR_SEN;
    }

    inline void enable() const {
        // Disable the slave (see [1] pages 2761-2762)
        regs().SCR |= LPI2C_SCR_SEN;
    }

    // Receives a message and writes it to *message*, the address
    // for which the message was received is saved in *address* and
    // *n* is the size of *message*.
    //
    // Returns number of bytes written to *message* or a negative
    // number on error.
    int recv(uint8_t &address, uint8_t *message, unsigned int n, uint32_t timeout);
};

} /* namespace Twi */
} /* namespace Teensy */

#endif /* TEENSY_TWI_HPP */
