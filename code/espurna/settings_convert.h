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

String serialize(duration::Microseconds);

String serialize(duration::Milliseconds);

String serialize(duration::Seconds);

String serialize(duration::Minutes);

String serialize(duration::Hours);

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
            out = (*it).string().toString();
            break;
        }
    }

    return out;
}

} // namespace internal
} // namespace settings
} // namespace espurna

