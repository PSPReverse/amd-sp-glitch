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

#include "prompt.h"
#include "amd_cmds.h"

#include "attack.h"
#include "restart.h"
#include "glitch.h"

uint32_t attack_waits = DefaultAttackWaits;

cli_param_u32 attack_waits_this = make_cli_param_u32(attack_waits, DefaultAttackWaits, 0, 0xffffffff);
cli_param attack_waits_param = make_cli_param_u32_param("waits", attack_waits_desc, attack_waits_this, 0);

bool attack_armed = false;
bool attack_was_off = false;

bool attack_arm(void * pThis) {
    attack_armed = true;
    attack_was_off = false;
    println("Attack armed!");
    return true;
}

cli_command attack_arm_cmd = {
    .name           = "",
    .description    = attack_cmd_desc,
    .pThis          = 0,
    .exec           = &attack_arm,
    .next           = 0,
};

cli_module attack_module = {
    .name           = "attack",
    .description    = attack_mod_desc,
    .param          = &attack_waits_param,
    .cmd            = &attack_arm_cmd,
    .next           = 0,
};

void attack_process_trigger() {

    if (!attack_armed) return;
    // Attack armed

    if (!attack_was_off) {
        if (restart_is_off())
            attack_was_off = true;
        return;
    }

    if (hw.cs_pin.is_high() || hw.cs_pin.is_high()) return;

    // Attack triggered
    hw_trigger_attack_set_high();
    attack_armed = false;

    uint32_t timeout;

    for (uint32_t i = 0; i < attack_waits; i++) {

        // longest example found was 33 us
        timeout = rough_busy_wait_us(100);
        do {
            BUSY_LOOP_WHILE_PIN_LOW(attack_cs_low, timeout, hw.cs_pin);
        } while (hw.cs_pin.is_low() && hw.cs_pin.is_low());
        hw_trigger_attack_set_low();

        if (timeout == 0) {
            prompt_use_new_line();
            println("Attack failed!");
            println("Error: CS was low for too long!");
            return;
        }

        // longest example found was 15 us
        do {
            timeout = rough_busy_wait_us(50);
            BUSY_LOOP_WHILE_PIN_HIGH(attack_cs_high, timeout, hw.cs_pin);
        } while (hw.cs_pin.is_high() && hw.cs_pin.is_high());
        hw_trigger_attack_set_high();

        if (timeout == 0) {
            hw_trigger_attack_set_low();
            prompt_use_new_line();
            println("Attack failed!");
            println("Error: CS was high for too long!");
            return;
        }

    }
    hw_trigger_attack_set_low();

    // Glitch now
    glitch_result result = glitch();

    prompt_use_new_line();
    println("Attack triggered!");
    if (glitch_cs_was_low_at_glitch)
        println("Chip-Select was low at glitch time!");
    glitch_print_result(result);

}


