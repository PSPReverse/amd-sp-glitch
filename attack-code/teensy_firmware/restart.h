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

#ifndef RESTART_H
#define RESTART_H

#include "cli.h"


/*

detection_off
     |
     | set restart detection on
    \|/
  dut_off
     |
     |
    \|/
dut_running
     |
     | clk pin is low for more than wait_off cycles
    \|/
  dut_off

*/


enum e_restart_status : uint8_t {
    restart_detection_off,
    dut_running,
    dut_off,
};

constexpr uint8_t   DefaultRestartStatus            = dut_off;

constexpr bool      DefaultRestartDisableTelemetry  = true;

constexpr uint32_t  DefaultRestartWaitOff           = rough_busy_wait_ms( 10);

constexpr uint32_t  DefaultRestartWaitOn            = rough_busy_wait_ms(  1);

constexpr uint32_t  DefaultRestartDelay             = rough_busy_wait_ms(  2);

constexpr uint32_t  DefaultRestartResetLen          = rough_busy_wait_ms( 80);

#define restart_mod_desc \
    "This module controls the restart detecting and triggering."

#define restart_cmd_desc \
    "Manually triggers the restart routine. Note that this doesn't\r\n" \
    "restart the machine, but performs the packet injections as if\r\n" \
    "a restart was detected."

#define restart_reset_cmd_desc \
    "Restarts the device under test by pulling the reset line low."

#define restart_detect_desc \
    "Whether restart detection is enabled."
#define restart_disable_telemetry_desc \
    "Whether telemetry will be disabled on restart detection."
#define restart_wait_off_desc \
    "How many busy loop cycles does the SVD line needs to be low\r\n" \
    "for the restart detection to move the device under test\r\n" \
    "from the on state to the off state."
#define restart_wait_on_desc \
    "How many busy loop cycles does the SVD line needs to be high\r\n" \
    "for the restart detection to consider the device under test\r\n" \
    "turned on, when it is turned off."
#define restart_delay_desc \
    "After a restart was detected, this parameter specified how\r\n" \
    "many busy loop cycles will be waited before the default vids\r\n" \
    "are set (and the telemetry is turned off)."
#define restart_reset_len_desc \
    "The reset line will be pulled low for this many busy loop\r\n" \
    "cycles when the \"restart reset\" command is issued."

uint8_t restart_update_status();

bool restart();

bool restart_is_off();

extern cli_module restart_module;

#endif /* RESTART_H */
