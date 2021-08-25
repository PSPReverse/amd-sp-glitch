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

#include "teensy_pins.hpp"

#include "amd_svi2.hpp"

#include "hw.h"

#include "io.h"
#include "prompt.h"
#include "cli.h"

#include "amd_cmds.h"
#include "attack.h"
#include "glitch.h"
#include "restart.h"
#include "ping.h"

using namespace Teensy;

extern "C" int main(void) {

    io_init();
    hw_init();

    hw_trigger_cli_set_high();

    clear_screen();
    print_str(  "###########################\r\n"
                "# AMD SVI2 Injection Tool #\r\n"
                "###########################\r\n");
    print_str(  "Welcome type \"help\" for help!\r\n");


    cli_module *modules = 0;
    cli_modules_append(modules, attack_module);
    cli_modules_append(modules, glitch_module);
    cli_modules_append(modules, restart_module);
    cli_modules_append(modules, cmd_module);
    cli_modules_append(modules, soc_cmd_module);
    cli_modules_append(modules, core_cmd_module);
    cli_modules_append(modules, hw_module);
    cli_modules_append(modules, ping_module);

    prompt_action action = prompt_action_none;

    while (true) {
        if (action == prompt_action_none) {
            hw_trigger_cli_set_low();
            if (restart_update_status() == dut_running) {
                glitch_process_trigger();
            }
            attack_process_trigger();
            hw_trigger_cli_set_high();
        }

        if (action == prompt_action_execute) {
            unsigned n;
            char * cmd = prompt_get_line(n);
            
            cli_exec(cmd, n, modules);
        }

        action = prompt_handle_input();
    }
}

