/*

Part of the TERMINAL MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <cstring>
#include <iterator>
#include <vector>

#include "types.h"

namespace espurna {
namespace terminal {

using Argv = std::vector<String>;

namespace parser {

enum class Error {
    Ok,
    Uninitialized,     // parser never started / no text
    Busy,              // parser was already parsing something
    UnterminatedQuote, // parsing stopped without terminating a quoted entry
    NoSpaceAfterQuote, // parsing stopped since there was no space after quote
    InvalidEscape,     // escaped text was invalid
    UnexpectedLineEnd, // unexpected \r encounteted in the input
};

String error(Error);

} // namespace parser

struct CommandLine {
    Argv argv;
    parser::Error error;
};

// Buffer char data and check whether the received value has newlines in its internal
// storage works like a circular buffer; whenever buffer size exceedes capacity, we return
// to the start of the buffer and reset size.
// When buffer overflows, store internal flag until the storage is reset to the default state
template <size_t Capacity>
struct LineBuffer {
    // **only valid until the next append()**
    struct Result {
        StringView line;
        bool overflow;
    };

    LineBuffer() = default;

    Result line() {
        const auto begin = &_storage[_cursor];
        const auto end = &_storage[_size];

        if (begin != end) {
            const auto eol = std::find(begin, end, '\n');
            if (eol != end) {
                const auto after = std::next(eol);
                const auto out = Result{
                    .line = StringView{begin, after},
                    .overflow = _overflow };

                if (after != end) {
                    _cursor = std::distance(_storage.begin(), after);
                } else {
                    reset();
                }

                return out;
            }
        }

        return Result{
            .line = StringView(),
            .overflow = _overflow };
    }

    void reset() {
        _overflow = false;
        _cursor = 0;
        _size = 0;
    }

    static constexpr size_t capacity() {
        return Capacity;
    }

    size_t size() const {
        return _size;
    }

    bool overflow() const {
        return _overflow;
    }

    void append(const char* data, size_t length) {
        // adjust pointer and length when they immediatelly cause overflow
        auto output = &_storage[_size];

        auto capacity = Capacity - _size;
        while (length > capacity) {
            data += capacity;
            length -= capacity;
            capacity = Capacity;
            output = &_storage[0];
            _size = 0;
            _overflow = true;
        }

        if (length) {
            std::memcpy(output, data, length);
            _size += length;
        }
    }

    void append(StringView value) {
        append(value.c_str(), value.length());
    }

    void append(char value) {
        append(&value, 1);
    }

private:
    using Storage = std::array<char, Capacity>;
    Storage _storage;

    size_t _size { 0 };
    size_t _cursor { 0 };
    bool _overflow { false };
};

// Similar to line buffer, but instead work on an already existing string
// and yield line views on each call to line()
struct LineView {
    LineView(StringView lines) :
        _lines(lines)
    {}

    StringView line() {
        const auto Begin = begin();
        const auto End = begin();

        if (Begin != End) {
            const auto eol = std::find(begin(), end(), '\n');
            if (eol != End) {
                const auto after = std::next(eol);
                if (after != End) {
                    _cursor = std::distance(_lines.begin(), after);
                } else {
                    _cursor = _lines.length();
                }

                return StringView{Begin, after};
            }
        }

        return StringView();
    }

    explicit operator bool() const {
        return _cursor != _lines.length();
    }

    const char* begin() const {
        return _lines.begin() + _cursor;
    }

    const char* end() const {
        return _lines.end();
    }

    size_t length() const {
        return std::distance(begin(), end());
    }

    StringView get() const {
        return StringView{begin(), end()};
    }

private:
    StringView _lines;
    uintptr_t _cursor { 0 };
};

// Type wrapper that flushes output on finding '\n'.
// Inherit from `String` to allow us to manage internal buffer directly.
// (and also seamlessly behave like a usual `String`, we import most of its methods)
template <typename T>
class PrintLine final : public Print, public String {
private:
    static_assert(!std::is_reference<T>::value, "");

    using String::wbuffer;
    using String::buffer;
    using String::setLen;

public:
    using String::begin;
    using String::end;
    using String::reserve;
    using String::c_str;
    using String::length;

    PrintLine() = default;
    ~PrintLine() {
        send();
    }

    template <typename... Args>
    PrintLine(Args&&... args) :
        _output(std::forward<Args>(args)...)
    {}

    T& output() {
        return _output;
    }

    void flush() override {
        if (end() != std::find(begin(), end(), '\n')) {
            send();
            setLen(0);
            *wbuffer() = '\0';
        }
    }

    size_t write(const uint8_t* data, size_t size) override {
        if (!_lock && size && data && *data != '\0') {
            ReentryLock lock(_lock);

            concat(reinterpret_cast<const char*>(data), size);
            flush();

            return size;
        }

        return 0;
    }

    size_t write(uint8_t ch) override {
        return write(&ch, 1);
    }

private:
    void send() {
        _output(buffer());
    }

    T _output;
    bool _lock { false };
};

// Generic command line parser
// - `argv` array contains copies or every 'split' string found in the source line
//   (usual `argc` is expected to be equal to the `argv.size()`)
// - `error` set to any parser errors encountered, or `Ok` when everything is fine
CommandLine parse_line(StringView line);

} // namespace terminal
} // namespace espurna
