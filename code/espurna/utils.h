/*

UTILS MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <Arduino.h>

#include "system.h"

String prettyDuration(espurna::duration::Seconds);

bool sslCheckFingerPrint(const char * fingerprint);
bool sslFingerPrintArray(const char * fingerprint, unsigned char * bytearray);
bool sslFingerPrintChar(const char * fingerprint, char * destination);

char* strnstr(const char* buffer, const char* token, size_t n);
bool isNumber(const char* begin, const char* end);
bool isNumber(const String&);

uint32_t randomNumber();
uint32_t randomNumber(uint32_t minimum, uint32_t maximum);

double roundTo(double num, unsigned char positions);
bool almostEqual(double lhs, double rhs, int ulp);
bool almostEqual(double lhs, double rhs);

struct ParseUnsignedResult {
    bool ok;
    uint32_t value;
};

ParseUnsignedResult parseUnsigned(espurna::StringView, int base);
ParseUnsignedResult parseUnsigned(espurna::StringView);
String formatUnsigned(uint32_t value, int base);

char* hexEncode(const uint8_t* in_begin, const uint8_t* in_end, char* out_begin, char* out_end);
size_t hexEncode(const uint8_t* in, size_t in_size, char* out, size_t out_size);
String hexEncode(const uint8_t* begin, const uint8_t* end);

template <size_t Size>
inline String hexEncode(const uint8_t (&buffer)[Size]) {
    return hexEncode(std::begin(buffer), std::end(buffer));
}

inline String hexEncode(uint8_t value) {
    uint8_t buffer[1] { value };
    return hexEncode(buffer);
}

uint8_t* hexDecode(const char* in_begin, const char* in_end, uint8_t* out_begin, uint8_t* out_end);
size_t hexDecode(const char* in, size_t in_size, uint8_t* out, size_t out_size);

bool tryParseId(espurna::StringView, size_t limit, size_t& out);
bool tryParseIdPath(espurna::StringView, size_t limit, size_t& out);

espurna::StringView stripNewline(espurna::StringView);

size_t consumeAvailable(Stream&);
