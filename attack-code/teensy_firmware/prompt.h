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

#ifndef PROMPT_H
#define PROMPT_H

#include <stdint.h>

enum prompt_action : uint8_t {
    prompt_action_none,
    prompt_action_execute,
    prompt_action_complete
};

// non-blockingly reads from serial port
//
// Does some minor terminal emulation (echo, backspace, up and down)
//
// If no prompt action is required prompt_action_none is returned.
//
// When a prompt should be executed (enter pressed) prompt_action_execute
// is returned and if a completion is requested (tab pressed)
// prompt_action_complete is returned.
prompt_action prompt_handle_input();

// Get the current prompt text.
//
// The returned string is only valid until the next call to
// prompt_handle_input or prompt_use_new_line.
char * prompt_get_line(unsigned &n);

// Appends a string to the prompt text.
void prompt_line_append(const char *s, unsigned n);

// This function should be called before printing to the serial output
// in between calls to prompt_handle_input.
//
// A fresh line will be used for any prints and the next call to
// prompt_handle_input will also use a fresh line for the prompt.
void prompt_use_new_line();

#endif /* PROMPT_H */
