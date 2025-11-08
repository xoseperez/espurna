/*

Part of the TERMINAL MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

Heavily inspired by the Embedis design:
- https://github.com/thingSoC/embedis

*/

#include <Arduino.h>

#include "terminal_parsing.h"
#include "terminal_commands.h"
#include "libs/Delimiter.h"

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

Buffer parsed_to_buffer(ParsedLine& parsed) {
    Buffer out;
    out.reserve(parsed.tokens.size());

    for (auto& token : parsed.tokens) {
        auto buffered = std::find_if(
            parsed.buffer.begin(),
            parsed.buffer.end(),
            [&](const String& other) {
                return token.data() == other.begin();
            });

        if (buffered != parsed.buffer.end()) {
            out.push_back(std::move(*buffered));
            parsed.buffer.erase(buffered);
        } else {
            out.push_back(token.toString());
        }
    }

    parsed.tokens.clear();
    parsed.buffer.clear();

    return out;
}

String prepare_error(parser::Error error) {
    String out;
    out += STRING_VIEW("TERMINAL: ");
    out += parser::error(error);

    return out;
}

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
    print.printf_P(PSTR("-ERROR: %s\n"),
        message.length()
            ? message.c_str()
            : PSTR("UNKNOWN"));
}

void error(const espurna::terminal::CommandContext& ctx, const String& message) {
    error(ctx.error, message);
}

bool find_and_call(ParsedLine parsed, Print& output, Print& error_output) {
    if (!parsed.tokens.size()) {
        return false;
    }

    const auto* command = find(parsed.tokens[0]);
    if (command) {
        (*command).func(
            CommandContext{
                .argv = internal::parsed_to_buffer(parsed),
                .output = output,
                .error = error_output,
            });

        return true;
    }

    error(error_output, F("Command not found"));
    return false;
}

bool find_and_call(ParsedLine parsed, Print& output) {
    return find_and_call(std::move(parsed), output, output);
}

bool find_and_call(StringView cmd, Print& output, Print& error_output) {
    auto result = parse_terminated(cmd);
    if (result.error != parser::Error::Ok) {
        error(error_output, internal::prepare_error(result.error));
        return false;
    }

    if (!result.tokens.size()) {
        return false;
    }

    return find_and_call(std::move(result), output, error_output);
}

bool find_and_call(StringView cmd, Print& output) {
    return find_and_call(cmd, output, output);
}

bool api_find_and_call(StringView cmd, Print& output, Print& error_output) {
    bool out { false };
    auto input = cmd;

    while (input.length()) {
        auto result = parse_terminated(input);
        input = result.remaining;

        if (result.error != parser::Error::Ok) {
            error(error_output, internal::prepare_error(result.error));
            out = false;
            break;
        }

        if (!result.tokens.size()) {
            out = false;
            break;
        }

        if (!find_and_call(std::move(result), output, error_output)) {
            out = false;
            break;
        }

        out = true;
    }

    return out;
}

bool api_find_and_call(StringView cmd, Print& output) {
    return api_find_and_call(cmd, output, output);
}

} // namespace terminal
} // namespace espurna
