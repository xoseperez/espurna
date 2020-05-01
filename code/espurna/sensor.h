/*

SENSOR MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include "espurna.h"

//--------------------------------------------------------------------------------

namespace sensor {

namespace type {

enum Type : unsigned char {
    Base = 0,
    Emon = 1 << 0,
    Analog = 1 << 1
};

} // namespace type

enum class Unit : int {
    Min_,
    None,
    Celcius,
    Farenheit,
    Kelvin,
    Percentage,
    Hectopascal,
    Ampere,
    Volt,
    Voltampere,
    Kilovoltampere,
    VoltampereReactive,
    KilovoltampereReactive,
    Watt,
    Kilowatt,
    WattSecond,
    Joule = WattSecond,
    KilowattHour,
    PartsPerMillion,
    Ohm,
    MicrogrammPerCubicMeter,   // The concentration of an air pollutant
    MilligrammPerCubicMeter,   //
    Lux,
    UltravioletIndex,          // "measurement of the strength of sunburn-producing ultraviolet (UV) radiation at a particular place and time"
                               // (XXX: Not a unit. Distinguish from None and specify decimals)
    CountsPerMinute,           // Unit of local dose rate (Geiger counting)
    MicrosievertPerHour,       // 2nd unit of local dose rate (Geiger counting)
    Meter,
    Max_
};

// Base units are 32 bit since we are the fastest with them.

struct Ws {
    Ws();
    Ws(uint32_t);
    uint32_t value;
};

struct Wh {
    Wh();
    Wh(Ws);
    Wh(uint32_t);
    uint32_t value;
};

struct KWh {
    KWh();
    KWh(Ws);
    KWh(Wh);
    KWh(uint32_t);
    uint32_t value;
};

struct Energy {

    constexpr static uint32_t KwhMultiplier = 3600000ul;
    constexpr static uint32_t KwhLimit = ((1ul << 31ul) / KwhMultiplier);

    Energy() = default;

    // TODO: while we accept ws >= the kwh conversion limit,
    // should this be dealt with on the unit level?
    Energy(double);
    Energy(KWh, Ws);
    Energy(KWh);
    Energy(Wh);
    Energy(Ws);

    // Sets internal counters to zero
    void reset();

    // Check whether we have *any* energy recorded. Can be zero:
    // - on cold boot
    // - on overflow
    // - when we call `reset()`
    operator bool();

    // Generic conversion as-is
    double asDouble();

    // Convert back to input unit, with overflow mechanics when kwh values goes over 32 bit
    Ws asWs();

    // Generic sensors output energy in joules / watt-second
    Energy& operator +=(Ws);
    Energy operator +(Ws);

    // But sometimes we want to accept asDouble() value back
    Energy& operator =(double);

    // We are storing a kind-of integral and fractional parts
    // Using watt-second to avoid loosing precision, we don't expect these to be accessed directly
    KWh kwh;
    Ws ws;
};

}

String magnitudeName(unsigned char index);
String magnitudeUnits(unsigned char index);
unsigned char magnitudeType(unsigned char index);

// XXX: without param name it is kind of vague what exactly unsigned char is
//      consider using index instead of type or adding stronger param type
String magnitudeTopic(unsigned char type);

unsigned char sensorCount();
unsigned char magnitudeCount();

double magnitudeValue(unsigned char index);

unsigned char magnitudeIndex(unsigned char index);
String magnitudeTopicIndex(unsigned char index);

void sensorWebSocketMagnitudes(JsonObject& root, const String& prefix);

void sensorSetup();
void sensorLoop();

