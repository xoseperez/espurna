/*

Part of the TERMINAL MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include "terminal_parsing.h"

#include <unordered_map>
#include <vector>

namespace terminal {

struct Terminal;

// We need to be able to pass arbitrary Args structure into the command function
// Like Embedis implementation, we only pass things that we actually use instead of complete obj instance
struct CommandContext {

    friend class Terminal;

	std::vector<String> argv;
	size_t argc;
    Print& output;

    private:

    CommandContext(parsing::CommandLine& cl, Stream& stream) :
        argv(cl.argv),
        argc(cl.argc),
        output(stream)
    {}

    CommandContext(parsing::CommandLine&& cl, Stream& stream) :
        argv(std::move(cl.argv)),
        argc(std::move(cl.argc)),
        output(stream)
    {}

    CommandContext(std::vector<String> const& argv, size_t argc, Stream& stream) :
        argv(argv),
        argc(argc),
        output(stream)
    {}

    CommandContext(std::vector<String>&& argv, size_t argc, Stream& stream) :
        argv(std::move(argv)),
        argc(argc),
        output(stream)
    {}

};

struct Terminal {

    enum class Result {
        Error,           // Genric error condition
        Command,         // We successfully parsed the line and executed the callback specified via addCommand
        CommandNotFound, // ... similar to the above, but command was never added via addCommand
        BufferOverflow,  // Command line processing failed, no \r\n / \n before buffer was filled
        Pending,         // We got something in the buffer, but can't yet do anything with it
        NoInput          // We got nothing in the buffer and stream read() returns -1
    };

    using CommandFunc = void(*)(const CommandContext& ctx);
    using process_f = bool(*)(Result);

    // stream      - see `stream` description below
    // buffer_size - set internal limit for the total command line length
    Terminal(Stream& stream, size_t buffer_size = 128) :
        stream(stream),
        buffer_size(buffer_size)
    {
        buffer.reserve(buffer_size);
    }

    static void addCommand(const String& name, CommandFunc func);
    static size_t commandsSize();
    static std::vector<String> commandNames();

    // Try to process a single line (until either `\r\n` or just `\n`)
    Result processLine();

    // Calls processLine() repeatedly.
    // Blocks until the stream no longer has any data available.
    // `process_f` will return each individual processLine() Result,
    // and we can either stop (false) or continue (true) the function.
    void process(process_f = defaultProcessFunc);

    private:

    static bool defaultProcessFunc(Result);

    // general input / output stream:
    // - stream.read() should return user iput
    // - stream.write() can be called from the command callback
    // - stream.write() can be called by us to show error messages
    Stream& stream;

    // buffer for the input stream, fixed in size
	std::vector<char> buffer;
    const size_t buffer_size;

    // TODO: every command is shared, instance should probably also have an
    //       option to add 'private' commands list?
    static std::unordered_map<String, CommandFunc, parsing::LowercaseFnv1Hash<String>, parsing::LowercaseEquals<String>> commands;

};
}
