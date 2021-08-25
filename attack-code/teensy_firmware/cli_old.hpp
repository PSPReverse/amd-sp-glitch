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

#ifndef CLI_HPP
#define CLI_HPP

#include "amd_svi2.hpp"

using namespace AmdSvi2;

//////////////////
// Reset Values
//////////////////

constexpr bool DefaultRestartDetect = true;
constexpr bool DefaultRestartDisableTelemetry = true;

constexpr uint32_t DefaultTimeout = 60000000; // at least one second

constexpr uint32_t DefaultGlitchDelayUs     = 1000;
constexpr uint32_t DefaultGlitchTimeUs      =   30;
constexpr uint8_t  DefaultGlitchVid         = 0x90;

constexpr uint32_t DefaultGlitchRepeats     =   10;
constexpr uint32_t DefaultGlitchCooldownUs  =  300;

constexpr uint32_t DefaultGlitchResultWait1 = 100;
constexpr uint32_t DefaultGlitchResultWait2 = 30;


void restart();
bool glitch();
void glitch(const char * module, unsigned module_n);
void inject();
void receive();

#endif /* CLI_HPP */
