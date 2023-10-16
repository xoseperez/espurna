/*

Part of the TERMINAL MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include "terminal_parsing.h"

#include <forward_list>
#include <functional>
#include <vector>

namespace espurna {
namespace terminal {

// We need to be able to pass arbitrary Args structure into the command function
// Like Embedis implementation, we only pass things that we actually use instead of complete obj instance
struct CommandContext {
    Argv argv;
    Print& output;
    Print& error;
};

using CommandFunc = void(*)(CommandContext&&);
struct Command {
    StringView name;
    CommandFunc func;
};

struct Commands {
    const Command* begin;
    const Command* end;
};

// store name<->func association at runtime 
void add(Commands);

template <size_t Size>
void add(const Command (&command)[Size]) {
    add(Commands{
        .begin = &command[0],
        .end = &command[Size],
    });
}

void add(StringView, CommandFunc);

// total number of registered commands
size_t size();

using CommandNames = std::vector<StringView>;
CommandNames names();

// find registered command with 'name' or 'nullptr' on failure
const Command* find(StringView name);

// try to parse and call command line string
bool find_and_call(StringView, Print& output);

// try to parse and call command line string
bool find_and_call(StringView, Print& output, Print& error);

// try and call an already parsed command line
bool find_and_call(CommandLine, Print& output);

// try and call an already parsed command line
bool find_and_call(CommandLine, Print& output, Print& error);

// search the given string for valid commands and call them in sequence
bool api_find_and_call(StringView, Print& output);

// search the given string for valid commands and call them in sequence
bool api_find_and_call(StringView, Print& output, Print& error);

// helper functions for most common success output
void ok(Print&);
void ok(const espurna::terminal::CommandContext&);

// helper functions for when the function fails
void error(Print&, const String&);
void error(const espurna::terminal::CommandContext&, const String&);


} // namespace terminal
} // namespace espurna
