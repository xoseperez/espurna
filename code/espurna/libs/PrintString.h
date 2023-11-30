/*

Arduino Print buffer. Size is fixed, unlike StreamString.

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>
#include <Print.h>

struct PrintString final : public Print, public String {
    PrintString() = delete;
    PrintString(size_t reserved) :
        _reserved(reserved)
    {
        reserve(reserved);
    }

    size_t write(const uint8_t* data, size_t size) override {
        if (!size || !data) {
            return 0;
        }

        // we *will* receive C-strings as input
        size_t want = length() + size;
        if (data[size - 1] == '\0') {
            size -= 1;
            want -= 1;
        }

        if (want > _reserved) {
            return 0;
        }

        concat(reinterpret_cast<const char*>(data), size);

        return size;
    }

    size_t write(uint8_t ch) override {
        if (length() + 1 > _reserved) {
            return 0;
        }

        return concat(static_cast<char>(ch));
    }

private:
    size_t _reserved;
};
