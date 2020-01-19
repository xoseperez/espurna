/*

SETTINGS MODULE

*/

#pragma once

#include <functional>
#include <utility>

// --------------------------------------------------------------------------

template <typename T>
struct settings_convert_t {
    static T convert(const String& value);
};

template <>
float settings_convert_t<float>::convert(const String& value) {
    return value.toFloat();
}

template <>
double settings_convert_t<double>::convert(const String& value) {
    return value.toFloat();
}

template <>
int settings_convert_t<int>::convert(const String& value) {
    return value.toInt();
}

template <>
long settings_convert_t<long>::convert(const String& value) {
    return value.toInt();
}

template <>
bool settings_convert_t<bool>::convert(const String& value) {
    return settings_convert_t<int>::convert(value) == 1;
}

template <>
unsigned long settings_convert_t<unsigned long>::convert(const String& value) {
    return strtoul(value.c_str(), nullptr, 10);
}

template <>
unsigned int settings_convert_t<unsigned int>::convert(const String& value) {
    return settings_convert_t<unsigned long>::convert(value);
}

template <>
unsigned short settings_convert_t<unsigned short>::convert(const String& value) {
    return settings_convert_t<unsigned long>::convert(value);
}

template <>
unsigned char settings_convert_t<unsigned char>::convert(const String& value) {
    return settings_convert_t<unsigned long>::convert(value);
}

template <>
String settings_convert_t<String>::convert(const String& value) {
    return value;
}

