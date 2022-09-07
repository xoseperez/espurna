/*

Part of the TERMINAL MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

Heavily inspired by the Embedis design:
- https://github.com/thingSoC/embedis

*/

#include <Arduino.h>

#include "terminal_parsing.h"
#include "terminal_commands.h"

#include <algorithm>
#include <memory>

namespace espurna {
namespace terminal {
namespace {

// TODO: register commands throught static object, and operate on lists
// instead of individual command addition through the add()

namespace internal {

using Commands = std::forward_list<Command>;
Commands commands;

} // namespace internal
} // namespace

size_t size() {
    return std::distance(internal::commands.begin(), internal::commands.end());
}

CommandNames names() {
    CommandNames out;
    out.reserve(size());

    for (auto& command : internal::commands) {
        out.push_back(command.name);
    }

    return out;
}

void add(Command command) {
    if (command.func) {
        internal::commands.emplace_front(std::move(command));
    }
}

void add(const __FlashStringHelper* name, CommandFunc func) {
    add(Command{
        .name = name,
        .func = func });
}

const Command* find(StringView name) {
    auto found = std::find_if(
        internal::commands.begin(),
        internal::commands.end(),
        // TODO: StringView comparison
        // note that `String::equalsIgnoreCase(const __FlashStringHelper*)` does not exist, and will create a temporary `String`
        // both use read-1-byte-at-a-time for PROGMEM, however this variant saves around 200μs in time since there's no temporary object
        [&](const Command& command) {
            const auto* lhs = name.c_str();
            const auto* rhs = reinterpret_cast<const char*>(command.name);
            const auto len = strlen_P(rhs);

            return (name.length() == len)
                && (0 == strncasecmp_P(lhs, rhs, len));
        });

    if (found == internal::commands.end()) {
        return nullptr;
    }

    return &(*found);
}

void ok(Print& out) {
    out.print(F("+OK\n"));
}

void ok(const espurna::terminal::CommandContext& ctx) {
    ok(ctx.output);
}

void error(Print& print, const String& message) {
    print.printf_P(PSTR("-ERROR: %s\n"), message.c_str());
}

void error(const espurna::terminal::CommandContext& ctx, const String& message) {
    error(ctx.output, message);
}

bool find_and_call(CommandLine cmd, Print& out) {
    const auto* command = find(cmd.argv[0]);
    if (command) {
        (*command).func(CommandContext{
            .argv = std::move(cmd.argv),
            .output = out });
        return true;
    }

    error(out, F("Command not found"));
    return false;
}

bool find_and_call(StringView cmd, Print& out) {
    auto result = parse_line(cmd);
    if (result.error != parser::Error::Ok) {
        String message;
        message += PSTR("TERMINAL: ");
        message += parser::error(result.error);
        error(out, message);
        return false;
    }

    if (!result.argv.size()) {
        return false;
    }

    return find_and_call(std::move(result), out);
}

bool api_find_and_call(StringView cmd, Print& out) {
    bool result { true };

    LineView lines(cmd);
    while (lines) {
        const auto line = lines.line();
        if (!line.length()) {
            break;
        }

        // prefer to break early when commands are missing
        if (!find_and_call(line, out)) {
            result = false;
            break;
        }
    }

    return result;
}

} // namespace terminal
} // namespace espurna
