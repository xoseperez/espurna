/*

Generic digital pin interface

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <cstdint>
#include <memory>

constexpr unsigned char GPIO_NONE { 0x99 };

class BasePin {
public:
    BasePin() = default;
    virtual ~BasePin() = default;

    BasePin(const BasePin&) = delete;
    BasePin(BasePin&&) = delete;

    BasePin& operator=(const BasePin&) = delete;
    BasePin& operator=(BasePin&&) = delete;

    virtual String description() const;

    virtual const char* id() const = 0;
    virtual unsigned char pin() const = 0;

    virtual void pinMode(int8_t mode) = 0;
    virtual void digitalWrite(int8_t val) = 0;
    virtual int digitalRead() = 0;

    explicit operator bool() const {
        return GPIO_NONE != pin();
    }
};

using BasePinPtr = std::unique_ptr<BasePin>;
