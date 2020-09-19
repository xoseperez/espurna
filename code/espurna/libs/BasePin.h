/*

Generic digital pin interface

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <cstdint>
#include "../config/types.h"

class BasePin {
    public:

    // TODO: we always need to explicitly call the constructor from the child
    // class, because we need to set the const `pin` member on construction
    // - https://isocpp.org/wiki/faq/multiple-inheritance#virtual-inheritance-ctors

    // TODO: vtable anchoring? same applies to every implemented ..._pin.h
    // not a problem when using single-source aka unity build (build with `env ESPURNA_BUILD_SINGLE_SOURCE=1`)
    //
    // Some sources:
    // - https://llvm.org/docs/CodingStandards.html#provide-a-virtual-method-anchor-for-classes-in-headers
    // > If a class is defined in a header file and has a vtable (either it has virtual methods or it derives from classes with virtual methods),
    // > it must always have at least one out-of-line virtual method in the class. Without this, the compiler will copy the vtable and RTTI into
    // > every .o file that #includes the header, bloating .o file sizes and increasing link times.
    // - http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1263r0.pdf
    // > This technique is unfortunate as it relies on detailed knowledge of how common toolchains work, and it may also require creating
    // > a dummy virtual function.

    explicit BasePin(unsigned char pin) :
        pin(pin)
    {}

    virtual ~BasePin() {
    }

    virtual operator bool() {
        return GPIO_NONE != pin;
    }

    virtual void pinMode(int8_t mode) = 0;
    virtual void digitalWrite(int8_t val) = 0;
    virtual int digitalRead() = 0;
    virtual String description() const = 0;

    const unsigned char pin { GPIO_NONE };
};
