/*

Part of SETTINGS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2023 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include <Arduino.h>

#include "utils.h"

#include "settings_convert.h"
#include "settings_helpers.h"

namespace espurna {
namespace settings {
namespace internal {
namespace duration_convert {
namespace {

// Input is always normalized to Pair, specific units are converted on demand

constexpr auto MicrosecondsPerSecond =
    duration::Microseconds{ duration::Microseconds::period::den };

void adjust_microseconds(Pair& pair) {
    if (pair.microseconds >= MicrosecondsPerSecond) {
        pair.seconds += duration::Seconds{ 1 };
        pair.microseconds -= MicrosecondsPerSecond;
    }
}

Pair from_chrono_duration(duration::Microseconds microseconds) {
    Pair out{};

    while (microseconds > MicrosecondsPerSecond) {
        out.seconds += duration::Seconds{ 1 };
        microseconds -= MicrosecondsPerSecond;
    }

    out.microseconds += microseconds;
    adjust_microseconds(out);

    return out;
}

constexpr auto MillisecondsPerSecond =
    duration::Milliseconds{ duration::Milliseconds::period::den };

Pair from_chrono_duration(duration::Milliseconds milliseconds) {
    Pair out{};

    while (milliseconds >= MillisecondsPerSecond) {
        out.seconds += duration::Seconds{ 1 };
        milliseconds -= MillisecondsPerSecond;
    }

    const auto microseconds =
        std::chrono::duration_cast<duration::Microseconds>(milliseconds);
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
Pair& operator+=(Pair& result, duration::Microseconds microseconds) {
    result += from_chrono_duration(microseconds);
    return result;
}

template <>
Pair& operator+=(Pair& result, duration::Milliseconds milliseconds) {
    result += from_chrono_duration(milliseconds);
    return result;
}

template <>
Pair& operator+=(Pair& result, duration::Hours hours) {
    result.seconds += std::chrono::duration_cast<duration::Seconds>(hours);
    return result;
}

template <>
Pair& operator+=(Pair& result, duration::Minutes minutes) {
    result.seconds += std::chrono::duration_cast<duration::Seconds>(minutes);
    return result;
}

template <>
Pair& operator+=(Pair& result, duration::Seconds seconds) {
    result.seconds += seconds;
    return result;
}

// Besides decimal or raw input with the specified ratio,
// string parser also supports type specifiers at the end of decimal number

enum class Type {
    Unknown,
    Seconds,
    Minutes,
    Hours,
};

bool validNextType(Type lhs, Type rhs) {
    switch (lhs) {
    case Type::Unknown:
        return true;
    case Type::Hours:
        return (rhs == Type::Minutes) || (rhs == Type::Seconds);
    case Type::Minutes:
        return (rhs == Type::Seconds);
    case Type::Seconds:
        break;
    }

    return false;
}

} // namespace

Result parse(StringView view, int num, int den) {
    Result out;
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

        case 'm':
            if (validNextType(last, Type::Minutes)) {
                type = Type::Minutes;
                goto update_spec;
            }
            goto reset;

        case 's':
            if (validNextType(last, Type::Seconds)) {
                type = Type::Seconds;
                goto update_spec;
            }
            goto reset;

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
                std::chrono::duration_cast<duration::Milliseconds>(seconds);

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
                out.value += duration::Hours{ result.value };
                break;

            case Type::Minutes:
                out.value += duration::Minutes{ result.value };
                break;

            case Type::Seconds:
                out.value += duration::Seconds{ result.value };
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

} // namespace duration_convert

template <>
duration::Microseconds convert(const String& value) {
    return duration_convert::unchecked_parse<duration::Microseconds>(value);
}

template <>
duration::Milliseconds convert(const String& value) {
    return duration_convert::unchecked_parse<duration::Milliseconds>(value);
}

template <>
duration::Seconds convert(const String& value) {
    return duration_convert::unchecked_parse<duration::Seconds>(value);
}

template <>
duration::Minutes convert(const String& value) {
    return duration_convert::unchecked_parse<duration::Minutes>(value);
}

template <>
duration::Hours convert(const String& value) {
    return duration_convert::unchecked_parse<duration::Hours>(value);
}

template <>
float convert(const String& value) {
    return strtod(value.c_str(), nullptr);
}

template <>
double convert(const String& value) {
    return strtod(value.c_str(), nullptr);
}

template <>
signed char convert(const String& value) {
    return value.toInt();
}

template <>
short convert(const String& value) {
    return value.toInt();
}

template <>
int convert(const String& value) {
    return value.toInt();
}

template <>
long convert(const String& value) {
    return value.toInt();
}

template <>
bool convert(const String& value) {
    if (value.length()) {
        if ((value == "0")
            || (value == "n")
            || (value == "no")
            || (value == "false")
            || (value == "off")) {
            return false;
        }

        return (value == "1")
            || (value == "y")
            || (value == "yes")
            || (value == "true")
            || (value == "on");
    }

    return false;
}

template <>
uint32_t convert(const String& value) {
    return parseUnsigned(value).value;
}

String serialize(uint32_t value, int base) {
    return formatUnsigned(value, base);
}

template <>
unsigned long convert(const String& value) {
    return convert<unsigned int>(value);
}

template <>
unsigned short convert(const String& value) {
    return convert<unsigned long>(value);
}

template <>
unsigned char convert(const String& value) {
    return convert<unsigned long>(value);
}

} // namespace internal
} // namespace settings
} // namespace espurna
