/*

SETTINGS MODULE

*/

#pragma once

#include <functional>
#include <utility>

// --------------------------------------------------------------------------

template <typename T>
T _settingsConvert(const String& value);

template <>
float _settingsConvert(const String& value) {
    return value.toFloat();
}

template <>
double _settingsConvert(const String& value) {
    return _settingsConvert<float>(value);
}

template <>
int _settingsConvert(const String& value) {
    return value.toInt();
}

template <>
long _settingsConvert(const String& value) {
    return value.toInt();
}

template <>
bool _settingsConvert(const String& value) {
    return _settingsConvert<int>(value) == 1;
}

template <>
unsigned long _settingsConvert(const String& value) {
    return strtoul(value.c_str(), nullptr, 10);
}

template <>
unsigned int _settingsConvert(const String& value) {
    return _settingsConvert<unsigned long>(value);
}

template <>
unsigned short _settingsConvert(const String& value) {
    return _settingsConvert<unsigned long>(value);
}

template <>
unsigned char _settingsConvert(const String& value) {
    return _settingsConvert<unsigned long>(value);
}

template <>
String _settingsConvert(const String& value) {
    return value;
}

