/*

Arduino Print without any buffer.

*/

#pragma once

#include <Arduino.h>

#include <cstdint>
#include <cstddef>

struct EphemeralPrint : public Print {
    size_t write(uint8_t) override {
        return 0;
    }

    size_t write(const uint8_t*, size_t) override {
        return 0;
    }
};
