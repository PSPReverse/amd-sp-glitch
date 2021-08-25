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


#include "prompt.h"

#include "io.h"

#ifndef LINE_COUNT
#   define LINE_COUNT 128
#endif
#ifndef LINE_SIZE
#   define LINE_SIZE 1024
#endif


// Whether the output is currently controlled by the prompt module
bool prompt_active = false;


// OUTPUT

void move_cursor_backward(unsigned n) {
    for (; n > 0; n--)
        print_str("\x1b" "[D");
}
void move_cursor_forward(unsigned n) {
    for (; n > 0; n--)
        print_str("\x1b" "[C");
}
void clear_line_till_end() {
    print_str("\x1b" "[K");
}


// CURRENT LINE

char current_line[LINE_SIZE];
unsigned current_line_fill = 0;
unsigned current_line_pos = 0;

bool current_line_is_empty() {
    return current_line_fill == 0;
}

char * current_line_get(unsigned &n) {
    n = current_line_fill;
    return current_line;
}

void current_line_print() {
    prompt_active = true;
    print_char('\r');
    clear_line_till_end();
    print_str("> ");
    print_str(current_line, current_line_fill);
    move_cursor_backward(current_line_fill - current_line_pos);
}

void current_line_clear() {
    current_line_fill = 0;
    current_line_pos = 0;
    current_line[0] = 0;
    if (prompt_active)
        current_line_print();
}

void current_line_jump_to_end() {
    if (prompt_active)
        move_cursor_forward(current_line_fill - current_line_pos);
    current_line_pos = current_line_fill;
}

void current_line_insert(char c) {

    // check sensibility
    if (current_line_fill + 1 >= LINE_SIZE) return;
    if (!is_printable_ascii(c)) return;

    // null termination
    current_line_fill++;
    current_line[current_line_fill] = 0;

    // move chars after pos back
    for (unsigned i = current_line_fill-1; current_line_pos < i; i--) {
        current_line[i] = current_line[i-1];
    }

    // add char
    current_line[current_line_pos] = c;
    current_line_pos++;

    // print changes
    if (prompt_active) {
        print_char(c);
        if (current_line_pos != current_line_fill) {
            // TODO does this work?
            print_str(current_line + current_line_pos, current_line_fill - current_line_pos);
            move_cursor_backward(current_line_fill - current_line_pos);
        }
    }
}

void current_line_append(const char * s, unsigned n) {
    current_line_jump_to_end();

    n = str_len(s, n);
    if (current_line_fill + n >= LINE_SIZE)
        n = LINE_SIZE - current_line_fill - 1;

    for (unsigned i = 0; i < n; i++)
        current_line[current_line_fill+i] = s[i];
    current_line_fill += n;
    current_line[current_line_fill] = 0;
    current_line_pos = current_line_fill;

    if (prompt_active)
        print_str(s, n);
}

void current_line_set(const char *s, unsigned n) {
    n = str_len(s, n);
    current_line_fill = n >= LINE_SIZE ? LINE_SIZE : n;
    current_line_pos = current_line_fill;
    for (n = 0; n < current_line_fill; n++)
        current_line[n] = s[n];
    current_line[current_line_fill] = 0;
    if (prompt_active)
        current_line_print();
}

void current_line_move_forward() {
    if (current_line_pos >= current_line_fill) return;
    current_line_pos++;
    if (prompt_active)
        move_cursor_forward(1);
}

void current_line_move_backward() {
    if (current_line_pos == 0) return;
    current_line_pos--;
    if (prompt_active)
        move_cursor_backward(1);
}

void current_line_delete() {

    if (current_line_pos == 0) return;
    current_line_pos--;

    // move chars forward after pos -> copies null byte
    for (unsigned i = current_line_pos; i < current_line_fill; i++) {
        current_line[i] = current_line[i+1];
    }
    current_line_fill--;

    // print changes
    if (prompt_active) {
        move_cursor_backward(1);
        clear_line_till_end();
        if (current_line_pos != current_line_fill) {
            // TODO does this work?
            print_str(current_line + current_line_pos, current_line_fill - current_line_pos);
            move_cursor_backward(current_line_fill - current_line_pos);
        }
    }
}


// LINE BUFFER

char lines[LINE_COUNT][LINE_SIZE];

unsigned first_line = 0;
unsigned last_line = 0xffffffff;
unsigned this_line = 0;

// needs to be called before any other method is used
// - moves last_line to a fresh (empty) line
// - increase first line if necessary
// - points this_line to last_line
void line_buffer_new_last_line() {

    if (last_line == 0xffffffff)
        // initialization
        last_line = 0;
    else {
        // increase is simple
        if (last_line < LINE_COUNT-1)
            last_line++;
        else
            last_line = 0;

        // increase first line if needed
        if (last_line == first_line) {
            // increase first line
            if (first_line < LINE_COUNT-1)
                first_line++;
            else
                first_line = 0;
        }
    }

    // set this line to last line
    this_line = last_line;

    // empty the line
    lines[last_line][0] = 0;
}

// sets the contents of last line
void line_buffer_set_last_line(const char *s, unsigned n) {
    n = str_len(s, n);
    if (n >= LINE_SIZE)
        n = LINE_SIZE - 1;
    char * line = lines[last_line];
    for (unsigned i = 0; i < n; i++)
        line[i] = s[i];
    line[n] = 0;
}

void line_buffer_set_last_line_to_current_line() {
    line_buffer_set_last_line(current_line, LINE_SIZE);
}

// - copies current_line to last_line if this_line == last_line
// - moves this_line to it's predecessor
// - sets current_line to this_line if changes occured
void line_buffer_move_to_previous_line() {

    if (this_line == last_line)
        line_buffer_set_last_line_to_current_line();

    if (first_line < this_line) {
        this_line--;
    } else if (this_line < first_line) {
        if (this_line == 0)
            this_line = LINE_COUNT-1;
        else
            this_line--;
    } else {
        // this_line == first_line
        return;
    }

    current_line_set(lines[this_line], LINE_SIZE);
}

// - moves this_line to it's successor
// - sets current_line to this_line if changes occured
void line_buffer_move_to_next_line() {
    if (this_line < last_line) {
        this_line++;
    } else if (last_line < this_line) {
        if (this_line == LINE_COUNT-1)
            this_line = 0;
        else
            this_line++;
    } else {
        // this_line == last_line
        return;
    }

    current_line_set(lines[this_line], LINE_SIZE);
}


// ANSI terminal

unsigned csi_n = 0;

void handle_csi(char csi_c) {
    switch (csi_c) {
        case 'A': // Cursor Up
            line_buffer_move_to_previous_line();
            break;
        case 'B': // Cursor Down
            line_buffer_move_to_next_line();
            break;
        case 'C': // Cursor Forward
            current_line_move_forward();
            break;
        case 'D': // Cursor Back
            current_line_move_backward();
            break;
        default:  // Unknown
            prompt_use_new_line();
            println("Error: unknown csi sequence ");
            print_hex_value(csi_c, int);
            print_hex_value(csi_n, int);
    }
}


// IMPLEMENT FUNCTIONS

char * prompt_get_line(unsigned &n) {
    char * line = current_line_get(n);
    strip_start(line, n);
    strip_end(line, n);
    return line;
}

void prompt_line_append(const char *s, unsigned n) {
    current_line_append(s, n);
}

void prompt_use_new_line() {
    if (prompt_active) {
        println();
        prompt_active = false;
    }
}

bool use_fresh_line = true;
bool in_csi_seq = false;
char last_char = 0;

prompt_action prompt_handle_char(const char c);

prompt_action prompt_handle_input() {

    if (use_fresh_line) {
        use_fresh_line = false;
        current_line_clear();
        line_buffer_new_last_line();
    }

    if (!prompt_active)
        current_line_print();

    while (has_available()) {

        char c = get_char();

        if (c == -1) {
            // No data available
            last_char = c;
            return prompt_action_none;
        }

        prompt_action action = prompt_handle_char(c);

        last_char = c;

        if (action != prompt_action_none)
            return action;

        // prints (deactivations) might happen in between
        if (!prompt_active)
            current_line_print();

    }

    return prompt_action_none;
}

prompt_action prompt_handle_char(const char c) {

    if (c == '\n' || c == '\r') {
        if (c == '\n' && last_char == '\r')
            // we just had an CRLF
            return prompt_action_none;
        
        // enable printing for others
        prompt_active = false;
        println();
        
        if (!current_line_is_empty()) {
            // we have read a line --> go to a fresh line
            use_fresh_line = true;

            // set the last line in the buffer to the executed line
            line_buffer_set_last_line_to_current_line();
        }

        return prompt_action_execute;
    }

    if (in_csi_seq) {
        if ('0' <= c && c <= '9') {
            // another entry to the csi number
            csi_n *= 10;
            csi_n += c - '0';
        } else {
            // finally the csi char
            handle_csi(c);
            in_csi_seq = false;
        }
        return prompt_action_none;
    }

    if (last_char == 0x1b /*ESC*/ && c == '[') {
        // CSI escape sequence
        in_csi_seq = true;
        csi_n = 0;
        return prompt_action_none;
    }

    if (is_printable_ascii(c)) {
        // we just read normal data
        current_line_insert(c);
        return prompt_action_none;
    }

    if (c == 0x1b)
        // ESC --> don't do anything yet (might become a CSI)
        return prompt_action_none;

    if (c == '\b') {
        // backspace
        current_line_delete();
        return prompt_action_none;
    }

    if (c == 0x03) {
        // ctrl-c
        println("^C");
        current_line_clear();
        return prompt_action_none;
    }

    // we should never get here!
    prompt_use_new_line();
    print_str("Error: unknown char ");
    print_hex_value(c, byte);

    return prompt_action_none;
}

