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

namespace terminal {

struct Terminal;

// We need to be able to pass arbitrary Args structure into the command function
// Like Embedis implementation, we only pass things that we actually use instead of complete obj instance
struct CommandContext {
    std::vector<String> argv;
    Print& output;
};

class Terminal {
public:
    enum class Result {
        Error,           // Genric error condition
        Command,         // We successfully parsed the line and executed the callback specified via addCommand
        CommandNotFound, // ... similar to the above, but command was never added via addCommand
        BufferOverflow,  // Command line processing failed, no \r\n / \n before buffer was filled
        Pending,         // We got something in the buffer, but can't yet do anything with it
        NoInput          // We got nothing in the buffer and stream read() returns -1
    };

    using CommandFunc = void(*)(CommandContext&&);
    using ProcessFunc = bool(*)(Result);

    using Names = std::vector<const __FlashStringHelper*>;
    using Command = std::pair<const __FlashStringHelper*, CommandFunc>;
    using Commands = std::forward_list<Command>;

    // stream      - see `stream` description below
    // buffer_size - set internal limit for the total command line length
    Terminal(Stream& stream, size_t buffer_size = 128) :
        _stream(stream),
        _buffer_size(buffer_size)
    {
        _buffer.reserve(buffer_size);
    }

    static void addCommand(const __FlashStringHelper* name, CommandFunc func);
    static size_t commands();
    static Names names();

    // Try to process a single line (until either `\r\n` or just `\n`)
    Result processLine();

    // Calls processLine() repeatedly.
    // Blocks until the stream no longer has any data available.
    // `process_f` will return each individual processLine() Result,
    // and we can either stop (false) or continue (true) the function.
    void process(ProcessFunc = defaultProcessFunc);

private:
    static bool defaultProcessFunc(Result);

    // general input / output stream:
    // - stream.read() should return user input
    // - stream.write() can be called from the command callback
    // - stream.write() can be called by us to show error messages
    Stream& _stream;

    // input stream is buffered until it can be parsed
    // in case parsing did not happen and we filled the buffer up to it's size,
    // the error will be returned and the buffer parsing will start from the beginning
    std::vector<char> _buffer;
    const size_t _buffer_size;

    static Commands _commands;
};

} // namespace terminal
