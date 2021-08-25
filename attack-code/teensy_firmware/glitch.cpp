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

#include "glitch.h"

bool        glitch_cs_was_low_at_glitch = false;

bool        glitch_armed            = false;

Command     glitch_cmd              = DefaultGlitchCmd;

uint32_t    glitch_delay            = DefaultGlitchDelay;
uint32_t    glitch_duration         = DefaultGlitchDuration;
uint32_t    glitch_cooldown         = DefaultGlitchCooldown;
uint32_t    glitch_repeats          = DefaultGlitchRepeats;

uint32_t    glitch_cs_timeout       = DefaultGlitchCSTimeout;
uint32_t    glitch_ping_wait        = DefaultGlitchPingWait;
uint32_t    glitch_success_wait     = DefaultGlitchSuccessWait;

cmd_param glitch_cmd_this = { .pCmd = &glitch_cmd, .pDefault = &DefaultGlitchCmd };

cli_param_u32 glitch_delay_this         = make_cli_param_u32(glitch_delay,          DefaultGlitchDelay,         0, 0xffffffff);
cli_param_u32 glitch_duration_this      = make_cli_param_u32(glitch_duration,       DefaultGlitchDuration,      0, 0xffffffff);
cli_param_u32 glitch_cooldown_this      = make_cli_param_u32(glitch_cooldown,       DefaultGlitchCooldown,      0, 0xffffffff);
cli_param_u32 glitch_repeats_this       = make_cli_param_u32(glitch_repeats,        DefaultGlitchRepeats,       0, 0xffffffff);
cli_param_u32 glitch_cs_timeout_this    = make_cli_param_u32(glitch_cs_timeout,     DefaultGlitchCSTimeout,     0, 0xffffffff);
cli_param_u32 glitch_ping_wait_this     = make_cli_param_u32(glitch_ping_wait,      DefaultGlitchPingWait,      0, 0xffffffff);
cli_param_u32 glitch_success_wait_this  = make_cli_param_u32(glitch_success_wait,   DefaultGlitchSuccessWait,   0, 0xffffffff);

cli_param glitch_success_wait_param = make_cli_param_u32_param("success_wait",  glitch_success_wait_desc,   glitch_success_wait_this,   0);
cli_param glitch_ping_wait_param    = make_cli_param_u32_param("ping_wait",     glitch_ping_wait_desc,      glitch_ping_wait_this,      &glitch_success_wait_param);
cli_param glitch_cs_timeout_param   = make_cli_param_u32_param("cs_timeout",    glitch_cs_timeout_desc,     glitch_cs_timeout_this,     &glitch_ping_wait_param);
cli_param glitch_repeats_param      = make_cli_param_u32_param("repeats",       glitch_repeats_desc,        glitch_repeats_this,        &glitch_cs_timeout_param);
cli_param glitch_cooldown_param     = make_cli_param_u32_param("cooldown",      glitch_cooldown_desc,       glitch_cooldown_this,       &glitch_repeats_param);
cli_param glitch_duration_param     = make_cli_param_u32_param("duration",      glitch_duration_desc,       glitch_duration_this,       &glitch_cooldown_param);
cli_param glitch_delay_param        = make_cli_param_u32_param("delay",         glitch_delay_desc,          glitch_delay_this,          &glitch_duration_param);

cli_param glitch_core_param         = make_cmd_core_param(                                                  glitch_cmd_this,            &glitch_delay_param);
cli_param glitch_soc_param          = make_cmd_soc_param(                                                   glitch_cmd_this,            &glitch_core_param);
cli_param glitch_vid_param          = make_cmd_vid_param(                                                   glitch_cmd_this,            &glitch_soc_param);

bool glitch_print_result(glitch_result result) {
    if (glitch_delay < glitch_duration)
        println("Warning: duration is larger than delay!");

    switch (result) {

        case glitch_target_running:
            println("Target continues running!");
            return true;

        case glitch_target_broken:
            println("Target is broken!");
            return true;

        case glitch_success:
            println("Target glitched successfully!");
            return true;

        case glitch_error:
            println("Error: The injection of one of the commands/packets failed!");
            return false;

        default:
            println("Error: Unknown result (this should never happen)!");
            return false;

    }
}

bool glitch_arm(void * pThis) {
    glitch_armed = true;
    println("Glitch armed!");
    return true;
}

bool do_glitch(void * pThis) {

    glitch_result result = glitch();

    // Do serial io only after time-critical code
    println("Glitch manually triggered!");

    return glitch_print_result(result);
}

cli_command glitch_arm_cmd = {
    .name           = "arm",
    .description    = glitch_arm_cmd_desc,
    .pThis          = 0,
    .exec           = &glitch_arm,
    .next           = 0,
};

cli_command glitch_man_cmd = {
    .name           = "",
    .description    = glitch_cmd_desc,
    .pThis          = 0,
    .exec           = &do_glitch,
    .next           = &glitch_arm_cmd,
};

cli_module glitch_module = {
    .name           = "glitch",
    .description    = glitch_mod_desc,
    .param          = &glitch_vid_param,
    .cmd            = &glitch_man_cmd,
    .next           = 0,
};




void glitch_process_trigger() {

    if (!glitch_armed) return;
    // Glitch armed

    if (hw.cs_pin.is_high() || hw.cs_pin.is_high()) return;
    // Glitch triggered

    uint32_t timeout = glitch_cs_timeout;
    BUSY_LOOP_WHILE_PIN_LOW(cs_low_trigger, timeout, hw.cs_pin);
    if (timeout == 0) return; // CS was low for too long
    // Glitch triggered

    glitch_armed = false;
    glitch_result result = glitch();

    // Do serial io only after time-critical code
    prompt_use_new_line();
    println("Glitch triggered!");

    glitch_print_result(result);

}

glitch_result glitch() {
    // Glitch triggered

    hw_trigger_glitch_set_high();
    uint32_t timeout = glitch_delay - glitch_duration;
    if (timeout <= glitch_delay)
        BUSY_LOOP(glitch_delay, timeout);
    hw_trigger_glitch_set_low();

    for (uint32_t i = 0; i < glitch_repeats; i++) {

        // Glitch start
        hw_trigger_glitch_set_high();
        if (glitch_cmd.send(twi_master, twi_timeout) < 0) {
            // Error recovery
            if (glitch_cmd.core)
                core_cmd.send(twi_master, twi_timeout);
            if (glitch_cmd.soc)
                soc_cmd.send(twi_master, twi_timeout);
            hw_trigger_glitch_set_low();
            return glitch_error;
        }

        timeout = glitch_duration;
        BUSY_LOOP(glitch_duration, timeout);

        // Glitch end
        if (glitch_cmd.soc)
            if (soc_cmd.send(twi_master, twi_timeout) < 0) {
                if (glitch_cmd.core)
                    core_cmd.send(twi_master, twi_timeout);
                hw_trigger_glitch_set_low();
                return glitch_error;
            }
        if (glitch_cmd.core)
            if (core_cmd.send(twi_master, twi_timeout) < 0) {
                hw_trigger_glitch_set_low();
                return glitch_error;
            }
        glitch_cs_was_low_at_glitch = hw.cs_pin.is_low();
        hw_trigger_glitch_set_low();

        timeout = glitch_cooldown;
        BUSY_LOOP(glitch_cooldown, timeout);
    }

    // Glitch done
    timeout = glitch_ping_wait;
    BUSY_LOOP_WHILE_PIN_HIGH(wait_ping, timeout, hw.cs_pin);
    if (timeout == 0) {
        hw_trigger_glitch_broken_set_high();
        timeout = 10;
        BUSY_LOOP(glitch_broken_trigger, timeout);
        hw_trigger_glitch_broken_set_low();
        return glitch_target_broken; // No ping detected
    }

    // Ping detected
    timeout = glitch_cs_timeout;
    BUSY_LOOP_WHILE_PIN_LOW(cs_low_ping, timeout, hw.cs_pin);
    if (timeout == 0) {
        hw_trigger_glitch_broken_set_high();
        timeout = 10;
        BUSY_LOOP(glitch_broken_trigger, timeout);
        hw_trigger_glitch_broken_set_low();
        return glitch_target_broken; // Might not have been a ping
    }

    // Target running
    timeout = glitch_success_wait;
    BUSY_LOOP_WHILE_PIN_HIGH(success_wait, timeout, hw.cs_pin);

    if (timeout == 0) {
        // No success ping
        hw_trigger_glitch_running_set_high();
        timeout = 10;
        BUSY_LOOP(glitch_running_trigger, timeout);
        hw_trigger_glitch_running_set_low();
        return glitch_target_running;
    }

    // Success ping detected
    hw_trigger_glitch_success_set_high();
    timeout = 10;
    BUSY_LOOP(glitch_success_trigger, timeout);
    hw_trigger_glitch_success_set_low();
    return glitch_success;
}

