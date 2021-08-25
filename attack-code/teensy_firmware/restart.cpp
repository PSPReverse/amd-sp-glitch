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

#include "hw.h"
#include "prompt.h"
#include "amd_cmds.h"

#include "restart.h"

uint8_t     restart_status      = DefaultRestartStatus;

bool restart_is_off() { return restart_status == dut_off; }

bool        disable_telemetry   = DefaultRestartDisableTelemetry;

uint32_t    restart_wait_off    = DefaultRestartWaitOff;
uint32_t    restart_wait_on     = DefaultRestartWaitOn ;
uint32_t    restart_delay       = DefaultRestartDelay;
uint32_t    restart_reset_len   = DefaultRestartResetLen;



cli_param_u32 restart_reset_len_this    = make_cli_param_u32(restart_reset_len, DefaultRestartResetLen, 0, 0xffffffff);
cli_param_u32 restart_delay_this        = make_cli_param_u32(restart_delay,     DefaultRestartDelay,    0, 0xffffffff);
cli_param_u32 restart_wait_off_this     = make_cli_param_u32(restart_wait_off,  DefaultRestartWaitOff,  0, 0xffffffff);
cli_param_u32 restart_wait_on_this      = make_cli_param_u32(restart_wait_on,   DefaultRestartWaitOn,   0, 0xffffffff);

cli_param restart_reset_len_param       = make_cli_param_u32_param("reset_len", restart_reset_len_desc, restart_reset_len_this, 0);
cli_param restart_delay_param           = make_cli_param_u32_param("delay",     restart_delay_desc,     restart_delay_this,     &restart_reset_len_param);
cli_param restart_wait_off_param        = make_cli_param_u32_param("wait_off",  restart_wait_off_desc,  restart_wait_off_this,  &restart_delay_param);
cli_param restart_wait_on_param         = make_cli_param_u32_param("wait_on",   restart_wait_on_desc,   restart_wait_on_this,   &restart_wait_off_param);

bool restart_disable_telemetry_set(void * pThis, const char *value, unsigned n);
bool restart_disable_telemetry_reset(void * pThis);
bool restart_disable_telemetry_print(void *pThis);

cli_param restart_disable_telemetry_param = {
    .name           = "disable_telemetry",
    .description    = restart_disable_telemetry_desc,
    .pThis          = 0,
    .set            = restart_disable_telemetry_set,
    .reset          = restart_disable_telemetry_reset,
    .print          = restart_disable_telemetry_print,
    .next           = &restart_wait_on_param,
};

bool restart_detect_set(void * pThis, const char *value, unsigned n);
bool restart_detect_reset(void * pThis);
bool restart_detect_print(void *pThis);

cli_param restart_detect_param = {
    .name           = "detect",
    .description    = restart_detect_desc,
    .pThis          = 0,
    .set            = restart_detect_set,
    .reset          = restart_detect_reset,
    .print          = restart_detect_print,
    .next           = &restart_disable_telemetry_param,
};


bool restart_reset(void *) {
    println("Resetting target!");

    uint32_t timeout = restart_reset_len;
    hw.reset_pin.set_low();
    BUSY_LOOP(reset, timeout);
    hw.reset_pin.set_high();

    return true;
}

cli_command restart_reset_cmd = {
    .name           = "reset",
    .description    = restart_reset_cmd_desc,
    .pThis          = 0,
    .exec           = &restart_reset,
    .next           = 0,
};


bool do_restart(void *) {
    println("Manual restart!");
    // TODO maybe reset here
    return restart();
}

cli_command restart_cmd = {
    .name           = "",
    .description    = restart_cmd_desc,
    .pThis          = 0,
    .exec           = &do_restart,
    .next           = &restart_reset_cmd,
};

cli_module restart_module = {
    .name           = "restart",
    .description    = restart_mod_desc,
    .param          = &restart_detect_param,
    .cmd            = &restart_cmd,
    .next           = 0,
};


uint8_t restart_update_status() {

    if (restart_status == restart_detection_off)
        return restart_detection_off;


    if (restart_status == dut_running) {

        uint32_t timeout = restart_wait_off;
        BUSY_LOOP_WHILE_PIN_LOW(wait_off, timeout, hw.sda_in_pin);
        if (timeout)
            // sda was low only shortly
            return dut_running;

            // sda was low for long enough
        prompt_use_new_line();
        println("Target is now offline!");

        restart_status = dut_off;
        return dut_off;
    }

    if (restart_status == dut_off) {

        uint32_t timeout = restart_wait_on;
        BUSY_LOOP_WHILE_PIN_HIGH(wait_on, timeout, hw.sda_in_pin);
        if (timeout)
            // sda was high only shortly
            return dut_off;

        // sda was high for long enough
        prompt_use_new_line();
        println("Restart detected!");
        restart_status = dut_running;

        // wait delay
        hw_trigger_restart_set_high();
        timeout = restart_delay;
        BUSY_LOOP(restart_delay, timeout);
        hw_trigger_restart_set_low();

        restart();

        return dut_running;
    }

    prompt_use_new_line();
    println("Unkown restart status (this should never happen)!");

    println("Resetting restart status!");
    restart_detect_reset(0);

    return restart_status;
}

void wait_for_free_bus() {

/*

With Telemetry:

              _ tel. packet _              _ tel. packet _
             /               \            /               \
SCL   -------+ +-+ +-+  //-+ +-----//-----+ +-+  //-+ +-+ +------
             +-+ +-+ +-//  +-+            +-+ +-//  +-+ +-+
             \_/   \_/        \__________/
           .2 us  .1 us         33.65 us

*/


    uint32_t timeout;
    uint32_t tries;
    
    // wait for telemetry packet (scl low for at least .1 us)
    // or until it is clear no telemetry packets are send
    // (scl high for more than 70 us)
    tries = rough_busy_wait_us(70);
    do {
        timeout = rough_busy_wait_us_f(.1);
        BUSY_LOOP_WHILE_PIN_LOW(wait_tele, timeout, hw.scl_in_pin);
    } while (tries-- && timeout);

    // wait for free bus (scl high for at least 1 us)
    // or until it is clear scl isn't going high
    // (scl low for more than 70 us)
    tries = rough_busy_wait_us(70);
    do {
        timeout = rough_busy_wait_us(1);
        BUSY_LOOP_WHILE_PIN_HIGH(wait_free, timeout, hw.scl_in_pin);
    } while (tries-- && timeout);
}

bool restart() {
    bool result = true;

    hw_trigger_restart_set_high();


    println("Setting VSoc!");
    wait_for_free_bus();
    if (soc_cmd.send(twi_master, twi_timeout) < 0) {
        result = false;
        println("Error: Problem while sending core_cmd!");
    }

    Command cmd = core_cmd;

    if (disable_telemetry) {
        println("Setting VCore and disabling telemetry!");
        cmd.tfn = true;
    } else {
        println("Setting VCore!");
    }

    wait_for_free_bus();
    if (cmd.send(twi_master, twi_timeout) < 0) {
        result = false;
        println("Error: Problem while sending soc_cmd!");
    }

    hw_trigger_restart_set_low();

    return result;
}

bool restart_disable_telemetry_set(void * pThis, const char *value, unsigned n) {
    if (
        str_cmp(value, n, "true", sizeof("true")) == 0
        || str_cmp(value, n, "yes", sizeof("yes")) == 0
        || str_cmp(value, n, "1", sizeof("1")) == 0
    ) {
        disable_telemetry = true;
        return true;
    }
    if (
        str_cmp(value, n, "false", sizeof("false")) == 0
        || str_cmp(value, n, "no", sizeof("no")) == 0
        || str_cmp(value, n, "0", sizeof("0")) == 0
    ) {
        disable_telemetry = false;
        return true;
    }
    println("Error: Couldn't parse value, use yes/no, true/false or 1/0!");
    return false;
}

bool restart_disable_telemetry_reset(void * pThis) {
    disable_telemetry = DefaultRestartDisableTelemetry;
    return true;
}

bool restart_disable_telemetry_print(void *pThis) {
    if (disable_telemetry)
        print_str("yes");
    else
        print_str("no");
    return true;
}

bool restart_detect_set(void * pThis, const char *value, unsigned n) {
    if (
        str_cmp(value, n, "true", sizeof("true")) == 0
        || str_cmp(value, n, "yes", sizeof("yes")) == 0
        || str_cmp(value, n, "on", sizeof("on")) == 0
        || str_cmp(value, n, "1", sizeof("1")) == 0
    ) {
        if (restart_status == restart_detection_off)
            restart_status = dut_off;
        return true;
    }
    if (
        str_cmp(value, n, "false", sizeof("false")) == 0
        || str_cmp(value, n, "no", sizeof("no")) == 0
        || str_cmp(value, n, "off", sizeof("off")) == 0
        || str_cmp(value, n, "0", sizeof("0")) == 0
    ) {
        restart_status = restart_detection_off;
        return true;
    }
    println("Error: Couldn't parse value, use yes/no, true/false, on/off or 1/0!");
    return false;
}

bool restart_detect_reset(void * pThis) {
    restart_status = DefaultRestartStatus;
    return true;
}

bool restart_detect_print(void * pThis) {
    switch (restart_status) {
        case restart_detection_off:
            print_str("off");
            break;
        case dut_running:
            print_str("on (target is running)");
            break;
        case dut_off:
            print_str("on (target is off)");
            break;
        default:
            print_str("unknown (this should never happen)");
            return false;
    }
    return true;
}



