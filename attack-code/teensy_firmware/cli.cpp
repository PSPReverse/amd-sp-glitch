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
#include "prompt.h"

#include "cli.h"

void cli_modules_append(cli_module * &modules, cli_module &next) {
    if (!modules) {
        modules = &next;
        return;
    }
    cli_module * last_module = modules;
    while (last_module->next)
            last_module = last_module->next;
    last_module->next = &next;
}

bool cli_exec(char * line, unsigned n, cli_module *modules) {

    char * mod = line;
    unsigned mod_n = get_word(mod, line, n);

    if (mod_n == 0) {
        // allow empty commands
        return true;
    } else if (str_cmp(mod, mod_n, "help", sizeof("help")) == 0) {
        return cli_help(line, n, modules);
    } else if (str_cmp(mod, mod_n, "set", sizeof("set")) == 0) {
        return cli_set(line, n, modules);
    } else if (str_cmp(mod, mod_n, "reset", sizeof("reset")) == 0) {
        return cli_reset(line, n, modules);
    } else if (str_cmp(mod, mod_n, "print", sizeof("print")) == 0) {
        return cli_print(line, n, modules);
    } else {
        cli_module *module = cli_find_module(mod, mod_n, modules);
        if (module)
            return cli_exec_cmd(line, n, module->cmd);
    }

    println("Error: unknown module/command!");
    print_str("Options are: help, set, reset, print");
    cli_list_modules(false, modules);
    println();

    return false;
}

bool cli_exec_cmd(char * line, unsigned n, cli_command *commands) {

    char * cmd = line;
    unsigned cmd_n = get_word(cmd, line, n);

    cli_command * command = cli_find_command(cmd, cmd_n, commands);
    if (command)
        return command->exec(command->pThis);

    println("Error: unknown command for module!");
    print_str("Options are: ");
    cli_list_commands(true, commands);
    println();

    return false;
}

bool cli_set_param(const char *value, unsigned n, cli_module *mod, cli_param *par) {
    if (par->set(par->pThis, value, n))
        return true;
    print_str("Error: There was a problem setting ");
    print_str(mod->name);
    print_char(' ');
    print_str(par->name);
    println("!");
    return false;
}

bool cli_set_module(char * line, unsigned n, cli_module *module) {

    if (n) {
        char * par = line;
        unsigned par_n = get_word(par, line, n);

        cli_param *param = cli_find_param(par, par_n, module->param);

        if (param)
            return cli_set_param(line, n, module, param);

        println("Error: unknown parameter!");
    } else
        println("Error: You need to specify a parameter to set!");
    print_str("Options are: ");
    cli_list_params(true, module->param);
    println();

    return false;
}

bool cli_set(char * line, unsigned n, cli_module *modules) {

    if (n) {
        char * mod = line;
        unsigned mod_n = get_word(mod, line, n);

        cli_module *module = cli_find_module(mod, mod_n, modules);

        if (module)
            return cli_set_module(line, n, module);

        println("Error: unknown module!");
    } else
        println("Error: You need to specify a module!");
    print_str("Options are: ");
    cli_list_modules(true, modules);
    println();

    return false;
}

bool cli_print_param(cli_module *mod, cli_param *par);

bool cli_reset_param(cli_module *mod, cli_param *par) {
    if (par->reset(par->pThis))
        return cli_print_param(mod, par);
    print_str("Error: Couldn't reset ");
    print_str(mod->name);
    print_char(' ');
    print_str(par->name);
    println("!");
    return false;
}

bool cli_reset_module(char * par, unsigned n, cli_module *module) {

    cli_param * params = module->param;

    if (n && *par) {
        cli_param *param = cli_find_param(par, n, params);

        if (param)
            return cli_reset_param(module, param);

        println("Error: unknown parameter!");
        print_str("Options are: <none> (to reset all)");
        cli_list_params(false, params);
        println();

        return false;
    }

    if (!params) {
        println("Nothing to reset, sorry.");
        return true;
    }

    while (params && cli_reset_param(module, params))
        params = params->next;

    return !params;
}

bool cli_reset(char * line, unsigned n, cli_module *modules) {

    if (n && *line) {
        char * mod = line;
        unsigned mod_n = get_word(mod, line, n);

        cli_module *module = cli_find_module(mod, mod_n, modules);

        if (module)
            return cli_reset_module(line, n, module);

        println("Error: unknown module!");
        print_str("Options are: <none> (to reset all)");
        cli_list_modules(false, modules);
        println();

        return false;
    }

    if (!modules) {
        println("Nothing to reset, sorry.");
        return true;
    }

    while (modules && cli_reset_module(0, 0, modules))
        modules = modules->next;

    return !modules;
}

bool cli_print_param(cli_module *mod, cli_param *par) {
    print_str(mod->name);
    print_char(' ');
    print_str(par->name);
    print_char(' ');
    if (par->print(par->pThis)) {
        println();
        return true;
    }
    println("Error: Problem while printing!");
    return false;
}

bool cli_print_module(char * par, unsigned n, cli_module *module) {

    cli_param * params = module->param;

    if (n && *par) {
        cli_param *param = cli_find_param(par, n, params);

        if (param)
            return cli_print_param(module, param);

        println("Error: unknown parameter!");
        print_str("Options are: <none> (to print all)");
        cli_list_params(false, params);
        println();

        return false;
    }

    if (!params) {
        println("Nothing to print, sorry.");
        return true;
    }

    while (params && cli_print_param(module, params))
        params = params->next;

    return !params;
}

bool cli_print(char * line, unsigned n, cli_module *modules) {

    if (n && *line) {
        char * mod = line;
        unsigned mod_n = get_word(mod, line, n);

        cli_module *module = cli_find_module(mod, mod_n, modules);

        if (module)
            return cli_print_module(line, n, module);

        println("Error: unknown module!");
        print_str("Options are: <none> (to print all)");
        cli_list_modules(false, modules);
        println();

        return false;
    }

    if (!modules) {
        println("Nothing to print, sorry.");
        return true;
    }

    while (modules && cli_print_module(0, 0, modules))
        modules = modules->next;

    return !modules;
}

bool cli_help_module(char * line, unsigned n, cli_module *module) {

    print_str("## Module: ");
    print_str(module->name);
    print_str(" #");
    for (unsigned i = str_len(module->name); i < 57; i++)
        print_char('#');
    println();

    cli_param * params = module->param;
    cli_command * commands = module->cmd;

    if (n && *line) {

        cli_param *param = cli_find_param(line, n, params);

        if (param) {
            print_str("\r\n## ");
            print_str(param->name);
            print_str(" (parameter)\r\n  ");
            print_with_indent(2, param->description);
            println();
        }

        cli_command *command = cli_find_command(line, n, commands);

        if (command) {
            print_str("\r\n## ");
            print_str(command->name);
            print_str(" (command)\r\n  ");
            print_with_indent(2, command->description);
            println();
        }

        if (!param && !command) {
            println("Error: no parameter or command!");
            print_str("Options are: <none> (to get help for all)");
            cli_list_params(false, params);
            cli_list_commands(false, commands);
            println();
            return false;
        }

        println();
        return true;
    }

    println();
    println(module->description);

    if (commands) {
        println("\r\n## Commands");

        do {
            print_str("\r\n  ");
            unsigned length;
            if (str_cmp(commands->name, "", 1) == 0) {
                print_str("<empty str>");
                length = sizeof("<empty str>")-1;
            } else {
                print_str(commands->name);
                length = str_len(commands->name);
            }
            if (length > 16)
                print_str("\r\n                    ");
            else
                for (unsigned i = 2 + length; i < 20; i++)
                    print_char(' ');
            print_with_indent(20, commands->description);
            println();

            commands = commands->next;
        } while (commands);
    }

    if (params) {
        println("\r\n## Paramters");

        do {
            print_str("\r\n  ");
            print_str(params->name);
            unsigned length = str_len(params->name);
            if (length > 16)
                print_str("\r\n                    ");
            else
                for (unsigned i = 2 + length; i < 20; i++)
                    print_char(' ');
            print_with_indent(20, params->description);
            println();

            params = params->next;
        } while (params);
    }

    println();
    return true;
}

bool cli_help(char * line, unsigned n, cli_module *modules) {

    bool all = n == 0 || *line == 0;

    char * mod = line;
    unsigned mod_n = get_word(mod, line, n);

    if (all
        || str_cmp(mod, mod_n, "set", sizeof("set")) == 0
        || str_cmp(mod, mod_n, "reset", sizeof("reset")) == 0
        || str_cmp(mod, mod_n, "print", sizeof("print")) == 0
    ) {
        println(
            "\r\n"
            "## set/reset/print ###################################################\r\n"
            "\r\n"
            "Command for the control of parameters.\r\n"
            "\r\n"
            "## Commands\r\n"
            "\r\n"
            "  set <module> <param> <value>\r\n"
            "                    Sets a parameter to a value.\r\n"
            "\r\n"
            "  reset [<module> [<param>]]\r\n"
            "                    Resets parameters to their inital value. A\r\n"
            "                    single parameter to be reset can either be\r\n"
            "                    specified, all parameters of a module can be\r\n"
            "                    reset or all parameters of all modules.\r\n"
            "\r\n"
            "  print [<module> [<param>]]\r\n"
            "                    Prints the current values of parameters.\r\n"
            "                    If no paramter is given, all parameter of\r\n"
            "                    the module are printed and if no module is\r\n"
            "                    given, all parameters are printed.\r\n"
        );

        if (!all) {
            println();
            return true;
        }

        while (modules && cli_help_module(0, 0, modules))
            modules = modules->next;

        if (modules)
            return false;

        println();
        return true;
    }

    // help for a single module
    cli_module *module = cli_find_module(mod, mod_n, modules);

    if (module)
        return cli_help_module(line, n, module);

    println("Error: unknown command/module!");
    print_str("Options are: <none> (to get help for all), set, reset, print");
    cli_list_modules(false, modules);
    println();

    return false;
}




cli_module * cli_find_module(const char *mod, unsigned mod_n, cli_module *modules) {
    while (modules) {
        if (str_cmp(mod, modules->name, mod_n) == 0)
            return modules;
        modules = modules->next;
    }
    return 0;
}

cli_command * cli_find_command(const char *cmd, unsigned cmd_n, cli_command *commands) {
    while (commands) {
        if (str_cmp(cmd, commands->name, cmd_n) == 0)
            return commands;
        commands = commands->next;
    }
    return 0;
}

cli_param * cli_find_param(const char *par, unsigned par_n, cli_param *params) {
    while (params) {
        if (str_cmp(par, params->name, par_n) == 0)
            return params;
        params = params->next;
    }
    return 0;
}

void cli_list_modules(bool first, cli_module *modules) {
    while (modules) {
        if (first)          first = false;
        else if (modules->next)  print_str(", ");
        else                print_str(" and ");

        if (str_cmp(modules->name, "", 1) == 0)
            print_str("<empty str>");
        else
            print_str(modules->name);

        modules = modules->next;
    }
}

void cli_list_commands(bool first, cli_command *commands) {
    while (commands) {
        if (first)          first = false;
        else if (commands->next) print_str(", ");
        else                print_str(" and ");

        if (str_cmp(commands->name, "", 1) == 0)
            print_str("<empty str>");
        else
            print_str(commands->name);

        commands = commands->next;
    }
}

void cli_list_params(bool first, cli_param *params) {
    while (params) {
        if (first)          first = false;
        else if (params->next)   print_str(", ");
        else                print_str(" and ");

        if (str_cmp(params->name, "", 1) == 0)
            print_str("<empty str>");
        else
            print_str(params->name);

        params = params->next;
    }
}






bool cli_param_u32_set(void * pThis, const char * value, unsigned value_n) {
    cli_param_u32 This = *(cli_param_u32*) pThis;
    unsigned value_parsed;
    if (stou(value_parsed, value, value_n)) {
        if (This.min <= value_parsed && value_parsed <= This.max) {
            *This.value = value_parsed;
            return true;
        }
        println("Error: Value out of range!");
        print_str("Allowed range is from ");
        print_hex_int(This.min);
        print_str(" to ");
        print_hex_int(This.max);
        println(".");
        return false;
    }
    println("Error: Couldn't parse numeric value!");
    return false;
}

bool cli_param_u32_reset (void * pThis) {
    cli_param_u32 This = *(cli_param_u32*) pThis;
    *This.value = This.reset;
    return true;
}

bool cli_param_u32_print (void * pThis) {
    cli_param_u32 This = *(cli_param_u32*) pThis;
    print_hex_int(*This.value);
    return true;
}






bool cli_param_bool_set(void * pThis, const char * value, unsigned value_n) {
    cli_param_bool This = *(cli_param_bool*) pThis;
    if (
        str_cmp(value, value_n, "true", sizeof("true")) == 0
        || str_cmp(value, value_n, "yes", sizeof("yes")) == 0
        || str_cmp(value, value_n, "on", sizeof("on")) == 0
        || str_cmp(value, value_n, "1", sizeof("1")) == 0
    ) {
        *This.value = true;
        return true;
    }
    if (
        str_cmp(value, value_n, "false", sizeof("false")) == 0
        || str_cmp(value, value_n, "no", sizeof("no")) == 0
        || str_cmp(value, value_n, "off", sizeof("off")) == 0
        || str_cmp(value, value_n, "0", sizeof("0")) == 0
    ) {
        *This.value = false;
        return true;
    }
    println("Error: Couldn't parse boolean, use true/false, yes/no, on/off or 1/0!");
    return false;
}

bool cli_param_bool_reset (void * pThis) {
    cli_param_bool This = *(cli_param_bool*) pThis;
    *This.value = This.reset;
    return true;
}

bool cli_param_bool_print (void * pThis) {
    cli_param_bool This = *(cli_param_bool*) pThis;
    print_str(*This.value ? "true" : "false");
    return true;
}

