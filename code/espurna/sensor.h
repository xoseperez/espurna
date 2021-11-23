/*

SENSOR MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

namespace sensor {
namespace type {

static constexpr unsigned char Base { 0 };
static constexpr unsigned char Emon { 1 << 0 };
static constexpr unsigned char AnalogEmon { 1 << 1 };
static constexpr unsigned char Analog { 1 << 2 };

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
    Hertz,
    Ph,
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
    explicit Energy(double);
    explicit Energy(KWh, Ws);
    explicit Energy(KWh);
    explicit Energy(Wh);
    explicit Energy(Ws);

    // Sets internal counters to zero
    void reset();

    // Check whether we have *any* energy recorded. Can be zero:
    // - on cold boot
    // - on overflow
    // - when we call `reset()`
    explicit operator bool() const;

    // Generic conversion as-is
    double asDouble() const;
    String asString() const;

    // Convert back to input unit, with overflow mechanics when kwh values goes over 32 bit
    Ws asWs() const;

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

struct ReadValue {
    double raw;
    double processed;
    double filtered;
};

struct Value {
    double get();

    bool real_time;
    double last;
    double reported;
    unsigned char decimals;
};

} // namespace sensor

//--------------------------------------------------------------------------------

using MagnitudeReadHandler = void(*)(const String&, unsigned char, double, const char*);
void sensorOnMagnitudeRead(MagnitudeReadHandler handler);
void sensorOnMagnitudeReport(MagnitudeReadHandler handler);

String magnitudeUnits(unsigned char index);
String magnitudeDescription(unsigned char index);
unsigned char magnitudeType(unsigned char index);

unsigned char magnitudeIndex(unsigned char index);
String magnitudeTopicIndex(unsigned char index);

unsigned char magnitudeCount();

sensor::Value magnitudeValue(unsigned char index);
void magnitudeFormat(const sensor::Value& value, char* output, size_t size);

// XXX: without param name it is kind of vague what exactly unsigned char is
//      consider adding stronger param type e.g. enum class
String magnitudeTopic(unsigned char type);
String magnitudeName(unsigned char type);

String sensorError(unsigned char error);

using SensorWebSocketMagnitudesCallback = void(*)(JsonArray&, size_t);
void sensorWebSocketMagnitudes(JsonObject& root, const char* prefix, SensorWebSocketMagnitudesCallback);

unsigned char sensorCount();
void sensorSetup();
void sensorLoop();
