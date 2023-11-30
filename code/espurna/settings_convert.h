/*

Part of SETTINGS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2023 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include "settings_helpers.h"

namespace espurna {
namespace settings {
namespace internal {
namespace duration_convert {

// A more loosely typed duration, so we could have a single type
struct Pair {
    duration::Seconds seconds{};
    duration::Microseconds microseconds{};
};

struct Result {
    Pair value;
    bool ok { false };
};

template <typename T, typename Rep = typename T::rep, typename Period = typename T::period>
std::chrono::duration<Rep, Period> to_chrono_duration(Pair result) {
    using Type = std::chrono::duration<Rep, Period>;
    return std::chrono::duration_cast<Type>(result.seconds)
        + std::chrono::duration_cast<Type>(result.microseconds);
}

// Attempt to parse the given string with the specific ratio
// Same as chrono, std::ratio<1> is a second
Result parse(StringView, int num, int den);

template <intmax_t Num, intmax_t Den>
Result parse(StringView view, std::ratio<Num, Den>) {
    return parse(view, Num, Den);
}

template <typename T>
T unchecked_parse(StringView view) {
    const auto result = parse(view, typename T::period{});
    if (result.ok) {
        return to_chrono_duration<T>(result.value);
    }

    return T{}.min();
}

} // namespace duration_convert

template <typename T>
T convert(const String& value);

template <>
float convert(const String& value);

template <>
double convert(const String& value);

template <>
signed char convert(const String& value);

template <>
short convert(const String& value);

template <>
int convert(const String& value);

template <>
long convert(const String& value);

template <>
bool convert(const String& value);

template <>
unsigned long convert(const String& value);

template <>
unsigned int convert(const String& value);

template <>
unsigned short convert(const String& value);

template <>
unsigned char convert(const String& value);

template <>
duration::Microseconds convert(const String&);

template <>
duration::Milliseconds convert(const String&);

template <>
duration::Seconds convert(const String&);

template <>
duration::Minutes convert(const String&);

template <>
duration::Hours convert(const String&);

inline String serialize(uint8_t value, int base = 10) {
    return String(value, base);
}

inline String serialize(uint16_t value, int base = 10) {
    return String(value, base);
}

String serialize(uint32_t value, int base = 10);

inline String serialize(unsigned long value, int base = 10) {
    return serialize(static_cast<uint32_t>(value), base);
}

inline String serialize(int16_t value, int base = 10) {
    return String(value, base);
}

inline String serialize(int32_t value, int base = 10) {
    return String(value, base);
}

inline String serialize(int8_t value, int base = 10) {
    return serialize(static_cast<int32_t>(value), base);
}

inline String serialize(long value, int base = 10) {
    return String(value, base);
}

inline String serialize(bool value) {
    return value ? PSTR("true") : PSTR("false");
}

inline String serialize(float value) {
    return String(value, 3);
}

inline String serialize(double value) {
    return String(value, 3);
}

template <typename Container, typename T>
T convert(const Container& options, const String& value, T defaultValue) {
    if (value.length()) {
        using espurna::settings::options::Enumeration;
        using UnderlyingType = typename Enumeration<T>::UnderlyingType;
        typename Enumeration<T>::Numeric numeric;
        numeric.check(value, convert<UnderlyingType>);

        for (auto it = std::begin(options); it != std::end(options); ++it) {
            if (numeric && ((*it).numeric() == numeric.value())) {
                return static_cast<T>(numeric.value());
            } else if (!numeric && ((*it) == value)) {
                return (*it).value();
            }
        }
    }

    return defaultValue;
}

template <typename Container, typename T>
String serialize(const Container& options, T value) {
    String out;

    for (auto it = std::begin(options); it != std::end(options); ++it) {
        if ((*it).value() == value) {
            out = FPSTR((*it).string());
            break;
        }
    }

    return out;
}

} // namespace internal
} // namespace settings
} // namespace espurna
