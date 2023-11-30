/*

SENSOR MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020-2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if SENSOR_SUPPORT

#include "sensor.h"

#include "api.h"
#include "domoticz.h"
#include "i2c.h"
#include "mqtt.h"
#include "ntp.h"
#include "relay.h"
#include "terminal.h"
#include "thingspeak.h"
#include "rtcmem.h"
#include "ws.h"

#include <cfloat>
#include <cmath>
#include <cstring>

#include <limits>
#include <vector>

//--------------------------------------------------------------------------------

#include "sensors/BaseSensor.h"
#include "sensors/BaseEmonSensor.h"
#include "sensors/BaseAnalogEmonSensor.h"
#include "sensors/BaseAnalogSensor.h"

#if DUMMY_SENSOR_SUPPORT
    #include "sensors/DummySensor.h"
#endif

#if AM2320_SUPPORT
    #include "sensors/AM2320Sensor.h"
#endif

#if ANALOG_SUPPORT
    #include "sensors/AnalogSensor.h"
#endif

#if BH1750_SUPPORT
    #include "sensors/BH1750Sensor.h"
#endif

#if BMP180_SUPPORT
    #include "sensors/BMP180Sensor.h"
#endif

#if BMX280_SUPPORT
    #include "sensors/BMX280Sensor.h"
#endif

#if BME680_SUPPORT
    #include "sensors/BME680Sensor.h"
#endif

#if CSE7766_SUPPORT
    #include "sensors/CSE7766Sensor.h"
#endif

#if DALLAS_SUPPORT
    #include "sensors/DallasSensor.h"
#endif

#if DHT_SUPPORT
    #include "sensors/DHTSensor.h"
#endif

#if DIGITAL_SUPPORT
    #include "sensors/DigitalSensor.h"
#endif

#if ECH1560_SUPPORT
    #include "sensors/ECH1560Sensor.h"
#endif

#if EMON_ADC121_SUPPORT
    #include "sensors/EmonADC121Sensor.h"
#endif

#if EMON_ADS1X15_SUPPORT
    #include "sensors/EmonADS1X15Sensor.h"
#endif

#if EMON_ANALOG_SUPPORT
    #include "sensors/EmonAnalogSensor.h"
#endif

#if EVENTS_SUPPORT
    #include "sensors/EventSensor.h"
#endif

#if EZOPH_SUPPORT
    #include "sensors/EZOPHSensor.h"
#endif

#if GEIGER_SUPPORT
    #include "sensors/GeigerSensor.h"
#endif

#if GUVAS12SD_SUPPORT
    #include "sensors/GUVAS12SDSensor.h"
#endif

#if HLW8012_SUPPORT
    #include "sensors/HLW8012Sensor.h"
#endif

#if INA219_SUPPORT
    #include "sensors/INA219Sensor.h"
#endif

#if LDR_SUPPORT
    #include "sensors/LDRSensor.h"
#endif

#if MAX6675_SUPPORT
    #include "sensors/MAX6675Sensor.h"
#endif

#if MICS2710_SUPPORT
    #include "sensors/MICS2710Sensor.h"
#endif

#if MICS5525_SUPPORT
    #include "sensors/MICS5525Sensor.h"
#endif

#if MHZ19_SUPPORT
    #include "sensors/MHZ19Sensor.h"
#endif

#if NTC_SUPPORT
    #include "sensors/NTCSensor.h"
#endif

#if SDS011_SUPPORT
    #include "sensors/SDS011Sensor.h"
#endif

#if SENSEAIR_SUPPORT
    #include "sensors/SenseAirSensor.h"
#endif

#if PM1006_SUPPORT
    #include "sensors/PM1006Sensor.h"
#endif

#if PMSX003_SUPPORT
    #include "sensors/PMSX003Sensor.h"
#endif

#if PULSEMETER_SUPPORT
    #include "sensors/PulseMeterSensor.h"
#endif

#if PZEM004T_SUPPORT
    #include "sensors/PZEM004TSensor.h"
#endif

#if SHT3X_I2C_SUPPORT
    #include "sensors/SHT3XI2CSensor.h"
#endif

#if SI7021_SUPPORT
    #include "sensors/SI7021Sensor.h"
#endif

#if SM300D2_SUPPORT
    #include "sensors/SM300D2Sensor.h"
#endif

#if SONAR_SUPPORT
    #include "sensors/SonarSensor.h"
#endif

#if T6613_SUPPORT
    #include "sensors/T6613Sensor.h"
#endif

#if TMP3X_SUPPORT
    #include "sensors/TMP3XSensor.h"
#endif

#if V9261F_SUPPORT
    #include "sensors/V9261FSensor.h"
#endif

#if VEML6075_SUPPORT
    #include "sensors/VEML6075Sensor.h"
#endif

#if VL53L1X_SUPPORT
    #include "sensors/VL53L1XSensor.h"
#endif

#if ADE7953_SUPPORT
    #include "sensors/ADE7953Sensor.h"
#endif

#if SI1145_SUPPORT
    #include "sensors/SI1145Sensor.h"
#endif

#if HDC1080_SUPPORT
    #include "sensors/HDC1080Sensor.h"
#endif

#if PZEM004TV30_SUPPORT
    #include "sensors/PZEM004TV30Sensor.h"
#endif

#include "filters/LastFilter.h"
#include "filters/MaxFilter.h"
#include "filters/MedianFilter.h"
#include "filters/MovingAverageFilter.h"
#include "filters/SumFilter.h"

//--------------------------------------------------------------------------------

namespace espurna {
namespace sensor {

Value::operator bool() const {
    return !std::isinf(value) && !std::isnan(value);
}

String error(unsigned char error) {
    const char* result { nullptr };

    switch (error) {
    case SENSOR_ERROR_OK:
        result = PSTR("OK");
        break;
    case SENSOR_ERROR_OUT_OF_RANGE:
        result = PSTR("Out of Range");
        break;
    case SENSOR_ERROR_WARM_UP:
        result = PSTR("Warming Up");
        break;
    case SENSOR_ERROR_TIMEOUT:
        result = PSTR("Timeout");
        break;
    case SENSOR_ERROR_UNKNOWN_ID:
        result = PSTR("Unknown ID");
        break;
    case SENSOR_ERROR_CRC:
        result = PSTR("CRC / Data Error");
        break;
    case SENSOR_ERROR_I2C:
        result = PSTR("I2C Error");
        break;
    case SENSOR_ERROR_GPIO_USED:
        result = PSTR("GPIO Already Used");
        break;
    case SENSOR_ERROR_CALIBRATION:
        result = PSTR("Calibration Error");
        break;
    case SENSOR_ERROR_OVERFLOW:
        result = PSTR("Value Overflow");
        break;
    case SENSOR_ERROR_NOT_READY:
        result = PSTR("Not Ready");
        break;
    case SENSOR_ERROR_CONFIG:
        result = PSTR("Invalid Configuration");
        break;
    case SENSOR_ERROR_SUPPORT:
        result = PSTR("Not Supported");
        break;
    case SENSOR_ERROR_OTHER:
    default:
        result = PSTR("Other / Unknown Error");
        break;
    }

    return result;
}

template <typename T>
void forEachError(T&& callback) {
    for (unsigned char error = SENSOR_ERROR_OK; error < SENSOR_ERROR_MAX; ++error) {
        callback(error);
    }
}

struct ReadValue {
    double raw;       // as the sensor returns it
    double processed; // after applying units and decimals
    double filtered;  // after applying filters, units and decimals
};

enum class Filter : int {
    Last,
    Max,
    Median,
    MovingAverage,
    Sum,
};

// Generic storage. Most of the time we init this on boot with both members or start at 0 and increment with watt-second

Energy::Energy(Energy::Pair pair) :
    _kwh(pair.kwh),
    _ws(pair.ws)
{}

Energy::Energy(WattSeconds ws) {
    _ws.value = ws.value;
    while (_ws.value >= WattSecondsMax) {
        _ws.value -= WattSecondsMax;
        ++_kwh.value;
    }
}

Energy::Energy(WattHours other) :
    Energy(static_cast<double>(other.value) / 1000.0)
{}

Energy::Energy(double kwh) {
    double lhs;
    double rhs = fs_modf(kwh, &lhs);

    _kwh.value = lhs;
    _ws.value = rhs * static_cast<double>(KilowattHours::Ratio::num);
}

Energy& Energy::operator+=(WattSeconds other) {
    return *this += Energy(other);
}

Energy Energy::operator+(WattSeconds other) {
    Energy result(*this);
    result += other;

    return result;
}

Energy& Energy::operator+=(const Energy& other) {
    _kwh.value += other._kwh.value;

    const auto left = WattSecondsMax - _ws.value;
    if (other._ws.value >= left) {
        _kwh.value += 1;
        _ws.value += (other._ws.value - left);
    } else {
        _ws.value += other._ws.value;
    }

    return *this;
}

Energy::operator bool() const {
    return (_kwh.value > 0) && (_ws.value > 0);
}

WattSeconds Energy::asWattSeconds() const {
    using Type = WattSeconds::Type;

    static constexpr auto TypeMax = std::numeric_limits<Type>::max();
    static constexpr Type KwhMax { TypeMax / WattSecondsMax };

    auto kwh = _kwh.value;
    while (kwh >= KwhMax) {
        kwh -= KwhMax;
    }

    WattSeconds out;
    out.value += _ws.value;
    out.value += kwh * WattSecondsMax;

    return out;
}

double Energy::asDouble() const {
    return static_cast<double>(_kwh.value)
        + static_cast<double>(_ws.value)
        / static_cast<double>(WattSecondsMax);
}

String Energy::asString() const {
    String out;

    // Value without `+` is treated as just `<kWh>`
    out += String(_kwh.value, 10);
    if (_ws.value) {
        out += '+';
        out += String(_ws.value, 10);
    }

    return out;
}

void Energy::reset() {
    *this = Energy{};
}

namespace {

class BaseSensorPtr {
public:
    BaseSensorPtr() = delete;

    constexpr BaseSensorPtr(const BaseSensorPtr&) = default;
    constexpr BaseSensorPtr(BaseSensorPtr&&) noexcept = default;

#if __cplusplus > 201103L
    constexpr BaseSensorPtr& operator=(const BaseSensorPtr&) = default;
    constexpr BaseSensorPtr& operator=(BaseSensorPtr&&) noexcept = default;
#else
    BaseSensorPtr& operator=(const BaseSensorPtr&) = default;
    BaseSensorPtr& operator=(BaseSensorPtr&&) noexcept = default;
#endif

    constexpr BaseSensorPtr(std::nullptr_t) = delete;
    constexpr BaseSensorPtr& operator=(std::nullptr_t) = delete;

    constexpr BaseSensorPtr(BaseSensor* ptr) :
        _ptr(ptr)
    {}

    constexpr BaseSensor* get() const {
        return _ptr;
    }

    constexpr BaseSensor* operator->() const {
        return _ptr;
    }

private:
    BaseSensor* _ptr;
};

using BaseFilterPtr = std::unique_ptr<BaseFilter>;

class Magnitude {
private:
    static unsigned char _counts[MAGNITUDE_MAX];

public:
    static size_t counts(unsigned char type) {
        return _counts[type];
    }

    Magnitude() = delete;

    Magnitude(const Magnitude&) = delete;
    Magnitude& operator=(const Magnitude&) = delete;

    Magnitude(Magnitude&& other) noexcept = default;
    Magnitude& operator=(Magnitude&&) noexcept = default;

    Magnitude(BaseSensorPtr, unsigned char slot, unsigned char type);

    BaseSensorPtr sensor; // Sensor object, *cannot be empty*
    unsigned char slot; // Sensor slot # taken by the magnitude, used to access the measurement
    unsigned char type; // Type of measurement, returned by the BaseSensor::type(slot)

    unsigned char index_global; // N'th magnitude of it's type, across all of the active sensors

    Unit units { Unit::None }; // Units of measurement
    unsigned char decimals { 0u }; // Number of decimals in textual representation

    Filter filter_type { Filter::Median }; // Instead of using raw value, filter it through a filter object
    BaseFilterPtr filter; // *cannot be empty*

    double last { Value::Unknown }; // Last raw value from sensor (unfiltered)
    double reported { Value::Unknown }; // Last reported value

    double min_delta { 0.0 }; // Minimum value change to report
    double max_delta { 0.0 }; // Maximum value change to report
    double correction { 0.0 }; // Value correction (applied when processing)
    double zero_threshold { Value::Unknown }; // Reset value to zero when below threshold (applied when reading)
};

static_assert(
    std::is_nothrow_move_constructible<Magnitude>::value,
    "std::vector<Magnitude> should be able to work with resize()"
);

static_assert(
    !std::is_copy_constructible<Magnitude>::value,
    "std::vector<Magnitude> should only use move ctor"
);

Magnitude::Magnitude(BaseSensorPtr sensor, unsigned char slot, unsigned char type) :
    sensor(std::move(sensor)),
    slot(slot),
    type(type),
    index_global(_counts[type])
{
    ++_counts[type];
}

unsigned char Magnitude::_counts[MAGNITUDE_MAX] = {0};

bool isEmon(BaseSensorPtr sensor) {
    return (sensor->kind() == BaseEmonSensor::Kind)
        || (sensor->kind() == BaseAnalogEmonSensor::Kind);
}

bool isAnalogEmon(BaseSensorPtr sensor) {
    return sensor->kind() == BaseAnalogEmonSensor::Kind;
}

bool isAnalog(BaseSensorPtr sensor) {
    return sensor->kind() == BaseAnalogSensor::Kind;
}

} // namespace

namespace convert {
namespace temperature {
namespace {

struct Base {
    constexpr Base() = default;
    constexpr explicit Base(double value) :
        _value(value)
    {}

    constexpr double value() const {
        return _value;
    }

    constexpr operator double() const {
        return _value;
    }

private:
    double _value { 0.0 };
};

struct Kelvin : public Base {
    using Base::Base;
};

struct Farenheit : public Base {
    using Base::Base;
};

struct Celcius : public Base {
    using Base::Base;
};

static constexpr Celcius AbsoluteZero { -273.15 };

namespace internal {

template <typename To, typename From, typename Same = void>
struct Converter {
};

template <typename To, typename From>
struct Converter<To, From, typename std::enable_if<std::is_same<To, From>::value>::type> {
    static constexpr To convert(To value) {
        return value;
    }
};

static constexpr double celcius_to_kelvin(double celcius) {
    return celcius - AbsoluteZero;
}

static constexpr double celcius_to_farenheit(double celcius) {
    return (celcius * (9.0 / 5.0)) + 32.0;
}

static constexpr double farenheit_to_celcius(double farenheit) {
    return (farenheit - 32.0) * (5.0 / 9.0);
}

static constexpr double farenheit_to_kelvin(double farenheit) {
    return celcius_to_kelvin(farenheit_to_celcius(farenheit));
}

static constexpr double kelvin_to_celcius(double kelvin) {
    return kelvin + AbsoluteZero;
}

static constexpr double kelvin_to_farenheit(double kelvin) {
    return celcius_to_farenheit(kelvin_to_celcius(kelvin));
}

static_assert(celcius_to_kelvin(kelvin_to_celcius(0.0)) == 0.0, "");
static_assert(celcius_to_farenheit(farenheit_to_celcius(0.0)) == 0.0, "");
static_assert(farenheit_to_kelvin(kelvin_to_farenheit(0.0)) == 0.0, "");
static_assert(farenheit_to_celcius(celcius_to_farenheit(0.0)) == 0.0, "");
static_assert(kelvin_to_celcius(celcius_to_kelvin(0.0)) == 0.0, "");

// ref. https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
static constexpr bool almost_equal(double lhs, double rhs, int ulp) {
    // the machine epsilon has to be scaled to the magnitude of the values used
    // and multiplied by the desired precision in ULPs (units in the last place)
    return __builtin_fabs(lhs - rhs) <= std::numeric_limits<double>::epsilon() * __builtin_fabs(lhs + rhs) * ulp
        // unless the result is subnormal
        || __builtin_fabs(lhs - rhs) < std::numeric_limits<double>::min();
}

static_assert(almost_equal(10.0, kelvin_to_farenheit(farenheit_to_kelvin(10.0)), 3), "");

template <>
struct Converter<Celcius, Kelvin> {
    static constexpr Celcius convert(Kelvin kelvin) {
        return Celcius{ kelvin_to_celcius(kelvin.value()) };
    }
};

template <>
struct Converter<Farenheit, Kelvin> {
    static constexpr Farenheit convert(Kelvin kelvin) {
        return Farenheit{ kelvin_to_farenheit(kelvin.value()) };
    }
};

template <>
struct Converter<Kelvin, Celcius> {
    static constexpr Kelvin convert(Celcius celcius) {
        return Kelvin{ celcius_to_kelvin(celcius.value()) };
    }
};

template <>
struct Converter<Farenheit, Celcius> {
    static constexpr Farenheit convert(Celcius celcius) {
        return Farenheit{ celcius_to_farenheit(celcius.value()) };
    }
};

template <>
struct Converter<Kelvin, Farenheit> {
    static constexpr Kelvin convert(Farenheit farenheit) {
        return Kelvin{ farenheit_to_kelvin(farenheit.value()) };
    }
};

template <>
struct Converter<Celcius, Farenheit> {
    static constexpr Celcius convert(Farenheit farenheit) {
        return Celcius{ farenheit_to_celcius(farenheit.value()) };
    }
};

// just some sanity checks. note that floating point will not always produce exact results
// (and it might not be a good idea to actually have anything compare with the Farenheit one)

static_assert(Converter<Kelvin, Kelvin>::convert(Kelvin{0.0}) == Kelvin{0.0}, "");
static_assert(Converter<Kelvin, Celcius>::convert(AbsoluteZero) == Kelvin{0.0}, "");
static_assert(Converter<Celcius, Celcius>::convert(AbsoluteZero) == AbsoluteZero, "");
static_assert(Converter<Celcius, Kelvin>::convert(Kelvin{0.0}) == AbsoluteZero, "");

} // namespace internal

template <typename To, typename From>
constexpr To unit_cast(From value) {
    return internal::Converter<To, From>::convert(value);
}

static_assert(unit_cast<Kelvin>(AbsoluteZero).value() == 0.0, "");
static_assert(unit_cast<Celcius>(AbsoluteZero).value() == AbsoluteZero.value(), "");

constexpr bool supported(Unit unit) {
    return (unit == Unit::Celcius)
        || (unit == Unit::Kelvin)
        || (unit == Unit::Farenheit);
}

// since the outside api only works with the enumeration, make sure to cast it to our types for conversion
// a table like this could've also worked
// > {Unit(from), Unit(to), Converter(double(*)(double))}
// but, it is ~0.6KiB vs. ~0.1KiB for this one. plus, some obstacles with c++11 implementation
// although, there may be a way to make this cheaper in both compile-time and runtime

// attempt to convert the input value from one unit to the other
// will return the input value when units match or there's no known conversion
constexpr double convert(double value, Unit from, Unit to) {
#define UNIT_CAST(LHS, RHS) \
    ((from == Unit::LHS) && (to == Unit::RHS)) \
        ? (unit_cast<RHS, LHS>(LHS{value})) : \
    ((from == Unit::RHS) && (to == Unit::LHS)) \
        ? (unit_cast<LHS, RHS>(RHS{value}))

     return UNIT_CAST(Kelvin, Celcius) :
        UNIT_CAST(Kelvin, Farenheit) :
        UNIT_CAST(Celcius, Farenheit) : value;

#undef UNIT_CAST
}

} // namespace
} // namespace temperature

// right now, limited to plain and kilo values
// (since we mostly care about a fairly small values)
// type conversion should only work for related types
namespace metric {
namespace {

template <typename __Ratio>
struct Base {
    using Type = double;
    using Ratio = __Ratio;

    constexpr Base() = default;
    constexpr explicit Base(Type value) :
        _value(value)
    {}

    constexpr Type value() const {
        return _value;
    }

    constexpr operator Type() const {
        return _value;
    }

private:
    Type _value { 0.0 };
};

template <typename To, typename From>
struct convertible_base : std::false_type {
};

template <typename To, typename From>
constexpr bool is_convertible_base() {
    return std::is_same<To, From>::value
        || std::is_base_of<std::true_type, convertible_base<To, From>>::value
        || std::is_base_of<std::true_type, convertible_base<From, To>>::value;
}

template <typename To, typename From>
using is_convertible = std::enable_if<is_convertible_base<To, From>()>;

template <typename To, typename From,
          typename Divide = std::ratio_divide<typename From::Ratio, typename To::Ratio>,
          typename = typename is_convertible<To, From>::type>
constexpr To unit_cast(From value) {
    return To(value.value()
            * static_cast<typename To::Type>(Divide::num)
            / static_cast<typename To::Type>(Divide::den));
}

struct Watt : public Base<std::ratio<1, 1>> {
    using Base::Base;
};

struct Kilowatt : public Base<std::ratio<1000, 1>> {
    using Base::Base;
};

template <>
struct convertible_base<Watt, Kilowatt> : std::true_type {
};

struct Voltampere : public Base<std::ratio<1, 1>> {
    using Base::Base;
};

struct Kilovoltampere : public Base<std::ratio<1000, 1>> {
    using Base::Base;
};

template <>
struct convertible_base<Voltampere, Kilovoltampere> : std::true_type {
};

struct VoltampereReactive : public Base<std::ratio<1, 1>> {
    using Base::Base;
};

struct KilovoltampereReactive : public Base<std::ratio<1000, 1>> {
    using Base::Base;
};

template <>
struct convertible_base<VoltampereReactive, KilovoltampereReactive> : std::true_type {
};

struct WattSecond : public Base<std::ratio<1, 1>> {
    using Base::Base;
};

using Joule = WattSecond;

struct KilowattHour : public Base<std::ratio<3600000, 1>> {
    using Base::Base;
};

template <>
struct convertible_base<WattSecond, KilowattHour> : std::true_type {
};

static_assert(is_convertible_base<Voltampere, Kilovoltampere>(), "");
static_assert(is_convertible_base<Kilovoltampere, Voltampere>(), "");

static_assert(!is_convertible_base<KilovoltampereReactive, Voltampere>(), "");
static_assert(is_convertible_base<Joule, WattSecond>(), "");

static_assert(unit_cast<Joule>(KilowattHour{0.02}) == 72000.0, "");
static_assert(unit_cast<VoltampereReactive>(KilovoltampereReactive{1234.0}) == 1234000.0, "");

constexpr bool supported(Unit unit) {
    return (unit == Unit::Voltampere)
        || (unit == Unit::Kilovoltampere)
        || (unit == Unit::VoltampereReactive)
        || (unit == Unit::KilovoltampereReactive)
        || (unit == Unit::Watt)
        || (unit == Unit::Kilowatt)
        || (unit == Unit::Joule)
        || (unit == Unit::WattSecond)
        || (unit == Unit::KilowattHour);
}

// Here we only care about the direct counterparts
// Plus, we still don't enforce supported() at compile time,
// only safeguard is unit_cast<> failing for 'incompatible' base types

constexpr double convert(double value, Unit from, Unit to) {
#define UNIT_CAST(LHS, RHS) \
    ((from == Unit::LHS) && (to == Unit::RHS)) \
        ? (unit_cast<RHS, LHS>(LHS{value})) : \
    ((from == Unit::RHS) && (to == Unit::LHS)) \
        ? (unit_cast<LHS, RHS>(RHS{value}))

    return UNIT_CAST(Watt, Kilowatt) :
        UNIT_CAST(Voltampere, Kilovoltampere) :
        UNIT_CAST(VoltampereReactive, KilovoltampereReactive) :
        UNIT_CAST(Joule, KilowattHour) :
        UNIT_CAST(WattSecond, KilowattHour) : value;

#undef UNIT_CAST
}

} // namespace
} // namespace metric
} // namespace convert

namespace build {
namespace {

constexpr double DefaultMinDelta { 0.0 };
constexpr double DefaultMaxDelta { 0.0 };

constexpr espurna::duration::Seconds initInterval() {
    return espurna::duration::Seconds(SENSOR_INIT_INTERVAL);
}

constexpr espurna::duration::Seconds ReadIntervalMin { SENSOR_READ_MIN_INTERVAL };
constexpr espurna::duration::Seconds ReadIntervalMax { SENSOR_READ_MAX_INTERVAL };

constexpr espurna::duration::Seconds readInterval() {
    return espurna::duration::Seconds(SENSOR_READ_INTERVAL);
}

constexpr size_t ReportEveryMin PROGMEM { SENSOR_REPORT_MIN_EVERY };
constexpr size_t ReportEveryMax PROGMEM { SENSOR_REPORT_MAX_EVERY };

constexpr size_t reportEvery() {
    return SENSOR_REPORT_EVERY;
}

constexpr size_t saveEvery() {
    return SENSOR_SAVE_EVERY;
}

constexpr bool realTimeValues() {
    return SENSOR_REAL_TIME_VALUES == 1;
}

constexpr bool useIndex() {
    return SENSOR_USE_INDEX == 1;
}

} // namespace
} // namespace build

namespace settings {
namespace filters {
namespace {

PROGMEM_STRING(Last, "last");
PROGMEM_STRING(Max, "max");
PROGMEM_STRING(Median, "median");
PROGMEM_STRING(MovingAverage, "moving-average");
PROGMEM_STRING(Sum, "sum");

static constexpr espurna::settings::options::Enumeration<Filter> Options[] PROGMEM {
    {Filter::Last, Last},
    {Filter::Max, Max},
    {Filter::Median, Median},
    {Filter::MovingAverage, MovingAverage},
    {Filter::Sum, Sum},
};

} // namespace
} // namespace filters

namespace units {
namespace {

PROGMEM_STRING(Farenheit, "°F");
PROGMEM_STRING(Celcius, "°C");
PROGMEM_STRING(Kelvin, "K");
PROGMEM_STRING(Percentage, "%");
PROGMEM_STRING(Hectopascal, "hPa");
PROGMEM_STRING(Ampere, "A");
PROGMEM_STRING(Volt, "V");
PROGMEM_STRING(Watt, "W");
PROGMEM_STRING(Kilowatt, "kW");
PROGMEM_STRING(Voltampere, "VA");
PROGMEM_STRING(Kilovoltampere, "kVA");
PROGMEM_STRING(VoltampereReactive, "VAR");
PROGMEM_STRING(KilovoltampereReactive, "kVAR");
PROGMEM_STRING(Joule, "J");
PROGMEM_STRING(KilowattHour, "kWh");
PROGMEM_STRING(MicrogrammPerCubicMeter, "µg/m³");
PROGMEM_STRING(PartsPerMillion, "ppm");
PROGMEM_STRING(Lux, "lux");
PROGMEM_STRING(UltravioletIndex, "UVindex");
PROGMEM_STRING(Ohm, "ohm");
PROGMEM_STRING(MilligrammPerCubicMeter, "mg/m³");
PROGMEM_STRING(CountsPerMinute, "cpm");
PROGMEM_STRING(MicrosievertPerHour, "µSv/h");
PROGMEM_STRING(Meter, "m");
PROGMEM_STRING(Hertz, "Hz");
PROGMEM_STRING(Ph, "pH");
PROGMEM_STRING(None, "none");

static constexpr espurna::settings::options::Enumeration<Unit> Options[] PROGMEM {
    {Unit::Farenheit, Farenheit},
    {Unit::Celcius, Celcius},
    {Unit::Kelvin, Kelvin},
    {Unit::Percentage, Percentage},
    {Unit::Hectopascal, Hectopascal},
    {Unit::Ampere, Ampere},
    {Unit::Volt, Volt},
    {Unit::Watt, Watt},
    {Unit::Kilowatt, Kilowatt},
    {Unit::Voltampere, Voltampere},
    {Unit::Kilovoltampere, Kilovoltampere},
    {Unit::VoltampereReactive, VoltampereReactive},
    {Unit::KilovoltampereReactive, KilovoltampereReactive},
    {Unit::Joule, Joule},
    {Unit::WattSecond, Joule},
    {Unit::KilowattHour, KilowattHour},
    {Unit::MicrogrammPerCubicMeter, MicrogrammPerCubicMeter},
    {Unit::PartsPerMillion, PartsPerMillion},
    {Unit::Lux, Lux},
    {Unit::UltravioletIndex, UltravioletIndex},
    {Unit::Ohm, Ohm},
    {Unit::MilligrammPerCubicMeter, MilligrammPerCubicMeter},
    {Unit::CountsPerMinute, CountsPerMinute},
    {Unit::MicrosievertPerHour, MicrosievertPerHour},
    {Unit::Meter, Meter},
    {Unit::Hertz, Hertz},
    {Unit::Ph, Ph},
    {Unit::None, None},
};

} // namespace
} // namespace units

namespace prefix {
namespace {

PROGMEM_STRING(Sensor, "sns");
PROGMEM_STRING(Power, "pwr");

PROGMEM_STRING(Temperature, "tmp");
PROGMEM_STRING(Humidity, "hum");
PROGMEM_STRING(Pressure, "press");
PROGMEM_STRING(Current, "curr");
PROGMEM_STRING(Voltage, "volt");
PROGMEM_STRING(PowerActive, "pwrP");
PROGMEM_STRING(PowerApparent, "pwrQ");
PROGMEM_STRING(PowerReactive, "pwrModS");
PROGMEM_STRING(PowerFactor, "pwrPF");
PROGMEM_STRING(Energy, "ene");
PROGMEM_STRING(EnergyDelta, "eneDelta");
PROGMEM_STRING(Analog, "analog");
PROGMEM_STRING(Digital, "digital");
PROGMEM_STRING(Event, "event");
PROGMEM_STRING(Pm1Dot0, "pm1dot0");
PROGMEM_STRING(Pm2Dot5, "pm2dot5");
PROGMEM_STRING(Pm10, "pm10");
PROGMEM_STRING(Co2, "co2");
PROGMEM_STRING(Voc, "voc");
PROGMEM_STRING(Iaq, "iaq");
PROGMEM_STRING(IaqAccuracy, "iaqAccuracy");
PROGMEM_STRING(IaqStatic, "iaqStatic");
PROGMEM_STRING(Lux, "lux");
PROGMEM_STRING(Uva, "uva");
PROGMEM_STRING(Uvb, "uvb");
PROGMEM_STRING(Uvi, "uvi");
PROGMEM_STRING(Distance, "distance");
PROGMEM_STRING(Hcho, "hcho");
PROGMEM_STRING(GeigerCpm, "gcpm");
PROGMEM_STRING(GeigerSievert, "gsiev");
PROGMEM_STRING(Count, "count");
PROGMEM_STRING(No2, "no2");
PROGMEM_STRING(Co, "co");
PROGMEM_STRING(Resistance, "res");
PROGMEM_STRING(Ph, "ph");
PROGMEM_STRING(Frequency, "freq");
PROGMEM_STRING(Tvoc, "tvoc");
PROGMEM_STRING(Ch2o, "ch2o");

PROGMEM_STRING(Unknown, "unknown");

constexpr StringView get(unsigned char type) {
    return (type == MAGNITUDE_TEMPERATURE) ? Temperature :
        (type == MAGNITUDE_HUMIDITY) ? Humidity :
        (type == MAGNITUDE_PRESSURE) ? Pressure :
        (type == MAGNITUDE_CURRENT) ? Current :
        (type == MAGNITUDE_VOLTAGE) ? Voltage :
        (type == MAGNITUDE_POWER_ACTIVE) ? PowerActive :
        (type == MAGNITUDE_POWER_APPARENT) ? PowerApparent :
        (type == MAGNITUDE_POWER_REACTIVE) ? PowerReactive :
        (type == MAGNITUDE_POWER_FACTOR) ? PowerFactor :
        (type == MAGNITUDE_ENERGY) ? Energy :
        (type == MAGNITUDE_ENERGY_DELTA) ? EnergyDelta :
        (type == MAGNITUDE_ANALOG) ? Analog :
        (type == MAGNITUDE_DIGITAL) ? Digital :
        (type == MAGNITUDE_EVENT) ? Event :
        (type == MAGNITUDE_PM1DOT0) ? Pm1Dot0 :
        (type == MAGNITUDE_PM2DOT5) ? Pm2Dot5 :
        (type == MAGNITUDE_PM10) ? Pm10 :
        (type == MAGNITUDE_CO2) ? Co2 :
        (type == MAGNITUDE_VOC) ? Voc :
        (type == MAGNITUDE_IAQ) ? Iaq :
        (type == MAGNITUDE_IAQ_ACCURACY) ? IaqAccuracy :
        (type == MAGNITUDE_IAQ_STATIC) ? IaqStatic :
        (type == MAGNITUDE_LUX) ? Lux :
        (type == MAGNITUDE_UVA) ? Uva :
        (type == MAGNITUDE_UVB) ? Uvb :
        (type == MAGNITUDE_UVI) ? Uvi :
        (type == MAGNITUDE_DISTANCE) ? Distance :
        (type == MAGNITUDE_HCHO) ? Hcho :
        (type == MAGNITUDE_GEIGER_CPM) ? GeigerCpm :
        (type == MAGNITUDE_GEIGER_SIEVERT) ? GeigerSievert :
        (type == MAGNITUDE_COUNT) ? Count :
        (type == MAGNITUDE_NO2) ? No2 :
        (type == MAGNITUDE_CO) ? Co :
        (type == MAGNITUDE_RESISTANCE) ? Resistance :
        (type == MAGNITUDE_PH) ? Ph :
        (type == MAGNITUDE_FREQUENCY) ? Frequency :
        (type == MAGNITUDE_TVOC) ? Tvoc :
        (type == MAGNITUDE_CH2O) ? Ch2o :
        Unknown;
}

} // namespace
} // namespace prefix

namespace suffix {
namespace {

PROGMEM_STRING(Correction, "Correction");
PROGMEM_STRING(MaxDelta, "MaxDelta");
PROGMEM_STRING(MinDelta, "MinDelta");
PROGMEM_STRING(Precision, "Precision");
PROGMEM_STRING(Ratio, "Ratio");
PROGMEM_STRING(Units, "Units");
PROGMEM_STRING(ZeroThreshold, "ZeroThreshold");

PROGMEM_STRING(Mains, "Mains");
PROGMEM_STRING(Reference, "Reference");

PROGMEM_STRING(Total, "Total");

PROGMEM_STRING(Filter, "Filter");

} // namespace
} // namespace suffix

namespace keys {
namespace {

PROGMEM_STRING(ReadInterval, "snsRead");
PROGMEM_STRING(InitInterval, "snsInit");
PROGMEM_STRING(ReportEvery, "snsReport");
PROGMEM_STRING(SaveEvery, "snsSave");
PROGMEM_STRING(RealTimeValues, "snsRealTime");

espurna::settings::Key get(espurna::StringView prefix, espurna::StringView suffix, size_t index) {
    String key;
    key.reserve(prefix.length() + suffix.length() + 4);
    key.concat(prefix.c_str(), prefix.length());
    key.concat(suffix.c_str(), suffix.length());

    return espurna::settings::Key(std::move(key), index);
}

espurna::settings::Key get(const Magnitude& magnitude, espurna::StringView suffix) {
    return get(prefix::get(magnitude.type), suffix, magnitude.index_global);
}

} // namespace
} // namespace keys

namespace {

espurna::duration::Seconds readInterval() {
    return std::clamp(getSetting(FPSTR(keys::ReadInterval), build::readInterval()),
            build::ReadIntervalMin, build::ReadIntervalMax);
}

espurna::duration::Seconds initInterval() {
    return std::clamp(getSetting(FPSTR(keys::InitInterval), build::initInterval()),
            build::ReadIntervalMin, build::ReadIntervalMax);
}

size_t reportEvery() {
    return std::clamp(getSetting(FPSTR(keys::ReportEvery), build::reportEvery()),
            build::ReportEveryMin, build::ReportEveryMax);
}

int saveEvery() {
    return getSetting(FPSTR(keys::SaveEvery), build::saveEvery());
}

bool realTimeValues() {
    return getSetting(FPSTR(keys::RealTimeValues), build::realTimeValues());
}

} // namespace
} // namespace settings

alignas(4) static constexpr char List[] PROGMEM_STRING_ATTR =
#if ADE7953_SUPPORT
    "ADE7953 "
#endif
#if AM2320_SUPPORT
    "AM2320_I2C "
#endif
#if ANALOG_SUPPORT
    "ANALOG "
#endif
#if BH1750_SUPPORT
    "BH1750 "
#endif
#if BMP180_SUPPORT
    "BMP180 "
#endif
#if BMX280_SUPPORT
    "BMX280 "
#endif
#if BME680_SUPPORT
    "BME680 "
#endif
#if CSE7766_SUPPORT
    "CSE7766 "
#endif
#if DALLAS_SUPPORT
    "DALLAS "
#endif
#if DHT_SUPPORT
    "DHTXX "
#endif
#if DIGITAL_SUPPORT
    "DIGITAL "
#endif
#if ECH1560_SUPPORT
    "ECH1560 "
#endif
#if EMON_ADC121_SUPPORT
    "EMON_ADC121 "
#endif
#if EMON_ADS1X15_SUPPORT
    "EMON_ADX1X15 "
#endif
#if EMON_ANALOG_SUPPORT
    "EMON_ANALOG "
#endif
#if EVENTS_SUPPORT
    "EVENTS "
#endif
#if GEIGER_SUPPORT
    "GEIGER "
#endif
#if GUVAS12SD_SUPPORT
    "GUVAS12SD "
#endif
#if HDC1080_SUPPORT
    "HDC1080 "
#endif
#if HLW8012_SUPPORT
    "HLW8012 "
#endif
#if INA219_SUPPORT
    "INA219 "
#endif
#if LDR_SUPPORT
    "LDR "
#endif
#if MAX6675_SUPPORT
    "MAX6675 "
#endif
#if MHZ19_SUPPORT
    "MHZ19 "
#endif
#if MICS2710_SUPPORT
    "MICS2710 "
#endif
#if MICS5525_SUPPORT
    "MICS5525 "
#endif
#if NTC_SUPPORT
    "NTC "
#endif
#if PM1006_SUPPORT
    "PM1006 "
#endif
#if PMSX003_SUPPORT
    "PMSX003 "
#endif
#if PULSEMETER_SUPPORT
    "PULSEMETER "
#endif
#if PZEM004T_SUPPORT
    "PZEM004T "
#endif
#if PZEM004TV30_SUPPORT
    "PZEM004TV30 "
#endif
#if SDS011_SUPPORT
    "SDS011 "
#endif
#if SENSEAIR_SUPPORT
    "SENSEAIR "
#endif
#if SHT3X_I2C_SUPPORT
    "SHT3X_I2C "
#endif
#if SI7021_SUPPORT
    "SI7021 "
#endif
#if SM300D2_SUPPORT
    "SM300D2 "
#endif
#if SONAR_SUPPORT
    "SONAR "
#endif
#if T6613_SUPPORT
    "T6613 "
#endif
#if TMP3X_SUPPORT
    "TMP3X "
#endif
#if V9261F_SUPPORT
    "V9261F "
#endif
#if VEML6075_SUPPORT
    "VEML6075 "
#endif
#if VL53L1X_SUPPORT
    "VL53L1X "
#endif
#if EZOPH_SUPPORT
    "EZOPH "
#endif
#if DUMMY_SENSOR_SUPPORT
    "DUMMY "
#endif
#if SI1145_SUPPORT
    "SI1145 "
#endif
    "";

} // namespace sensor

namespace settings {
namespace internal {

template <>
espurna::sensor::Unit convert(const String& value) {
    return convert(espurna::sensor::settings::units::Options, value,
            espurna::sensor::Unit::None);
}

String serialize(espurna::sensor::Unit unit) {
    return serialize(espurna::sensor::settings::units::Options, unit);
}

template <>
espurna::sensor::Filter convert(const String& value) {
    return convert(espurna::sensor::settings::filters::Options, value,
            espurna::sensor::Filter::Median);
}

String serialize(espurna::sensor::Filter filter) {
    return serialize(espurna::sensor::settings::filters::Options, filter);
}

} // namespace internal
} // namespace settings

namespace sensor {
namespace magnitude {
namespace traits {

constexpr bool correction_supported(unsigned char type) {
  return (type == MAGNITUDE_TEMPERATURE)
      || (type == MAGNITUDE_HUMIDITY)
      || (type == MAGNITUDE_PRESSURE)
      || (type == MAGNITUDE_LUX);
}

static constexpr unsigned char ratio_types[] {
    MAGNITUDE_CURRENT,
    MAGNITUDE_VOLTAGE,
    MAGNITUDE_POWER_ACTIVE,
    MAGNITUDE_ENERGY,
};

constexpr bool ratio_supported(unsigned char type) {
    return (type == MAGNITUDE_CURRENT)
        || (type == MAGNITUDE_VOLTAGE)
        || (type == MAGNITUDE_POWER_ACTIVE)
        || (type == MAGNITUDE_ENERGY);
}

} // namespace traits

namespace build {

static constexpr double correction(unsigned char type) {
    return (
        (type == MAGNITUDE_TEMPERATURE) ? (SENSOR_TEMPERATURE_CORRECTION) :
        (type == MAGNITUDE_HUMIDITY) ? (SENSOR_HUMIDITY_CORRECTION) :
        (type == MAGNITUDE_LUX) ? (SENSOR_LUX_CORRECTION) :
        (type == MAGNITUDE_PRESSURE) ? (SENSOR_PRESSURE_CORRECTION) :
        0.0
    );
}

} // namespace build

namespace {

String format(const Magnitude& magnitude, double value) {
    // XXX: dtostrf only handles basic floating point values and will never produce scientific notation
    //      ensure decimals is within some sane limit and the actual value never goes above this buffer size
    char buffer[64];
    dtostrf(value, 1, magnitude.decimals, buffer);

    return buffer;
}

String name(unsigned char type) {
    const char* result = nullptr;

    switch (type) {
    case MAGNITUDE_TEMPERATURE:
        result = PSTR("Temperature");
        break;
    case MAGNITUDE_HUMIDITY:
        result = PSTR("Humidity");
        break;
    case MAGNITUDE_PRESSURE:
        result = PSTR("Pressure");
        break;
    case MAGNITUDE_CURRENT:
        result = PSTR("Current");
        break;
    case MAGNITUDE_VOLTAGE:
        result = PSTR("Voltage");
        break;
    case MAGNITUDE_POWER_ACTIVE:
        result = PSTR("Active Power");
        break;
    case MAGNITUDE_POWER_APPARENT:
        result = PSTR("Apparent Power");
        break;
    case MAGNITUDE_POWER_REACTIVE:
        result = PSTR("Reactive Power");
        break;
    case MAGNITUDE_POWER_FACTOR:
        result = PSTR("Power Factor");
        break;
    case MAGNITUDE_ENERGY:
        result = PSTR("Energy");
        break;
    case MAGNITUDE_ENERGY_DELTA:
        result = PSTR("Energy (delta)");
        break;
    case MAGNITUDE_ANALOG:
        result = PSTR("Analog");
        break;
    case MAGNITUDE_DIGITAL:
        result = PSTR("Digital");
        break;
    case MAGNITUDE_EVENT:
        result = PSTR("Event");
        break;
    case MAGNITUDE_PM1DOT0:
        result = PSTR("PM1.0");
        break;
    case MAGNITUDE_PM2DOT5:
        result = PSTR("PM2.5");
        break;
    case MAGNITUDE_PM10:
        result = PSTR("PM10");
        break;
    case MAGNITUDE_CO2:
        result = PSTR("CO2");
        break;
    case MAGNITUDE_VOC:
        result = PSTR("VOC");
        break;
    case MAGNITUDE_IAQ_STATIC:
        result = PSTR("IAQ (Static)");
        break;
    case MAGNITUDE_IAQ:
        result = PSTR("IAQ");
        break;
    case MAGNITUDE_IAQ_ACCURACY:
        result = PSTR("IAQ Accuracy");
        break;
    case MAGNITUDE_LUX:
        result = PSTR("Lux");
        break;
    case MAGNITUDE_UVA:
        result = PSTR("UVA");
        break;
    case MAGNITUDE_UVB:
        result = PSTR("UVB");
        break;
    case MAGNITUDE_UVI:
        result = PSTR("UVI");
        break;
    case MAGNITUDE_DISTANCE:
        result = PSTR("Distance");
        break;
    case MAGNITUDE_HCHO:
        result = PSTR("HCHO");
        break;
    case MAGNITUDE_GEIGER_CPM:
    case MAGNITUDE_GEIGER_SIEVERT:
        result = PSTR("Local Dose Rate");
        break;
    case MAGNITUDE_COUNT:
        result = PSTR("Count");
        break;
    case MAGNITUDE_NO2:
        result = PSTR("NO2");
        break;
    case MAGNITUDE_CO:
        result = PSTR("CO");
        break;
    case MAGNITUDE_RESISTANCE:
        result = PSTR("Resistance");
        break;
    case MAGNITUDE_PH:
        result = PSTR("pH");
        break;
    case MAGNITUDE_FREQUENCY:
        result = PSTR("Frequency");
        break;
    case MAGNITUDE_TVOC:
        result = PSTR("TVOC");
        break;
    case MAGNITUDE_CH2O:
        result = PSTR("CH2O");
        break;
    case MAGNITUDE_NONE:
    default:
        break;
    }

    return String(result);
}

String topic(unsigned char type) {
    const char* result = PSTR("unknown");

    switch (type) {
    case MAGNITUDE_TEMPERATURE:
        result = PSTR("temperature");
        break;
    case MAGNITUDE_HUMIDITY:
        result = PSTR("humidity");
        break;
    case MAGNITUDE_PRESSURE:
        result = PSTR("pressure");
        break;
    case MAGNITUDE_CURRENT:
        result = PSTR("current");
        break;
    case MAGNITUDE_VOLTAGE:
        result = PSTR("voltage");
        break;
    case MAGNITUDE_POWER_ACTIVE:
        result = PSTR("power");
        break;
    case MAGNITUDE_POWER_APPARENT:
        result = PSTR("apparent");
        break;
    case MAGNITUDE_POWER_REACTIVE:
        result = PSTR("reactive");
        break;
    case MAGNITUDE_POWER_FACTOR:
        result = PSTR("factor");
        break;
    case MAGNITUDE_ENERGY:
        result = PSTR("energy");
        break;
    case MAGNITUDE_ENERGY_DELTA:
        result = PSTR("energy_delta");
        break;
    case MAGNITUDE_ANALOG:
        result = PSTR("analog");
        break;
    case MAGNITUDE_DIGITAL:
        result = PSTR("digital");
        break;
    case MAGNITUDE_EVENT:
        result = PSTR("event");
        break;
    case MAGNITUDE_PM1DOT0:
        result = PSTR("pm1dot0");
        break;
    case MAGNITUDE_PM2DOT5:
        result = PSTR("pm2dot5");
        break;
    case MAGNITUDE_PM10:
        result = PSTR("pm10");
        break;
    case MAGNITUDE_CO2:
        result = PSTR("co2");
        break;
    case MAGNITUDE_VOC:
        result = PSTR("voc");
        break;
    case MAGNITUDE_IAQ:
        result = PSTR("iaq");
        break;
    case MAGNITUDE_IAQ_ACCURACY:
        result = PSTR("iaq_accuracy");
        break;
    case MAGNITUDE_IAQ_STATIC:
        result = PSTR("iaq_static");
        break;
    case MAGNITUDE_LUX:
        result = PSTR("lux");
        break;
    case MAGNITUDE_UVA:
        result = PSTR("uva");
        break;
    case MAGNITUDE_UVB:
        result = PSTR("uvb");
        break;
    case MAGNITUDE_UVI:
        result = PSTR("uvi");
        break;
    case MAGNITUDE_DISTANCE:
        result = PSTR("distance");
        break;
    case MAGNITUDE_HCHO:
        result = PSTR("hcho");
        break;
    case MAGNITUDE_GEIGER_CPM:
        result = PSTR("ldr_cpm"); // local dose rate [Counts per minute]
        break;
    case MAGNITUDE_GEIGER_SIEVERT:
        result = PSTR("ldr_uSvh"); // local dose rate [µSievert per hour]
        break;
    case MAGNITUDE_COUNT:
        result = PSTR("count");
        break;
    case MAGNITUDE_NO2:
        result = PSTR("no2");
        break;
    case MAGNITUDE_CO:
        result = PSTR("co");
        break;
    case MAGNITUDE_RESISTANCE:
        result = PSTR("resistance");
        break;
    case MAGNITUDE_PH:
        result = PSTR("ph");
        break;
    case MAGNITUDE_FREQUENCY:
        result = PSTR("frequency");
        break;
    case MAGNITUDE_TVOC:
        result = PSTR("tvoc");
        break;
    case MAGNITUDE_CH2O:
        result = PSTR("ch2o");
        break;
    case MAGNITUDE_NONE:
    default:
        break;
    }

    return String(result);
}

String topic(const Magnitude& magnitude) {
    return topic(magnitude.type);
}

String topicWithIndex(const Magnitude& magnitude) {
    auto out = topic(magnitude);
    if (sensor::build::useIndex() || (Magnitude::counts(magnitude.type) > 1)) {
        out += '/' + String(magnitude.index_global, 10);
    }

    return out;
}

String description(const Magnitude& magnitude) {
    return magnitude.sensor->description(magnitude.slot);
}

sensor::Filter defaultFilter(unsigned char type) {
    switch (type) {
    case MAGNITUDE_IAQ:
    case MAGNITUDE_IAQ_STATIC:
    case MAGNITUDE_ENERGY:
        return Filter::Last;
    case MAGNITUDE_EVENT:
    case MAGNITUDE_DIGITAL:
        return Filter::Max;
    case MAGNITUDE_COUNT:
    case MAGNITUDE_ENERGY_DELTA:
        return Filter::Sum;
    case MAGNITUDE_GEIGER_CPM:
    case MAGNITUDE_GEIGER_SIEVERT:
        return Filter::MovingAverage;
    }

    return Filter::Median;
}

Filter defaultFilter(const Magnitude& magnitude) {
    return defaultFilter(magnitude.type);
}

BaseFilterPtr makeFilter(Filter filter) {
    BaseFilterPtr out;

    switch (filter) {
    case Filter::Last:
        out = std::make_unique<LastFilter>();
        break;
    case Filter::Max:
        out = std::make_unique<MaxFilter>();
        break;
    case Filter::Sum:
        out = std::make_unique<SumFilter>();
        break;
    case Filter::MovingAverage:
        out = std::make_unique<MovingAverageFilter>();
        break;
    case Filter::Median:
        out = std::make_unique<MedianFilter>();
        break;
    }

    return out;
}

// Hardcoded decimals for each magnitude
unsigned char decimals(Unit unit) {
    switch (unit) {
    case Unit::Celcius:
    case Unit::Farenheit:
        return 1;
    case Unit::Percentage:
        return 0;
    case Unit::Hectopascal:
        return 2;
    case Unit::Ampere:
        return 3;
    case Unit::Volt:
        return 0;
    case Unit::Watt:
    case Unit::Voltampere:
    case Unit::VoltampereReactive:
        return 0;
    case Unit::Kilowatt:
    case Unit::Kilovoltampere:
    case Unit::KilovoltampereReactive:
        return 3;
    case Unit::KilowattHour:
        return 3;
    case Unit::WattSecond:
        return 0;
    case Unit::CountsPerMinute:
    case Unit::MicrosievertPerHour:
        return 4;
    case Unit::Meter:
        return 3;
    case Unit::Hertz:
        return 1;
    case Unit::UltravioletIndex:
        return 3;
    case Unit::Ph:
        return 3;
    case Unit::None:
    default:
        break;
    }

    return 0;
}

double process(const Magnitude& magnitude, double value) {
    // Process input (sensor) units and convert to the ones that magnitude specifies as output
    const auto sensor_units = magnitude.sensor->units(magnitude.slot);
    if (sensor_units != magnitude.units) {
        using namespace sensor::convert;
        if (temperature::supported(sensor_units) && temperature::supported(magnitude.units)) {
            value = temperature::convert(value, sensor_units, magnitude.units);
        } else if (metric::supported(sensor_units) && metric::supported(magnitude.units)) {
            value = metric::convert(value, sensor_units, magnitude.units);
        }
    }

    // Right now, correction is a simple offset.
    // TODO: math expression?
    value = value + magnitude.correction;

    // RAW value might have more decimal points than necessary.
    return roundTo(value, magnitude.decimals);
}

} // namespace

namespace internal {
namespace {

std::vector<Magnitude> magnitudes;

using ReadHandlers = std::forward_list<MagnitudeReadHandler>;
ReadHandlers read_handlers;
ReadHandlers report_handlers;

} // namespace
} // namespace internal

size_t count(unsigned char type) {
    return Magnitude::counts(type);
}

size_t count() {
    return internal::magnitudes.size();
}

void add(BaseSensorPtr sensor, unsigned char slot, unsigned char type) {
    internal::magnitudes.emplace_back(sensor, slot, type);
}

const Magnitude* find(unsigned char type, unsigned char index) {
    const Magnitude* out { nullptr };

    const auto result = std::find_if(
        std::cbegin(internal::magnitudes),
        std::cend(internal::magnitudes),
        [&](const Magnitude& magnitude) {
            return (magnitude.type == type) && (magnitude.index_global == index);
        });

    if (result != internal::magnitudes.end()) {
        out = std::addressof(*result);
    }

    return out;
}

Magnitude& get(size_t index) {
    return internal::magnitudes[index];
}

template <typename T>
void forEachInstance(T&& callback) {
    for (auto& magnitude : internal::magnitudes) {
        callback(magnitude);
    }
}

template <typename T>
void forEachCounted(T&& callback) {
    for (unsigned char type = MAGNITUDE_NONE + 1; type < MAGNITUDE_MAX; ++type) {
        if (count(type)) {
            callback(type);
        }
    }
}

// check if `callback(type)` returns `true` at least once
template <typename T>
bool forEachCountedCheck(T&& callback) {
    for (unsigned char type = MAGNITUDE_NONE + 1; type < MAGNITUDE_MAX; ++type) {
        if (count(type) && callback(type)) {
            return true;
        }
    }

    return false;
}

void onRead(MagnitudeReadHandler handler) {
    internal::read_handlers.push_front(handler);
}

void read(const Value& value) {
    for (auto& handler : internal::read_handlers) {
        handler(value);
    }
}

void onReport(MagnitudeReadHandler handler) {
    internal::report_handlers.push_front(handler);
}

void report(const Value& report) {
    for (auto& handler : internal::report_handlers) {
        handler(report);
    }
}

Info info(const Magnitude& magnitude) {
    return Info{
        .type = magnitude.type,
        .index = magnitude.index_global,
        .units = magnitude.units,
        .decimals = magnitude.decimals,
        .topic = topicWithIndex(magnitude),
    };
}

Value value(const Magnitude& magnitude, double value) {
    return Value{
        .type = magnitude.type,
        .index = magnitude.index_global,
        .units = magnitude.units,
        .decimals = magnitude.decimals,
        .topic = topicWithIndex(magnitude),
        .value = value,
        .repr = format(magnitude, value),
    };
}

template <typename T>
Value safe_value(size_t index, T&& retrieve) {
    Value out;
    out.value = Value::Unknown;

    if (index < count()) {
        const auto& magnitude = get(index);
        out = value(magnitude, retrieve(magnitude));
    }

    return out;
}

Value safe_value_last(size_t index) {
    return safe_value(
        index,
        [](const Magnitude& magnitude) {
            return magnitude.last;
        });
}

Value safe_value_reported(size_t index) {
    return safe_value(
        index,
        [](const Magnitude& magnitude) {
            return magnitude.reported;
        });
}

} // namespace magnitude

using TimeSource = espurna::time::CoreClock;

enum class State {
    None,
    Initial,
    Idle,
    Resume,
    Ready,
    Reading,
};

namespace internal {

std::vector<BaseSensorPtr> sensors;
size_t read_count;

bool real_time { build::realTimeValues() };
size_t report_every { build::reportEvery() };

duration::Seconds read_interval { build::readInterval() };
duration::Seconds init_interval { build::initInterval() };

} // namespace internal

bool realTimeValues() {
    return internal::real_time;
}

void realTimeValues(bool value) {
    internal::real_time = value;
}

size_t reportEvery() {
    return internal::report_every;
}

void reportEvery(size_t value) {
    internal::report_every = value;
}

duration::Seconds readInterval() {
    return internal::read_interval;
}

void readInterval(duration::Seconds value) {
    internal::read_interval = value;
}

duration::Seconds initInterval() {
    return internal::init_interval;
}

void initInterval(duration::Seconds value) {
    internal::init_interval = value;
}

template <typename T>
void forEachInstance(T&& callback) {
    for (auto sensor : internal::sensors) {
        callback(sensor);
    }
}

void add(BaseSensor* sensor) {
    internal::sensors.push_back(sensor);
}

size_t count() {
    return internal::sensors.size();
}

void tick() {
    for (auto sensor : internal::sensors) {
        sensor->tick();
    }
}

void pre() {
    for (auto sensor : internal::sensors) {
        sensor->pre();
        if (!sensor->status()) {
            DEBUG_MSG_P(PSTR("[SENSOR] Could not read from %s (%s)\n"),
                sensor->description().c_str(), error(sensor->error()).c_str());
        }
    }
}

void post() {
    for (auto sensor : internal::sensors) {
        sensor->post();
    }
}

// Registers available sensor classes.
//
// Notice that *every* available sensor (*_SUPPORT set to 1) is queued for initialization.
// For the time being, failure to `begin()` any sensor will stall all subsequent sensors.
//
// Future updates *should* work out whether we need to:
// - allow to 'enable' specific sensor in settings
//   (...would we have too much key prefixes?)
// - 'probe' sensor (bus scan, attempt to read) separate from actual loading
// - allow to soft-fail begin()
//   (although, removing stable magnitude IDs)
//
// If you want to add another sensor instance of the same type, just duplicate
// the initialization block and change the respective method arguments.
// For example, to add a second DHT sensor:
//
// #if DHT_SUPPORT
// {
//     auto* sensor = new DHTSensor();
//     sensor->setGPIO(DHT2_PIN);
//     sensor->setType(DHT2_TYPE);
//     add(sensor);
// }
// #endif
//
// Obviously, both DHT2_PIN and DHT2_TYPE should be accessible
// - use `build_src_flags = -DDHT2_PIN=... -DDHT2_TYPE=...`
// - update config/custom.h or config/sensor.h, adding `#define DHT2_PIN ...` and `#define DHT2_TYPE ...`

void load() {
#if AM2320_SUPPORT
    {
        auto* sensor = new AM2320Sensor();
        sensor->setAddress(AM2320_ADDRESS);
        add(sensor);
    }
#endif

#if ANALOG_SUPPORT
    {
        auto* sensor = new AnalogSensor();
        sensor->setSamples(ANALOG_SAMPLES);
        sensor->setDelay(ANALOG_DELAY);
        sensor->setFactor(ANALOG_FACTOR);
        sensor->setOffset(ANALOG_OFFSET);
        add(sensor);
    }
#endif

#if BH1750_SUPPORT
    {
        auto* sensor = new BH1750Sensor();
        sensor->setAddress(BH1750_ADDRESS);
        sensor->setAccuracy(BH1750_ACCURACY);
        sensor->setSensitivity(BH1750_SENSITIVITY);
        sensor->setMode(BH1750_MODE);
        add(sensor);
    }
#endif

#if BMP180_SUPPORT
    {
        auto* sensor = new BMP180Sensor();
        sensor->setAddress(BMP180_ADDRESS);
        add(sensor);
    }
#endif

#if BMX280_SUPPORT
    {
        // TODO: bmx280AddressN, do some migrate code based on number?
        // Support up to two sensors with full auto-discovery.
        const auto number = std::clamp(getSetting("bmx280Number", BMX280_NUMBER), 1, 2);

        // For second sensor, if BMX280_ADDRESS is 0x00 then auto-discover
        // otherwise choose the other unnamed sensor address
        static constexpr uint8_t Address { BMX280_ADDRESS };
        const decltype(Address) first = getSetting("bmx280Address", Address);
        const decltype(Address) second = (first == 0x00) ? 0x00 : (0x76 + 0x77 - first);

        const decltype(Address) address_map[2] { first, second };
        for (unsigned char n=0; n < number; ++n) {
            auto* sensor = new BMX280Sensor();
            sensor->setAddress(address_map[n]);
            add(sensor);
        }
    }
#endif

#if BME680_SUPPORT
    {
        auto* sensor = new BME680Sensor();
        sensor->setAddress(BME680_I2C_ADDRESS);
        add(sensor);
    }
#endif

#if CSE7766_SUPPORT
    {
        const auto port = uartPort(CSE7766_PORT - 1);
        if (!port) {
            return;
        }

        auto* sensor = new CSE7766Sensor();
        sensor->setPort(port->stream);

        add(sensor);
    }
#endif

#if DALLAS_SUPPORT
    {
        auto* sensor = new DallasSensor();
        sensor->setGPIO(DALLAS_PIN);
        add(sensor);
    }
#endif

#if DHT_SUPPORT
    {
        auto* sensor = new DHTSensor();
        sensor->setGPIO(DHT_PIN);
        sensor->setType(DHT_TYPE);
        add(sensor);
    }
#endif

#if DIGITAL_SUPPORT
    {
        const auto pins = gpioPins();
        for (size_t index = 0; index < pins; ++index) {
            const auto pin = DigitalSensor::defaultPin(index);
            if (pin == GPIO_NONE) {
                break;
            }

            auto* sensor = new DigitalSensor();
            sensor->setPin(pin);
            sensor->setPinMode(DigitalSensor::defaultPinMode(index));
            sensor->setDefault(DigitalSensor::defaultState(index));

            add(sensor);
        }
    }
#endif

#if DUMMY_SENSOR_SUPPORT
    {
        add(new DummySensor());
    }
#endif

#if ECH1560_SUPPORT
    {
        auto* sensor = new ECH1560Sensor();
        sensor->setCLK(ECH1560_CLK_PIN);
        sensor->setMISO(ECH1560_MISO_PIN);
        sensor->setInverted(ECH1560_INVERTED);
        add(sensor);
    }
#endif

#if EMON_ADC121_SUPPORT
    {
        auto* sensor = new EmonADC121Sensor();
        sensor->setAddress(EMON_ADC121_I2C_ADDRESS);
        sensor->setVoltage(EMON_MAINS_VOLTAGE);
        sensor->setReferenceVoltage(EMON_REFERENCE_VOLTAGE);
        add(sensor);
    }
#endif

#if EMON_ADS1X15_SUPPORT
    {
        auto port = std::make_shared<EmonADS1X15Sensor::I2CPort>(
                EMON_ADS1X15_I2C_ADDRESS, EMON_ADS1X15_TYPE, EMON_ADS1X15_GAIN, EMON_ADS1X15_DATARATE);

        constexpr unsigned char FirstBit { 1 };
        unsigned char mask { EMON_ADS1X15_MASK };
        unsigned char channel { 0 };

        while (mask) {
            if (mask & FirstBit) {
                auto* sensor = new EmonADS1X15Sensor(port);
                sensor->setVoltage(EMON_MAINS_VOLTAGE);
                sensor->setChannel(channel);
                add(sensor);
            }
            ++channel;
            mask >>= 1;
        }
    }
#endif

#if EMON_ANALOG_SUPPORT
    {
        auto* sensor = new EmonAnalogSensor();
        sensor->setVoltage(EMON_MAINS_VOLTAGE);
        sensor->setReferenceVoltage(EMON_REFERENCE_VOLTAGE);
        sensor->setResolution(EMON_ANALOG_RESOLUTION);
        add(sensor);
    }
#endif

#if EVENTS_SUPPORT
    {
        for (size_t index = 0; index < EventSensor::SensorsMax; ++index) {
            const auto pin = EventSensor::defaultPin(index);
            if (pin == GPIO_NONE) {
                break;
            }

            auto* sensor = new EventSensor();
            sensor->setPin(pin);
            sensor->setPinMode(
                EventSensor::defaultPinMode(index));
            sensor->setDebounceTime(
                EventSensor::defaultDebounceTime(index));
            sensor->setInterruptMode(
                EventSensor::defaultInterruptMode(index));
            add(sensor);
        }
    }
#endif

#if GEIGER_SUPPORT
    {
        auto* sensor = new GeigerSensor();
        sensor->setGPIO(GEIGER_PIN);
        sensor->setMode(GEIGER_PIN_MODE);
        sensor->setDebounceTime(
            GeigerSensor::TimeSource::duration { GEIGER_DEBOUNCE });
        sensor->setInterruptMode(GEIGER_INTERRUPT_MODE);
        sensor->setCPM2SievertFactor(GEIGER_CPM2SIEVERT);
        add(sensor);
    }
#endif

#if GUVAS12SD_SUPPORT
    {
        auto* sensor = new GUVAS12SDSensor();
        sensor->setGPIO(GUVAS12SD_PIN);
        add(sensor);
    }
#endif

#if SONAR_SUPPORT
    {
        auto* sensor = new SonarSensor();
        sensor->setEcho(SONAR_ECHO);
        sensor->setIterations(SONAR_ITERATIONS);
        sensor->setMaxDistance(SONAR_MAX_DISTANCE);
        sensor->setTrigger(SONAR_TRIGGER);
        add(sensor);
    }
#endif

#if HLW8012_SUPPORT
    {
        auto* sensor = new HLW8012Sensor();
        sensor->setSEL(getSetting(F("hlw8012SEL"), HLW8012_SEL_PIN));
        sensor->setCF(getSetting(F("hlw8012CF"), HLW8012_CF_PIN));
        sensor->setCF1(getSetting(F("hlw8012CF1"), HLW8012_CF1_PIN));
        sensor->setSELCurrent(HLW8012_SEL_CURRENT);
        add(sensor);
    }
#endif

#if INA219_SUPPORT
    {
        auto* sensor = new INA219Sensor();
        sensor->setAddress(INA219_ADDRESS);
        sensor->setOperatingMode(INA219Sensor::INA219_OPERATING_MODE);
        sensor->setShuntMode(INA219Sensor::INA219_SHUNT_MODE);
        sensor->setBusMode(INA219Sensor::INA219_BUS_MODE);
        sensor->setBusRange(INA219Sensor::INA219_BUS_RANGE);
        sensor->setGain(INA219Sensor::INA219_GAIN);
        sensor->setShuntResistance(INA219_SHUNT_RESISTANCE);
        sensor->setMaxExpectedCurrent(INA219_MAX_EXPECTED_CURRENT);
        add(sensor);
    }
#endif

#if LDR_SUPPORT
    {
        auto* sensor = new LDRSensor();
        sensor->setSamples(LDR_SAMPLES);
        sensor->setDelay(LDR_DELAY);
        sensor->setType(LDR_TYPE);
        sensor->setPhotocellPositionOnGround(LDR_ON_GROUND);
        sensor->setResistor(LDR_RESISTOR);
        sensor->setPhotocellParameters(LDR_MULTIPLICATION, LDR_POWER);
        add(sensor);
    }
#endif

#if MHZ19_SUPPORT
    {
        const auto port = uartPort(MHZ19_PORT - 1);
        if (!port) {
            return;
        }

        auto* sensor = new MHZ19Sensor();
        sensor->setPort(port->stream);
        sensor->setCalibrateAuto(
            getSetting("mhz19CalibrateAuto", MHZ19_CALIBRATE_AUTO == 1));
        sensor->setDetectionRange(
            getSetting("mhz19DetectionRange", uint32_t{ MHZ19_DETECTION_RANGE }));
        add(sensor);
    }
#endif

#if MICS2710_SUPPORT
    {
        auto* sensor = new MICS2710Sensor();
        sensor->setPreHeatGPIO(MICS2710_PRE_PIN);
        sensor->setR0(MICS2710_R0);
        sensor->setRL(MICS2710_RL);
        sensor->setRS(0);
        add(sensor);
    }
#endif

#if MICS5525_SUPPORT
    {
        auto* sensor = new MICS5525Sensor();
        sensor->setR0(MICS5525_R0);
        sensor->setRL(MICS5525_RL);
        sensor->setRS(0);
        add(sensor);
    }
#endif

#if NTC_SUPPORT
    {
        auto* sensor = new NTCSensor();
        sensor->setSamples(NTC_SAMPLES);
        sensor->setDelay(NTC_DELAY);
        sensor->setUpstreamResistor(NTC_R_UP);
        sensor->setDownstreamResistor(NTC_R_DOWN);
        sensor->setInputVoltage(NTC_INPUT_VOLTAGE);
        sensor->setBeta(NTC_BETA);
        sensor->setR0(NTC_R0);
        sensor->setT0(NTC_T0);
        add(sensor);
    }
#endif

#if PM1006_SUPPORT
    {
        const auto port = uartPort(PM1006_PORT - 1);
        if (!port) {
            return;
        }

        auto* sensor = new PM1006Sensor();
        sensor->setPort(port->stream);
        add(sensor);
    }
#endif

#if PMSX003_SUPPORT
    {
        const auto port = uartPort(PMS_PORT - 1);
        if (!port) {
            return;
        }

        auto* sensor = new PMSX003Sensor();
        sensor->setPort(port->stream);
        sensor->setType(PMS_TYPE);
        add(sensor);
    }
#endif

#if PULSEMETER_SUPPORT
    {

        auto* sensor = new PulseMeterSensor();
        sensor->setPin(PULSEMETER_PIN);
        sensor->setInterruptMode(PULSEMETER_INTERRUPT_ON);
        sensor->setDebounceTime(
            PulseMeterSensor::TimeSource::duration{PULSEMETER_DEBOUNCE});
        add(sensor);
    }
#endif

#if PZEM004T_SUPPORT
    {
        const auto port = uartPort(PZEM004T_PORT - 1);
        if (!port) {
            return;
        }

        auto serial = std::make_shared<PZEM004TSensor::SerialPort>(port->stream);

        bool initialized { false };
#if !defined(PZEM004T_ADDRESSES)
        for (size_t index = 0; index < PZEM004TSensor::DevicesMax; ++index) {
            auto address = getSetting({"pzemAddr", index}, PZEM004TSensor::defaultAddress(index));
            if (!address.isSet()) {
                break;
            }

            auto* ptr = PZEM004TSensor::make(serial, address);
            if (ptr) {
                add(ptr);
                initialized = true;
            }
        }
#else
        String addrs = getSetting("pzemAddr", F(PZEM004T_ADDRESSES));

        constexpr size_t BufferSize{64};
        char buffer[BufferSize]{0};

        if (addrs.length() < BufferSize) {
            std::copy(addrs.c_str(), addrs.c_str() + addrs.length(), buffer);
            buffer[addrs.length()] = '\0';

            size_t device{0};
            char* address{strtok(buffer, " ")};
            while ((device < PZEM004TSensor::DevicesMax) && (address != nullptr)) {
                auto* ptr = PZEM004TSensor::make(serial, IPAddress(address));
                if (ptr) {
                    add(ptr);
                    initialized = true;
                }
            }
        }
#endif
        if (initialized) {
            PZEM004TSensor::registerTerminalCommands();
        }
    }
#endif

#if SENSEAIR_SUPPORT
    {
        const auto port = uartPort(SENSEAIR_PORT - 1);
        if (!port) {
            return;
        }

        auto* sensor = new SenseAirSensor();
        sensor->setPort(port->stream);
        add(sensor);
    }
#endif

#if SDS011_SUPPORT
    {
        const auto port = uartPort(SDS011_PORT - 1);
        if (!port) {
            return;
        }

        auto* sensor = new SDS011Sensor();
        sensor->setPort(port->stream);
        add(sensor);
    }
#endif

#if SHT3X_I2C_SUPPORT
    {
        auto* sensor = new SHT3XI2CSensor();
        sensor->setAddress(SHT3X_I2C_ADDRESS);
        add(sensor);
    }
#endif

#if SI7021_SUPPORT
    {
        auto* sensor = new SI7021Sensor();
        sensor->setAddress(SI7021_ADDRESS);
        add(sensor);
    }
#endif

#if SM300D2_SUPPORT
    {
        const auto port = uartPort(SM300D2_PORT - 1);
        if (!port) {
            return;
        }

        auto* sensor = new SM300D2Sensor();
        sensor->setPort(port->stream);
        add(sensor);
    }
#endif

#if T6613_SUPPORT
    {
        const auto port = uartPort(T6613_PORT - 1);
        if (!port) {
            return;
        }

        auto* sensor = new T6613Sensor();
        sensor->setPort(port->stream);
        add(sensor);
    }
#endif

#if TMP3X_SUPPORT
    {
        auto* sensor = new TMP3XSensor();
        sensor->setType(TMP3X_TYPE);
        add(sensor);
    }
#endif

#if V9261F_SUPPORT
    {
        const auto port = uartPort(V9261F_PORT - 1);
        if (!port) {
            return;
        }

        auto* sensor = new V9261FSensor();
        sensor->setPort(port->stream);
        add(sensor);
    }
#endif

#if MAX6675_SUPPORT
    {
        auto* sensor = new MAX6675Sensor();
        sensor->setCS(MAX6675_CS_PIN);
        sensor->setSO(MAX6675_SO_PIN);
        sensor->setSCK(MAX6675_SCK_PIN);
        add(sensor);
    }
#endif

#if VEML6075_SUPPORT
    {
        auto* sensor = new VEML6075Sensor();
        sensor->setIntegrationTime(VEML6075_INTEGRATION_TIME);
        sensor->setDynamicMode(VEML6075_DYNAMIC_MODE);
        add(sensor);
    }
#endif

#if VL53L1X_SUPPORT
    {
        auto* sensor = new VL53L1XSensor();
        sensor->setInterMeasurementPeriod(
            VL53L1XSensor::InterMeasurementPeriod{VL53L1X_INTER_MEASUREMENT_PERIOD});
        sensor->setMeasurementTimingBudget(
            VL53L1XSensor::MeasurementTimingBudget{VL53L1X_MEASUREMENT_TIMING_BUDGET});
        sensor->setDistanceMode(VL53L1X_DISTANCE_MODE);
        add(sensor);
    }
#endif

#if EZOPH_SUPPORT
    {
        const auto port = uartPort(EZOPH_PORT - 1);
        if (!port) {
            return;
        }

        auto* sensor = new EZOPHSensor();
        sensor->setPort(port->stream);
        add(sensor);
    }
#endif

#if ADE7953_SUPPORT
    {
        auto* sensor = new ADE7953Sensor();
        sensor->setAddress(ADE7953_ADDRESS);
        add(sensor);
    }
#endif

#if SI1145_SUPPORT
    {
        auto* sensor = new SI1145Sensor();
        sensor->setAddress(SI1145_ADDRESS);
        add(sensor);
    }
#endif

#if HDC1080_SUPPORT
    {
        auto* sensor = new HDC1080Sensor();
        sensor->setAddress(HDC1080_ADDRESS);
        add(sensor);
    }
#endif

#if PZEM004TV30_SUPPORT
    {
        const auto port = uartPort(PZEM004TV30_PORT - 1);
        if (!port) {
            return;
        }

        auto* sensor = PZEM004TV30Sensor::make(port->stream,
            getSetting("pzemv30Addr", PZEM004TV30Sensor::DefaultAddress),
            getSetting("pzemv30ReadTimeout", PZEM004TV30Sensor::DefaultReadTimeout));
        sensor->setDebug(
            getSetting("pzemv30Debug", PZEM004TV30Sensor::DefaultDebug));

        add(sensor);
    }
#endif
}

namespace units {
namespace {

struct Range {
    Range() = default;

    template <size_t Size>
    explicit Range(const Unit (&units)[Size]) :
        _begin(std::begin(units)),
        _end(std::end(units))
    {}

    template <size_t Size>
    Range& operator=(const Unit (&units)[Size]) {
        _begin = std::begin(units);
        _end = std::end(units);
        return *this;
    }

    const Unit* begin() const {
        return _begin;
    }

    const Unit* end() const {
        return _end;
    }

private:
    const Unit* _begin { nullptr };
    const Unit* _end { nullptr };
};

Range range(unsigned char type) {
#define MAGNITUDE_UNITS_RANGE(...)\
    static const Unit units[] PROGMEM {\
        __VA_ARGS__\
    };\
\
    out = units

    Range out;

    switch (type) {

    case MAGNITUDE_TEMPERATURE: {
        MAGNITUDE_UNITS_RANGE(
            Unit::Celcius,
            Unit::Farenheit,
            Unit::Kelvin
        );
        break;
    }

    case MAGNITUDE_HUMIDITY:
    case MAGNITUDE_POWER_FACTOR: {
        MAGNITUDE_UNITS_RANGE(
            Unit::Percentage
        );
        break;
    }

    case MAGNITUDE_PRESSURE: {
        MAGNITUDE_UNITS_RANGE(
            Unit::Hectopascal
        );
        break;
    }

    case MAGNITUDE_CURRENT: {
        MAGNITUDE_UNITS_RANGE(
            Unit::Ampere
        );
        break;
    }

    case MAGNITUDE_VOLTAGE: {
        MAGNITUDE_UNITS_RANGE(
            Unit::Volt
        );
        break;
    }

    case MAGNITUDE_POWER_ACTIVE: {
        MAGNITUDE_UNITS_RANGE(
            Unit::Watt,
            Unit::Kilowatt
        );
        break;
    }

    case MAGNITUDE_POWER_APPARENT: {
        MAGNITUDE_UNITS_RANGE(
            Unit::Voltampere,
            Unit::Kilovoltampere
        );
        break;
    }

    case MAGNITUDE_POWER_REACTIVE: {
        MAGNITUDE_UNITS_RANGE(
            Unit::VoltampereReactive,
            Unit::KilovoltampereReactive
        );
        break;
    }

    case MAGNITUDE_ENERGY_DELTA: {
        MAGNITUDE_UNITS_RANGE(
            Unit::Joule
        );
        break;
    }

    case MAGNITUDE_ENERGY: {
        MAGNITUDE_UNITS_RANGE(
            Unit::Joule,
            Unit::KilowattHour
        );
        break;
    }

    case MAGNITUDE_PM1DOT0:
    case MAGNITUDE_PM2DOT5:
    case MAGNITUDE_PM10:
    case MAGNITUDE_TVOC:
    case MAGNITUDE_CH2O: {
        MAGNITUDE_UNITS_RANGE(
            Unit::MicrogrammPerCubicMeter,
            Unit::MilligrammPerCubicMeter
        );
        break;
    }

    case MAGNITUDE_CO:
    case MAGNITUDE_CO2:
    case MAGNITUDE_NO2:
    case MAGNITUDE_VOC: {
        MAGNITUDE_UNITS_RANGE(
            Unit::PartsPerMillion
        );
        break;
    }

    case MAGNITUDE_LUX: {
        MAGNITUDE_UNITS_RANGE(
            Unit::Lux
        );
        break;
    }

    case MAGNITUDE_RESISTANCE: {
        MAGNITUDE_UNITS_RANGE(
            Unit::Ohm
        );
        break;
    }

    case MAGNITUDE_HCHO: {
        MAGNITUDE_UNITS_RANGE(
            Unit::MilligrammPerCubicMeter
        );
        break;
    }

    case MAGNITUDE_GEIGER_CPM: {
        MAGNITUDE_UNITS_RANGE(
            Unit::CountsPerMinute
        );
        break;
    }

    case MAGNITUDE_GEIGER_SIEVERT: {
        MAGNITUDE_UNITS_RANGE(
            Unit::MicrosievertPerHour
        );
        break;
    }

    case MAGNITUDE_DISTANCE: {
        MAGNITUDE_UNITS_RANGE(
            Unit::Meter
        );
        break;
    }

    case MAGNITUDE_FREQUENCY: {
        MAGNITUDE_UNITS_RANGE(
            Unit::Hertz
        );
        break;
    }

    case MAGNITUDE_PH: {
        MAGNITUDE_UNITS_RANGE(
            Unit::Ph
        );
        break;
    }

    }

#undef MAGNITUDE_UNITS_RANGE
    return out;
}

bool supported(const Magnitude& magnitude, Unit unit) {
    const auto range = units::range(magnitude.type);
    return std::any_of(range.begin(), range.end(), [&](sensor::Unit supported) {
        return (unit == supported);
    });
}

sensor::Unit filter(const Magnitude& magnitude, Unit unit) {
    return supported(magnitude, unit) ? unit : magnitude.units;
}

String name(Unit unit) {
    return espurna::settings::internal::serialize(unit);
}

String name(const Magnitude& magnitude) {
    return name(magnitude.units);
}

} // namespace
} // namespace units

// -----------------------------------------------------------------------------
// Energy persistence
// -----------------------------------------------------------------------------

namespace energy {
namespace {

struct Persist {
    Persist(size_t index, Energy energy) :
        _index(index),
        _energy(energy)
    {}

    void operator()() const {
        setSetting({F("eneTotal"), _index}, _energy.asString());
#if NTP_SUPPORT
        if (ntpSynced()) {
            setSetting({F("eneTime"), _index}, ntpDateTime());
        }
#endif
    }

private:
    size_t _index;
    Energy _energy;
};

struct Tracker {
    using Reference = std::reference_wrapper<const Magnitude>;

    struct Counter {
        Reference magnitude;
        int value;
    };

    using Counters = std::vector<Counter>;

    explicit operator bool() const {
        return _every > 0;
    }

    int every() const {
        return _every;
    }

    void add(Reference magnitude) {
        _count.push_back(Counter{
            .magnitude = magnitude,
            .value = 0
        });
    }

    size_t size() const {
        return _count.size();
    }

    int count(size_t index) const {
        return _count[index].value;
    }

    template <typename Callback>
    void tick(unsigned char index, Callback&& callback) {
        _count[index].value = (_count[index].value + 1) % _every;
        if (_count[index].value == 0) {
            callback();
        }
    }

    void every(int every) {
        _every = every;
        for (auto& count : _count) {
            count.value = 0;
        }
    }

private:
    Counters _count;
    int _every;
};

struct ParseResult {
    ParseResult() = default;
    ParseResult& operator=(sensor::Energy value) {
        _value = value;
        _result = true;
        return *this;
    }

    explicit operator bool() const {
        return _result;
    }

    Energy value() const {
        return _value;
    }

private:
    bool _result { false };
    Energy _value;
};

namespace internal {

Tracker tracker;

} // namespace internal

Energy get_rtcmem(unsigned char index) {
    return Energy {
        Energy::Pair {
            .kwh = KilowattHours(Rtcmem->energy[index].kwh),
            .ws = WattSeconds(Rtcmem->energy[index].ws),
        }};
}

void set_rtcmem(unsigned char index, const Energy& source) {
    const auto pair = source.pair();
    Rtcmem->energy[index].kwh = pair.kwh.value;
    Rtcmem->energy[index].ws = pair.ws.value;
}


ParseResult convert(StringView value) {
    ParseResult out;
    if (!value.length()) {
        return out;
    }

    const auto begin = value.begin();
    const auto end = value.end();

    String kwh_number;

    auto it = begin;
    while (it != end) {
        if (*it == '+') {
            break;
        }

        kwh_number += *it;
        ++it;
    }

    KilowattHours::Type kwh { 0 };
    WattSeconds::Type ws { 0 };

    const auto result = parseUnsigned(kwh_number, 10);
    if (!result.ok) {
        return out;
    }

    kwh = result.value;

    if ((it != end) && (*it == '+')) {
        ++it;
        if (it == end) {
            return out;
        }

        const auto result = parseUnsigned(
                StringView(it, end), 10);
        if (!result.ok) {
            return out;
        }

        ws = result.value;
    }

    out = Energy {
        Energy::Pair {
            .kwh = KilowattHours(kwh),
            .ws = WattSeconds(ws),
        }};

    return out;
}

Energy get_settings(unsigned char index) {
    using namespace settings;
    const auto current = getSetting(
        keys::get(prefix::get(MAGNITUDE_ENERGY), suffix::Total, index));
    return convert(current).value();
}

void set(const Magnitude& magnitude, const Energy& energy) {
    if (isEmon(magnitude.sensor)) {
        auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor.get());
        sensor->resetEnergy(magnitude.slot, energy);
    }
}

void set(const Magnitude& magnitude, StringView payload) {
    if (!payload.length()) {
        return;
    }

    auto energy = convert(payload);
    if (!energy) {
        return;
    }

    set(magnitude, energy.value());
}

Energy get(unsigned char index) {
    Energy result;

    if (rtcmemStatus() && (index < (sizeof(Rtcmem->energy) / sizeof(*Rtcmem->energy)))) {
        result = get_rtcmem(index);
    } else {
        result = get_settings(index);
    }

    return result;
}

void reset(unsigned char index) {
    delSetting({F("eneTotal"), index});
    delSetting({F("eneTime"), index});
    if (index < (sizeof(Rtcmem->energy) / sizeof(*Rtcmem->energy))) {
        Rtcmem->energy[index].kwh = 0;
        Rtcmem->energy[index].ws = 0;
    }
}

int every() {
    return internal::tracker.every();
}

void every(int value) {
    internal::tracker.every(value);
}

void update(const Magnitude& magnitude, bool persistent) {
    if (!isEmon(magnitude.sensor)) {
        return;
    }

    auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor.get());
    const auto energy = sensor->totalEnergy(magnitude.slot);

    // Always save to RTCMEM
    if (magnitude.index_global < (sizeof(Rtcmem->energy) / sizeof(*Rtcmem->energy))) {
        set_rtcmem(magnitude.index_global, energy);
    }

    // Save to EEPROM every '_sensor_save_every' readings
    if (persistent && internal::tracker) {
        internal::tracker.tick(magnitude.index_global,
            Persist{magnitude.index_global, energy});
    }
}

void reset() {
    for (auto type : magnitude::traits::ratio_types) {
        for (size_t index = 0; index < Magnitude::counts(type); ++index) {
            delSetting(settings::keys::get(settings::prefix::get(type), settings::suffix::Ratio, index));
        }
    }

    for (auto ptr : sensor::internal::sensors) {
        if (isEmon(ptr)) {
            DEBUG_MSG_P(PSTR("[EMON] Resetting %s\n"), ptr->description().c_str());
            static_cast<BaseEmonSensor*>(ptr.get())->resetRatios();
        }
    }
}

double ratioFromValue(const Magnitude& magnitude, double expected) {
    if (!isEmon(magnitude.sensor)) {
        return BaseEmonSensor::DefaultRatio;
    }

    auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor.get());
    return sensor->ratioFromValue(magnitude.slot, sensor->value(magnitude.slot), expected);
}

void setup(const Magnitude& magnitude) {
    if (!isEmon(magnitude.sensor)) {
        return;
    }

    auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor.get());
    sensor->initialEnergy(magnitude.slot, get(magnitude.index_global));
    internal::tracker.add(magnitude);

    DEBUG_MSG_P(PSTR("[ENERGY] Tracking %s/%u for %s\n"),
            magnitude::topic(magnitude).c_str(),
            magnitude.index_global,
            magnitude.sensor->description().c_str());
}

} // namespace
} // namespace energy

namespace settings {
namespace query {
namespace {

namespace getter {

struct Type {
    using Check = bool(*)(unsigned char);
    using Get = String(*)(const Magnitude&);

    StringView suffix;
    Check check;
    Get get;
};

#define EXACT_VALUE(NAME)\
String NAME (const Magnitude& magnitude) {\
    return espurna::settings::internal::serialize(magnitude.NAME);\
}

EXACT_VALUE(correction)
EXACT_VALUE(decimals)
EXACT_VALUE(filter_type)

String ratio(const Magnitude& magnitude) {
    const auto ptr = reinterpret_cast<BaseEmonSensor*>(magnitude.sensor.get());
    return String(ptr->defaultRatio(magnitude.slot));
}

EXACT_VALUE(units)

#undef EXACT_VALUE

static constexpr std::array<Type, 5> List PROGMEM {{
    {suffix::Correction, magnitude::traits::correction_supported, correction},
    {suffix::Filter, nullptr, filter_type},
    {suffix::Precision, nullptr, decimals},
    {suffix::Ratio, magnitude::traits::ratio_supported, ratio},
    {suffix::Units, nullptr, units},
}};

} // namespace getter

bool check(StringView key) {
    if (key.length() < 3) {
        return false;
    }

    using espurna::settings::query::samePrefix;
    if (samePrefix(key, settings::prefix::Sensor)) {
        return true;
    }

    if (samePrefix(key, settings::prefix::Power)) {
        return true;
    }

    return magnitude::forEachCountedCheck([&](unsigned char type) {
        return samePrefix(key, prefix::get(type));
    });
}

String get(StringView key) {
    String out;

    for (auto& magnitude : magnitude::internal::magnitudes) {
        for (const auto& type : getter::List) {
            if (type.check && !type.check(magnitude.type)) {
                continue;
            }

            const auto expected = keys::get(magnitude, type.suffix);
            if (key == expected.value()) {
                out = type.get(magnitude);
                goto out;
            }
        }
    }

out:
    return out;
}

void setup() {
    settingsRegisterQueryHandler({
        .check = check,
        .get = get,
    });
}

} // namespace
} // namespace query

void migrate(int version) {
    auto firstKey = [](unsigned char type, StringView suffix)  {
        return keys::get(prefix::get(type), suffix, 0).value();
    };

    // Some keys from older versions were longer
    if (version < 3) {
        moveSetting(F("powerUnits"), F("pwrUnits"));
        moveSetting(F("energyUnits"), F("eneUnits"));
    }

    // Energy is now indexed (based on magnitude.index_global)
	// Also update PZEM004T energy total across multiple devices
    if (version < 5) {
        moveSetting(F("eneTotal"), firstKey(MAGNITUDE_ENERGY, suffix::Total));
        moveSettings(F("pzEneTotal"), prefix::get(MAGNITUDE_ENERGY).toString() + StringView(suffix::Total));
    }

    // Unit ID is no longer shared, drop when equal to Min_ or None
    if (version < 5) {
        delSetting(F("pwrUnits"));
        delSetting(F("eneUnits"));
        delSetting(F("tmpUnits"));
    }

    // Generic pwr settings now have type-specific prefixes
    // (index 0, assuming there's only one emon sensor)
    if (version < 7) {
        moveSetting(F("pwrVoltage"), firstKey(MAGNITUDE_VOLTAGE, suffix::Mains));
        moveSetting(F("pwrRatioC"), firstKey(MAGNITUDE_CURRENT, suffix::Ratio));
        moveSetting(F("pwrRatioV"), firstKey(MAGNITUDE_VOLTAGE, suffix::Ratio));
        moveSetting(F("pwrRatioP"), firstKey(MAGNITUDE_POWER_ACTIVE, suffix::Ratio));
        moveSetting(F("pwrRatioE"), firstKey(MAGNITUDE_ENERGY, suffix::Ratio));
    }

#if HLW8012_SUPPORT
    if (version < 9) {
        moveSetting(F("snsHlw8012SelGPIO"), F("hlw8012SEL"));
        moveSetting(F("snsHlw8012CfGPIO"), F("hlw8012CF"));
        moveSetting(F("snsHlw8012Cf1GPIO"), F("hlw8012CF1"));
    }
#endif

    if (version < 11) {
        moveSetting(F("apiRealTime"), FPSTR(keys::RealTimeValues));
        moveSetting(F("tmpMinDelta"), firstKey(MAGNITUDE_TEMPERATURE, suffix::MinDelta));
        moveSetting(F("humMinDelta"), firstKey(MAGNITUDE_HUMIDITY, suffix::MinDelta));
        moveSetting(F("eneMaxDelta"), firstKey(MAGNITUDE_ENERGY, suffix::MaxDelta));
    }
}

} // namespace settings

// -----------------------------------------------------------------------------
// WebUI value display and actions
// -----------------------------------------------------------------------------

#if WEB_SUPPORT
namespace web {
namespace {

bool onKeyCheck(StringView key, const JsonVariant&) {
    return settings::query::check(key);
}

// Entries related to things reported by the module.
// - types of magnitudes that are available and the string values associated with them
// - error types and stringified versions of them
// - units are the value types of the magnitude
// TODO: magnitude types have some common keys and some specific ones, only implemented for the type
// e.g. voltMains is specific to the MAGNITUDE_VOLTAGE but *only* in analog mode, or eneRatio specific to MAGNITUDE_ENERGY
// but, notice that the sensor will probably be used to 'get' certain properties, to generate certain keys list

void types(JsonObject& root) {
    espurna::web::ws::EnumerablePayload payload{root, STRING_VIEW("types")};
    payload(STRING_VIEW("values"), {MAGNITUDE_NONE + 1, MAGNITUDE_MAX},
        [](size_t type) {
            return Magnitude::counts(type) > 0;
        },
        {{STRING_VIEW("type"), [](JsonArray& out, size_t index) {
            out.add(index);
        }},
        {STRING_VIEW("prefix"), [](JsonArray& out, size_t index) {
            out.add(FPSTR(settings::prefix::get(index).c_str()));
        }},
        {STRING_VIEW("name"), [](JsonArray& out, size_t index) {
            out.add(sensor::magnitude::name(index));
        }}
    });
}

void errors(JsonObject& root) {
    espurna::web::ws::EnumerablePayload payload{root, STRING_VIEW("errors")};
    payload(STRING_VIEW("values"), SENSOR_ERROR_MAX,
        {{STRING_VIEW("type"), [](JsonArray& out, size_t index) {
            out.add(index);
        }},
        {STRING_VIEW("name"), [](JsonArray& out, size_t index) {
            out.add(error(index));
        }}
    });
}

void units(JsonObject& root) {
    espurna::web::ws::EnumerablePayload payload{root, STRING_VIEW("units")};
    payload(STRING_VIEW("values"), magnitude::internal::magnitudes.size(),
        {{STRING_VIEW("supported"), [](JsonArray& out, size_t index) {
            JsonArray& units = out.createNestedArray();
            const auto range = units::range(magnitude::get(index).type);
            for (auto it = range.begin(); it != range.end(); ++it) {
                JsonArray& unit = units.createNestedArray();
                unit.add(static_cast<int>(*it)); // raw id
                unit.add(units::name(*it));  // as string
            }
        }}
    });
}

void initial(JsonObject& root) {
    if (!magnitude::count()) {
        return;
    }

    JsonObject& container = root.createNestedObject(F("magnitudes-init"));
    types(container);
    errors(container);
    units(container);
}

void list(JsonObject& root) {
    if (!magnitude::count()) {
        return;
    }

    espurna::web::ws::EnumerablePayload payload{root, STRING_VIEW("magnitudes-list")};
    payload(STRING_VIEW("values"), magnitude::count(),
        {{STRING_VIEW("index_global"), [](JsonArray& out, size_t index) {
            out.add(magnitude::get(index).index_global);
        }},
        {STRING_VIEW("type"), [](JsonArray& out, size_t index) {
            out.add(magnitude::get(index).type);
        }},
        {STRING_VIEW("description"), [](JsonArray& out, size_t index) {
            out.add(magnitude::description(magnitude::get(index)));
        }},
        {STRING_VIEW("units"), [](JsonArray& out, size_t index) {
            out.add(static_cast<int>(magnitude::get(index).units));
        }}
    });
}

void settings(JsonObject& root) {
    if (!magnitude::count()) {
        return;
    }

    // XXX: inject 'null' in the output. need this for optional fields, since the current
    // version of serializer only does this for char ptr and even makes NaN serialized as
    // NaN, instead of more commonly used null (but, expect this to be fixed after switching to v6+)
    static const char* const NullSymbol { nullptr };

    espurna::web::ws::EnumerablePayload payload{root, STRING_VIEW("magnitudes-settings")};
    payload(STRING_VIEW("values"), magnitude::count(),
        {{settings::suffix::Correction, [](JsonArray& out, size_t index) {
            const auto& magnitude = magnitude::get(index);
            if (magnitude::traits::correction_supported(magnitude.type)) {
                out.add(magnitude.correction);
            } else {
                out.add(NullSymbol);
            }
        }},
        {settings::suffix::Ratio, [](JsonArray& out, size_t index) {
            const auto& magnitude = magnitude::get(index);
            if (magnitude::traits::ratio_supported(magnitude.type)) {
                out.add(static_cast<BaseEmonSensor*>(magnitude.sensor.get())->getRatio(magnitude.slot));
            } else {
                out.add(NullSymbol);
            }
        }},
        {settings::suffix::ZeroThreshold, [](JsonArray& out, size_t index) {
            const auto threshold = magnitude::get(index).zero_threshold;
            if (!std::isnan(threshold)) {
                out.add(threshold);
            } else {
                out.add(NullSymbol);
            }
        }},
        {settings::suffix::MinDelta, [](JsonArray& out, size_t index) {
            out.add(magnitude::get(index).min_delta);
        }},
        {settings::suffix::MaxDelta, [](JsonArray& out, size_t index) {
            out.add(magnitude::get(index).max_delta);
        }}
    });

    root[FPSTR(settings::keys::RealTimeValues)] = realTimeValues();

    root[FPSTR(settings::keys::ReadInterval)] = readInterval().count();
    root[FPSTR(settings::keys::InitInterval)] = initInterval().count();
    root[FPSTR(settings::keys::ReportEvery)] = reportEvery();

    root[FPSTR(settings::keys::SaveEvery)] = energy::internal::tracker.every();
}

void energy(JsonObject& root) {
#if NTP_SUPPORT
    if (!energy::internal::tracker || !energy::internal::tracker.size()) {
        return;
    }

    espurna::web::ws::EnumerablePayload payload{root, STRING_VIEW("energy")};
    payload(STRING_VIEW("values"), espurna::settings::Iota(magnitude::count()),
        [](size_t index) {
            return magnitude::get(index).type == MAGNITUDE_ENERGY;
        },
        {{STRING_VIEW("id"), [](JsonArray& out, size_t index) {
            out.add(index);
        }},
        {STRING_VIEW("saved"), [](JsonArray& out, size_t index) {
            if (energy::internal::tracker) {
                out.add(getSetting({F("eneTime"), magnitude::get(index).index_global}, F("(unknown)")));
            } else {
                out.add("");
            }
        }}
    });
#endif
}

void magnitudes(JsonObject& root) {
    espurna::web::ws::EnumerablePayload payload{root, STRING_VIEW("magnitudes")};
    payload(STRING_VIEW("values"), magnitude::count(), {
        {STRING_VIEW("value"), [](JsonArray& out, size_t index) {
            const auto& magnitude = magnitude::get(index);
            out.add(magnitude::format(magnitude,
                magnitude::process(magnitude, magnitude.last)));
        }},
        {STRING_VIEW("units"), [](JsonArray& out, size_t index) {
            out.add(static_cast<int>(magnitude::get(index).units));
        }},
        {STRING_VIEW("error"), [](JsonArray& out, size_t index) {
            out.add(magnitude::get(index).sensor->error());
        }},
    });
}

void onData(JsonObject& root) {
    if (magnitude::count()) {
        magnitudes(root);
        energy(root);
    }
}

void onAction(uint32_t client_id, const char* action, JsonObject& data) {
    if (STRING_VIEW("emon-expected") == action) {
        auto id = data["id"].as<size_t>();
        if (id < magnitude::count()) {
            auto expected = data["expected"].as<float>();
            wsPost(client_id, [id, expected](JsonObject& root) {
                const auto& magnitude = magnitude::get(id);

                String key { F("result:") };
                key += settings::keys::get(
                    magnitude, settings::suffix::Ratio).value();

                root[key] = energy::ratioFromValue(magnitude, expected);
            });
        }
        return;
    }

    if (STRING_VIEW("emon-reset-ratios") == action) {
        energy::reset();
        return;
    }
}

void onVisible(JsonObject& root) {
    wsPayloadModule(root, PSTR("sns"));
    for (auto sensor : internal::sensors) {
        if (isEmon(sensor)) {
            wsPayloadModule(root, PSTR("emon"));
        }

        if (isAnalog(sensor)) {
            wsPayloadModule(root, PSTR("analog"));
        }

        switch (sensor->id()) {
#if HLW8012_SUPPORT
        case SENSOR_HLW8012_ID:
            wsPayloadModule(root, PSTR("hlw"));
            break;
#endif
#if CSE7766_SUPPORT
        case SENSOR_CSE7766_ID:
            wsPayloadModule(root, PSTR("cse"));
            break;
#endif
#if PZEM004T_SUPPORT || PZEM004TV30_SUPPORT
        case SENSOR_PZEM004T_ID:
        case SENSOR_PZEM004TV30_ID:
            wsPayloadModule(root, PSTR("pzem"));
            break;
#endif
#if PULSEMETER_SUPPORT
        case SENSOR_PULSEMETER_ID:
            wsPayloadModule(root, PSTR("pm"));
            break;
#endif
        }
    }
}

void module(JsonObject& root, const char* prefix, SensorWebSocketMagnitudesCallback callback) {
    espurna::web::ws::EnumerablePayload payload{root, STRING_VIEW("magnitudes-module")};

    auto& container = payload.root();
    container[F("prefix")] = FPSTR(prefix);

    payload(STRING_VIEW("values"), magnitude::count(),
        {{STRING_VIEW("type"), [](JsonArray& out, size_t index) {
            out.add(magnitude::get(index).type);
        }},
        {STRING_VIEW("index_global"), [](JsonArray& out, size_t index) {
            out.add(magnitude::get(index).index_global);
        }},
        {STRING_VIEW("index_module"), callback}
    });
}

void setup() {
    wsRegister()
        .onConnected(initial)
        .onConnected(list)
        .onConnected(settings)
        .onVisible(onVisible)
        .onData(onData)
        .onAction(onAction)
        .onKeyCheck(onKeyCheck);
}

} // namespace
} // namespace web
#endif

#if API_SUPPORT
namespace api {
namespace {

template <typename T>
bool tryHandle(ApiRequest& request, unsigned char type, T&& callback) {
    size_t index = 0;
    if (request.wildcards()) {
        const auto param = request.wildcard(0);
        if (!::tryParseId(param, magnitude::count(type), index)) {
            return false;
        }
    }

    const auto* magnitude = magnitude::find(type, index);
    if (magnitude) {
        callback(*magnitude);
        return true;
    }

    return false;
}

void setup() {
    apiRegister(F("magnitudes"),
        [](ApiRequest&, JsonObject& root) {
            JsonArray& magnitudes = root.createNestedArray("magnitudes");
            for (auto& magnitude : magnitude::internal::magnitudes) {
                JsonArray& data = magnitudes.createNestedArray();
                data.add(sensor::magnitude::topicWithIndex(magnitude));
                data.add(magnitude.last);
                data.add(magnitude.reported);
            }
            return true;
        },
        nullptr
    );

    magnitude::forEachCounted([](unsigned char type) {
        auto pattern = magnitude::topic(type);
        if (sensor::build::useIndex() || (magnitude::count(type) > 1)) {
            pattern += STRING_VIEW("/+");
        }

        ApiBasicHandler get = [type](ApiRequest& request) {
            return tryHandle(request, type,
                [&](const Magnitude& magnitude) {
                    request.send(magnitude::format(magnitude,
                        realTimeValues() ? magnitude.last : magnitude.reported));
                    return true;
                });
        };

        ApiBasicHandler put;
        if (type == MAGNITUDE_ENERGY) {
            put = [](ApiRequest& request) {
                return tryHandle(request, MAGNITUDE_ENERGY,
                    [&](const Magnitude& magnitude) {
                        energy::set(magnitude, request.param(F("value")));
                    });
            };
        }

        apiRegister(std::move(pattern), std::move(get), std::move(put));
    });
}

} // namespace
} // namespace api
#endif

#if MQTT_SUPPORT
namespace mqtt {
namespace {

void report(const Value& report, const Magnitude& magnitude) {
    mqttSend(report.topic.c_str(), report.repr.c_str());

#if SENSOR_PUBLISH_ADDRESSES
    STRING_VIEW_INLINE(AddressTopic, SENSOR_ADDRESS_TOPIC);

    String address_topic;
    address_topic.reserve(1 + report.topic.length() + AddressTopic.length());
    address_topic.concat(AddressTopic.data(), AddressTopic.length());
    address_topic += '/';
    address_topic += report.topic;

    mqttSend(address_topic.c_str(), magnitude.sensor->address(magnitude.slot).c_str());
#endif
}

void callback(unsigned int type, StringView topic, StringView payload) {
    if (!magnitude::count(MAGNITUDE_ENERGY)) {
        return;
    }

    static const auto base = magnitude::topic(MAGNITUDE_ENERGY);

    switch (type) {
    case MQTT_MESSAGE_EVENT:
    {
        auto t = mqttMagnitude(topic);
        if (!t.startsWith(base)) {
            break;
        }

        size_t index;
        if (!tryParseIdPath(t, magnitude::count(MAGNITUDE_ENERGY), index)) {
            break;
        }

        const auto* magnitude = magnitude::find(MAGNITUDE_ENERGY, index);
        if (magnitude) {
            energy::set(*magnitude, payload.toString());
        }

        break;
    }

    case MQTT_CONNECT_EVENT:
        mqttSubscribe((base + F("/+")).c_str());
        break;

    }
}

void setup() {
    ::mqttRegister(callback);
}

} // namespace
} // namespace mqtt
#endif

#if TERMINAL_SUPPORT
namespace terminal {
namespace {

namespace commands {

PROGMEM_STRING(Magnitudes, "MAGNITUDES");

void magnitudes(::terminal::CommandContext&& ctx) {
    if (!magnitude::count()) {
        terminalError(ctx, F("No magnitudes"));
        return;
    }

    size_t index = 0;
    for (const auto& magnitude : magnitude::internal::magnitudes) {
        ctx.output.printf_P(PSTR("%2zu * %s @ %s (read:%s reported:%s units:%s)\n"),
            index++, magnitude::topicWithIndex(magnitude).c_str(),
            magnitude::description(magnitude).c_str(),
            magnitude::format(magnitude, magnitude.last).c_str(),
            magnitude::format(magnitude, magnitude.reported).c_str(),
            units::name(magnitude).c_str());
    }

    terminalOK(ctx);
}

PROGMEM_STRING(Expected, "EXPECTED");

void expected(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() == 3) {
        const auto id = espurna::settings::internal::convert<size_t>(ctx.argv[1]);
        if (id < magnitude::count()) {
            const auto& magnitude = magnitude::get(id);

            const auto result = energy::ratioFromValue(
                magnitude, espurna::settings::internal::convert<double>(ctx.argv[2]));
            const auto key = settings::keys::get(
                magnitude, settings::suffix::Ratio);
            ctx.output.printf("%s => %s\n", key.c_str(), String(result).c_str());
            terminalOK(ctx);
            return;
        }

        terminalError(ctx, F("Invalid magnitude ID"));
        return;
    }

    terminalError(ctx, F("EXPECTED <ID> <VALUE>"));
}

PROGMEM_STRING(ResetRatios, "RESET.RATIOS");

void reset_ratios(::terminal::CommandContext&& ctx) {
    energy::reset();
    terminalOK(ctx);
}

PROGMEM_STRING(Energy, "ENERGY");

void energy(::terminal::CommandContext&& ctx) {
    using IndexType = decltype(Magnitude::index_global);

    if (ctx.argv.size() < 2) {
        terminalError(ctx, F("ENERGY <ID> [<VALUE>]"));
        return;
    }

    const auto index = espurna::settings::internal::convert<IndexType>(ctx.argv[1]);

    const auto* magnitude = magnitude::find(MAGNITUDE_ENERGY, index);
    if (!magnitude) {
        terminalError(ctx, F("Invalid magnitude ID"));
        return;
    }

    if (ctx.argv.size() == 2) {
        ctx.output.printf_P(PSTR("%s => %s (%s)\n"),
            magnitude::topicWithIndex(*magnitude).c_str(),
            magnitude::format(*magnitude, magnitude->reported).c_str(),
            units::name(*magnitude).c_str());
        terminalOK(ctx);
        return;
    }

    if (ctx.argv.size() == 3) {
        const auto energy = energy::convert(ctx.argv[2]);
        if (!energy) {
            terminalError(ctx, F("Invalid energy string"));
            return;
        }

        energy::set(*magnitude, energy.value());
    }
}

static constexpr ::terminal::Command List[] PROGMEM {
    {Magnitudes, commands::magnitudes},
    {Expected, commands::expected},
    {ResetRatios, commands::reset_ratios},
    {Energy, commands::energy},
};

} // namespace commands

void setup() {
    espurna::terminal::add(commands::List);
}

} // namespace
} // namespace terminal
#endif

// -----------------------------------------------------------------------------
// Sensor initialization
// -----------------------------------------------------------------------------

namespace internal {

State state;

TimeSource::time_point last_init;
TimeSource::time_point last_reading;

} // namespace internal

void suspend() {
    for (auto& sensor : internal::sensors) {
        sensor->suspend();
    }
}

void resume() {
    internal::last_init = TimeSource::now();
    internal::last_reading = TimeSource::now();
    internal::read_count = 1;

    magnitude::forEachInstance(
        [](sensor::Magnitude& instance) {
            instance.filter->reset();
        });

    for (auto& sensor : internal::sensors) {
        sensor->resume();
    }
}

bool init() {
    bool out { true };

    for (auto sensor : internal::sensors) {
        // Do not process an already initialized sensor
        if (sensor->ready()) {
            continue;
        }

        // Force sensor to reload config
        DEBUG_MSG_P(PSTR("[SENSOR] Initializing %s\n"),
            sensor->description().c_str());
        sensor->begin();

        if (!sensor->ready()) {
            const auto error = sensor->error();
            if (error != SENSOR_ERROR_OK) {
                DEBUG_MSG_P(PSTR("[SENSOR]  -> ERROR %s (%hhu)\n"),
                    sensor::error(error).c_str(), error);
            }
            out = false;
            break;
        }

        const auto slots = sensor->count();
        for (auto slot = 0; slot < slots; ++slot) {
            magnitude::add(sensor, slot, sensor->type(slot));
        }
    }

    // Energy tracking is implemented by looking at the specific magnitude & it's index at read time
    // TODO: shuffle some functions around so that debug can be in the init func instead and still be inline?
    for (auto& magnitude : magnitude::internal::magnitudes) {
        if (MAGNITUDE_ENERGY == magnitude.type) {
            energy::setup(magnitude);
        }
    }

    if (out) {
        internal::state = State::Ready;
        DEBUG_MSG_P(PSTR("[SENSOR] Finished initialization for %zu sensor(s) and %zu magnitude(s)\n"),
            sensor::count(), magnitude::count());
    }

    return out;
}

bool try_init() {
    const auto timestamp = TimeSource::now();
    if (timestamp - internal::last_init > initInterval()) {
        internal::last_init = timestamp;
        return init();
    }

    return false;
}

bool ready_to_read() {
    const auto timestamp = TimeSource::now();
    if (timestamp - internal::last_reading > readInterval()) {
        internal::last_reading = timestamp;
        internal::read_count = (internal::read_count + 1) % reportEvery();
        return true;
    }

    return false;
}

bool ready_to_report() {
    return internal::read_count == 0;
}

void loop() {
    // TODO: allow to do nothing
    if (internal::state == State::Idle) {
        return;
    }

    // Continiously repeat initialization if there are still some un-initialized sensors after setup()
    if (internal::state == State::None) {
        internal::state = State::Initial;
    }

    // General initialization, generate magnitudes from available sensors
    if (internal::state == State::Initial) {
        if (try_init()) {
            internal::state = State::Ready;
        }
    }

    // If magnitudes were initialized and we are ready, prepare to read sensor data
    if (internal::state == State::Ready) {
        if (magnitude::internal::magnitudes.size() != 0) {
            internal::state = State::Reading;
        }
    }

    if (internal::state != State::Reading) {
        return;
    }

    // Tick hook, called every loop()
    sensor::tick();

    if (ready_to_read()) {
        // Pre-read hook, called every reading
        sensor::pre();

        // XXX: Filter out certain magnitude types when relay is turned OFF
#if RELAY_SUPPORT && SENSOR_POWER_CHECK_STATUS
        const bool relay_off = (relayCount() == 1) && (relayStatus(0) == 0);
#endif

        auto value = sensor::ReadValue{};

        for (size_t index = 0; index < magnitude::count(); ++index) {
            auto& magnitude = magnitude::get(index);
            if (!magnitude.sensor->status()) {
                continue;
            }

            // -------------------------------------------------------------
            // RAW value, returned from the sensor
            // -------------------------------------------------------------

            value.raw = magnitude.sensor->value(magnitude.slot);

            // But, completely remove spurious values if relay is OFF
#if RELAY_SUPPORT && SENSOR_POWER_CHECK_STATUS
            switch (magnitude.type) {
            case MAGNITUDE_POWER_ACTIVE:
            case MAGNITUDE_POWER_REACTIVE:
            case MAGNITUDE_POWER_APPARENT:
            case MAGNITUDE_POWER_FACTOR:
            case MAGNITUDE_CURRENT:
            case MAGNITUDE_ENERGY_DELTA:
                if (relay_off) {
                    value.raw = 0.0;
                }
                break;
            default:
                break;
            }
#endif

            // We also check that value is above a certain threshold
            if ((!std::isnan(magnitude.zero_threshold)) && ((value.raw < magnitude.zero_threshold))) {
                value.raw = 0.0;
            }

            magnitude.last = value.raw;
            magnitude.filter->update(value.raw);

            // -------------------------------------------------------------
            // Procesing (units and decimals)
            // -------------------------------------------------------------

            value.processed = magnitude::process(magnitude, value.raw);
            magnitude::read(magnitude::value(magnitude, value.processed));

            // -------------------------------------------------------------------
            // Reporting
            // -------------------------------------------------------------------

            // Initial status or after report counter overflows
            bool report { ready_to_report() };

            // In case magnitude was configured with ${name}MaxDelta, override report check
            // when the value change is greater than the delta
            if (!std::isnan(magnitude.reported) && (magnitude.max_delta > build::DefaultMaxDelta)) {
                report = std::abs(value.processed - magnitude.reported) >= magnitude.max_delta;
            }

            // Special case for energy, save readings to RAM and EEPROM
            if (MAGNITUDE_ENERGY == magnitude.type) {
                energy::update(magnitude, report);
            }

            if (report) {
                value.filtered = magnitude::process(magnitude, magnitude.filter->value());

                // Make sure that report value is calculated using every read value before it
                magnitude.filter->reset();

                // Check ${name}MinDelta if there is a minimum change threshold to report
                if (std::isnan(magnitude.reported) || (std::abs(value.filtered - magnitude.reported) >= magnitude.min_delta)) {
                    const auto report = magnitude::value(magnitude, value.filtered);
                    magnitude::report(report);

#if MQTT_SUPPORT
                    mqtt::report(report, magnitude);
#endif
#if THINGSPEAK_SUPPORT
                    tspkEnqueueMagnitude(index, report.repr);
#endif
#if DOMOTICZ_SUPPORT
                    domoticzSendMagnitude(index, report);
#endif
                    magnitude.reported = value.filtered;
                }

            }

#if SENSOR_DEBUG
            {
                auto withUnits = [&](double value, Unit units) {
                    String out;
                    out += magnitude::format(magnitude, value);
                    if (units != Unit::None) {
                        out += units::name(units);
                    }

                    return out;
                };

                DEBUG_MSG_P(PSTR("[SENSOR] %s -> raw %s processed %s filtered %s\n"),
                    magnitude::topic(magnitude).c_str(),
                    withUnits(value.raw, magnitude.sensor->units(magnitude.slot)).c_str(),
                    withUnits(value.processed, magnitude.units).c_str(),
                    withUnits(value.filtered, magnitude.units).c_str());
            }
#endif
        }

        sensor::post();

#if WEB_SUPPORT
        wsPost(web::onData);
#endif
    }
}

void configure() {
    // Read interval is shared between every sensor
    // TODO: implement scheduling in the sensor itself.
    // allow reads faster than 1sec, not just internal ones via tick()
    // allow 'manual' sensors that may be triggered programatically
    readInterval(sensor::settings::readInterval());
    initInterval(sensor::settings::initInterval());
    reportEvery(sensor::settings::reportEvery());
    realTimeValues(sensor::settings::realTimeValues());

    // TODO: something more generic? energy is an accumulating value, only allow for similar ones?
    // TODO: move to an external module?
    energy::every(sensor::settings::saveEvery());

    // Update magnitude config, filter sizes and reset energy if needed
    // TODO: namespace and various helpers need some naming tweaks...
    for (auto& magnitude : magnitude::internal::magnitudes) {
        // Only initialized once, notify about reset requirement?
        if (!magnitude.filter) {
            magnitude.filter_type = getSetting(
                settings::keys::get(magnitude, settings::suffix::Filter),
                magnitude::defaultFilter(magnitude));
            magnitude.filter = magnitude::makeFilter(magnitude.filter_type);
        }

        // Some filters must be able store up to a certain amount of readings.
        if (magnitude.filter->capacity() != reportEvery()) {
            magnitude.filter->resize(reportEvery());
        }

        // process emon-specific settings first. ensure that settings use global index and we access sensor with the local one
        if (isEmon(magnitude.sensor) && magnitude::traits::ratio_supported(magnitude.type)) {
            auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor.get());
            sensor->setRatio(magnitude.slot, getSetting(
                settings::keys::get(magnitude, settings::suffix::Ratio),
                sensor->defaultRatio(magnitude.slot)));
        }

        // analog variant of emon sensor has some additional settings
        if (isAnalogEmon(magnitude.sensor) && (magnitude.type == MAGNITUDE_VOLTAGE)) {
            auto* sensor = static_cast<BaseAnalogEmonSensor*>(magnitude.sensor.get());
            sensor->setVoltage(getSetting(
                settings::keys::get(magnitude, settings::suffix::Mains),
                sensor->defaultVoltage()));
            sensor->setReferenceVoltage(getSetting(
                settings::keys::get(magnitude, settings::suffix::Reference),
                sensor->defaultReferenceVoltage()));
        }

        // adjust units based on magnitude's type
        magnitude.units = units::filter(magnitude,
            getSetting(
                settings::keys::get(magnitude, settings::suffix::Units),
                magnitude.sensor->units(magnitude.slot)));

        // adjust resulting value (simple plus or minus)
        // TODO: inject math or rpnlib expression?
        if (magnitude::traits::correction_supported(magnitude.type)) {
            magnitude.correction = getSetting(
                settings::keys::get(magnitude, settings::suffix::Correction),
                magnitude::build::correction(magnitude.type));
        }

        // pick decimal precision either from our (sane) defaults of from the sensor itself
        // (specifically, when sensor has more or less precision than we expect)
        {
            const auto decimals = magnitude.sensor->decimals(magnitude.units);
            magnitude.decimals = getSetting(
                settings::keys::get(magnitude, settings::suffix::Precision),
                    (decimals >= 0)
                        ? static_cast<unsigned char>(decimals)
                        : magnitude::decimals(magnitude.units));
        }

        // Per-magnitude min & max delta settings for reporting the value
        // - ${prefix}MinDelta${index} controls whether we report when report counter overflows
        //   (default is set to 0.0 aka value has changed from the last recorded one)
        // - ${prefix}MaxDelta${index} will trigger report as soon as read value is greater than the specified delta
        //   (default is 0.0 as well, but this needs to be >0 to actually do something)
        magnitude.min_delta = getSetting(
            settings::keys::get(magnitude, settings::suffix::MinDelta),
            build::DefaultMinDelta);
        magnitude.max_delta = getSetting(
            settings::keys::get(magnitude, settings::suffix::MaxDelta),
            build::DefaultMaxDelta);

        // Sometimes we want to ensure the value is above certain threshold before reporting
        magnitude.zero_threshold = getSetting(
            settings::keys::get(magnitude, settings::suffix::ZeroThreshold),
            Value::Unknown);

        // When we don't save energy, purge existing value in both RAM & settings
        if (isEmon(magnitude.sensor) && (MAGNITUDE_ENERGY == magnitude.type) && (0 == energy::every())) {
            energy::reset(magnitude.index_global);
        }
    }
}

void setup() {
    migrateVersion(settings::migrate);

    sensor::load();
    sensor::init();

    // Configure based on settings
    sensor::configure();

    // Allow us to query key default
    sensor::settings::query::setup();

    // Websockets integration, send sensor readings and configuration
#if WEB_SUPPORT
    web::setup();
#endif

    // Publishes sensor reports, and {re,}set energy
#if MQTT_SUPPORT
    mqtt::setup();
#endif

#if API_SUPPORT
    api::setup();
#endif

#if TERMINAL_SUPPORT
    terminal::setup();
#endif

    systemBeforeSleep(sensor::suspend);
    systemAfterSleep(sensor::resume);

    espurnaRegisterLoop(sensor::loop);
    espurnaRegisterReload(sensor::configure);
}

} // namespace sensor
} // namespace espurna

// -----------------------------------------------------------------------------
// Public
// -----------------------------------------------------------------------------

#if WEB_SUPPORT
// Used by modules to generate magnitude_id<->module_id mapping for the WebUI
// Prefix controls the UI templates, supplied callback should retrieve module-specific value Id
void sensorWebSocketMagnitudes(JsonObject& root, const char* prefix, SensorWebSocketMagnitudesCallback callback) {
    espurna::sensor::web::module(root, prefix, callback);
}
#endif // WEB_SUPPORT

void sensorOnMagnitudeRead(MagnitudeReadHandler handler) {
    espurna::sensor::magnitude::onRead(handler);
}

void sensorOnMagnitudeReport(MagnitudeReadHandler handler) {
    espurna::sensor::magnitude::onReport(handler);
}

size_t magnitudeCount() {
    return espurna::sensor::magnitude::count();
}

unsigned char magnitudeIndex(unsigned char index) {
    using namespace espurna::sensor;

    if (index < magnitude::count()) {
        return magnitude::get(index).index_global;
    }

    return 0;
}

unsigned char magnitudeType(unsigned char index) {
    using namespace espurna::sensor;

    if (index < magnitude::count()) {
        return magnitude::get(index).type;
    }

    return MAGNITUDE_NONE;
}

espurna::sensor::Value magnitudeReadValue(unsigned char index) {
    return espurna::sensor::magnitude::safe_value_last(index);
}

espurna::sensor::Value magnitudeReportValue(unsigned char index) {
    return espurna::sensor::magnitude::safe_value_reported(index);
}

espurna::sensor::Value magnitudeValue(unsigned char index) {
    return espurna::sensor::realTimeValues()
        ? espurna::sensor::magnitude::safe_value_last(index)
        : espurna::sensor::magnitude::safe_value_reported(index);
}

String magnitudeDescription(unsigned char index) {
    using namespace espurna::sensor;

    if (index < magnitude::count()) {
        return magnitude::description(magnitude::get(index));
    }

    return String();
}

String magnitudeTopic(unsigned char index) {
    using namespace espurna::sensor;

    if (index < magnitude::count()) {
        return magnitude::topicWithIndex(magnitude::get(index));
    }

    return String();
}

String magnitudeTypeTopic(unsigned char type) {
    return espurna::sensor::magnitude::topic(type);
}

String magnitudeUnitsName(espurna::sensor::Unit units) {
    return espurna::sensor::units::name(units);
}

espurna::sensor::Info magnitudeInfo(unsigned char index) {
    using namespace espurna::sensor;

    if (index < magnitude::count()) {
        return magnitude::info(magnitude::get(index));
    }

    return Info {
        .type = MAGNITUDE_NONE,
        .index = 0,
        .units = Unit::None,
        .decimals = 0,
    };
}

espurna::StringView sensorList() {
    return espurna::sensor::List;
}

void sensorSetup() {
    espurna::sensor::setup();
}

#endif // SENSOR_SUPPORT
