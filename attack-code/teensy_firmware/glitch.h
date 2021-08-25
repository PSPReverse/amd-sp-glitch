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

#ifndef GLITCH_H
#define GLITCH_H

/*

         --- time --->

SPI Chip --+             +-----------------------------------//------+      +---------+      +-
Select     +-------------+                                           +--//--+         +--//--+

SVD      -------------------------++++-----++++--------------//--------------------------------
                                  ++++     ++++

glitch                   +---------------------------+
status   ----------------+                           +-------//--------------------------------

         ^     ^         ^        ^  ^     ^  ^      ^               ^      ^         ^
         |     |         |        |  |     |  |      |               |      |         |
     Glitch  CS low    Glitch    Glitch   Glitch   Glitch          Ping   Target     Glitch
     armed  detected  triggered  start     end      done        detected  running   successful
                \_______/ \_______/  \_____/  \_____/ \_____________/ \____/ \_______/
                busy loop  busy l.   busy l.  busy l.    busy loop    busy l. busy l.
                  /         /     ^    |        |    ^       |         |         |
       max. cs_timeout   delay    |duration  cooldown|  max ping_wait  | max success_wait
                  \               \__________________/       | max cs_timeout    \
                 Nothing             repeats times           \        /       Target running
                                                           Target broken        
*/

#include "hw.h"
#include "cli.h"
#include "amd_cmds.h"

constexpr uint8_t   DefaultGlitchVid            = 0x9e; // good vid!
constexpr Command   DefaultGlitchCmd            = DefaultSocCmd.Vid(DefaultGlitchVid);

constexpr uint32_t  DefaultGlitchDelay          = rough_busy_wait_us(   200);
constexpr uint32_t  DefaultGlitchDuration       = rough_busy_wait_us_f(  20.5);

constexpr uint32_t  DefaultGlitchCooldown       = rough_busy_wait_us(    80);
constexpr uint32_t  DefaultGlitchRepeats        = 1;

constexpr uint32_t  DefaultGlitchCSTimeout      = rough_busy_wait_us(   100);
constexpr uint32_t  DefaultGlitchPingWait       = rough_busy_wait_ms(   500);
constexpr uint32_t  DefaultGlitchSuccessWait    = rough_busy_wait_us(    10);


#define glitch_mod_desc \
    "A glitch can either be triggered by an attack, by a chip-select\r\n" \
    "pulse (active-low) or manually. After being triggered we firstly\r\n" \
    "wait delay - duration many busy loop cycles. Then repeats many\r\n" \
    "times the following steps will be executed:\r\n" \
    "  - sets the configured vid (for the configured voltage)\r\n" \
    "  - wait duration many busy loop cycles\r\n" \
    "  - sets the configured default vid (in cmd_soc or cmd_core)\r\n" \
    "  - wait cooldown many busy loop cycles\r\n" \
    "Then the result detection starts:\r\n" \
    "  - wait for a low chip-select pulse\r\n" \
    "     -> if a pulse at most cs_timeout long is detected continue\r\n" \
    "     -> for a longer pulse we are in an error state\r\n" \
    "     -> with no pulse after ping_wait many busy loop cycles,\r\n" \
    "        the target is considered broken\r\n" \
    "  - wait for another chip-select pulse\r\n" \
    "     -> if detected the glitch is considered to be successful\r\n" \
    "     -> with no pulse after success_wait many busy loop cycles,\r\n" \
    "        the target is considered to be restarting"

#define glitch_cmd_desc \
    "Manually triggers a glitch."
#define glitch_arm_cmd_desc \
    "Arms a glitch to be triggered on the next chip-select pulse."

#define glitch_delay_desc \
    "The time to wait before trying to glitch the target (the\r\n" \
    "duration of the glitch will be subtracted beforehand)."
#define glitch_duration_desc \
    "The time for which the configured vid will be active."
#define glitch_cooldown_desc \
    "The time to wait before executing another glitch or starting\r\n" \
    "the chip-select detection."
#define glitch_repeats_desc \
    "How many glitches should be executed."
#define glitch_cs_timeout_desc \
    "Maximum length for a chip-select pulse."
#define glitch_ping_wait_desc \
    "How long to wait for a chip-select pulse after the cooldown."
#define glitch_success_wait_desc \
    "How long to wait for the second chip-select pulse."

extern bool glitch_cs_was_low_at_glitch;

enum glitch_result : uint8_t {
    glitch_target_running,
    glitch_success,
    glitch_target_broken,
    glitch_error,
};

bool glitch_print_result(glitch_result result);

void glitch_process_trigger();

glitch_result glitch();

extern cli_module glitch_module;

#endif /* GLITCH_H */
