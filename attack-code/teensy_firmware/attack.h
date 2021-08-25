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

#ifndef ATTACK_H
#define ATTACK_H

#include "hw.h"
#include "cli.h"


constexpr uint32_t DefaultAttackWaits = 20;

void attack_process_trigger();


#define attack_mod_desc \
    "Module for carrying out an attack on a AMD ZenX CPU.\r\n" \
    "The glitch module will be used to carry out the actual attack."
#define attack_cmd_desc \
    "Arms the attack, it will be performed when the next restart is detected."
#define attack_waits_desc \
    "The specified amount of chip-select low-pulses will be waited for,\r\n" \
    "before the glitch will be triggered."

extern cli_module attack_module;


#endif /* ATTACK_H */
