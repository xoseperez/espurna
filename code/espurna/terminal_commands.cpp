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

using CommandsView = std::forward_list<Commands>;
CommandsView commands;

} // namespace internal
} // namespace

size_t size() {
    size_t out { 0 };
    for (const auto commands : internal::commands) {
        out += commands.end - commands.begin;
    }

    return out;
}

CommandNames names() {
    CommandNames out;
    out.reserve(size());

    for (const auto commands : internal::commands) {
        for (auto it = commands.begin; it != commands.end; ++it) {
            out.push_back((*it).name);
        }
    }

    return out;
}

void add(Commands commands) {
    internal::commands.emplace_front(std::move(commands));
}

void add(StringView name, CommandFunc func) {
    const auto cmd = new Command{
        .name = name,
        .func = func,
    };

    add(Commands{
        .begin = cmd,
        .end = cmd + 1,
    });
}

const Command* find(StringView name) {
    for (const auto commands : internal::commands) {
        const auto found = std::find_if(
            commands.begin,
            commands.end,
            [&](const Command& command) {
                return name.equalsIgnoreCase(command.name);
            });

        if (found != commands.end) {
            return found;
        }
    }

    return nullptr;
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
    error(ctx.error, message);
}

bool find_and_call(CommandLine cmd, Print& output, Print& error_output) {
    const auto* command = find(cmd.argv[0]);
    if (command) {
        (*command).func(
            CommandContext{
                .argv = std::move(cmd.argv),
                .output = output,
                .error = error_output,
            });

        return true;
    }

    error(error_output, F("Command not found"));
    return false;
}

bool find_and_call(CommandLine cmd, Print& output) {
    return find_and_call(cmd, output, output);
}

bool find_and_call(StringView cmd, Print& output, Print& error_output) {
    auto result = parse_line(cmd);
    if (result.error != parser::Error::Ok) {
        String message;
        message += STRING_VIEW("TERMINAL: ");
        message += parser::error(result.error);
        error(error_output, message);
        return false;
    }

    if (!result.argv.size()) {
        return false;
    }

    return find_and_call(std::move(result), output, error_output);
}

bool find_and_call(StringView cmd, Print& output) {
    return find_and_call(cmd, output, output);
}

bool api_find_and_call(StringView cmd, Print& output, Print& error_output) {
    bool result { true };

    LineView lines(cmd);
    while (lines) {
        const auto line = lines.line();
        if (!line.length()) {
            break;
        }

        // prefer to break early when commands are missing
        if (!find_and_call(line, output, error_output)) {
            result = false;
            break;
        }
    }

    return result;
}

bool api_find_and_call(StringView cmd, Print& output) {
    return api_find_and_call(cmd, output, output);
}

} // namespace terminal
} // namespace espurna
