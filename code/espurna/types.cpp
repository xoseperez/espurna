/*

Part of the SYSTEM MODULE

Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "types.h"
#include "utils.h"

namespace espurna {

void Callback::swap(Callback& other) noexcept {
    if (_type == other._type) {
        switch (_type) {
        case StorageType::Empty:
            break;
        case StorageType::Simple:
            std::swap(_storage.simple, other._storage.simple);
            break;
        case StorageType::Wrapper:
            std::swap(_storage.wrapper, other._storage.wrapper);
            break;
        }
        return;
    }

    auto moved = std::move(*this);
    *this = std::move(other);
    other = std::move(moved);
}

void Callback::operator()() const {
    switch (_type) {
    case StorageType::Empty:
        break;
    case StorageType::Simple:
        (*_storage.simple)();
        break;
    case StorageType::Wrapper:
        _storage.wrapper();
        break;
    }
}

void Callback::copy(const Callback& other) {
    _type = other._type;

    switch (other._type) {
    case StorageType::Empty:
        break;
    case StorageType::Simple:
        _storage.simple = other._storage.simple;
        break;
    case StorageType::Wrapper:
        new (&_storage.wrapper) WrapperType(
            other._storage.wrapper);
        break;
    }
}

void Callback::move(Callback& other) noexcept {
    _type = other._type;

    switch (other._type) {
    case StorageType::Empty:
        break;
    case StorageType::Simple:
        _storage.simple = other._storage.simple;
        break;
    case StorageType::Wrapper:
        new (&_storage.wrapper) WrapperType(
            std::move(other._storage.wrapper));
        break;
    }

    other._storage.simple = nullptr;
    other._type = StorageType::Empty;
}

void Callback::reset() {
    switch (_type) {
    case StorageType::Empty:
    case StorageType::Simple:
        break;
    case StorageType::Wrapper:
        _storage.wrapper.~WrapperType();
        break;
    }

    _storage.simple = nullptr;
    _type = StorageType::Empty;
}

Callback& Callback::operator=(Callback&& other) noexcept {
    reset();
    move(other);
    return *this;
}

bool StringView::equals(StringView other) const {
    if (other._len == _len) {
        if (inFlash(_ptr) && inFlash(other._ptr)) {
            return _ptr == other._ptr;
        } else if (inFlash(_ptr)) {
            return memcmp_P(other._ptr, _ptr, _len) == 0;
        } else if (inFlash(other._ptr)) {
            return memcmp_P(_ptr, other._ptr, _len) == 0;
        }

        return __builtin_memcmp(_ptr, other._ptr, _len) == 0;
    }

    return false;
}

bool StringView::equalsIgnoreCase(StringView other) const {
    if (other._len == _len) {
        if (inFlash(_ptr) && inFlash(other._ptr) && (_ptr == other._ptr)) {
            return true;
        } else if (inFlash(_ptr) || inFlash(other._ptr)) {
            String copy;
            const char* ptr = _ptr;
            if (inFlash(_ptr)) {
                copy = toString();
                ptr = copy.begin();
            }

            return strncasecmp_P(ptr, other._ptr, _len) == 0;
        }

        return __builtin_strncasecmp(_ptr, other._ptr, _len) == 0;
    }

    return false;
}

bool StringView::startsWith(StringView other) const {
    if (other._len <= _len) {
        return StringView(begin(), begin() + other._len).equals(other);
    }

    return false;
}

bool StringView::endsWith(StringView other) const {
    if (other._len <= _len) {
        return StringView(end() - other._len, end()).equals(other);
    }

    return false;
}

StringView StringView::slice(size_t index, size_t len) const {
    return StringView(_ptr + std::min(index, _len), std::min(len, _len - index));
}

StringView StringView::slice(size_t index) const {
    return slice(index, _len - index);
}

namespace duration {
namespace {

// Input is always normalized to Pair, specific units are converted on demand

constexpr auto MicrosecondsPerSecond =
    Microseconds{ Microseconds::period::den };

void adjust_microseconds(Pair& pair) {
    if (pair.microseconds >= MicrosecondsPerSecond) {
        pair.seconds += Seconds{ 1 };
        pair.microseconds -= MicrosecondsPerSecond;
    }
}

Pair from_chrono(Microseconds microseconds) {
    Pair out{};

    while (microseconds > MicrosecondsPerSecond) {
        out.seconds += Seconds{ 1 };
        microseconds -= MicrosecondsPerSecond;
    }

    out.microseconds += microseconds;
    adjust_microseconds(out);

    return out;
}

constexpr auto MillisecondsPerSecond =
    Milliseconds{ Milliseconds::period::den };

Pair from_chrono(Milliseconds milliseconds) {
    Pair out{};

    while (milliseconds >= MillisecondsPerSecond) {
        out.seconds += Seconds{ 1 };
        milliseconds -= MillisecondsPerSecond;
    }

    const auto microseconds =
        std::chrono::duration_cast<Microseconds>(milliseconds);
    out.microseconds += microseconds;
    adjust_microseconds(out);

    return out;
}

Pair& operator+=(Pair& lhs, const Pair& rhs) {
    lhs.seconds += rhs.seconds;
    lhs.microseconds += rhs.microseconds;

    adjust_microseconds(lhs);

    return lhs;
}

template <typename T>
Pair& operator+=(Pair&, T);

template <>
Pair& operator+=(Pair& result, Microseconds microseconds) {
    result += from_chrono(microseconds);
    return result;
}

template <>
Pair& operator+=(Pair& result, Milliseconds milliseconds) {
    result += from_chrono(milliseconds);
    return result;
}

template <>
Pair& operator+=(Pair& result, Hours hours) {
    result.seconds += std::chrono::duration_cast<Seconds>(hours);
    return result;
}

template <>
Pair& operator+=(Pair& result, Minutes minutes) {
    result.seconds += std::chrono::duration_cast<Seconds>(minutes);
    return result;
}

template <>
Pair& operator+=(Pair& result, Seconds seconds) {
    result.seconds += seconds;
    return result;
}

// Besides decimal or raw input with the specified ratio,
// string parser also supports type specifiers at the end of decimal number

enum class Type {
    Unknown,
    Microseconds,
    Milliseconds,
    Seconds,
    Minutes,
    Hours,
};

bool validNextType(Type lhs, Type rhs) {
    switch (lhs) {
    case Type::Unknown:
        return true;
    case Type::Hours:
        return (rhs == Type::Minutes)
            || (rhs == Type::Seconds)
            || (rhs == Type::Milliseconds)
            || (rhs == Type::Microseconds);
    case Type::Minutes:
        return (rhs == Type::Seconds)
            || (rhs == Type::Milliseconds)
            || (rhs == Type::Microseconds);
    case Type::Seconds:
        return (rhs == Type::Milliseconds)
            || (rhs == Type::Microseconds);
    case Type::Milliseconds:
        return (rhs == Type::Microseconds);
    case Type::Microseconds:
        break;
    }

    return false;
}

} // namespace

PairResult parse(StringView view, int num, int den) {
    PairResult out;
    out.ok = false;

    String token;
    Type last { Type::Unknown };
    Type type { Type::Unknown };

    const char* ptr { view.begin() };
    if (!view.begin() || !view.length()) {
        goto output;
    }

loop:
    while (ptr != view.end()) {
        switch (*ptr) {
        case '0'...'9':
            token += (*ptr);
            ++ptr;
            break;

        case 'h':
            if (validNextType(last, Type::Hours)) {
                type = Type::Hours;
                goto update_spec;
            }
            goto reset;

        case 's':
            if (validNextType(last, Type::Seconds)) {
                type = Type::Seconds;
                goto update_spec;
            }
            goto reset;

        case 'm':
            goto read_minutes_or_millis;

        case '\xce':
            goto read_micros_utf8;

        case 'u':
            goto read_micros;

        case 'e':
        case 'E':
            goto read_floating_exponent;

        case ',':
        case '.':
            if (out.ok) {
                goto reset;
            }

            goto read_floating;

        default:
            goto reset;
        }
    }

    if (token.length()) {
        goto update_decimal;
    }

    goto output;

update_floating:
    {
        // only seconds and up, anything down of milli does not make sense here
        if (den > 1) {
            goto reset;
        }

        char* endp { nullptr };
        auto value = strtod(token.c_str(), &endp);
        if (endp && (endp != token.c_str()) && endp[0] == '\0') {
            using Seconds = std::chrono::duration<float, std::ratio<1> >;

            const auto seconds = Seconds(num * value);
            const auto milliseconds =
                std::chrono::duration_cast<Milliseconds>(seconds);

            out.value += milliseconds;
            out.ok = true;

            goto output;
        }

        goto reset;
    }

update_decimal:
    {
        const auto result = parseUnsigned(token, 10);
        if (result.ok) {
            // num and den are constexpr and bound to ratio types, so duration cast has to happen manually
            if ((num == 1) && (den == 1)) {
                out.value += duration::Seconds{ result.value };
            } else if ((num == 1) && (den > 1)) {
                out.value += duration::Seconds{ result.value / den };
                out.value += duration::Microseconds{ result.value % den * duration::Microseconds::period::den / den };
            } else if ((num > 1) && (den == 1)) {
                out.value += duration::Seconds{ result.value * num };
            } else {
                goto reset;
            }

            out.ok = true;
            goto output;
        }

        goto reset;
    }

update_spec:
    last = type;
    ++ptr;

    if (type != Type::Unknown) {
        const auto result = parseUnsigned(token, 10);
        if (result.ok) {
            switch (type) {
            case Type::Hours:
                out.value += Hours{ result.value };
                break;

            case Type::Minutes:
                out.value += Minutes{ result.value };
                break;

            case Type::Seconds:
                out.value += Seconds{ result.value };
                break;

            case Type::Milliseconds:
                out.value += Milliseconds{ result.value };
                break;

            case Type::Microseconds:
                out.value += Microseconds{ result.value };
                break;

            case Type::Unknown:
                goto reset;
            }

            out.ok = true;
            type = Type::Unknown;
            token = "";

            goto loop;
        }
    }

    goto reset;

read_minutes:
    if (validNextType(last, Type::Minutes)) {
        type = Type::Minutes;
        goto update_spec;
    }

    goto reset;


read_minutes_or_millis:
    if (std::next(ptr) == view.end()) {
        goto read_minutes;
    }

    if (*std::next(ptr) != 's') {
        goto read_minutes;
    }

    ++ptr;

    if (validNextType(last, Type::Milliseconds)) {
        type = Type::Milliseconds;
        goto update_spec;
    }

    goto reset;

read_micros_utf8:
    ++ptr;

    if ((ptr != view.end()) && (*ptr == '\xbc')) {
        goto read_micros;
    }

    goto reset;

read_micros:
    ++ptr;

    if (ptr == view.end()) {
        goto reset;
    }

    if (((*ptr) == 's')
     && validNextType(last, Type::Microseconds))
    {
        type = Type::Microseconds;
        goto update_spec;
    }

    goto reset;

read_floating:
    switch (*ptr) {
    case ',':
    case '.':
        token += '.';
        ++ptr;
        break;

    default:
        goto reset;
    }

    while (ptr != view.end()) {
        switch (*ptr) {
        case '0'...'9':
            token += (*ptr);
            break;

        case 'e':
        case 'E':
            goto read_floating_exponent;

        case ',':
        case '.':
            goto reset;
        }

        ++ptr;
    }

    goto update_floating;

read_floating_exponent:
    {
        token += (*ptr);
        ++ptr;

        bool sign { false };

        while (ptr != view.end()) {
            switch (*ptr) {
            case '-':
            case '+':
                if (sign) {
                    goto reset;
                }

                sign = true;

                token += (*ptr);
                ++ptr;
                break;

            case '0'...'9':
                token += (*ptr);
                ++ptr;
                break;

            default:
                goto reset;
            }
        }

        goto update_floating;
    }

reset:
    out.ok = false;

output:
    return out;
}

} // namespace duration
} // namespace espurna
