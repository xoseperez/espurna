/*

Arduino Print buffer. Size is fixed, unlike StreamString.

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>
#include <Print.h>

#include <core_version.h>


struct PrintString final : public Print, public String {

    PrintString(size_t reserved) :
        _reserved(reserved)
    {
        reserve(reserved);
    }

    size_t write(const uint8_t* data, size_t size) override {
        if (!size || !data) return 0;

        // we *will* receive C-strings as input
        size_t want = length() + size;
        if (data[size - 1] == '\0') {
            size -= 1;
            want -= 1;
        }

        if (want > _reserved) return 0;

// XXX: 2.3.0 uses str... methods that expect '0' at the end of the 'data'
//      see WString{.cpp,.h} for the implementation
#if defined(ARDUINO_ESP8266_RELEASE_2_3_0)
        std::copy(data, data + size, buffer + len);
        len = want;
        buffer[len] = '\0';
#else
        concat(reinterpret_cast<const char*>(data), size);
#endif

        return size;
    }

    size_t write(uint8_t ch) override {
        if (length() + 1 > _reserved) return 0;
        return concat(static_cast<char>(ch));
    }

    private:

    const size_t _reserved;

};
