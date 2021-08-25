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

#include "io.h"
#include "cli.h"
#include "ping.h"

uint32_t ping_repeats = 5;

cli_param_u32   ping_repeats_this = make_cli_param_u32(ping_repeats, 5, 1, 10);

cli_param       ping_repeats_param = make_cli_param_u32_param(
    "repeats", "How often to pong back.",
    ping_repeats_this,
    0
);

bool ping(void * pThis) {
    for (uint32_t i = 0; i < ping_repeats; i++)
        println("Pong!");
    return true;
}

cli_command ping_cmd = {
    .name = "",
    .description = "Pongs back!",
    .pThis = 0,
    .exec = &ping,
    .next = 0,
};

cli_module ping_module = {
    .name = "ping",
    .description =
"A small utility to check whether the\r\n"
"device is still responding.",
    .param = &ping_repeats_param,
    .cmd = &ping_cmd,
    .next = 0,
};

