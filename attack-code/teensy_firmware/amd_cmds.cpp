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

#include "amd_cmds.h"



Command cmd = DefaultCmd;

cmd_param cmd_this = { .pCmd = &cmd, .pDefault = &DefaultCmd };

cli_param cmd_ll        = make_cmd_ll_param(cmd_this, 0);
cli_param cmd_tele      = make_cmd_tele_param(cmd_this, &cmd_ll);
cli_param cmd_offt      = make_cmd_offt_param(cmd_this, &cmd_tele);
cli_param cmd_power     = make_cmd_power_param(cmd_this, &cmd_offt);
cli_param cmd_vid       = make_cmd_vid_param(cmd_this, &cmd_power);
cli_param cmd_soc       = make_cmd_soc_param(cmd_this, &cmd_vid);
cli_param cmd_core      = make_cmd_core_param(cmd_this, &cmd_soc);

cli_module cmd_module = {
    .name           = "cmd",
    .description    = cmd_mod_desc,
    .param          = &cmd_core,
    .cmd            = 0,
    .next           = 0,
};



Command core_cmd = DefaultCoreCmd;

cmd_param core_cmd_this = { .pCmd = &core_cmd, .pDefault = &DefaultCoreCmd };

cli_param core_cmd_ll       = make_cmd_ll_param(core_cmd_this, 0);
cli_param core_cmd_offt     = make_cmd_offt_param(core_cmd_this, &core_cmd_ll);
cli_param core_cmd_power    = make_cmd_power_param(core_cmd_this, &core_cmd_offt);
cli_param core_cmd_vid      = make_cmd_vid_param(core_cmd_this, &core_cmd_power);

cli_module core_cmd_module = {
    .name           = "core_cmd",
    .description    = core_cmd_mod_desc,
    .param          = &core_cmd_vid,
    .cmd            = 0,
    .next           = 0,
};



Command soc_cmd = DefaultSocCmd;

cmd_param soc_cmd_this = { .pCmd = &soc_cmd, .pDefault = &DefaultSocCmd };

cli_param soc_cmd_ll       = make_cmd_ll_param(soc_cmd_this, 0);
cli_param soc_cmd_offt     = make_cmd_offt_param(soc_cmd_this, &soc_cmd_ll);
cli_param soc_cmd_power    = make_cmd_power_param(soc_cmd_this, &soc_cmd_offt);
cli_param soc_cmd_vid      = make_cmd_vid_param(soc_cmd_this, &soc_cmd_power);

cli_module soc_cmd_module = {
    .name           = "soc_cmd",
    .description    = soc_cmd_mod_desc,
    .param          = &soc_cmd_vid,
    .cmd            = 0,
    .next           = 0,
};



#include "io.h"

bool cmd_core_set(void *pThis, const char *value, unsigned n) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    if (
        str_cmp(value, n, "true", sizeof("true")) == 0
        || str_cmp(value, n, "yes", sizeof("yes")) == 0
        || str_cmp(value, n, "on", sizeof("on")) == 0
        || str_cmp(value, n, "1", sizeof("1")) == 0
    ) {
        pCmdParam->pCmd->core = 1;
        return true;
    }
    if (
        str_cmp(value, n, "false", sizeof("false")) == 0
        || str_cmp(value, n, "no", sizeof("no")) == 0
        || str_cmp(value, n, "off", sizeof("off")) == 0
        || str_cmp(value, n, "0", sizeof("0")) == 0
    ) {
        pCmdParam->pCmd->core = 0;
        return true;
    }
    println("Error: Couldn't parse value, use yes/no, true/false, on/off or 1/0!");
    return false;
}

bool cmd_core_reset(void *pThis) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    pCmdParam->pCmd->core = pCmdParam->pDefault->core;
    return true;
}

bool cmd_core_print(void *pThis) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    if (pCmdParam->pCmd->core)
        print_str("true");
    else
        print_str("false");
    return true;
}



bool cmd_soc_set(void *pThis, const char *value, unsigned n) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    if (
        str_cmp(value, n, "true", sizeof("true")) == 0
        || str_cmp(value, n, "yes", sizeof("yes")) == 0
        || str_cmp(value, n, "on", sizeof("on")) == 0
        || str_cmp(value, n, "1", sizeof("1")) == 0
    ) {
        pCmdParam->pCmd->soc = 1;
        return true;
    }
    if (
        str_cmp(value, n, "false", sizeof("false")) == 0
        || str_cmp(value, n, "no", sizeof("no")) == 0
        || str_cmp(value, n, "off", sizeof("off")) == 0
        || str_cmp(value, n, "0", sizeof("0")) == 0
    ) {
        pCmdParam->pCmd->soc = 0;
        return true;
    }
    println("Error: Couldn't parse value, use yes/no, true/false, on/off or 1/0!");
    return false;
}

bool cmd_soc_reset(void *pThis) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    pCmdParam->pCmd->soc = pCmdParam->pDefault->soc;
    return true;
}

bool cmd_soc_print(void *pThis) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    if (pCmdParam->pCmd->soc)
        print_str("true");
    else
        print_str("false");
    return true;
}



bool cmd_vid_set(void *pThis, const char *value, unsigned n) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    unsigned vid;
    if (stou(vid, value, n)) {
        if (SafeVidMax <= vid && vid <= 0xff) {
            pCmdParam->pCmd->vid_code = vid;
            return true;
        }
        print_str("Error: Can't set vid below");
        print_hex_byte(SafeVidMax);
        println(" or above 0xff!");
        return false;
    }
    println("Error: Couldn't parse numeric value!");
    return false;
}

bool cmd_vid_reset(void *pThis) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    pCmdParam->pCmd->vid_code = pCmdParam->pDefault->vid_code;
    return true;
}

bool cmd_vid_print(void *pThis) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    print_hex_byte(pCmdParam->pCmd->vid_code);
    return true;
}



bool cmd_power_set(void *pThis, const char *value, unsigned n) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    uint8_t power_level;
    if (ParsePowerLevel(power_level, value, n)) {
        *pCmdParam->pCmd = pCmdParam->pCmd->PowerLevel(power_level);
        return true;
    }
    println("Error: Couldn't parse value, possibilities are:");
    PrintPowerLevelOptions();
    return false;
}

bool cmd_power_reset(void *pThis) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    pCmdParam->pCmd->psi0_l = pCmdParam->pDefault->psi0_l;
    pCmdParam->pCmd->psi1_l = pCmdParam->pDefault->psi1_l;
    return true;
}

bool cmd_power_print(void *pThis) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    PrintPowerLevel((pCmdParam->pCmd->psi1_l<<1) | pCmdParam->pCmd->psi0_l);
    return true;
}



bool cmd_offt_set(void *pThis, const char *value, unsigned n) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    uint8_t offset;
    if (ParseOffsetTrim(offset, value, n)) {
        pCmdParam->pCmd->offset_trim = offset;
        return true;
    }
    println("Error: Couldn't parse value, possibilities are:");
    PrintOffsetTrimOptions();
    return false;
}

bool cmd_offt_reset(void *pThis) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    pCmdParam->pCmd->offset_trim = pCmdParam->pDefault->offset_trim;
    return true;
}

bool cmd_offt_print(void *pThis) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    PrintOffsetTrim(pCmdParam->pCmd->offset_trim);
    return true;
}



bool cmd_tele_set(void *pThis, const char *value, unsigned n) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    if (
        str_cmp(value, n, "true", sizeof("true")) == 0
        || str_cmp(value, n, "yes", sizeof("yes")) == 0
        || str_cmp(value, n, "on", sizeof("on")) == 0
        || str_cmp(value, n, "1", sizeof("1")) == 0
    ) {
        pCmdParam->pCmd->tfn = 1;
        return true;
    }
    if (
        str_cmp(value, n, "false", sizeof("false")) == 0
        || str_cmp(value, n, "no", sizeof("no")) == 0
        || str_cmp(value, n, "off", sizeof("off")) == 0
        || str_cmp(value, n, "0", sizeof("0")) == 0
    ) {
        pCmdParam->pCmd->tfn = 0;
        return true;
    }
    println("Error: Couldn't parse value, use yes/no, true/false or 1/0!");
    return false;
}

bool cmd_tele_reset(void *pThis) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    pCmdParam->pCmd->tfn = pCmdParam->pDefault->tfn;
    return true;
}

bool cmd_tele_print(void *pThis) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    if (pCmdParam->pCmd->tfn)
        print_str("true");
    else
        print_str("false");
    return true;
}



bool cmd_ll_set(void *pThis, const char *value, unsigned n) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    uint8_t ll_slope_trim;
    if (ParseLoadLineSlopeTrim(ll_slope_trim, value, n)) {
        pCmdParam->pCmd->ll_slope_trim = ll_slope_trim;
        return true;
    }
    println("Error: Couldn't parse value, possibilities are:");
    PrintLoadLineSlopeTrimOptions();
    return false;
}

bool cmd_ll_reset(void *pThis) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    pCmdParam->pCmd->ll_slope_trim = pCmdParam->pDefault->ll_slope_trim;
    return true;
}

bool cmd_ll_print(void *pThis) {
    cmd_param * pCmdParam = (cmd_param*) pThis;
    PrintLoadLineSlopeTrim(pCmdParam->pCmd->ll_slope_trim);
    return true;
}

