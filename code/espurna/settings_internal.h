/*

SETTINGS MODULE

*/

#pragma once

#include <cstdlib>

// --------------------------------------------------------------------------

namespace settings {
namespace internal {

template <typename T>
using convert_t = T(*)(const String& value);

template <typename T>
T convert(const String& value);

// --------------------------------------------------------------------------

template <>
float convert(const String& value) {
    return value.toFloat();
}

template <>
double convert(const String& value) {
    return value.toFloat();
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
    return convert<int>(value) == 1;
}

template <>
unsigned long convert(const String& value) {
    return strtoul(value.c_str(), nullptr, 10);
}

template <>
unsigned int convert(const String& value) {
    return convert<unsigned long>(value);
}

template <>
unsigned short convert(const String& value) {
    return convert<unsigned long>(value);
}

template <>
unsigned char convert(const String& value) {
    return convert<unsigned long>(value);
}

template <>
String convert(const String& value) {
    return value;
}

} // namespace settings::internal
} // namespace settings
