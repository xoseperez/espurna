/*

Part of the TERMINAL MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

Heavily inspired by the Embedis design:
- https://github.com/thingSoC/embedis

*/

#include <Arduino.h>

#include "terminal_commands.h"

#include <algorithm>
#include <memory>

namespace terminal {

Terminal::Commands Terminal::_commands;

// TODO: `name` is never checked for uniqueness, unlike the previous implementation with the `unordered_map`
// (and note that the map used hash IDs instead of direct string comparison)
//
// One possible workaround is to delegate command matching to a callback function of a module:
//
// > addCommandMatcher([](const String& argv0) -> Callback {
// >     if (argv0.equalsIgnoreCase(F("one")) {
// >         return cmd_one;
// >     } else if (argv0.equalsIgnoreCase(F("two")) {
// >         return cmd_two;
// >     }
// >     return nullptr;
// > });
// 
// Or, using a PROGMEM static array of `{progmem_name, callback_ptr}` pairs.
// There would be a lot of PROGMEM boilerplate, though, since PROGMEM strings cannot be
// written inline with the array itself, and must be declared with a symbol name beforehand.
// (unless, progmem_name is a fixed-size char[], but then it must have a special limit for command length)

void Terminal::addCommand(const __FlashStringHelper* name, CommandFunc func) {
    if (func) {
        _commands.emplace_front(std::make_pair(name, func));
    }
}

size_t Terminal::commands() {
    return std::distance(_commands.begin(), _commands.end());
}

Terminal::Names Terminal::names() {
    Terminal::Names out;
    out.reserve(commands());
    for (auto& command : _commands) {
        out.push_back(command.first);
    }
    return out;
}

Terminal::Result Terminal::processLine() {

    // Arduino stream API returns either `char` >= 0 or -1 on error
    int c { -1 };
    while ((c = _stream.read()) >= 0) {
        if (_buffer.size() >= (_buffer_size - 1)) {
            _buffer.clear();
            return Result::BufferOverflow;
        }
        _buffer.push_back(c);
        if (c == '\n') {
            // in case we see \r\n, offset minus one and overwrite \r
            auto end = _buffer.end() - 1;
            if (*(end - 1) == '\r') {
                --end;
            }
            *end = '\0';

            // parser should pick out at least one arg aka command
            auto cmdline = parsing::parse_commandline(_buffer.data());
            _buffer.clear();
            if ((cmdline.argv.size() >= 1) && (cmdline.argv[0].length())) {
                auto found = std::find_if(_commands.begin(), _commands.end(), [&cmdline](const Command& command) {
                    // note that `String::equalsIgnoreCase(const __FlashStringHelper*)` does not exist, and will create a temporary `String`
                    // both use read-1-byte-at-a-time for PROGMEM, however this variant saves around 200μs in time since there's no temporary object
                    auto* lhs = cmdline.argv[0].c_str();
                    auto* rhs = reinterpret_cast<const char*>(command.first);
                    auto len = strlen_P(rhs);

                    return (cmdline.argv[0].length() == len) && (0 == strncasecmp_P(lhs, rhs, len));
                });
                if (found == _commands.end()) return Result::CommandNotFound;
                (*found).second(CommandContext{std::move(cmdline.argv), _stream});
                return Result::Command;
            }
        }
    }

    // we need to notify about the fixable things
    if (_buffer.size() && (c < 0)) {
        return Result::Pending;
    } else if (!_buffer.size() && (c < 0)) {
        return Result::NoInput;
    // ... and some unexpected conditions
    } else {
        return Result::Error;
    }

}

bool Terminal::defaultProcessFunc(Result result) {
    return (result != Result::Error) && (result != Result::NoInput);
}

void Terminal::process(ProcessFunc func) {
    while (func(processLine())) {
    }    
}

} // namespace terminal
