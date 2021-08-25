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

#include "cli_old.hpp"
#include "hw.h"

#include <core_pins.h>

#include "teensy_twi.hpp"

using namespace AmdSvi2;
using namespace Teensy;

extern Command cmd;
extern Command core_cmd;
extern Command soc_cmd;

bool restart_detect = DefaultRestartDetect;
bool restart_disable_telemetry = DefaultRestartDisableTelemetry;

bool restart_detected           = false;
bool restart_is_off             = false;
uint32_t restart_clk_low_since  = 0;

uint32_t telemetry_timeout = DefaultTimeout;
uint32_t receive_timeout = DefaultTimeout;
uint32_t injection_timeout = DefaultTimeout;

bool     glitch_armed       = false;
uint32_t glitch_delay_us    = DefaultGlitchDelayUs;
uint32_t glitch_time_us     = DefaultGlitchTimeUs;
uint8_t  glitch_vid         = DefaultGlitchVid;

uint32_t glitch_repeats     = DefaultGlitchRepeats;
uint32_t glitch_cooldown_us = DefaultGlitchCooldownUs;
uint32_t glitch_wait1       = DefaultGlitchResultWait1;
uint32_t glitch_wait2       = DefaultGlitchResultWait2;

void cli_init() {

    io_init();
    AmdSvi2::init();

    print_str(
        "\x1b" "[2J" // clears the terminal
        "###########################\r\n"
        "# AMD SVI2 Injection Tool #\r\n"
        "###########################\r\n"
        "Welcome type \"help\" for help!\r\n"
    );

    cli_status_pin.clear();
    glitch_status_pin.clear();
    restart_status_pin.clear();

    cli_status_pin.Output().apply();
    glitch_status_pin.Output().apply();
    restart_status_pin.Output().apply();
    cs_pin.Input().apply();

}

void process_triggers() {

    bool can_arm = true;

    if (restart_detect) {

        if (restart_is_off) {
            can_arm = false;
            // if clk goes high we have a restart
            scl_pin.Input().apply_temporarily();
            if (scl_pin.is_high()) {
                restart_detected = true;
                restart_is_off = false;
            }
        } else {
            // if clk long low the machine is off
            scl_pin.Input().apply_temporarily();
            if (scl_pin.is_low()) {
                can_arm = false;
                restart_clk_low_since++;
                if (restart_clk_low_since >= 1000) {
                    restart_clk_low_since = 0;
                    restart_is_off = true;
                }
            } else {
                restart_clk_low_since = 0;
            }
        }

        if (restart_detected) {
            can_arm = false;
            if (true) {
                sda_pin.Input().apply_temporarily();
                uint32_t sda_high_for = 0;
                do {
                    // wait until sda is high
                    if (sda_pin.is_high())
                        sda_high_for++;
                    else
                        sda_high_for = 0;
                } while (sda_high_for < 100);
            }

            delay(1);
            restart_detected = false;

            println("\r\nRestart Detected!");
            restart();
            print_str("> ");
            //print_current_line();
        }

    }

    if (can_arm && glitch_armed && cs_pin.is_low()) {
        glitch_armed = false;
        bool successful = glitch();
        println("\r\nGlitch triggered!");
        if (successful)
            println("Glitch successful!");
        print_str("> ");
        //print_current_line();
    }
}

#define if_str_eq(var, str) \
    if (var ## _n >= sizeof(str) && str_cmp(var, str, sizeof(str)) == 0)

#define if_str_eq_or(var, str, cond) \
    if ((var ## _n >= sizeof(str) && str_cmp(var, str, sizeof(str)) == 0) || cond)

void process_command(char * line, unsigned n) {
    char * cmd, * module;
    unsigned cmd_n = get_word(cmd, line, n);
    unsigned module_n = get_word(module, line, n);

    if_str_eq(cmd, "restart") {
        restart();
        return;
    }

    if_str_eq(cmd, "glitch") {
        glitch(module, module_n);
        return;
    }

    if_str_eq(cmd, "inject") {
        inject();
        return;
    }

    if_str_eq(cmd, "receive") {
        receive();
        return;
    }

    println("Error: Unknown command, type \"help\" for help!");
}

bool set(const char* module, unsigned module_n,
        const char* param, unsigned param_n,
        const char* value, unsigned value_n) {
    if_str_eq(module, "timeout") {
        uint32_t *timeout;
        if_str_eq(param, "telemetry") timeout = &telemetry_timeout;
        else if_str_eq(param, "receive") timeout = &receive_timeout;
        else if_str_eq(param, "injection") timeout = &injection_timeout;
        else {
            println(
                "Error: Unknown timeout to set!\r\n"
                "Options are: telemetry, receive or injection");
            return false;
        }
        unsigned v;
        if (stou(v, value, value_n)) {
            *timeout = v;
            return true;
        }
        println("Error: Couldn't parse timeout value!");
        return false;
    }
    if_str_eq(module, "restart") {
        bool *flag;
        if_str_eq(param, "detect") flag = &restart_detect;
        else if_str_eq(param, "disable_telemetry") flag = &restart_disable_telemetry;
        else {
            println(
                "Error: Unknown option to set!\r\n"
                "Options are: detect and disable_telemetry");
            return false;
        }
        unsigned v;
        if (stou(v, value, value_n)) {
            if (v <= 1) {
                *flag = v != 0;
                return true;
            }
            println("Error: Value too large, must be either 0 or 1!");
            return false;
        }
        println("Error: Couldn't parse value!");
        return false;
    }
    if_str_eq(module, "glitch") {
        if_str_eq(param, "vid") {
            unsigned vid;
            if (stou(vid, value, value_n)) {
                if (0 <= vid && vid <= 0xff) {
                    glitch_vid = vid;
                    return true;
                }
                print_str("Error: Value too large, must be between ");
                println(" and 0xff!");
                return false;
            }
            println("Error: Couldn't parse value!");
            return false;
        }
        uint32_t *p;
        if_str_eq(param, "delay_us") p = &glitch_delay_us;
        else if_str_eq(param, "time_us") p = &glitch_time_us;
        else if_str_eq(param, "repeats") p = &glitch_repeats;
        else if_str_eq(param, "cooldown_us") p = &glitch_cooldown_us;
        else if_str_eq(param, "wait1") p = &glitch_wait1;
        else if_str_eq(param, "wait2") p = &glitch_wait2;
        else {
            println(
                "Error: Unknown parameter!\r\n"
                "Options are: vid, delay_us, time_us, repeats,\r\n"
                "             cooldown_us, wait1 and wait2");
            return false;
        }
        unsigned v;
        if (stou(v, value, value_n)) {
            *p = v;
            return true;
        }
        println("Error: Couldn't parse value!");
        return false;
    }
    println("Error: Unknown module!\r\n"
            "Options are:\r\n"
            "       cmd, core_cmd, soc_cmd, timeout,\r\n"
            "       restart and glitch");
    return false;
}


bool reset(const char* module, unsigned module_n) {
    if_str_eq(module, "timeout") {
        telemetry_timeout = DefaultTimeout;
        receive_timeout = DefaultTimeout;
        injection_timeout = DefaultTimeout;
        return true;
    }
    if_str_eq(module, "restart") {
        restart_detect = DefaultRestartDetect;
        restart_disable_telemetry = DefaultRestartDisableTelemetry;
        return true;
    }
    if_str_eq(module, "glitch") {
        glitch_delay_us    = DefaultGlitchDelayUs;
        glitch_time_us     = DefaultGlitchTimeUs;
        glitch_vid         = DefaultGlitchVid;
        glitch_repeats     = DefaultGlitchRepeats;
        glitch_cooldown_us = DefaultGlitchCooldownUs;
        glitch_wait1       = DefaultGlitchResultWait1;
        glitch_wait2       = DefaultGlitchResultWait2;
        return true;
    }
    println("Error: Unknown module!\r\n"
            "Options are:\r\n"
            "       cmd, soc_cmd, core_cmd, timeout,"
            "       restart and glitch");
    return false;
}

void print(const char* module, unsigned module_n) {
    if_str_eq(module, "cmd") {
        print_str("cmd = ");
        cmd.print();
        return;
    }
    if_str_eq(module, "soc_cmd") {
        print_str("soc_cmd = ");
        soc_cmd.print();
        return;
    }
    if_str_eq(module, "core_cmd") {
        print_str("core_cmd = ");
        core_cmd.print();
        return;
    }
    if_str_eq(module, "timeout") {
        print_hex_param("injection", injection_timeout, int);
        print_hex_param("telemetry", telemetry_timeout, int);
        print_hex_param("receive", receive_timeout, int);
        return;
    }
    if_str_eq(module, "restart") {
        print_param("detect", restart_detect, bool);
        print_param("disable_telemetry", restart_disable_telemetry, bool);
        return;
    }
    if_str_eq(module, "glitch") {
        print_hex_param("delay_us", glitch_delay_us, int);
        print_hex_param("time_us" ,glitch_time_us, int);
        print_hex_param("vid", glitch_vid, byte);
        print_hex_param("repeats", glitch_repeats, int);
        print_hex_param("cooldown_us", glitch_cooldown_us, int);
        print_hex_param("wait1", glitch_wait1, int);
        print_hex_param("wait2", glitch_wait2, int);
        return;
    }
    println("Error: Unknown module!\r\n"
            "Options are:\r\n"
            "       cmd, soc_cmd, core_cmd, timeout,"
            "       restart and glitch");
}

// Firstly waits for the bus the become busy and then for the bus to be idle again
void wait_for_non_busy(uint32_t idle_iters = 150, uint32_t busy_iters = 10, uint32_t timeout = 1000) {

    // Use clock in as gpio (in this scope)
    scl_pin.Input().apply_temporarily();

    // signal that we are now waiting
    restart_status_pin.set_high();

    // firstly we wait until there is some activity
    for (uint32_t c = 0; c < busy_iters; c++) {
        if (timeout-- == 0)
            break;
        while (scl_pin.is_high())
            c = 0;
    }

    // now we wait until the activity has stopped
    for (uint32_t c = 0; c < idle_iters; c++)
        while (scl_pin.is_low())
            c = 0;

    // signal that we are finished waiting
    restart_status_pin.set_low();
}

void restart() {
    
    restart_status_pin.set_high();
    delayMicroseconds(1);
    restart_status_pin.set_low();

    print_str("Setting VSoc (sending core_cmd) ... ");

    wait_for_non_busy();
    int rc = soc_cmd.send(injection_timeout);
    if (rc < 0) {
        print_hex_param("Error", rc, int);
        return;
    } else {
        println("Ok!");
    }

    auto timeout = injection_timeout;
    auto cmd = core_cmd;
    if (restart_disable_telemetry) {
        print_str("Setting VCore and disableing telemetry (sending soc_cmd + tfn) ... ");
        cmd = cmd.Telemetry();
        timeout = telemetry_timeout;
    } else {
        print_str("Setting VCore (sending soc_cmd) ... ");
    }

    wait_for_non_busy();
    rc = cmd.send(timeout);

    if (rc < 0) {
        print_hex_param("Error", rc, int);
        return;
    } else {
        println("Ok!");
    }
}

void glitch(const char* param, unsigned param_n) {
    if (param_n == 0) {
        println("Glitch manually triggered!");
        if (glitch())
            println("Glitch successful!");
        return;
    }
    if_str_eq(param, "arm") {
        glitch_armed = true;
        println("Armed the glitch!");
    } else {
        println(
            "Error: Unknown command!\r\n"
            "Options are: arm"
        );
    }
}

bool glitch() {
    glitch_status_pin.set_high();
    Command down = soc_cmd.Vid(glitch_vid), up = soc_cmd;
    glitch_status_pin.set_low();

    delayMicroseconds(glitch_delay_us);

    uint32_t i;
    bool is_down = false;
    int rc = 0; 
    for (i = 0; i < glitch_repeats; i++) {

        rc = down.send(injection_timeout);
        is_down = true;
        glitch_status_pin.set_high();
        if (rc < 0) break;

        delayMicroseconds(glitch_time_us);

        rc = up.send(injection_timeout);
        is_down = false;
        glitch_status_pin.set_low();
        if (rc < 0) break;

        delayMicroseconds(glitch_cooldown_us);
    }

    if (is_down) {
        print_str("Error: Left in low voltage state!\r\n"
                "Setting Voltage to normal level again ... ");

        int rc1 = up.send(injection_timeout);
        is_down = false;

        if (rc1 < 0) {
            print_str("Error (rc = ");
            print_hex_int(rc1);
            println(")");
        } else {
            println("Ok!");
        }
    }

    if (rc < 0) {
        print_str("Error: Problem in glitch round ");
        print_hex_int(i);
        print_str(" (rc = ");
        print_hex_int(rc);
        println(")");

        return false;
    }

    uint32_t timeout = glitch_wait1;
    glitch_status_pin.set_high();
    while (cs_pin.is_high()) {
        if (timeout-- == 0) {
            println("Target broken!");
            glitch_status_pin.set_low();
            return true;
        }
    }

    timeout = glitch_wait1;
    while (cs_pin.is_low()) {
        if (timeout-- == 0) {
            println("Target powered down?");
            glitch_status_pin.set_low();
            return true;
        }
    }

    timeout = glitch_wait2;
    while (cs_pin.is_high()) {
        if (timeout-- == 0) {
            println("Target running!");
            glitch_status_pin.set_low();
            return true;
        }
    }

    println("Target errored!");
    glitch_status_pin.set_low();
    return true;
}

void inject() {
    print_str("Sending cmd ... ");
    int rc = soc_cmd.send(injection_timeout);
    if (rc < 0) {
        print_hex_param("Error", rc, int);
        return;
    } else {
        println("Ok!");
    }
}

void receive() {
    print_str("Receiving cmd ... ");
    Command cmd;
    int rc = cmd.receive(receive_timeout);
    if (rc < 0) {
        print_hex_param("Error", rc, int);
        return;
    } else {
        println("Ok!");
    }
    cmd.print();
}

void help(const char* module, unsigned module_n) {

    bool all = false;
    if_str_eq_or(module, "all", (module_n == 0)) {
        all = true;
        println(
            "Printing module descriptions.\r\n"
            "\r\n"
            "Use \"help <module>\" for more information.\r\n"
        );
    }

    if (
        (module_n <= 4 && str_cmp(module, "set", 4) == 0)
        | (module_n <= 6 && str_cmp(module, "reset", 6) == 0)
        | (module_n <= 6 && str_cmp(module, "print", 6) == 0)
        | all
    ) {
        println(    
            "## set, reset, print #####\r\n"
            "       Control parameters.\r\n"
        );
        if (!all) {
            println(    
            "  Commands\r\n"
            "\r\n"
            "    set <module> <param> <value>\r\n"
            "\r\n"
            "       Set a parameter.\r\n"
            "\r\n"
            "    print <module>\r\n"
            "\r\n"
            "       Prints all parameters of a module.\r\n"
            "\r\n"
            "    reset <module>\r\n"
            "\r\n"
            "       Resets all parameters of a module.\r\n"
            );
            return;
        }
    }
    if_str_eq_or(module, "cmd", all) {
        println(    
            "## cmd #####\r\n"
            "       The amd svi2 cmd/packet that is sent with the \"inject\"\r\n"
            "       command.\r\n"
        );
        if (!all) {
            return;
        }
    }
    if_str_eq_or(module, "soc_cmd", all) {
        println(    
            "## soc_cmd #####\r\n"
            "       An amd svi2 cmd/packet that is sent when a restart\r\n"
            "       has been detected or after a glitch with the \"glitch\"\r\n"
            "       command.\r\n"
            "         Note: It is not possible to set the parameters soc,\r\n"
            "       core and telemetry.\r\n"
        );
        if (!all) {
            return;
        }
    }
    if_str_eq_or(module, "core_cmd", all) {
        println(    
            "## core_cmd #####\r\n"
            "       An amd svi2 cmd/packet that is sent when a restart\r\n"
            "       has been detected or after a glitch with the \"glitch\"\r\n"
            "       command.\r\n"
            "         Note: It is not possible to set the parameters soc,\r\n"
            "       core and telemetry.\r\n"
        );
        if (!all) {
            return;
        }
    }
    if_str_eq_or(module, "timeout", all) {
        println(    
            "## timeout #####\r\n"
            "       Controls various timeouts for receiving of injecting packates.\r\n"
            "       Each timout is roughly measuring cycles of the teensys processor.\r\n"
        );
        if (!all) {
            println(    
            "  Parameters\r\n"
            "\r\n"
            "    injection  u32     Timeout for any injection operation.\r\n"
            "\r\n"
            "    telemetry  u32     Overwrites the injection packet timeout for the\r\n"
            "                       packet that disables the telemetry function.\r\n"
            "\r\n"
            "    receive    u32     Timeout for the \"receive\" command.\r\n"
            );
            return;
        }
    }
    if_str_eq_or(module, "restart", all) {
        println(    
            "## restart #####\r\n"
            "       The restart detection and telemetry disabeling function.\r\n"
            "       When a restart is detected (or manually triggered) the\r\n"
            "       the voltages are set with the core_cmd and soc_cmd packets\r\n"
            "       and -- if enabled -- the telemetry function is disabled.\r\n"
        );
        if (!all) {
            println(    
            "  Commands\r\n"
            "    restart    Manually triggers the restart detection.\r\n"
            "\r\n"
            "  Parameters\r\n"
            "\r\n"
            "    detect             0-1     En-/Disable the restart detection.\r\n"
            "\r\n"
            "    disable_telemetry  0-1     Overwrites the injection packet timeout for the\r\n"
            );
            return;
        }
    }
    if_str_eq_or(module, "glitch", all) {
        println(    
            "## glitch #####\r\n"
            "       Unimplemented!\r\n"
        );
        if (!all) {
            //println(    
            //"\r\n"
            //);
            return;
        }
    }
    if_str_eq_or(module, "inject", all) {
        println(    
            "## inject #####\r\n"
            "       Inject a cmd/packet onto the AMD Svi2 Bus.\r\n"
        );
        if (!all) {
            println(    
            "  Commands\r\n"
            "    inject     Write the `cmd` packet onto the AMD Svi2 bus.\r\n"
            "\r\n"
            );
            return;
        }
    }
    if_str_eq_or(module, "receive", all) {
        println(    
            "## receive #####\r\n"
            "       Receive packets from the AMD Svi2 Bus.\r\n"
        );
        if (!all) {
            println(    
            "  Commands\r\n"
            "    receive    Receive and print the next packet on the Bus.\r\n"
            "\r\n"
            );
            return;
        }
    }

    if (all)
        return;

    println("Error: Unknown module!\r\n"
            "Options are:\r\n"
            "       set, reset, print, cmd, soc_cmd,\r\n"
            "       core_cmd, timeout, restart, glitch,\r\n"
            "       inject and receive\r\n"
    );
}

void help_cmd_parameters() {
    println(
        "  Parameters\r\n"
        "\r\n"
        "    soc        0-1     Set the VSoc voltage.\r\n"
        "\r\n"
        "    core       0-1     Set the VCore voltage.\r\n"
        "\r\n"
        "    vid        0-255   The voltage level to set. Each vid step\r\n"
        "                       corresponds to a -0.00625 Volt step.\r\n"
        "                       A vid of 0 corresponds to a voltage of\r\n"
        "                       1.55 Volts and vids above 248 completely\r\n"
        "                       turn of the voltages.\r\n"
        "                         Note: This value might be protected from\r\n"
        "                       being set too high.\r\n"
        "\r\n"
        "    power      str     Controls the low-power states.\r\n"
        "                       Possible values are: low, mid\r\n"
        "                       full_alt and full.\r\n"
        "                       In full and full_alt all mosfets\r\n"
        "                       drive the bus, in mid only a single\r\n"
        "                       phase is used and in low a diode\r\n"
        "                       emulation mode is entered.\r\n"
        "\r\n"
        "    offset     str     Gives finer control over the voltages.\r\n"
        "                       Possible value are: off, no_change,\r\n"
        "                       +25mV and -25mV.\r\n"
        "\r\n"
        "    telemetry  0-1     Controls the telemetry function of the\r\n"
        "                       voltage regulator. The soc and core bits\r\n"
        "                       are used in conjunction with this bit to:\r\n"
        "                       control what data is sent:\r\n"
        "                         tele.  core  soc\r\n"
        "                         1      0     0    voltages\r\n"
        "                         1      0     1    voltages and currents\r\n"
        "                         1      1     0    nothing\r\n"
        "                         1      1     0    reserved\r\n"
        "\r\n"
        "    load_line  str     Controls the slope of the load line.\r\n"
        "                       When the cpu draw higher currents the\r\n"
        "                       supply voltages drop, this is the slope\r\n"
        "                       of the load line. The units are % mOhm.\r\n"
        "                       Possible values are: off, no_change,\r\n"
        "                       -40%, -20%, +20%, +40%, +60% and +80%\r\n"
    );
}


