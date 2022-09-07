/*

Part of the TERMINAL MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include <vector>
#include <cctype>

#include "terminal_parsing.h"

namespace espurna {
namespace terminal {
namespace parser {

String error(Error value) {
    String out;

    switch (value) {
    case Error::Ok:
        out = PSTR("Ok");
        break;
    case Error::Uninitialized:
        out = PSTR("Uninitialized");
        break;
    case Error::Busy:
        out = PSTR("Busy");
        break;
    case Error::UnescapedText:
        out = PSTR("UnescapedText");
        break;
    case Error::UnterminatedQuote:
        out = PSTR("UnterminatedQuote");
        break;
    case Error::InvalidEscape:
        out = PSTR("InvalidEscape");
        break;
    case Error::UnexpectedLineEnd:
        out = PSTR("UnexpectedLineEnd");
        break;
    case Error::NoSpaceAfterQuote:
        out = PSTR("NoSpaceAfterQuote");
        break;
    }

    return out;
}

namespace {

// Original code is part of the SDSLib 2.0 -- A C dynamic strings library
// - https://github.com/antirez/sds/blob/master/sds.c
// - https://github.com/antirez/redis/blob/unstable/src/networking.c
// Replaced with a stateful parser to avoid random look-ahead issues in the code,
// and really make sure we **never** go out of bounds of the given view.
// (e.g. when we want to parse only a part of a larger string)

// Helper functions to handle \xHH codes that could encode
// non-printable characters for commands or arguments
bool is_hex_digit(char c) {
    switch (c) {
    case '0' ... '9':
    case 'a' ... 'f':
    case 'A' ... 'F':
        return true;
    }

    return false;
}

char hex_digit_to_byte(char c) {
    switch (c) {
    case '0'...'9':
        return c - '0';
    case 'a':
    case 'A':
        return 10;
    case 'b':
    case 'B':
        return 11;
    case 'c':
    case 'C':
        return 12;
    case 'd':
    case 'D':
        return 13;
    case 'e':
    case 'E':
        return 14;
    case 'f':
    case 'F':
        return 15;
    }

    return 0;
}

char hex_digit_to_value(char lhs, char rhs) {
    return (hex_digit_to_byte(lhs) << 4) | hex_digit_to_byte(rhs);
}

// printable ASCII character set
bool is_printable(char c) {
    switch (c) {
    case ' '...'~':
        return true;
    }

    return false;
}

// allowed 'special' input characters
char unescape_char(char c) {
    switch (c) {
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case 'b':
        return '\b';
    case 'a':
        return '\a';
    }

    return c;
}

struct Result {
    Result() = default;

    Result& operator=(Error error) {
        _error = error;
        _argv.clear();
        return *this;
    }

    Result& operator=(Argv&& argv) {
        _argv = std::move(argv);
        _error = Error::Ok;
        return *this;
    }

    explicit operator bool() const {
        return _error == Error::Ok;
    }

    CommandLine commandLine() {
        auto out = CommandLine{
            .argv = std::move(_argv),
            .error = _error };

        _error = Error::Uninitialized;
        return out;
    }

private:
    Error _error { Error::Uninitialized };
    Argv _argv;
};

struct Parser {
    Parser() = default;
    Result operator()(StringView);

private:
    // only tracked within our `operator()(<LINE>)`
    enum class State {
        Done,
        Initial,
        Text,
        CarriageReturn,
        CarriageReturnAfterText,
        SkipUntilNewLine,
        EscapedText,
        EscapedByteLhs,
        EscapedByteRhs,
        SingleQuote,
        EscapedQuote,
        DoubleQuote,
        AfterQuote,
    };

    // disallow re-entry with a custom lock handler
    struct Lock {
        Lock() = delete;

        Lock(const Lock&) = delete;
        Lock& operator=(const Lock&) = delete;

        Lock(Lock&&) = default;
        Lock& operator=(Lock&&) = delete;

        Lock(bool& handle) :
            _initialized(!handle),
            _handle(handle)
        {}

        ~Lock() {
            unlock();
        }

        bool initialized() const {
            return _initialized;
        }

        void lock() {
            if (initialized()) {
                _handle = true;
            }
        }

        void unlock() {
            if (initialized()) {
                _handle = false;
            }
        }
    private:
        bool _initialized;
        bool& _handle;
    };

    // intermediate storage for ARGV, text value and escaped characters
    // (stores the LHS of \x##, because we don't use look-ahead)
    struct Values {
        Argv argv;
        String chunk;
        char byte_lhs { 0 };
    };

    static void append_chunk(Values& values, char c) {
        values.chunk.concat(&c, 1);
    }

    static void push_chunk(Values& values) {
        values.argv.push_back(values.chunk);
        values.chunk = "";
    }

    bool _parsing { false };
};

Result Parser::operator()(StringView line) {
    Result result;
    Values values;

    State state { State::Initial };

    Lock lock(_parsing);
    if (!lock.initialized()) {
        result = Error::Busy;
        goto out;
    }

    for (auto it = line.begin(); it != line.end(); ++it) {
        switch (State(state)) {
        case State::Initial:
            switch (*it) {
            case ' ':
            case '\t':
                break;
            case '\r':
                state = State::CarriageReturn;
                break;
            case '\n':
                state = State::Done;
                break;
            default:
                state = State::Text;
                goto text;
            }
            break;

        case State::Done: 
            goto out;

        case State::Text:
text:
            switch (*it) {
            case ' ':
            case '\t':
                push_chunk(values);
                state = State::Initial;
                break;
            case '"':
                state = State::DoubleQuote;
                break;
            case '\'':
                state = State::SingleQuote;
                break;
            case '\r':
                state = State::CarriageReturnAfterText;
                break;
            case '\n':
                push_chunk(values);
                state = State::Done;
                break;
            default:
                if (is_printable(*it)) {
                    append_chunk(values, *it);
                } else {
                    result = Error::UnescapedText;
                    goto out;
                }
                break;
            }
            break;

        case State::CarriageReturn:
            if ((*it) == '\n') {
                state = State::Done;
            } else {
                result = Error::UnexpectedLineEnd;
                goto out;
            }
            break;

        case State::CarriageReturnAfterText:
            if ((*it) == '\n') {
                push_chunk(values);
                state = State::Done;
            } else {
                result = Error::UnexpectedLineEnd;
                goto out;
            }
            break;

        case State::SkipUntilNewLine:
            switch (*it) {
            case '\r':
                state = State::CarriageReturn;
                break;
            case '\n':
                state = State::Initial;
                break;
            }
            break;

        case State::EscapedText: {
            switch (*it) {
            case '\r':
            case '\n':
                result = Error::UnexpectedLineEnd;
                goto out;
            case 'x':
                state = State::EscapedByteLhs;
                break;
            default:
                append_chunk(values, unescape_char(*it));
                break;
            }
            break;
        }

        case State::EscapedByteLhs:
            if (is_hex_digit(*it)) {
                values.byte_lhs = *it;
                state = State::EscapedByteRhs;
            } else {
                result = Error::InvalidEscape;
                goto out;
            }
            break;

        case State::EscapedByteRhs:
            if (is_hex_digit(*it)) {
                append_chunk(values, hex_digit_to_value(values.byte_lhs, *it));
                state = State::DoubleQuote;
            } else {
                result = Error::InvalidEscape;
                goto out;
            }
            break;

        case State::SingleQuote:
            switch (*it) {
            case '\r':
            case '\n':
                result = Error::UnterminatedQuote;
                goto out;
            case '\\':
                state = State::EscapedQuote;
                break;
            case '\'':
                state = State::AfterQuote;
                break;
            default:
                if (is_printable(*it)) {
                    append_chunk(values, *it);
                } else {
                    result = Error::UnescapedText;
                    goto out;
                }
                break;
            }
            break;

        case State::EscapedQuote:
            switch (*it) {
            case '\'':
                values.chunk.concat(*it);
                state = State::SingleQuote;
                break;
            default:
                result = Error::InvalidEscape;
                goto out;
            }
            break;

        case State::AfterQuote:
            switch (*it) {
            case '\r':
                state = State::CarriageReturnAfterText;
                break;
            case ' ':
            case '\t':
                push_chunk(values);
                state = State::Initial;
                break;
            case '\n':
                push_chunk(values);
                state = State::Done;
                break;
            default:
                result = Error::NoSpaceAfterQuote;
                goto out;
            }
            break;

        case State::DoubleQuote:
            switch (*it) {
            case '\r':
            case '\n':
                result = Error::UnterminatedQuote;
                goto out;
            case '"':
                state = State::AfterQuote;
                break;
            case '\\':
                state = State::EscapedText;
                break;
            default:
                if (is_printable(*it)) {
                    append_chunk(values, *it);
                } else {
                    result = Error::UnescapedText;
                    goto out;
                }
                break;
            }
            break;

        }
    }

out:
    if (state == State::Done) {
        result = std::move(values.argv);
    }

    // whenever line ends before we are done parsing, make sure
    // result contains a valid error condition (same as in the switch above)
    if (!result) {
        switch (state) {
        case State::Done:
            break;
        case State::CarriageReturn:
        case State::CarriageReturnAfterText:
        case State::Text:
        case State::Initial:
        case State::SkipUntilNewLine:
            result = Error::UnexpectedLineEnd;
            break;
        case State::EscapedByteLhs:
        case State::EscapedByteRhs:
        case State::EscapedText:
        case State::EscapedQuote:
            result = Error::InvalidEscape;
            break;
        case State::SingleQuote:
        case State::DoubleQuote:
            result = Error::UnterminatedQuote;
            break;
        case State::AfterQuote:
            result = Error::NoSpaceAfterQuote;
            break;
        }
    }

    return result;
}

CommandLine parse_line(StringView line) {
    static Parser parser;
    return parser(line).commandLine();
}

} // namespace

// Fowler–Noll–Vo hash function to hash command strings that treats input as lowercase
// ref: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
//
// This is here in case `std::unordered_map` becomes viable
// TODO: afaik, map implementation should handle collisions (however rare they are in our case)
// if not, we can always roll static commands allocation and just match strings with strcmp_P

uint32_t lowercase_fnv1_hash(StringView value) {
    constexpr uint32_t fnv_prime = 16777619u;
    constexpr uint32_t fnv_basis = 2166136261u;

    uint32_t hash = fnv_basis;
    for (auto it = value.begin(); it != value.end(); ++it) {
        hash = hash ^ static_cast<uint32_t>(tolower(pgm_read_byte(it)));
        hash = hash * fnv_prime;
    }

    return hash;
}

} // namespace parser

CommandLine parse_line(StringView value) {
    return parser::parse_line(value);
}

} // namespace terminal
} // namespace espurna
