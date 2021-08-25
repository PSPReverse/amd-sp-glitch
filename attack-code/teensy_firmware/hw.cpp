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
#include "io.h"

Twi::Master     twi_master;
hardware_config hw = HwCfg1();

void hw_init(hardware_config cfg) {

    hw = cfg;

    hw.reset_pin.set_high();
    hw.reset_pin.Output().apply();

    hw.cs_pin.Input().apply();

    hw.trigger_pin.clear();
    hw.trigger_pin.Output().apply();

    hw.scl_in_pin.Input().apply();
    hw.sda_in_pin.Input().apply();

    twi_master = Twi::Master(
        { .ignore_nacks = true },
        cfg.twi_hardware.open_drain_output_only()
    );
    twi_master.setup();
    twi_master.enable();

}

void hw_deinit() {

    twi_master.disable();

    hw.sda_in_pin.write(Gpio::Config());
    hw.scl_in_pin.write(Gpio::Config());

    hw.trigger_pin.set_low();
    hw.trigger_pin.write(Gpio::Config());

    hw.cs_pin.write(Gpio::Config());

    hw.reset_pin.write(Gpio::Config());
}

bool hw_trigger_cli             = false;
bool hw_trigger_attack          = true;
bool hw_trigger_glitch          = true;
bool hw_trigger_glitch_running  = false;
bool hw_trigger_glitch_success  = false;
bool hw_trigger_glitch_broken   = false;
bool hw_trigger_restart         = true;

cli_param_bool hw_trigger_cli_this              = make_cli_param_bool(hw_trigger_cli,               false);
cli_param_bool hw_trigger_attack_this           = make_cli_param_bool(hw_trigger_attack,            true);
cli_param_bool hw_trigger_glitch_this           = make_cli_param_bool(hw_trigger_glitch,            true);
cli_param_bool hw_trigger_glitch_running_this   = make_cli_param_bool(hw_trigger_glitch_running,    false);
cli_param_bool hw_trigger_glitch_success_this   = make_cli_param_bool(hw_trigger_glitch_success,    false);
cli_param_bool hw_trigger_glitch_broken_this    = make_cli_param_bool(hw_trigger_glitch_broken,     false);
cli_param_bool hw_trigger_restart_this          = make_cli_param_bool(hw_trigger_restart,           true);

cli_param hw_trigger_cli_param                  = make_cli_param_bool_param("trigger_cli",              hw_trigger_cli_desc,            hw_trigger_cli_this,            0);
cli_param hw_trigger_attack_param               = make_cli_param_bool_param("trigger_attack",           hw_trigger_attack_desc,         hw_trigger_attack_this,         &hw_trigger_cli_param);
cli_param hw_trigger_glitch_param               = make_cli_param_bool_param("trigger_glitch",           hw_trigger_glitch_desc,         hw_trigger_glitch_this,         &hw_trigger_attack_param);
cli_param hw_trigger_glitch_running_param       = make_cli_param_bool_param("trigger_glitch_running",   hw_trigger_glitch_running_desc, hw_trigger_glitch_running_this, &hw_trigger_glitch_param);
cli_param hw_trigger_glitch_success_param       = make_cli_param_bool_param("trigger_glitch_success",   hw_trigger_glitch_success_desc, hw_trigger_glitch_success_this, &hw_trigger_glitch_running_param);
cli_param hw_trigger_glitch_broken_param        = make_cli_param_bool_param("trigger_glitch_broken",    hw_trigger_glitch_broken_desc,  hw_trigger_glitch_broken_this,  &hw_trigger_glitch_success_param);
cli_param hw_trigger_restart_param              = make_cli_param_bool_param("trigger_restart",          hw_trigger_restart_desc,        hw_trigger_restart_this,        &hw_trigger_glitch_broken_param);

void hw_trigger_cli_set_high() {            if (hw_trigger_cli) hw.trigger_pin.set_high(); }
void hw_trigger_cli_set_low() {             if (hw_trigger_cli) hw.trigger_pin.set_low(); }

void hw_trigger_attack_set_high() {         if (hw_trigger_attack) hw.trigger_pin.set_high(); }
void hw_trigger_attack_set_low() {          if (hw_trigger_attack) hw.trigger_pin.set_low(); }

void hw_trigger_glitch_set_high() {         if (hw_trigger_glitch) hw.trigger_pin.set_high(); }
void hw_trigger_glitch_set_low() {          if (hw_trigger_glitch) hw.trigger_pin.set_low(); }

void hw_trigger_glitch_running_set_high() { if (hw_trigger_glitch_running) hw.trigger_pin.set_high(); }
void hw_trigger_glitch_running_set_low() {  if (hw_trigger_glitch_running) hw.trigger_pin.set_low(); }

void hw_trigger_glitch_success_set_high() { if (hw_trigger_glitch_success) hw.trigger_pin.set_high(); }
void hw_trigger_glitch_success_set_low() {  if (hw_trigger_glitch_success) hw.trigger_pin.set_low(); }

void hw_trigger_glitch_broken_set_high() {  if (hw_trigger_glitch_broken) hw.trigger_pin.set_high(); }
void hw_trigger_glitch_broken_set_low() {   if (hw_trigger_glitch_broken) hw.trigger_pin.set_low(); }

void hw_trigger_restart_set_high() {        if (hw_trigger_restart) hw.trigger_pin.set_high(); }
void hw_trigger_restart_set_low() {         if (hw_trigger_restart) hw.trigger_pin.set_low(); }

enum {
    hw_uninited,
    hw_cfg_1,
    hw_cfg_2,
} hw_cfg_status = hw_uninited;


bool hw_cfg_reset(void * pThis) {
    if (hw_cfg_status != hw_uninited)
        hw_deinit();
    hw_init();
    hw_cfg_status = hw_cfg_1;
    return true;
}

bool hw_cfg_set(void * pThis, const char *value, unsigned n) {
    if (str_cmp(value, n, "1", sizeof("1")) == 0) {
        if (hw_cfg_status != hw_cfg_1) {
            if (hw_cfg_status != hw_uninited)
                hw_deinit();
            hw_init(HwCfg1());
            hw_cfg_status = hw_cfg_1;
        }
        return true;
    }
    if (str_cmp(value, n, "2", sizeof("2")) == 0) {
        if (hw_cfg_status != hw_cfg_2) {
            if (hw_cfg_status != hw_uninited)
                hw_deinit();
            hw_init(HwCfg2());
            hw_cfg_status = hw_cfg_2;
        }
        return true;
    }
    println("Error: Couldn't parse value, use 1 or 2!");
    return false;
}

bool hw_cfg_print(void * pThis) {
    switch (hw_cfg_status) {
        case hw_uninited:
            print_str("uninitialized");
            break;
        case hw_cfg_1:
            print_str("config 1 (trig=1, rst=2, cs=3, scl_out=19, sda_out=18, scl_in=20, sda_in=21");
            break;
        case hw_cfg_2:
            print_str("config 2 (trig=11, rst=9, cs=10, scl_out=16, sda_out=17, scl_in=15, sda_in=14");
            break;
        default:
            print_str("unknown (this should never happen)");
            return false;
    }
    return true;
}

cli_param hw_cfg_param = {
    .name           = "config",
    .description    = hw_cfg_desc,
    .pThis          = 0,
    .set            = &hw_cfg_set,
    .reset          = &hw_cfg_reset,
    .print          = &hw_cfg_print,
    .next           = &hw_trigger_restart_param,
};

cli_module hw_module = {
    .name           = "hw",
    .description    = hw_mod_desc,
    .param          = &hw_cfg_param,
    .cmd            = 0,
    .next           = 0,
};


