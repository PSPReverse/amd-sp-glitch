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

#ifndef AMD_CMDS_H
#define AMD_CMDS_H

#include "cli.h"

#include "amd_svi2.hpp"
using namespace AmdSvi2;




// Vids are restricted to between this value and 0xff
constexpr uint8_t   SafeVidMax = 0x50; // 1.05 Volt




#define cmd_mod_desc \
"The amd svi2 cmd/packet that is sent with the \"inject\" command."

extern cli_module   cmd_module;
constexpr Command   DefaultCmd = Command();
extern Command      cmd;




#define core_cmd_mod_desc \
"An amd svi2 cmd/packet whose purpose it is to set the \"normal\" VCore\r\n" \
"state of the voltage regulator. It is sent when a restart has been detected\r\n" \
"or after a glitch with the \"glitch\" command."

extern cli_module   core_cmd_module;
constexpr uint8_t   CoreBootVid = 0x59; // core vid at boot
constexpr Command   DefaultCoreCmd = DefaultCmd.Core().Vid(CoreBootVid);
extern Command      core_cmd;




#define soc_cmd_mod_desc \
"An amd svi2 cmd/packet whose purpose it is to set the \"normal\" VSoc state\r\n" \
"of the voltage regulator. It is sent when a restart has been detected or\r\n" \
"after a glitch with the \"glitch\" command.\r\n" \
"If \"restart disable-telemetry\" is enabled, this packet will also be used\r\n" \
"to disable the voltage regulators telemetry functionality."

extern cli_module   soc_cmd_module;
constexpr uint8_t   SocBootVid = 0x60; // soc vid at boot
constexpr Command   DefaultSocCmd = DefaultCmd.Soc().Vid(SocBootVid);
extern Command      soc_cmd;




// Command struct as cli parameters
typedef struct {
    Command         *pCmd;
    const Command   *pDefault;
} cmd_param;



#define cmd_core_name \
"set_core"
#define cmd_core_desc \
"Wether the VCore voltage will be set by this command.\r\n" \
"Can be \"true\" or \"false\", which is the default."

bool cmd_core_set(void *pThis, const char *value, unsigned n);
bool cmd_core_reset(void *pThis);
bool cmd_core_print(void *pThis);

#define make_cmd_core_param(THIS, NEXT) \
{                                       \
    .name           = cmd_core_name,    \
    .description    = cmd_core_desc,    \
    .pThis          = &THIS,            \
    .set            = cmd_core_set,     \
    .reset          = cmd_core_reset,   \
    .print          = cmd_core_print,   \
    .next           = NEXT,             \
}



#define cmd_soc_name \
"set_soc"
#define cmd_soc_desc \
"Wether the VSoc voltage will be set by this command.\r\n" \
"Can be \"true\" or \"false\", which is the default."

bool cmd_soc_set(void *pThis, const char *value, unsigned n);
bool cmd_soc_reset(void *pThis);
bool cmd_soc_print(void *pThis);

#define make_cmd_soc_param(THIS, NEXT)  \
{                                       \
    .name           = cmd_soc_name,     \
    .description    = cmd_soc_desc,     \
    .pThis          = &THIS,            \
    .set            = cmd_soc_set,      \
    .reset          = cmd_soc_reset,    \
    .print          = cmd_soc_print,    \
    .next           = NEXT,             \
}



#define cmd_vid_name \
"vid"
#define cmd_vid_desc \
"The voltage level to set. Each vid step corresponds to a\r\n" \
"-0.00625 Volt step. A vid of 0 corresponds to a voltage of\r\n" \
"1.55 Volts and vids above 248 completely turn of the\r\n" \
"voltages.\r\n" \
"Note: This value might be protected from being set too high."

bool cmd_vid_set(void *pThis, const char *value, unsigned n);
bool cmd_vid_reset(void *pThis);
bool cmd_vid_print(void *pThis);

#define make_cmd_vid_param(THIS, NEXT)  \
{                                       \
    .name           = cmd_vid_name,     \
    .description    = cmd_vid_desc,     \
    .pThis          = &THIS,            \
    .set            = cmd_vid_set,      \
    .reset          = cmd_vid_reset,    \
    .print          = cmd_vid_print,    \
    .next           = NEXT,             \
}



#define cmd_power_name \
"power"
#define cmd_power_desc \
"Controls the low-power states. Possible values are:\r\n" \
"low, mid, full_alt and full.\r\n" \
"In full and full_alt all mosfets drive the bus, in\r\n" \
"mid only a single phase is used and in low a diode\r\n" \
"emulation mode is entered."

bool cmd_power_set(void *pThis, const char *value, unsigned n);
bool cmd_power_reset(void *pThis);
bool cmd_power_print(void *pThis);

#define make_cmd_power_param(THIS, NEXT)    \
{                                           \
    .name           = cmd_power_name,       \
    .description    = cmd_power_desc,       \
    .pThis          = &THIS,                \
    .set            = cmd_power_set,        \
    .reset          = cmd_power_reset,      \
    .print          = cmd_power_print,      \
    .next           = NEXT,                 \
}



#define cmd_offt_name \
"offset_trim"
#define cmd_offt_desc \
"Gives finer control over the voltages.\r\n" \
"Possible value are: off, no_change, +25mV and -25mV."

bool cmd_offt_set(void *pThis, const char *value, unsigned n);
bool cmd_offt_reset(void *pThis);
bool cmd_offt_print(void *pThis);

#define make_cmd_offt_param(THIS, NEXT) \
{                                       \
    .name           = cmd_offt_name,    \
    .description    = cmd_offt_desc,    \
    .pThis          = &THIS,            \
    .set            = cmd_offt_set,     \
    .reset          = cmd_offt_reset,   \
    .print          = cmd_offt_print,   \
    .next           = NEXT,             \
}



#define cmd_tele_name \
"set_telemetry"
#define cmd_tele_desc \
"Controls the telemetry function of the voltage regulator.\r\n" \
"The soc and core bits are used in conjunction with this bit\r\n" \
"to control what data is sent:\r\n" \
"  tele.  core  soc\r\n" \
"    1      0     0    voltages\r\n" \
"    1      0     1    voltages and currents\r\n" \
"    1      1     0    nothing\r\n" \
"    1      1     0    reserved"

bool cmd_tele_set(void *pThis, const char *value, unsigned n);
bool cmd_tele_reset(void *pThis);
bool cmd_tele_print(void *pThis);

#define make_cmd_tele_param(THIS, NEXT) \
{                                       \
    .name           = cmd_tele_name,    \
    .description    = cmd_tele_desc,    \
    .pThis          = &THIS,            \
    .set            = cmd_tele_set,     \
    .reset          = cmd_tele_reset,   \
    .print          = cmd_tele_print,   \
    .next           = NEXT,             \
}



#define cmd_ll_name \
"load_line"
#define cmd_ll_desc \
"Controls the slope of the load line. When the cpu draws\r\n" \
"higher currents the supply voltages drop, this is the\r\n" \
"slope of the load line. The units are % mOhm and the\r\n" \
"Possible values are: off, no_change, -40%, -20%, +20%,\r\n" \
"+40%, +60% and +80%."

bool cmd_ll_set(void *pThis, const char *value, unsigned n);
bool cmd_ll_reset(void *pThis);
bool cmd_ll_print(void *pThis);

#define make_cmd_ll_param(THIS, NEXT)   \
{                                       \
    .name           = cmd_ll_name,      \
    .description    = cmd_ll_desc,      \
    .pThis          = &THIS,            \
    .set            = cmd_ll_set,       \
    .reset          = cmd_ll_reset,     \
    .print          = cmd_ll_print,     \
    .next           = NEXT,             \
}

#endif /* AMD_CMDS_H */
