/*

SENSOR MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ratio>

#include <ArduinoJson.h>

#include "system.h"

namespace espurna {
namespace sensor {

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
    Ph
};

struct Watts {
    using Type = double;
    using Ratio = std::ratio<1>;
    Type value;
};

struct WattSeconds {
    using Type = uint32_t;
    using Ratio = std::ratio_multiply<
        Watts::Ratio,
        espurna::duration::Seconds::period>;
    Type value { 0 };

    WattSeconds() = default;

    constexpr explicit WattSeconds(Type value) :
        value(value)
    {}

    constexpr explicit WattSeconds(float value) :
        value(static_cast<Type>(value))
    {}

    constexpr explicit WattSeconds(double value) :
        value(static_cast<Type>(value))
    {}

    constexpr WattSeconds(Watts watts, espurna::duration::Seconds seconds) :
        value(static_cast<Type>(watts.value * seconds.count()))
    {}
};

struct WattHours {
    using Type = uint32_t;
    using Ratio = std::ratio_multiply<
        Watts::Ratio,
        espurna::duration::Hours::period>;
    Type value { 0 };

    WattHours() = default;
    explicit WattHours(Type value) :
        value(value)
    {}

    WattHours(Watts watts, espurna::duration::Hours hours) :
        value(static_cast<Type>(watts.value * hours.count()))
    {}
};

struct Kilowatts {
    using Type = double;
    using Ratio = std::ratio<1000>;
    Type value;
};

struct KilowattHours {
    using Type = uint32_t;
    using Ratio = std::ratio_multiply<
        Kilowatts::Ratio,
        espurna::duration::Hours::period>;
    Type value { 0 };

    KilowattHours() = default;
    explicit KilowattHours(Type value) :
        value(value)
    {}

    KilowattHours(Kilowatts kilowatts, espurna::duration::Hours hours) :
        value(static_cast<Type>(kilowatts.value * hours.count()))
    {}
};

template <typename To, typename From,
          typename Divide = std::ratio_divide<typename From::Ratio, typename To::Ratio>>
struct Convert {
    static To from(From from) {
        return To(from.value
                * static_cast<typename To::Type>(Divide::num)
                / static_cast<typename To::Type>(Divide::den));
    }
};

struct Energy {
    struct Pair {
        KilowattHours kwh;
        WattSeconds ws;
    };

    static constexpr auto WattSecondsMax =
        WattSeconds::Type(KilowattHours::Ratio::num);

    Energy() = default;
    Energy(const Energy&) = default;
    Energy(Energy&&) = default;

    // energy always consists of kwh + ws pair
    explicit Energy(Pair);

    // prefer integral types
    explicit Energy(WattSeconds);
    explicit Energy(WattHours);
    explicit Energy(KilowattHours kwh) :
        _kwh(kwh)
    {}

    // special case for kwh input
    explicit Energy(double);

    Energy& operator=(const Energy&) = default;
    Energy& operator=(Energy&&) = default;

    // sets internal counters to zero
    void reset();

    // check whether we have *any* energy recorded. Can be zero:
    // - on cold boot
    // - on overflow
    // - when we call `reset()`
    explicit operator bool() const;

    // allow generic math operation when dealing with energy delta
    Energy& operator+=(const Energy&);

    // most sensor implementations handle energy in joules / watt-second
    Energy& operator+=(WattSeconds);
    Energy operator+(WattSeconds);

    // numeric representation as kWh
    double asDouble() const;

    // API representation as `<kWh>+<Ws>`
    String asString() const;

    // represent internal value as watt seconds / joules
    // **will rollover** when exceeding WattSeconds::Type capacity
    // (when kWh value is greater or equal to `maximum of Type / Ws per kWh`)
    WattSeconds asWattSeconds() const;

    // we are storing a kind-of integral and fractional parts
    // using watt-second to avoid loosing precision, we don't expect these to be accessed directly
    Pair pair() const {
        return Pair {
            .kwh = _kwh,
            .ws = _ws,
        };
    }

private:
    KilowattHours _kwh;
    WattSeconds _ws;
};

struct Value {
    static constexpr double Unknown { std::numeric_limits<double>::quiet_NaN() };

    unsigned char type;
    unsigned char index;

    Unit units;
    unsigned char decimals;

    double value;

    String topic;
    String repr;

    explicit operator bool() const;
};

struct Info {
    unsigned char type;
    unsigned char index;

    Unit units;
    unsigned char decimals;

    String topic;
    String description;
};

} // namespace sensor
} // namespace espurna

//--------------------------------------------------------------------------------

String magnitudeTypeTopic(unsigned char type);

using MagnitudeReadHandler = void(*)(const espurna::sensor::Value&);
void sensorOnMagnitudeRead(MagnitudeReadHandler handler);
void sensorOnMagnitudeReport(MagnitudeReadHandler handler);

size_t sensorCount();
size_t magnitudeCount();

// Base magnitude info. Will contain `.type = MAGNITUDE_NONE` when index is out of bounds
espurna::sensor::Info magnitudeInfo(unsigned char index);
String magnitudeUnits(espurna::sensor::Unit);

unsigned char magnitudeType(unsigned char index);
unsigned char magnitudeIndex(unsigned char index);

String magnitudeTopic(unsigned char index);
String magnitudeUnits(unsigned char index);
String magnitudeDescription(unsigned char index);

// Retrieves magnitude value. Depends on the internal 'real time' setting,
// whether the value is the latest read or the last reported.
espurna::sensor::Value magnitudeValue(unsigned char index);

using SensorWebSocketMagnitudesCallback = void(*)(JsonArray&, size_t);
void sensorWebSocketMagnitudes(JsonObject& root, const char* prefix, SensorWebSocketMagnitudesCallback);

espurna::StringView sensorList();
void sensorSetup();
