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

template <>
duration::Microseconds convert(const String& value) {
    return espurna::duration::unchecked_parse<duration::Microseconds>(value);
}

template <>
duration::Milliseconds convert(const String& value) {
    return espurna::duration::unchecked_parse<duration::Milliseconds>(value);
}

template <>
duration::Seconds convert(const String& value) {
    return espurna::duration::unchecked_parse<duration::Seconds>(value);
}

template <>
duration::Minutes convert(const String& value) {
    return espurna::duration::unchecked_parse<duration::Minutes>(value);
}

template <>
duration::Hours convert(const String& value) {
    return espurna::duration::unchecked_parse<duration::Hours>(value);
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

String serialize(duration::Seconds value) {
    return serialize(value.count());
}

String serialize(duration::Milliseconds value) {
    return serialize(value.count());
}

String serialize(duration::Minutes value) {
    return serialize(value.count()) + 'm';
}

String serialize(duration::Hours value) {
    return serialize(value.count()) + 'h';
}

} // namespace internal
} // namespace settings
} // namespace espurna

