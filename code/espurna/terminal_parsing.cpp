/*

Part of the TERMINAL MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include <vector>
#include <cctype>

#include "terminal_parsing.h"
#include "libs/Delimiter.h"

namespace espurna {
namespace terminal {
namespace parser {

String error(Error value) {
    StringView out;

    switch (value) {
    case Error::Uninitialized:
        out = STRING_VIEW("Uninitialized");
        break;
    case Error::Busy:
        out = STRING_VIEW("Busy");
        break;
    case Error::InvalidEscape:
        out = STRING_VIEW("InvalidEscape");
        break;
    case Error::NoSpaceAfterQuote:
        out = STRING_VIEW("NoSpaceAfterQuote");
        break;
    case Error::UnexpectedLineEnd:
        out = STRING_VIEW("UnexpectedLineEnd");
        break;
    case Error::UnterminatedQuote:
        out = STRING_VIEW("UnterminatedQuote");
        break;
    case Error::Ok:
        out = STRING_VIEW("Ok");
        break;
    }

    return out.toString();
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

// Intermediate storage for parsed tokens and (optionally) buffered values from escaped characters
struct Values {
    // contiguous view to the original input
    // passed as sv to 'tokens' when data can be used as-is
    const char* span_begin { nullptr };
    size_t span_len { 0 };

    // temporary for 'buffer' when escaped data appears and part of the string is copied here
    String token;
    char byte_lhs { 0 };

    Tokens tokens;
    Buffer buffer;

    bool token_available() const {
        return token.length() != 0;
    }

    StringView make_span_view() const {
        return StringView(span_begin, span_len);
    }

    bool span_available() const {
        return span_begin && span_len;
    }

    void reset_span(const char* it) {
        span_begin = it;
        span_len = 0;
    }

    void append_span(const char* it) {
        if (!span_available()) {
            reset_span(it);
        }

        ++span_len;
    }

    void push_span() {
        if (span_available()) {
            token += make_span_view();
            reset_span(nullptr);
        }
    }

    bool push_span_token() {
        if (span_available()) {
            tokens.push_back(make_span_view());
            reset_span(nullptr);
            return true;
        }

        return false;
    }

    bool push_buffered_token() {
        if (token_available()) {
            push_span();
            buffer.push_back(std::move(token));
            tokens.push_back(buffer.back());
            return true;
        }

        return false;
    }

    void append_token(char c) {
        push_span();
        token.concat(&c, 1);
    }

    void append_byte_lhs(char c) {
        byte_lhs = c;
    }

    void append_byte_rhs(char c) {
        append_token(hex_digit_to_value(byte_lhs, c));
    }

    void push_token() {
        if (!push_buffered_token() && !push_span_token()) {
            tokens.push_back(StringView{});
        }
    }
};

struct Result {
    Result() = default;

    Result& operator=(Error error) noexcept {
        _error = error;
        _buffer.clear();
        _tokens.clear();
        return *this;
    }

    Result& operator=(Values&& values) noexcept {
        _buffer = std::move(values.buffer);
        _tokens = std::move(values.tokens);
        _error = Error::Ok;
        return *this;
    }

    Result& operator=(StringView value) noexcept {
        _remaining = value;
        return *this;
    }

    explicit operator bool() const {
        return _error == Error::Ok;
    }

    Error error() const {
        return _error;
    }

    ParsedLine get() {
        auto error = _error;
        _error = Error::Uninitialized;

        auto remaining = _remaining;
        _remaining = StringView{};

        return ParsedLine{
            .tokens = std::move(_tokens),
            .buffer = std::move(_buffer),
            .remaining = std::move(remaining),
            .error = error,
        };
    }

private:
    Error _error { Error::Uninitialized };
    StringView _remaining;
    Tokens _tokens;
    Buffer _buffer;
};

constexpr auto Lf = StringView("\n");

struct Parser {
    Parser() = default;
    Result operator()(StringView view, bool inject_newline = false);

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

    bool _parsing { false };
};

Result Parser::operator()(StringView line, bool inject_newline) {
    Result result;
    Values values;

    auto it = line.begin();
    auto end = line.end();

    auto state = State::Initial;

    ReentryLock lock(_parsing);
    if (!lock.initialized()) {
        result = Error::Busy;
        goto out;
    }

loop:
    for (;it != end; ++it) {
        switch (state) {
        case State::Initial:
            switch (*it) {
            case ' ':
            case '\t':
                break;
            case '\r':
                state = State::CarriageReturn;
                break;
            case ';':
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
                values.push_token();
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
            case ';':
            case '\n':
                values.push_token();
                state = State::Done;
                break;
            default:
                values.append_span(it);
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
                values.push_token();
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
            case ';':
            case '\n':
                state = State::Initial;
                break;
            }
            break;

        case State::EscapedText: {
            switch (*it) {
            case ';':
            case '\r':
            case '\n':
                result = Error::UnexpectedLineEnd;
                goto out;
            case 'x':
                state = State::EscapedByteLhs;
                break;
            default:
                values.append_token(unescape_char(*it));
                break;
            }
            break;
        }

        case State::EscapedByteLhs:
            if (is_hex_digit(*it)) {
                values.append_byte_lhs(*it);
                state = State::EscapedByteRhs;
            } else {
                result = Error::InvalidEscape;
                goto out;
            }
            break;

        case State::EscapedByteRhs:
            if (is_hex_digit(*it)) {
                values.append_byte_rhs(*it);
                state = State::DoubleQuote;
            } else {
                result = Error::InvalidEscape;
                goto out;
            }
            break;

        case State::SingleQuote:
            switch (*it) {
            case ';':
            case '\n':
            case '\r':
                result = Error::UnterminatedQuote;
                goto out;
            case '\\':
                state = State::EscapedQuote;
                break;
            case '\'':
                state = State::AfterQuote;
                break;
            default:
                values.append_span(it);
                break;
            }
            break;

        case State::EscapedQuote:
            switch (*it) {
            case '\'':
                values.append_token(*it);
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
                values.push_token();
                state = State::Initial;
                break;
            case ';':
            case '\n':
                values.push_token();
                state = State::Done;
                break;
            default:
                result = Error::NoSpaceAfterQuote;
                goto out;
            }
            break;

        case State::DoubleQuote:
            switch (*it) {
            case ';':
            case '\n':
            case '\r':
                result = Error::UnterminatedQuote;
                goto out;
            case '"':
                state = State::AfterQuote;
                break;
            case '\\':
                state = State::EscapedText;
                break;
            default:
                values.append_span(it);
                break;
            }
            break;

        }
    }

out:
    switch (state) {
    case State::CarriageReturn:
    case State::CarriageReturnAfterText:
    case State::Text:
    case State::Initial:
    case State::SkipUntilNewLine:
    case State::AfterQuote:
        if (inject_newline) {
            inject_newline = false;
            it = Lf.begin();
            end = Lf.end();
            goto loop;
        }
        break;

    default:
        break;
    }

    if ((it >= line.begin()) && (it <= line.end())) {
        result = StringView(it, end);
    }

    if (state == State::Done) {
        result = std::move(values);
    }

    // whenever line ends before we are done parsing, make sure
    // result contains a valid error condition (same as in the switch above)
    if (result.error() == Error::Uninitialized) {
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

Parser parser_instance;

ParsedLine parse_line(StringView line, bool inject_newline) {
    return parser_instance(line, inject_newline).get();
}

} // namespace
} // namespace parser

ParsedLine parse_line(StringView value) {
    return parser::parse_line(value, false);
}

ParsedLine parse_terminated(StringView value) {
    return parser::parse_line(value, true);
}

} // namespace terminal

namespace {

// rough implementation for an arbitrary string<->string 'first match' of rhs within lhs
const char* find_first(StringView lhs, StringView rhs) {
    auto begin = lhs.begin();
    auto end = lhs.end();

    auto rhs_begin = rhs.begin();
    auto rhs_end = rhs.end();

    for (;;) {
        auto it = begin;

        for (auto rhs_it = rhs_begin;; ++it, ++rhs_it) {
            if (rhs_it == rhs_end) {
                return begin;
            }

            if (it == end) {
                return end;
            }

            if (*rhs_it != *it) {
                break;
            }
        }

        ++begin;
    }

    return end;
}

} // namespace

StringView without_trailing(StringView value, char c) {
    if (value.length()) {
        const auto last = value.length() - 1;
        if (value[last] == c) {
            return value.slice(0, last);
        }
    }

    return value;
}

LineView::LineView(StringView value) :
    DelimiterView(value, terminal::parser::Lf)
{}

StringView LineView::next() {
    return without_trailing(DelimiterView::next(), '\r');
}

namespace line_buffer_impl {

Base::Result Base::next() {
    auto next = DelimiterBuffer::next();
    return Result{
        .value = without_trailing(next.value, '\r'),
        .overflow = next.overflow,
    };
}

Base::Base(char *data, size_t size) :
    DelimiterBuffer(data, size, terminal::parser::Lf)
{}

} // namespace line_buffer_impl

DelimiterBuffer::DelimiterBuffer(char* storage, size_t capacity, StringView delimiter) :
    _storage(storage),
    _capacity(capacity),
    _delimiter(delimiter)
{}

DelimiterBuffer::DelimiterBuffer(char* storage, size_t capacity) :
    DelimiterBuffer(storage, capacity, terminal::parser::Lf)
{}

DelimiterBuffer::Result DelimiterBuffer::next() {
    const auto current = get();

    if (current.length()) {
        if (!_delimiter.length()) {
            auto overflow = _overflow;
            reset();

            return Result{
                .value = current,
                .overflow = overflow,
            };
        }

        auto first = find_first(current, _delimiter);
        if (first != current.end()) {
            const auto value = StringView{current.begin(), first};
            const auto out = Result{
                .value = value,
                .overflow = _overflow
            };

            const auto after = value.length() + _delimiter.length();
            const auto cursor = _cursor + after;
            if (cursor != _size) {
                _cursor = cursor;
            } else {
                reset();
            }

            return out;
        }
    }

    return Result{
        .value = StringView(),
        .overflow = _overflow
    };
}

void DelimiterBuffer::append(const char* data, size_t length) {
    // adjust pointer and length when they immediatelly cause overflow
    auto output = &_storage[_size];

    auto capacity = _capacity - _size;
    while (length > capacity) {
        data += capacity;
        length -= capacity;
        capacity = _capacity;
        output = &_storage[0];
        _size = 0;
        _overflow = true;
    }

    if (length) {
        std::memcpy(output, data, length);
        _size += length;
    }
}

void DelimiterBuffer::append(Stream& stream, size_t length) {
    auto output = &_storage[_size];
    auto capacity = _capacity - _size;

#if defined(ARDUINO_ESP8266_RELEASE_2_7_2) \
|| defined(ARDUINO_ESP8266_RELEASE_2_7_3) \
|| defined(ARDUINO_ESP8266_RELEASE_2_7_4)
#else
    const auto peek = stream.hasPeekBufferAPI();
#endif

    while (length > capacity) {
        const auto chunk = std::min(_capacity - _size, length);

#if defined(ARDUINO_ESP8266_RELEASE_2_7_2) \
|| defined(ARDUINO_ESP8266_RELEASE_2_7_3) \
|| defined(ARDUINO_ESP8266_RELEASE_2_7_4)
        stream.readBytes(output, chunk);
#else
        if (peek) {
            stream.peekConsume(chunk);
        } else {
            stream.readBytes(output, chunk);
        }
#endif

        length -= capacity;
        capacity = _capacity;
        output = &_storage[0];

        _size = 0;
        _overflow = true;
    }

    if (length) {
        stream.readBytes(output, length);
        _size += length;
    }
}

StringView DelimiterView::next() {
    const auto current = get();

    if (current.length()) {
        if (!_delimiter.length()) {
            _cursor = _view.length();
            return current;
        }

        const auto first = find_first(current, _delimiter);
        if (first != current.end()) {
            const auto value = StringView{current.begin(), first};

            _cursor += value.length();
            _cursor += _delimiter.length();

            return value;
        }
    }

    return StringView();
}

constexpr auto Space = StringView(" ");

SplitView::SplitView(StringView view) :
    _remaining(view),
    _delimiter(Space)
{}

SplitView::SplitView(StringView view, StringView delimiter) :
    _remaining(view),
    _delimiter(delimiter)
{}

void SplitView::remaining(const char* it) {
    _current = StringView(_remaining.begin(), it);
    _remaining = StringView(_current.end() + _delimiter.length(), _remaining.end());
}

void SplitView::reset() {
    _current = _remaining;
    _remaining = StringView(_remaining.end(), _remaining.end());
}

bool SplitView::next() {
    if (_remaining.length()) {
        if (_delimiter.length()) {
            const auto first = find_first(_remaining, _delimiter);
            if (first != _remaining.end()) {
                remaining(first);
            } else {
                reset();
            }
        } else {
            reset();
        }

        return true;
    }

    if (_current.length()) {
        _current = _remaining;
    }

    return false;
}

} // namespace espurna
