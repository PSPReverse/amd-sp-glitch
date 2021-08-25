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

#ifndef HW_H
#define HW_H

#include "teensy_pins.hpp"
#include "teensy_twi.hpp"

#include "cli.h"

using namespace Teensy;


////////////////
// CLI Module //
////////////////

#define hw_mod_desc \
    "Controls which hardware components are used and how."

#define hw_cfg_desc \
    "Allows switching between the two configurations (1 and 2).\r\n" \
    "                  cfg 1   cfg 2  \r\n" \
    "  ===============================\r\n" \
    "    trigger pin |     0 |     9 |\r\n" \
    "         cs pin |     1 |    10 |\r\n" \
    "      reset pin |     2 |    11 |\r\n" \
    "     svc in pin |    20 |    15 |\r\n" \
    "     svd in pin |    21 |    14 |\r\n" \
    "    svc out pin |    19 |    16 |\r\n" \
    "    svd out pin |    18 |    17 |"
#define hw_trigger_cli_desc \
    "Whether the trigger pin pulses on cli activity (for debugging)."
#define hw_trigger_attack_desc \
    "Whether the trigger pin pulses on attack activity."
#define hw_trigger_glitch_desc \
    "Whether the trigger pin pulses on glitch activity."
#define hw_trigger_glitch_running_desc \
    "Whether the trigger pin pulses on a glitch where the system\r\n" \
    "continues running."
#define hw_trigger_glitch_success_desc \
    "Whether the trigger pin pulses on a glitch that succeeds."
#define hw_trigger_glitch_broken_desc \
    "Whether the trigger pin pulses on a glitch that breaks the\r\n" \
    "device under test."
#define hw_trigger_restart_desc \
    "Whether the trigger pin pulses on a restart of the device\r\n" \
    "under test."

extern cli_module hw_module;


////////////////////////////
// Hardware configuration //
////////////////////////////

typedef struct {
    Gpio::Hardware  trigger_pin;
    Gpio::Hardware  cs_pin;
    Gpio::Hardware  reset_pin;
    Gpio::Hardware  scl_in_pin;
    Gpio::Hardware  sda_in_pin;
    Twi::Hardware   twi_hardware;
} hardware_config;

inline hardware_config HwCfg1() {
    return {
        .trigger_pin    = Gpio0(),
        .cs_pin         = Gpio1(),
        .reset_pin      = Gpio2(),
        .scl_in_pin     = Gpio20(),
        .sda_in_pin     = Gpio21(),
        .twi_hardware   = Twi::Hardware::Pins_19_18,
    };
}

inline hardware_config HwCfg2() {
    return {
        .trigger_pin    = Gpio9(),
        .cs_pin         = Gpio10(),
        .reset_pin      = Gpio11(),
        .scl_in_pin     = Gpio15(),
        .sda_in_pin     = Gpio14(),
        .twi_hardware   = Twi::Hardware::Pins_16_17,
    };
}

// TODO remove this
constexpr uint32_t twi_timeout = 60000000;


////////////////////////
// Hardware Interface //
////////////////////////

extern Twi::Master twi_master;
extern hardware_config hw;

void hw_init(hardware_config cfg = HwCfg1());
void hw_deinit();

void hw_trigger_cli_set_high();
void hw_trigger_cli_set_low();

void hw_trigger_attack_set_high();
void hw_trigger_attack_set_low();

void hw_trigger_glitch_set_high();
void hw_trigger_glitch_set_low();

void hw_trigger_glitch_broken_set_high();
void hw_trigger_glitch_broken_set_low();

void hw_trigger_glitch_success_set_high();
void hw_trigger_glitch_success_set_low();

void hw_trigger_glitch_running_set_high();
void hw_trigger_glitch_running_set_low();

void hw_trigger_restart_set_high();
void hw_trigger_restart_set_low();


////////////////
// busy loops //
////////////////

constexpr uint32_t rough_busy_wait_us_f(float us) {
    // 300 waits ~ 5 us
    // 60 waits ~ 1 us
    return us * 60;
}

constexpr uint32_t rough_busy_wait_us(uint32_t us) {
    // 300 waits ~ 5 us
    // 60 waits ~ 1 us
    return us * 60;
}

constexpr uint32_t rough_busy_wait_ms(uint32_t ms) {
    // 60000 waits ~ 1 ms
    return ms * 60000;
}

// Note: the use of memory ensures that the timing
//       is simliar to the busy loops with conditions
#define BUSY_LOOP(UID, TIMEOUT) \
    asm volatile ( \
"_busy_loop_" #UID ":\n" \
/* while ((TIMEOUT--) */ \
"   cbz %0, _busy_loop_end_" #UID "\n" \
"   sub %0, %0, #1\n" \
/* timing adjustments */ \
"   ldr r1, [%1]\n" \
"   tst r1, %0\n" \
"   b _busy_loop_" #UID "\n" \
"_busy_loop_end_" #UID ":\n" \
    : /* no outputs */ \
    : "r"(TIMEOUT), "r"(&hw.cs_pin.regs->PSR) \
    : "r1")

// If TIMEOUT == 0 afterwards then a timeout happened.
// Otherwise PIN was LOW (HIGH) two times in a row.
#define BUSY_LOOP_WHILE_PIN_HIGH(UID, TIMEOUT, PIN) \
    __BUSY_LOOP_WHILE_PIN(UID, TIMEOUT, PIN, bne)

#define BUSY_LOOP_WHILE_PIN_LOW(UID, TIMEOUT, PIN) \
    __BUSY_LOOP_WHILE_PIN(UID, TIMEOUT, PIN, beq)

#define __BUSY_LOOP_WHILE_PIN(UID, TIMEOUT, PIN, CB) \
    asm volatile ( \
"_busy_loop_" #UID ":\n" \
/* while ((TIMEOUT--) */ \
"   cbz %0, _busy_loop_end_" #UID "\n" \
"   sub %0, %0, #1\n" \
/* && (PIN.is_???() */ \
"   ldr r1, [%2]\n" \
"   tst r1, %3\n" \
"   " #CB " _busy_loop_" #UID "\n" \
/* || PIN.is_???())); */ \
"   ldr r1, [%2]\n" \
"   tst r1, %3\n" \
"   " #CB " _busy_loop_" #UID "\n" \
"_busy_loop_end_" #UID ":\n" \
    : "=r"(TIMEOUT) \
    : "0"(TIMEOUT), "r"(&PIN.regs->PSR), "r"(PIN.mask) \
    : "r1")


#endif /* HW_H */
