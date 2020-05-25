/*

Part of the TERMINAL MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

Heavily inspired by the Embedis design:
- https://github.com/thingSoC/embedis

*/

#include <Arduino.h>

#include "terminal_commands.h"

#include <memory>

namespace terminal {

std::unordered_map<String, Terminal::CommandFunc,
    parsing::LowercaseFnv1Hash<String>,
    parsing::LowercaseEquals<String>> Terminal::commands;

void Terminal::addCommand(const String& name, CommandFunc func) {
    if (!func) return;
    commands.emplace(std::make_pair(name, func));
}

size_t Terminal::commandsSize() {
    return commands.size();
}

std::vector<String> Terminal::commandNames() {
    std::vector<String> out;
    out.reserve(commands.size());
    for (auto& command : commands) {
        out.push_back(command.first);
    }
    return out;
}

Terminal::Result Terminal::processLine() {

    // Arduino stream API returns either `char` >= 0 or -1 on error
    int c = -1;
    while ((c = stream.read()) >= 0) {
        if (buffer.size() >= (buffer_size - 1)) {
            buffer.clear();
            return Result::BufferOverflow;
        }
        buffer.push_back(c);
        if (c == '\n') {
            // in case we see \r\n, offset minus one and overwrite \r
            auto end = buffer.end() - 1;
            if (*(end - 1) == '\r') {
                --end;
            }
            *end = '\0';

            // parser should pick out at least one arg (command)
            auto cmdline = parsing::parse_commandline(buffer.data());
            buffer.clear();
            if (cmdline.argc >= 1) {
                auto command = commands.find(cmdline.argv[0]);
                if (command == commands.end()) return Result::CommandNotFound;
                (*command).second(CommandContext{std::move(cmdline.argv), cmdline.argc, stream});
                return Result::Command;
            }
        }
    }

    // we need to notify about the fixable things
    if (buffer.size() && (c < 0)) {
        return Result::Pending;
    } else if (!buffer.size() && (c < 0)) {
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

} // ns terminal
