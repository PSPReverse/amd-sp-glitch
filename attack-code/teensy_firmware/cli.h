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

#ifndef CLI_H
#define CLI_H

#include <stdint.h>

typedef bool (*cli_param_set) (void * pThis, const char * value, unsigned value_n);
typedef bool (*cli_param_reset) (void * pThis);
typedef bool (*cli_param_print) (void * pThis);

typedef struct s_cli_param {
    const char          *name;
    const char          *description;
    void                *pThis;

    cli_param_set       set;
    cli_param_reset     reset;
    cli_param_print     print;

    struct s_cli_param  *next;
} cli_param;

typedef bool (*cli_command_exec) (void * pThis);

typedef struct s_cli_command {
    const char          *name;
    const char          *description;
    void                *pThis;

    cli_command_exec    exec;

    struct s_cli_command *next;
} cli_command;

typedef struct s_cli_module {
    const char          *name;
    const char          *description;

    cli_param           *param;
    cli_command         *cmd;

    struct s_cli_module *next;
} cli_module;

void cli_modules_append(cli_module *&modules, cli_module &next);

bool cli_exec(char * line, unsigned n, cli_module *modules);



typedef bool (*cli_param_set) (void * pThis, const char * value, unsigned value_n);
typedef bool (*cli_param_reset) (void * pThis);
typedef bool (*cli_param_print) (void * pThis);

typedef struct {
    uint32_t    *value;
    uint32_t reset, min, max;
} cli_param_u32;

bool cli_param_u32_set (void * pThis, const char * value, unsigned value_n);
bool cli_param_u32_reset (void * pThis);
bool cli_param_u32_print (void * pThis);

#define make_cli_param_u32(REF, RESET, MIN, MAX) \
{                   \
    .value = &REF,  \
    .reset = RESET, \
    .min = MIN,     \
    .max = MAX,     \
}

#define make_cli_param_u32_param(NAME, DESCRIPTION, THIS, NEXT) \
{                                       \
    .name        = NAME,                \
    .description = DESCRIPTION,         \
    .pThis       = &THIS,               \
    .set         = cli_param_u32_set,   \
    .reset       = cli_param_u32_reset, \
    .print       = cli_param_u32_print, \
    .next        = NEXT,                \
}


typedef struct {
    bool    *value;
    bool reset;
} cli_param_bool;

bool cli_param_bool_set (void * pThis, const char * value, unsigned value_n);
bool cli_param_bool_reset (void * pThis);
bool cli_param_bool_print (void * pThis);

#define make_cli_param_bool(REF, RESET) \
{                   \
    .value = &REF,  \
    .reset = RESET, \
}

#define make_cli_param_bool_param(NAME, DESCRIPTION, THIS, NEXT) \
{                                       \
    .name        = NAME,                \
    .description = DESCRIPTION,         \
    .pThis       = &THIS,               \
    .set         = cli_param_bool_set,   \
    .reset       = cli_param_bool_reset, \
    .print       = cli_param_bool_print, \
    .next        = NEXT,                \
}



cli_module * cli_find_module(const char * mod, unsigned mod_n, cli_module *modules);
cli_command * cli_find_command(const char * cmd, unsigned cmd_n, cli_command *commands);
cli_param * cli_find_param(const char * param, unsigned param_n, cli_param *params);

void cli_list_modules(bool first, cli_module *modules);
void cli_list_commands(bool first, cli_command *commands);
void cli_list_params(bool first, cli_param *params);

bool cli_exec_cmd(char * line, unsigned n, cli_command *command);

bool cli_set(char *line, unsigned n, cli_module *modules);
bool cli_reset(char *line, unsigned n, cli_module *modules);
bool cli_print(char *line, unsigned n, cli_module *modules);
bool cli_help(char *line, unsigned n, cli_module *modules);


#endif /* CLI_H */
