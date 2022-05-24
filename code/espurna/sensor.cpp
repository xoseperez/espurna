/*

SENSOR MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

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

namespace {

#include "filters/LastFilter.h"
#include "filters/MaxFilter.h"
#include "filters/MedianFilter.h"
#include "filters/MovingAverageFilter.h"
#include "filters/SumFilter.h"

} // namespace

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
// TODO: this is temporary, until we have external API giving us swserial stream objects
    #include <SoftwareSerial.h>
    #include "sensors/PZEM004TV30Sensor.h"
#endif

//--------------------------------------------------------------------------------

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

    Magnitude(BaseSensorPtr, BaseFilterPtr, unsigned char slot, unsigned char type);

    BaseSensorPtr sensor; // Sensor object, *cannot be empty*
    BaseFilterPtr filter; // Filter object, *could be empty*

    unsigned char slot; // Sensor slot # taken by the magnitude, used to access the measurement
    unsigned char type; // Type of measurement, returned by the BaseSensor::type(slot)

    unsigned char index_global; // N'th magnitude of it's type, across all of the active sensors

    sensor::Unit units { sensor::Unit::None }; // Units of measurement
    unsigned char decimals { 0u }; // Number of decimals in textual representation

    double last { sensor::Value::Unknown };     // Last raw value from sensor (unfiltered)
    double reported { sensor::Value::Unknown }; // Last reported value
    double min_delta { 0.0 };   // Minimum value change to report
    double max_delta { 0.0 };   // Maximum value change to report
    double correction { 0.0 };  // Value correction (applied when processing)

    double zero_threshold { sensor::Value::Unknown }; // Reset value to zero when below threshold (applied when reading)
};

static_assert(
    std::is_nothrow_move_constructible<Magnitude>::value,
    "std::vector<Magnitude> should be able to work with resize()"
);

static_assert(
    !std::is_copy_constructible<Magnitude>::value,
    "std::vector<Magnitude> should only use move ctor"
);

unsigned char Magnitude::_counts[MAGNITUDE_MAX] = {0};

} // namespace

namespace sensor {

// Base units
// TODO: implement through a single class and allow direct access to the ::value

KWh::KWh() :
    value(0)
{}

KWh::KWh(uint32_t value) :
    value(value)
{}

Ws::Ws() :
    value(0)
{}

Ws::Ws(uint32_t value) :
    value(value)
{}

// Generic storage. Most of the time we init this on boot with both members or start at 0 and increment with watt-second

Energy::Energy(KWh kwh, Ws ws) :
    kwh(kwh)
{
    *this += ws;
}

Energy::Energy(KWh kwh) :
    kwh(kwh),
    ws()
{}

Energy::Energy(Ws ws) :
    kwh()
{
    *this += ws;
}

Energy::Energy(double raw) {
    *this = raw;
}

Energy& Energy::operator =(double raw) {
    double _wh;
    kwh = modf(raw, &_wh);
    ws = _wh * 3600.0;
    return *this;
}

Energy& Energy::operator +=(Ws _ws) {
    while (_ws.value >= KwhMultiplier) {
        _ws.value -= KwhMultiplier;
        ++kwh.value;
    }
    ws.value += _ws.value;
    while (ws.value >= KwhMultiplier) {
        ws.value -= KwhMultiplier;
        ++kwh.value;
    }
    return *this;
}

Energy Energy::operator +(Ws watt_s) {
    Energy result(*this);
    result += watt_s;
    return result;
}

Energy::operator bool() const {
    return (kwh.value > 0) && (ws.value > 0);
}

Ws Energy::asWs() const {
    auto _kwh = kwh.value;
    while (_kwh >= KwhLimit) {
        _kwh -= KwhLimit;
    }

    return (_kwh * KwhMultiplier) + ws.value;
}

double Energy::asDouble() const {
    return (double)kwh.value + ((double)ws.value / (double)KwhMultiplier);
}

// Format is `<kwh>+<ws>`
// Value without `+` is treated as `<ws>`
// (internally, we *can* overflow ws that is converted into kwh)
String Energy::asString() const {
    String out;
    out.reserve(32);

    out += kwh.value;
    out += '+';
    out += ws.value;

    return out;
}

void Energy::reset() {
    kwh.value = 0;
    ws.value = 0;
}

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

template <typename To, typename From>
struct Converter;

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
struct Converter<Kelvin, Kelvin> {
    static constexpr Kelvin convert(Kelvin kelvin) {
        return kelvin;
    }
};

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
struct Converter<Celcius, Celcius> {
    static constexpr Celcius convert(Celcius celcius) {
        return celcius;
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
struct Converter<Farenheit, Farenheit> {
    static constexpr Farenheit convert(Farenheit farenheit) {
        return farenheit;
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
static_assert(Converter<Celcius, Celcius>::convert(AbsoluteZero) == AbsoluteZero, "");

} // namespace internal

template <typename To, typename From>
constexpr To unit_cast(From value) {
    return internal::Converter<To, From>::convert(value);
}

static_assert(unit_cast<Kelvin>(AbsoluteZero).value() == 0.0, "");
static_assert(unit_cast<Celcius>(AbsoluteZero).value() == AbsoluteZero.value(), "");

// since the outside api only works with the enumeration, make sure to cast it to our types for conversion
// a table like this could've also worked
// > {sensor::Unit(from), sensor::Unit(to), Converter(double(*)(double))}
// but, it is ~0.6KiB vs. ~0.1KiB for this one. plus, some obstacles with c++11 implementation
// although, there may be a way to make this cheaper in both compile-time and runtime

// attempt to convert the input value from one unit to the other
// will return the input value when units match or there's no known conversion
static constexpr double convert(double value, sensor::Unit from, sensor::Unit to) {
#define UNIT_CAST(FROM, TO) \
    ((from == sensor::Unit::FROM) && (to == sensor::Unit::TO)) \
        ? (unit_cast<TO>(FROM{value}))

     return UNIT_CAST(Kelvin, Kelvin) :
        UNIT_CAST(Kelvin, Celcius) :
        UNIT_CAST(Kelvin, Farenheit) :
        UNIT_CAST(Celcius, Celcius) :
        UNIT_CAST(Celcius, Kelvin) :
        UNIT_CAST(Celcius, Farenheit) :
        UNIT_CAST(Farenheit, Farenheit) :
        UNIT_CAST(Farenheit, Kelvin) :
        UNIT_CAST(Farenheit, Celcius) : value;

#undef UNIT_CAST
}

} // namespace
} // namespace temperature
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

constexpr size_t ReportEveryMin { SENSOR_REPORT_MIN_EVERY };
constexpr size_t ReportEveryMax { SENSOR_REPORT_MAX_EVERY };

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
namespace prefix {
namespace {

alignas(4) static constexpr char Sensor[] PROGMEM = "sns";
alignas(4) static constexpr char Power[] PROGMEM = "pwr";

alignas(4) static constexpr char Temperature[] = "tmp";
alignas(4) static constexpr char Humidity[] = "hum";
alignas(4) static constexpr char Pressure[] = "press";
alignas(4) static constexpr char Current[] = "curr";
alignas(4) static constexpr char Voltage[] = "volt";
alignas(4) static constexpr char PowerActive[] = "pwrP";
alignas(4) static constexpr char PowerApparent[] = "pwrQ";
alignas(4) static constexpr char PowerReactive[] = "pwrModS";
alignas(4) static constexpr char PowerFactor[] = "pwrPF";
alignas(4) static constexpr char Energy[] = "ene";
alignas(4) static constexpr char EnergyDelta[] = "eneDelta";
alignas(4) static constexpr char Analog[] = "analog";
alignas(4) static constexpr char Digital[] = "digital";
alignas(4) static constexpr char Event[] = "event";
alignas(4) static constexpr char Pm1Dot0[] = "pm1dot0";
alignas(4) static constexpr char Pm2Dot5[] = "pm2dot5";
alignas(4) static constexpr char Pm10[] = "pm10";
alignas(4) static constexpr char Co2[] = "co2";
alignas(4) static constexpr char Voc[] = "voc";
alignas(4) static constexpr char Iaq[] = "iaq";
alignas(4) static constexpr char IaqAccuracy[] = "iaqAccuracy";
alignas(4) static constexpr char IaqStatic[] = "iaqStatic";
alignas(4) static constexpr char Lux[] = "lux";
alignas(4) static constexpr char Uva[] = "uva";
alignas(4) static constexpr char Uvb[] = "uvb";
alignas(4) static constexpr char Uvi[] = "uvi";
alignas(4) static constexpr char Distance[] = "distance";
alignas(4) static constexpr char Hcho[] = "hcho";
alignas(4) static constexpr char GeigerCpm[] = "gcpm";
alignas(4) static constexpr char GeigerSievert[] = "gsiev";
alignas(4) static constexpr char Count[] = "count";
alignas(4) static constexpr char No2[] = "no2";
alignas(4) static constexpr char Co[] = "co";
alignas(4) static constexpr char Resistance[] = "res";
alignas(4) static constexpr char Ph[] = "ph";
alignas(4) static constexpr char Frequency[] = "freq";
alignas(4) static constexpr char Tvoc[] = "tvoc";
alignas(4) static constexpr char Ch2o[] = "ch2o";

alignas(4) static constexpr char Unknown[] = "unknown";

constexpr ::settings::StringView get(unsigned char type) {
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

alignas(4) static constexpr char Units[] PROGMEM = "Units";
alignas(4) static constexpr char Ratio[] PROGMEM = "Ratio";
alignas(4) static constexpr char Correction[] PROGMEM = "Correction";
alignas(4) static constexpr char ZeroThreshold[] PROGMEM = "ZeroThreshold";
alignas(4) static constexpr char MinDelta[] PROGMEM = "MinDelta";
alignas(4) static constexpr char MaxDelta[] PROGMEM = "MaxDelta";

alignas(4) static constexpr char Mains[] PROGMEM = "Mains";
alignas(4) static constexpr char Reference[] PROGMEM = "Reference";

alignas(4) static constexpr char Total[] PROGMEM = "Total";

} // namespace
} // namespace suffix

namespace keys {
namespace {

alignas(4) static constexpr char ReadInterval[] PROGMEM = "snsRead";
alignas(4) static constexpr char InitInterval[] PROGMEM = "snsInit";
alignas(4) static constexpr char ReportEvery[] PROGMEM = "snsReport";
alignas(4) static constexpr char SaveEvery[] PROGMEM = "snsSave";
alignas(4) static constexpr char RealTimeValues[] PROGMEM = "snsRealTime";

SettingsKey get(::settings::StringView prefix, ::settings::StringView suffix, size_t index) {
    String key;
    key.reserve(prefix.length() + suffix.length() + 4);
    key.concat(prefix.c_str(), prefix.length());
    key.concat(suffix.c_str(), suffix.length());

    return SettingsKey(std::move(key), index);
}

SettingsKey get(const Magnitude& magnitude, ::settings::StringView suffix) {
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

int reportEvery() {
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
} // namespace sensor

namespace settings {
namespace internal {
namespace {

alignas(4) static constexpr char Farenheit[] PROGMEM = "°F";
alignas(4) static constexpr char Celcius[] PROGMEM = "°C";
alignas(4) static constexpr char Kelvin[] PROGMEM = "K";
alignas(4) static constexpr char Percentage[] PROGMEM = "%";
alignas(4) static constexpr char Hectopascal[] PROGMEM = "hPa";
alignas(4) static constexpr char Ampere[] PROGMEM = "A";
alignas(4) static constexpr char Volt[] PROGMEM = "V";
alignas(4) static constexpr char Watt[] PROGMEM = "W";
alignas(4) static constexpr char Kilowatt[] PROGMEM = "kW";
alignas(4) static constexpr char Voltampere[] PROGMEM = "VA";
alignas(4) static constexpr char Kilovoltampere[] PROGMEM = "kVA";
alignas(4) static constexpr char VoltampereReactive[] PROGMEM = "VAR";
alignas(4) static constexpr char KilovoltampereReactive[] PROGMEM = "kVAR";
alignas(4) static constexpr char Joule[] PROGMEM = "J";
alignas(4) static constexpr char KilowattHour[] PROGMEM = "kWh";
alignas(4) static constexpr char MicrogrammPerCubicMeter[] PROGMEM = "µg/m³";
alignas(4) static constexpr char PartsPerMillion[] PROGMEM = "ppm";
alignas(4) static constexpr char Lux[] PROGMEM = "lux";
alignas(4) static constexpr char UltravioletIndex[] PROGMEM = "UVindex";
alignas(4) static constexpr char Ohm[] PROGMEM = "ohm";
alignas(4) static constexpr char MilligrammPerCubicMeter[] PROGMEM = "mg/m³";
alignas(4) static constexpr char CountsPerMinute[] PROGMEM = "cpm";
alignas(4) static constexpr char MicrosievertPerHour[] PROGMEM = "µSv/h";
alignas(4) static constexpr char Meter[] PROGMEM = "m";
alignas(4) static constexpr char Hertz[] PROGMEM = "Hz";
alignas(4) static constexpr char Ph[] PROGMEM = "pH";
alignas(4) static constexpr char None[] PROGMEM = "none";

static constexpr ::settings::options::Enumeration<sensor::Unit> SensorUnitOptions[] PROGMEM {
    {sensor::Unit::Farenheit, Farenheit},
    {sensor::Unit::Celcius, Celcius},
    {sensor::Unit::Kelvin, Kelvin},
    {sensor::Unit::Percentage, Percentage},
    {sensor::Unit::Hectopascal, Hectopascal},
    {sensor::Unit::Ampere, Ampere},
    {sensor::Unit::Volt, Volt},
    {sensor::Unit::Watt, Watt},
    {sensor::Unit::Kilowatt, Kilowatt},
    {sensor::Unit::Voltampere, Voltampere},
    {sensor::Unit::Kilovoltampere, Kilovoltampere},
    {sensor::Unit::VoltampereReactive, VoltampereReactive},
    {sensor::Unit::KilovoltampereReactive, KilovoltampereReactive},
    {sensor::Unit::Joule, Joule},
    {sensor::Unit::WattSecond, Joule},
    {sensor::Unit::KilowattHour, KilowattHour},
    {sensor::Unit::MicrogrammPerCubicMeter, MicrogrammPerCubicMeter},
    {sensor::Unit::PartsPerMillion, PartsPerMillion},
    {sensor::Unit::Lux, Lux},
    {sensor::Unit::UltravioletIndex, UltravioletIndex},
    {sensor::Unit::Ohm, Ohm},
    {sensor::Unit::MilligrammPerCubicMeter, MilligrammPerCubicMeter},
    {sensor::Unit::CountsPerMinute, CountsPerMinute},
    {sensor::Unit::MicrosievertPerHour, MicrosievertPerHour},
    {sensor::Unit::Meter, Meter},
    {sensor::Unit::Hertz, Hertz},
    {sensor::Unit::Ph, Ph},
    {sensor::Unit::None, None},
};

} // namespace

template <>
sensor::Unit convert(const String& value) {
    return convert(SensorUnitOptions, value, sensor::Unit::None);
}

String serialize(sensor::Unit unit) {
    return serialize(SensorUnitOptions, unit);
}

} // namespace internal
} // namespace settings

// -----------------------------------------------------------------------------
// Energy persistence
// -----------------------------------------------------------------------------

namespace {

struct SensorEnergyTracker {
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

SensorEnergyTracker _sensor_energy_tracker;

bool _sensorIsEmon(BaseSensorPtr sensor) {
    return (sensor->kind() == BaseEmonSensor::Kind)
        || (sensor->kind() == BaseAnalogEmonSensor::Kind);
}

bool _sensorIsAnalogEmon(BaseSensorPtr sensor) {
    return sensor->kind() == BaseAnalogEmonSensor::Kind;
}

bool _sensorIsAnalog(BaseSensorPtr sensor) {
    return sensor->kind() == BaseAnalogSensor::Kind;
}

sensor::Energy _sensorRtcmemLoadEnergy(unsigned char index) {
    return sensor::Energy {
        sensor::KWh { Rtcmem->energy[index].kwh },
        sensor::Ws { Rtcmem->energy[index].ws }
    };
}

void _sensorRtcmemSaveEnergy(unsigned char index, const sensor::Energy& source) {
    Rtcmem->energy[index].kwh = source.kwh.value;
    Rtcmem->energy[index].ws = source.ws.value;
}

struct EnergyParseResult {
    EnergyParseResult() = default;
    EnergyParseResult& operator=(sensor::Energy value) {
        _value = value;
        _result = true;
        return *this;
    }

    explicit operator bool() const {
        return _result;
    }

    sensor::Energy value() const {
        return _value;
    }

private:
    bool _result { false };
    sensor::Energy _value;
};

EnergyParseResult _sensorParseEnergy(const String& value) {
    EnergyParseResult out;
    if (!value.length()) {
        return out;
    }

    const char* p { value.c_str() };

    char* endp { nullptr };
    auto kwh = strtoul(p, &endp, 10);
    if (!endp || (endp == p)) {
        return out;
    }

    sensor::Energy energy{};
    energy.kwh = kwh;

    const char* plus { strchr(p, '+') };
    if (plus) {
        p = plus + 1;
        if (*p == '\0') {
            return out;
        }

        auto ws = strtoul(p, &endp, 10);
        if (!endp || (endp == p)) {
            return out;
        }

        energy.ws = ws;
    }

    out = energy;
    return out;
}

sensor::Energy _sensorSettingsLoadEnergy(unsigned char index) {
    using namespace ::sensor::settings;
    return _sensorParseEnergy(getSetting(
        keys::get(prefix::get(MAGNITUDE_ENERGY), suffix::Total, index))).value();
}

void _sensorApiResetEnergy(const Magnitude& magnitude, const String& payload) {
    if (!payload.length()) {
        return;
    }

    auto energy = _sensorParseEnergy(payload);
    if (!energy) {
        return;
    }

    auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor.get());
    sensor->resetEnergy(magnitude.slot, energy.value());
}

void _sensorApiResetEnergy(const Magnitude& magnitude, const char* payload) {
    if (!payload) {
        return;
    }

    _sensorApiResetEnergy(magnitude, payload);
}

sensor::Energy _sensorEnergyTotal(unsigned char index) {
    sensor::Energy result;

    if (rtcmemStatus() && (index < (sizeof(Rtcmem->energy) / sizeof(*Rtcmem->energy)))) {
        result = _sensorRtcmemLoadEnergy(index);
    } else {
        result = _sensorSettingsLoadEnergy(index);
    }

    return result;
}

void _sensorResetEnergyTotal(unsigned char index) {
    delSetting({F("eneTotal"), index});
    delSetting({F("eneTime"), index});
    if (index < (sizeof(Rtcmem->energy) / sizeof(*Rtcmem->energy))) {
        Rtcmem->energy[index].kwh = 0;
        Rtcmem->energy[index].ws = 0;
    }
}

struct SensorPersistEnergyTotal {
    SensorPersistEnergyTotal(size_t index, sensor::Energy energy) :
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
    sensor::Energy _energy;
};

void _magnitudeSaveEnergyTotal(const Magnitude& magnitude, bool persistent) {
    auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor.get());

    const auto energy = sensor->totalEnergy(magnitude.slot);

    // Always save to RTCMEM
    if (magnitude.index_global < (sizeof(Rtcmem->energy) / sizeof(*Rtcmem->energy))) {
        _sensorRtcmemSaveEnergy(magnitude.index_global, energy);
    }

    // Save to EEPROM every '_sensor_save_every' readings
    if (persistent && _sensor_energy_tracker) {
        _sensor_energy_tracker.tick(magnitude.index_global,
            SensorPersistEnergyTotal{magnitude.index_global, energy});
    }
}

void _sensorTrackEnergyTotal(const Magnitude& magnitude) {
    auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor.get());
    sensor->resetEnergy(magnitude.slot, _sensorEnergyTotal(magnitude.index_global));
    _sensor_energy_tracker.add(magnitude);
}

} // namespace

sensor::Energy sensorEnergyTotal() {
    return _sensorEnergyTotal(0);
}

// -----------------------------------------------------------------------------
// Data processing
// -----------------------------------------------------------------------------

namespace {

bool _sensors_ready { false };
std::vector<BaseSensorPtr> _sensors;

bool _sensor_real_time { sensor::build::realTimeValues() };
size_t _sensor_report_every { sensor::build::reportEvery() };

espurna::duration::Seconds _sensor_read_interval { sensor::build::readInterval() };
espurna::duration::Seconds _sensor_init_interval { sensor::build::initInterval() };

std::vector<Magnitude> _magnitudes;

using MagnitudeReadHandlers = std::forward_list<MagnitudeReadHandler>;
MagnitudeReadHandlers _magnitude_read_handlers;
MagnitudeReadHandlers _magnitude_report_handlers;

BaseFilterPtr _magnitudeCreateFilter(unsigned char type) {
    BaseFilterPtr filter;

    switch (type) {
    case MAGNITUDE_IAQ:
    case MAGNITUDE_IAQ_STATIC:
    case MAGNITUDE_ENERGY:
        filter = std::make_unique<LastFilter>();
        break;
    case MAGNITUDE_EVENT:
    case MAGNITUDE_DIGITAL:
        filter = std::make_unique<MaxFilter>();
        break;
    case MAGNITUDE_COUNT:
    case MAGNITUDE_ENERGY_DELTA:
        filter = std::make_unique<SumFilter>();
        break;
    case MAGNITUDE_GEIGER_CPM:
    case MAGNITUDE_GEIGER_SIEVERT:
        filter = std::make_unique<MovingAverageFilter>();
        break;
    }

    if (!filter) {
        filter = std::make_unique<MedianFilter>();
    }

    return filter;
}

Magnitude::Magnitude(BaseSensorPtr sensor, BaseFilterPtr filter, unsigned char slot, unsigned char type) :
    sensor(std::move(sensor)),
    filter(std::move(filter)),
    slot(slot),
    type(type),
    index_global(_counts[type])
{
    ++_counts[type];
}

// Hardcoded decimals for each magnitude

unsigned char _sensorUnitDecimals(sensor::Unit unit) {
    switch (unit) {
        case sensor::Unit::Celcius:
        case sensor::Unit::Farenheit:
            return 1;
        case sensor::Unit::Percentage:
            return 0;
        case sensor::Unit::Hectopascal:
            return 2;
        case sensor::Unit::Ampere:
            return 3;
        case sensor::Unit::Volt:
            return 0;
        case sensor::Unit::Watt:
        case sensor::Unit::Voltampere:
        case sensor::Unit::VoltampereReactive:
            return 0;
        case sensor::Unit::Kilowatt:
        case sensor::Unit::Kilovoltampere:
        case sensor::Unit::KilovoltampereReactive:
            return 3;
        case sensor::Unit::KilowattHour:
            return 3;
        case sensor::Unit::WattSecond:
            return 0;
        case sensor::Unit::CountsPerMinute:
        case sensor::Unit::MicrosievertPerHour:
            return 4;
        case sensor::Unit::Meter:
            return 3;
        case sensor::Unit::Hertz:
            return 1;
        case sensor::Unit::UltravioletIndex:
            return 3;
        case sensor::Unit::Ph:
            return 3;
        case sensor::Unit::None:
        default:
            return 0;
    }
}

String _magnitudeTopic(unsigned char type) {

    const __FlashStringHelper* result = nullptr;

    switch (type) {
        case MAGNITUDE_TEMPERATURE:
            result = F("temperature");
            break;
        case MAGNITUDE_HUMIDITY:
            result = F("humidity");
            break;
        case MAGNITUDE_PRESSURE:
            result = F("pressure");
            break;
        case MAGNITUDE_CURRENT:
            result = F("current");
            break;
        case MAGNITUDE_VOLTAGE:
            result = F("voltage");
            break;
        case MAGNITUDE_POWER_ACTIVE:
            result = F("power");
            break;
        case MAGNITUDE_POWER_APPARENT:
            result = F("apparent");
            break;
        case MAGNITUDE_POWER_REACTIVE:
            result = F("reactive");
            break;
        case MAGNITUDE_POWER_FACTOR:
            result = F("factor");
            break;
        case MAGNITUDE_ENERGY:
            result = F("energy");
            break;
        case MAGNITUDE_ENERGY_DELTA:
            result = F("energy_delta");
            break;
        case MAGNITUDE_ANALOG:
            result = F("analog");
            break;
        case MAGNITUDE_DIGITAL:
            result = F("digital");
            break;
        case MAGNITUDE_EVENT:
            result = F("event");
            break;
        case MAGNITUDE_PM1DOT0:
            result = F("pm1dot0");
            break;
        case MAGNITUDE_PM2DOT5:
            result = F("pm2dot5");
            break;
        case MAGNITUDE_PM10:
            result = F("pm10");
            break;
        case MAGNITUDE_CO2:
            result = F("co2");
            break;
        case MAGNITUDE_VOC:
            result = F("voc");
            break;
        case MAGNITUDE_IAQ:
            result = F("iaq");
            break;
        case MAGNITUDE_IAQ_ACCURACY:
            result = F("iaq_accuracy");
            break;
        case MAGNITUDE_IAQ_STATIC:
            result = F("iaq_static");
            break;
        case MAGNITUDE_LUX:
            result = F("lux");
            break;
        case MAGNITUDE_UVA:
            result = F("uva");
            break;
        case MAGNITUDE_UVB:
            result = F("uvb");
            break;
        case MAGNITUDE_UVI:
            result = F("uvi");
            break;
        case MAGNITUDE_DISTANCE:
            result = F("distance");
            break;
        case MAGNITUDE_HCHO:
            result = F("hcho");
            break;
        case MAGNITUDE_GEIGER_CPM:
            result = F("ldr_cpm"); // local dose rate [Counts per minute]
            break;
        case MAGNITUDE_GEIGER_SIEVERT:
            result = F("ldr_uSvh"); // local dose rate [µSievert per hour]
            break;
        case MAGNITUDE_COUNT:
            result = F("count");
            break;
        case MAGNITUDE_NO2:
            result = F("no2");
            break;
        case MAGNITUDE_CO:
            result = F("co");
            break;
        case MAGNITUDE_RESISTANCE:
            result = F("resistance");
            break;
        case MAGNITUDE_PH:
            result = F("ph");
            break;
        case MAGNITUDE_FREQUENCY:
            result = F("frequency");
            break;
        case MAGNITUDE_TVOC:
            result = F("tvoc");
            break;
        case MAGNITUDE_CH2O:
            result = F("ch2o");
            break;
        case MAGNITUDE_NONE:
        default:
            result = F("unknown");
            break;
    }

    return String(result);

}

String _magnitudeTopicIndex(const Magnitude& magnitude) {
    auto topic = _magnitudeTopic(magnitude.type);
    if (sensor::build::useIndex() || (Magnitude::counts(magnitude.type) > 1)) {
        topic += '/' + String(magnitude.index_global, 10);
    }

    return topic;
}

String _magnitudeUnits(sensor::Unit unit) {
    return ::settings::internal::serialize(unit);
}

} // namespace

String magnitudeUnits(unsigned char index) {
    if (index < _magnitudes.size()) {
        return _magnitudeUnits(_magnitudes[index].units);
    }

    return String();
}

namespace {

// Choose unit based on type of magnitude we use

struct MagnitudeUnitsRange {
    MagnitudeUnitsRange() = default;

    template <size_t Size>
    explicit MagnitudeUnitsRange(const sensor::Unit (&units)[Size]) :
        _begin(std::begin(units)),
        _end(std::end(units))
    {}

    template <size_t Size>
    MagnitudeUnitsRange& operator=(const sensor::Unit (&units)[Size]) {
        _begin = std::begin(units);
        _end = std::end(units);
        return *this;
    }

    const sensor::Unit* begin() const {
        return _begin;
    }

    const sensor::Unit* end() const {
        return _end;
    }

private:
    const sensor::Unit* _begin { nullptr };
    const sensor::Unit* _end { nullptr };
};

#define MAGNITUDE_UNITS_RANGE(...)\
    static const sensor::Unit units[] PROGMEM {\
        __VA_ARGS__\
    };\
\
    out = units

MagnitudeUnitsRange _magnitudeUnitsRange(unsigned char type) {
    MagnitudeUnitsRange out;

    switch (type) {

    case MAGNITUDE_TEMPERATURE: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::Celcius,
            sensor::Unit::Farenheit,
            sensor::Unit::Kelvin
        );
        break;
    }

    case MAGNITUDE_HUMIDITY:
    case MAGNITUDE_POWER_FACTOR: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::Percentage
        );
        break;
    }

    case MAGNITUDE_PRESSURE: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::Hectopascal
        );
        break;
    }

    case MAGNITUDE_CURRENT: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::Ampere
        );
        break;
    }

    case MAGNITUDE_VOLTAGE: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::Volt
        );
        break;
    }

    case MAGNITUDE_POWER_ACTIVE: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::Watt,
            sensor::Unit::Kilowatt
        );
        break;
    }

    case MAGNITUDE_POWER_APPARENT: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::Voltampere,
            sensor::Unit::Kilovoltampere
        );
        break;
    }

    case MAGNITUDE_POWER_REACTIVE: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::VoltampereReactive,
            sensor::Unit::KilovoltampereReactive
        );
        break;
    }

    case MAGNITUDE_ENERGY_DELTA: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::Joule
        );
        break;
    }

    case MAGNITUDE_ENERGY: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::Joule,
            sensor::Unit::KilowattHour
        );
        break;
    }

    case MAGNITUDE_PM1DOT0:
    case MAGNITUDE_PM2DOT5:
    case MAGNITUDE_PM10:
    case MAGNITUDE_TVOC:
    case MAGNITUDE_CH2O: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::MicrogrammPerCubicMeter,
            sensor::Unit::MilligrammPerCubicMeter
        );
        break;
    }

    case MAGNITUDE_CO:
    case MAGNITUDE_CO2:
    case MAGNITUDE_NO2:
    case MAGNITUDE_VOC: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::PartsPerMillion
        );
        break;
    }

    case MAGNITUDE_LUX: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::Lux
        );
        break;
    }

    case MAGNITUDE_RESISTANCE: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::Ohm
        );
        break;
    }

    case MAGNITUDE_HCHO: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::MilligrammPerCubicMeter
        );
        break;
    }

    case MAGNITUDE_GEIGER_CPM: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::CountsPerMinute
        );
        break;
    }

    case MAGNITUDE_GEIGER_SIEVERT: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::MicrosievertPerHour
        );
        break;
    }

    case MAGNITUDE_DISTANCE: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::Meter
        );
        break;
    }

    case MAGNITUDE_FREQUENCY: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::Hertz
        );
        break;
    }

    case MAGNITUDE_PH: {
        MAGNITUDE_UNITS_RANGE(
            sensor::Unit::Ph
        );
        break;
    }

    }

    return out;
}

bool _magnitudeUnitSupported(const Magnitude& magnitude, sensor::Unit unit) {
    const auto range = _magnitudeUnitsRange(magnitude.type);
    return std::any_of(range.begin(), range.end(), [&](sensor::Unit supported) {
        return (unit == supported);
    });
}

sensor::Unit _magnitudeUnitFilter(const Magnitude& magnitude, sensor::Unit unit) {
    return _magnitudeUnitSupported(magnitude, unit) ? unit : magnitude.units;
}

double _magnitudeProcess(const Magnitude& magnitude, double value) {

    // Process input (sensor) units and convert to the ones that magnitude specifies as output
    const auto source = magnitude.sensor->units(magnitude.slot);

    switch (source) {
    case sensor::Unit::Farenheit:
    case sensor::Unit::Kelvin:
    case sensor::Unit::Celcius:
        value = sensor::convert::temperature::convert(value, source, magnitude.units);
        break;
    case sensor::Unit::Percentage:
        value = std::clamp(value, 0.0, 100.0);
        break;
    case sensor::Unit::Watt:
    case sensor::Unit::Voltampere:
    case sensor::Unit::VoltampereReactive:
        if ((magnitude.units == sensor::Unit::Kilowatt)
            || (magnitude.units == sensor::Unit::Kilovoltampere)
            || (magnitude.units == sensor::Unit::KilovoltampereReactive)) {
            value = value / 1.0e+3;
        }
        break;
    case sensor::Unit::KilowattHour:
        // TODO: we may end up with inf at some point?
        if (magnitude.units == sensor::Unit::Joule) {
            value = value * 3.6e+6;
        }
        break;
    default:
        break;
    }

    value = value + magnitude.correction;

    return roundTo(value, magnitude.decimals);

}

String _magnitudeDescription(const Magnitude& magnitude) {
    return magnitude.sensor->description(magnitude.slot);
}

// -----------------------------------------------------------------------------

// do `callback(type)` for each present magnitude
template <typename T>
void _magnitudeForEachCounted(T&& callback) {
    for (unsigned char type = MAGNITUDE_NONE + 1; type < MAGNITUDE_MAX; ++type) {
        if (Magnitude::counts(type)) {
            callback(type);
        }
    }
}

// check if `callback(type)` returns `true` at least once
template <typename T>
bool _magnitudeForEachCountedCheck(T&& callback) {
    for (unsigned char type = MAGNITUDE_NONE + 1; type < MAGNITUDE_MAX; ++type) {
        if (Magnitude::counts(type) && callback(type)) {
            return true;
        }
    }

    return false;
}

// do `callback(type)` for each error type
template <typename T>
void _sensorForEachError(T&& callback) {
    for (unsigned char error = SENSOR_ERROR_OK; error < SENSOR_ERROR_MAX; ++error) {
        callback(error);
    }
}

constexpr double _magnitudeCorrection(unsigned char type) {
    return (
        (MAGNITUDE_TEMPERATURE == type) ? (SENSOR_TEMPERATURE_CORRECTION) :
        (MAGNITUDE_HUMIDITY == type) ? (SENSOR_HUMIDITY_CORRECTION) :
        (MAGNITUDE_LUX == type) ? (SENSOR_LUX_CORRECTION) :
        (MAGNITUDE_PRESSURE == type) ? (SENSOR_PRESSURE_CORRECTION) :
        0.0
    );
}

constexpr bool _magnitudeCorrectionSupported(unsigned char type) {
  return (MAGNITUDE_TEMPERATURE == type)
      || (MAGNITUDE_HUMIDITY == type)
      || (MAGNITUDE_PRESSURE == type)
      || (MAGNITUDE_LUX == type);
}

bool _sensorCheckKeyPrefix(::settings::StringView key) {
    if (key.length() < 3) {
        return false;
    }

    using settings::query::samePrefix;
    using settings::StringView;

    if (samePrefix(key, sensor::settings::prefix::Sensor)) {
        return true;
    }

    if (samePrefix(key, sensor::settings::prefix::Power)) {
        return true;
    }

    return _magnitudeForEachCountedCheck([&](unsigned char type) {
        return samePrefix(key, ::sensor::settings::prefix::get(type));
    });
}

constexpr bool _magnitudeRatioSupported(unsigned char type) {
    return (type == MAGNITUDE_CURRENT)
        || (type == MAGNITUDE_VOLTAGE)
        || (type == MAGNITUDE_POWER_ACTIVE)
        || (type == MAGNITUDE_ENERGY);
}

String _sensorQueryHandler(::settings::StringView key) {
    String out;

    using namespace ::sensor::settings;

    for (auto& magnitude : _magnitudes) {
        if (_magnitudeRatioSupported(magnitude.type)) {
            auto expected = keys::get(magnitude, suffix::Ratio);
            if (key == expected) {
                out = String(reinterpret_cast<BaseEmonSensor*>(magnitude.sensor.get())->defaultRatio(magnitude.slot));
                break;
            }
        }

        if (_magnitudeCorrectionSupported(magnitude.type)) {
            auto expected = keys::get(magnitude, suffix::Correction);
            if (key == expected) {
                out = String(magnitude.correction);
                break;
            }
        }

        auto expected = keys::get(magnitude, suffix::Units);
        if (key == expected) {
            out = ::settings::internal::serialize(magnitude.units);
            break;
        }
    }

    return out;
}

} // namespace

// -----------------------------------------------------------------------------
// Sensor calibration & emon ratios
// -----------------------------------------------------------------------------

namespace {

void _sensorAnalogInit(BaseAnalogSensor* sensor) {
    sensor->setR0(getSetting(F("snsR0"), sensor->getR0()));
    sensor->setRS(getSetting(F("snsRS"), sensor->getRS()));
    sensor->setRL(getSetting(F("snsRL"), sensor->getRL()));
}

void _sensorApiAnalogCalibrate() {
    for (auto ptr : _sensors) {
        if (_sensorIsAnalog(ptr)) {
            DEBUG_MSG_P(PSTR("[ANALOG] Calibrating %s\n"), ptr->description().c_str());

            auto* sensor = static_cast<BaseAnalogSensor*>(ptr.get());
            sensor->calibrate();
            setSetting("snsR0", sensor->getR0());
            break;
        }
    }
}

void _sensorApiEmonResetRatios() {
    static constexpr unsigned char types[] {
        MAGNITUDE_CURRENT,
        MAGNITUDE_VOLTAGE,
        MAGNITUDE_POWER_ACTIVE,
        MAGNITUDE_ENERGY
    };

    using namespace ::sensor::settings;

    for (const auto& type : types) {
        for (size_t index = 0; index < Magnitude::counts(type); ++index) {
            delSetting(keys::get(prefix::get(type), ::sensor::settings::suffix::Ratio, index));
        }
    }

    for (auto ptr : _sensors) {
        if (_sensorIsEmon(ptr)) {
            DEBUG_MSG_P(PSTR("[EMON] Resetting %s\n"), ptr->description().c_str());
            static_cast<BaseEmonSensor*>(ptr.get())->resetRatios();
        }
    }
}

double _sensorApiEmonExpectedValue(const Magnitude& magnitude, double expected) {
    if (!_sensorIsEmon(magnitude.sensor)) {
        return BaseEmonSensor::DefaultRatio;
    }

    auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor.get());
    return sensor->ratioFromValue(magnitude.slot, sensor->value(magnitude.slot), expected);
}

} // namespace

// -----------------------------------------------------------------------------
// WebUI Websockets API
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

namespace {

bool _sensorWebSocketOnKeyCheck(const char* key, JsonVariant&) {
    return _sensorCheckKeyPrefix(key);
}

String _sensorError(unsigned char error) {
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
    default:
    case SENSOR_ERROR_OTHER:
        result = PSTR("Other / Unknown Error");
        break;
    }

    return result;

}

String _magnitudeName(unsigned char type) {

    const __FlashStringHelper* result = nullptr;

    switch (type) {
        case MAGNITUDE_TEMPERATURE:
            result = F("Temperature");
            break;
        case MAGNITUDE_HUMIDITY:
            result = F("Humidity");
            break;
        case MAGNITUDE_PRESSURE:
            result = F("Pressure");
            break;
        case MAGNITUDE_CURRENT:
            result = F("Current");
            break;
        case MAGNITUDE_VOLTAGE:
            result = F("Voltage");
            break;
        case MAGNITUDE_POWER_ACTIVE:
            result = F("Active Power");
            break;
        case MAGNITUDE_POWER_APPARENT:
            result = F("Apparent Power");
            break;
        case MAGNITUDE_POWER_REACTIVE:
            result = F("Reactive Power");
            break;
        case MAGNITUDE_POWER_FACTOR:
            result = F("Power Factor");
            break;
        case MAGNITUDE_ENERGY:
            result = F("Energy");
            break;
        case MAGNITUDE_ENERGY_DELTA:
            result = F("Energy (delta)");
            break;
        case MAGNITUDE_ANALOG:
            result = F("Analog");
            break;
        case MAGNITUDE_DIGITAL:
            result = F("Digital");
            break;
        case MAGNITUDE_EVENT:
            result = F("Event");
            break;
        case MAGNITUDE_PM1DOT0:
            result = F("PM1.0");
            break;
        case MAGNITUDE_PM2DOT5:
            result = F("PM2.5");
            break;
        case MAGNITUDE_PM10:
            result = F("PM10");
            break;
        case MAGNITUDE_CO2:
            result = F("CO2");
            break;
        case MAGNITUDE_VOC:
            result = F("VOC");
            break;
        case MAGNITUDE_IAQ_STATIC:
            result = F("IAQ (Static)");
            break;
        case MAGNITUDE_IAQ:
            result = F("IAQ");
            break;
        case MAGNITUDE_IAQ_ACCURACY:
            result = F("IAQ Accuracy");
            break;
        case MAGNITUDE_LUX:
            result = F("Lux");
            break;
        case MAGNITUDE_UVA:
            result = F("UVA");
            break;
        case MAGNITUDE_UVB:
            result = F("UVB");
            break;
        case MAGNITUDE_UVI:
            result = F("UVI");
            break;
        case MAGNITUDE_DISTANCE:
            result = F("Distance");
            break;
        case MAGNITUDE_HCHO:
            result = F("HCHO");
            break;
        case MAGNITUDE_GEIGER_CPM:
        case MAGNITUDE_GEIGER_SIEVERT:
            result = F("Local Dose Rate");
            break;
        case MAGNITUDE_COUNT:
            result = F("Count");
            break;
        case MAGNITUDE_NO2:
            result = F("NO2");
            break;
        case MAGNITUDE_CO:
            result = F("CO");
            break;
        case MAGNITUDE_RESISTANCE:
            result = F("Resistance");
            break;
        case MAGNITUDE_PH:
            result = F("pH");
            break;
        case MAGNITUDE_FREQUENCY:
            result = F("Frequency");
            break;
        case MAGNITUDE_TVOC:
            result = F("TVOC");
            break;
        case MAGNITUDE_CH2O:
            result = F("CH2O");
            break;
        case MAGNITUDE_NONE:
        default:
            break;
    }

    return String(result);
}

// prepare available types and magnitudes config
// make sure these are properly ordered, as UI does not delay processing

void _sensorWebSocketTypes(JsonObject& root) {
    ::web::ws::EnumerablePayload payload{root, STRING_VIEW("types")};
    payload(STRING_VIEW("values"), {MAGNITUDE_NONE + 1, MAGNITUDE_MAX},
        [](size_t type) {
            return Magnitude::counts(type) > 0;
        },
        {
            {STRING_VIEW("type"), [](JsonArray& out, size_t index) {
                out.add(index);
            }},
            {STRING_VIEW("prefix"), [](JsonArray& out, size_t index) {
                out.add(FPSTR(::sensor::settings::prefix::get(index).c_str()));
            }},
            {STRING_VIEW("name"), [](JsonArray& out, size_t index) {
                out.add(_magnitudeName(index));
            }}
        });
}

void _sensorWebSocketErrors(JsonObject& root) {
    ::web::ws::EnumerablePayload payload{root, STRING_VIEW("errors")};
    payload(STRING_VIEW("values"), SENSOR_ERROR_MAX, {
        {STRING_VIEW("type"), [](JsonArray& out, size_t index) {
            out.add(index);
        }},
        {STRING_VIEW("name"), [](JsonArray& out, size_t index) {
            out.add(_sensorError(index));
        }}
    });
}

void _sensorWebSocketUnits(JsonObject& root) {
    ::web::ws::EnumerablePayload payload{root, STRING_VIEW("units")};
    payload(STRING_VIEW("values"), _magnitudes.size(), {
        {STRING_VIEW("supported"), [](JsonArray& out, size_t index) {
            JsonArray& units = out.createNestedArray();
            const auto range = _magnitudeUnitsRange(_magnitudes[index].type);
            for (auto it = range.begin(); it != range.end(); ++it) {
                JsonArray& unit = units.createNestedArray();
                unit.add(static_cast<int>(*it)); // raw id
                unit.add(_magnitudeUnits(*it));  // as string
            }
        }}
    });
}

void _sensorWebSocketList(JsonObject& root) {
    ::web::ws::EnumerablePayload payload{root, STRING_VIEW("magnitudes-list")};
    payload(STRING_VIEW("values"), _magnitudes.size(), {
        {STRING_VIEW("index_global"), [](JsonArray& out, size_t index) {
            out.add(_magnitudes[index].index_global);
        }},
        {STRING_VIEW("type"), [](JsonArray& out, size_t index) {
            out.add(_magnitudes[index].type);
        }},
        {STRING_VIEW("description"), [](JsonArray& out, size_t index) {
            out.add(_magnitudeDescription(_magnitudes[index]));
        }},
        {STRING_VIEW("units"), [](JsonArray& out, size_t index) {
            out.add(static_cast<int>(_magnitudes[index].units));
        }}
    });
}

void _sensorWebSocketSettings(JsonObject& root) {
    // XXX: inject 'null' in the output. need this for optional fields, since the current
    // version of serializer only does this for char ptr and even makes NaN serialized as
    // NaN, instead of more commonly used null (but, expect this to be fixed after switching to v6+)
    static const char* const NullSymbol { nullptr };

    ::web::ws::EnumerablePayload payload{root, STRING_VIEW("magnitudes-settings")};
    payload(STRING_VIEW("values"), _magnitudes.size(), {
        {::sensor::settings::suffix::Correction, [](JsonArray& out, size_t index) {
            const auto& magnitude = _magnitudes[index];
            if (_magnitudeCorrectionSupported(magnitude.type)) {
                out.add(magnitude.correction);
            } else {
                out.add(NullSymbol);
            }
        }},
        {::sensor::settings::suffix::Ratio, [](JsonArray& out, size_t index) {
            const auto& magnitude = _magnitudes[index];
            if (_magnitudeRatioSupported(magnitude.type)) {
                out.add(static_cast<BaseEmonSensor*>(magnitude.sensor.get())->getRatio(magnitude.slot));
            } else {
                out.add(NullSymbol);
            }
        }},
        {::sensor::settings::suffix::ZeroThreshold, [](JsonArray& out, size_t index) {
            const auto threshold = _magnitudes[index].zero_threshold;
            if (!std::isnan(threshold)) {
                out.add(threshold);
            } else {
                out.add(NullSymbol);
            }
        }},
        {::sensor::settings::suffix::MinDelta, [](JsonArray& out, size_t index) {
            out.add(_magnitudes[index].min_delta);
        }},
        {::sensor::settings::suffix::MaxDelta, [](JsonArray& out, size_t index) {
            out.add(_magnitudes[index].max_delta);
        }}
    });

    root[FPSTR(::sensor::settings::keys::ReadInterval)] = _sensor_read_interval.count();
    root[FPSTR(::sensor::settings::keys::InitInterval)] = _sensor_init_interval.count();
    root[FPSTR(::sensor::settings::keys::ReportEvery)] = _sensor_report_every;
    root[FPSTR(::sensor::settings::keys::SaveEvery)] = _sensor_energy_tracker.every();
    root[FPSTR(::sensor::settings::keys::RealTimeValues)] = _sensor_real_time;
}

void _sensorWebSocketSendData(JsonObject& root) {
    if (_magnitudes.size()) {
        ::web::ws::EnumerablePayload payload{root, STRING_VIEW("magnitudes")};
        payload(STRING_VIEW("values"), _magnitudes.size(), {
            {STRING_VIEW("value"), [](JsonArray& out, size_t index) {
                char buffer[64];
                dtostrf(_magnitudeProcess(
                    _magnitudes[index], _magnitudes[index].last),
                    1, _magnitudes[index].decimals, buffer);
                out.add(buffer);
            }},
            {STRING_VIEW("error"), [](JsonArray& out, size_t index) {
                out.add(_magnitudes[index].sensor->error());
            }},
            {STRING_VIEW("info"), [](JsonArray& out, size_t index) {
#if NTP_SUPPORT
                if ((_magnitudes[index].type == MAGNITUDE_ENERGY) && (_sensor_energy_tracker)) {
                    out.add(String(F("Last saved: "))
                        + getSetting({F("eneTime"), _magnitudes[index].index_global},
                            F("(unknown)")));
                } else {
#endif
                    out.add("");
#if NTP_SUPPORT
                }
#endif
            }}
        });
    }
}

void _sensorWebSocketOnAction(uint32_t client_id, const char* action, JsonObject& data) {
    if (strcmp(action, "emon-expected") == 0) {
        auto id = data["id"].as<size_t>();
        if (id < _magnitudes.size()) {
            auto expected = data["expected"].as<float>();
            wsPost(client_id, [id, expected](JsonObject& root) {
                const auto& magnitude = _magnitudes[id];

                String key { F("result:") };
                key += ::sensor::settings::keys::get(
                    magnitude, ::sensor::settings::suffix::Ratio).value();

                root[key] = _sensorApiEmonExpectedValue(magnitude, expected);
            });
        }
    } else if (strcmp(action, "emon-reset-ratios") == 0) {
        _sensorApiEmonResetRatios();
    } else if (strcmp(action, "analog-calibrate") == 0) {
        _sensorApiAnalogCalibrate();
    }
}

void _sensorWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, "sns");
    for (auto sensor : _sensors) {
        if (_sensorIsEmon(sensor)) {
            wsPayloadModule(root, "emon");
        }

        switch (sensor->id()) {
#if HLW8012_SUPPORT
        case SENSOR_HLW8012_ID:
            wsPayloadModule(root, "hlw");
            break;
#endif
#if CSE7766_SUPPORT
        case SENSOR_CSE7766_ID:
            wsPayloadModule(root, "cse");
            break;
#endif
#if PZEM004T_SUPPORT || PZEM004TV30_SUPPORT
        case SENSOR_PZEM004T_ID:
        case SENSOR_PZEM004TV30_ID:
            wsPayloadModule(root, "pzem");
            break;
#endif
#if PULSEMETER_SUPPORT
        case SENSOR_PULSEMETER_ID:
            wsPayloadModule(root, "pm");
            break;
#endif
#if MICS2710_SUPPORT || MICS5525_SUPPORT
        case SENSOR_MICS2710_ID:
        case SENSOR_MICS5525_ID:
            wsPayloadModule(root, "mics");
            break;
#endif
        }
    }
}

// Entries related to things reported by the module.
// - types of magnitudes that are available and the string values associated with them
// - error types and stringified versions of them
// - units are the value types of the magnitude
// TODO: magnitude types have some common keys and some specific ones, only implemented for the type
// e.g. voltMains is specific to the MAGNITUDE_VOLTAGE but *only* in analog mode, or eneRatio specific to MAGNITUDE_ENERGY
// but, notice that the sensor will probably be used to 'get' certain properties, to generate certain keys list
// TODO: report common keys either here or in the data payload
// some preprocessor magic might need to happen though, as prefixes are retrieved via `_magnitudeSettingsPrefix(type)`
// (also there is c++17 where string_view and char arrays may be concatenated at compile time)

void _sensorWebSocketOnConnectedInitial(JsonObject& root) {
    if (!_magnitudes.size()) {
        return;
    }

    JsonObject& container = root.createNestedObject(F("magnitudes-init"));
    _sensorWebSocketTypes(container);
    _sensorWebSocketErrors(container);
    _sensorWebSocketUnits(container);
}

// Entries specific to the Magnitude; type, info, description

void _sensorWebSocketOnConnectedList(JsonObject& root) {
    if (!_magnitudes.size()) {
        return;
    }

    _sensorWebSocketList(root);
}

void _sensorWebSocketOnConnectedSettings(JsonObject& root) {
    if (!_magnitudes.size()) {
        return;
    }

    _sensorWebSocketSettings(root);
}

} // namespace

// Used by modules to generate magnitude_id<->module_id mapping for the WebUI
// Prefix controls the UI templates, supplied callback should retrieve module-specific value Id

void sensorWebSocketMagnitudes(JsonObject& root, const char* prefix, SensorWebSocketMagnitudesCallback callback) {
    ::web::ws::EnumerablePayload payload{root, STRING_VIEW("magnitudes-module")};

    auto& container = payload.root();
    container[F("prefix")] = prefix;

    payload(STRING_VIEW("values"), _magnitudes.size(), {
        {STRING_VIEW("type"), [](JsonArray& out, size_t index) {
            out.add(_magnitudes[index].type);
        }},
        {STRING_VIEW("index_global"), [](JsonArray& out, size_t index) {
            out.add(_magnitudes[index].index_global);
        }},
        {STRING_VIEW("index_module"), callback}
    });
}

#endif // WEB_SUPPORT

#if API_SUPPORT

namespace {

bool _sensorApiTryParseMagnitudeIndex(const char* p, unsigned char type, unsigned char& magnitude_index) {
    char* endp { nullptr };
    const unsigned long result { strtoul(p, &endp, 10) };
    if ((endp == p) || (*endp != '\0') || (result >= Magnitude::counts(type))) {
        DEBUG_MSG_P(PSTR("[SENSOR] Invalid magnitude ID (%s)\n"), p);
        return false;
    }

    magnitude_index = result;
    return true;
}

template <typename T>
bool _sensorApiTryHandle(ApiRequest& request, unsigned char type, T&& callback) {
    unsigned char index { 0u };
    if (request.wildcards()) {
        auto index_param = request.wildcard(0);
        if (!_sensorApiTryParseMagnitudeIndex(index_param.c_str(), type, index)) {
            return false;
        }
    }

    for (auto& magnitude : _magnitudes) {
        if ((type == magnitude.type) && (index == magnitude.index_global)) {
            callback(magnitude);
            return true;
        }
    }

    return false;
}

void _sensorApiSetup() {
    apiRegister(F("magnitudes"),
        [](ApiRequest&, JsonObject& root) {
            JsonArray& magnitudes = root.createNestedArray("magnitudes");
            for (auto& magnitude : _magnitudes) {
                JsonArray& data = magnitudes.createNestedArray();
                data.add(_magnitudeTopicIndex(magnitude));
                data.add(magnitude.last);
                data.add(magnitude.reported);
            }
            return true;
        },
        nullptr
    );

    _magnitudeForEachCounted([](unsigned char type) {
        auto pattern = _magnitudeTopic(type);
        if (sensor::build::useIndex() || (Magnitude::counts(type) > 1)) {
            pattern += "/+";
        }

        ApiBasicHandler get {
            [type](ApiRequest& request) {
                return _sensorApiTryHandle(request, type, [&](const Magnitude& magnitude) {
                    char buffer[64] { 0 };
                    dtostrf(
                        _sensor_real_time ? magnitude.last : magnitude.reported,
                        1, magnitude.decimals,
                        buffer
                    );
                    request.send(String(buffer));
                    return true;
                });
            }
        };

        ApiBasicHandler put { nullptr };
        if (type == MAGNITUDE_ENERGY) {
            put = [](ApiRequest& request) {
                return _sensorApiTryHandle(request, MAGNITUDE_ENERGY, [&](const Magnitude& magnitude) {
                    _sensorApiResetEnergy(magnitude, request.param(F("value")));
                });
            };
        }

        apiRegister(pattern, std::move(get), std::move(put));
    });
}

} // namespace

#endif // API_SUPPORT == 1

#if MQTT_SUPPORT

namespace {

void _sensorMqttCallback(unsigned int type, const char* topic, char* payload) {
    static const auto energy_topic = _magnitudeTopic(MAGNITUDE_ENERGY);
    switch (type) {
        case MQTT_MESSAGE_EVENT: {
            String t = mqttMagnitude(topic);
            if (!t.startsWith(energy_topic)) break;

            unsigned int index = t.substring(energy_topic.length() + 1).toInt();
            if (index >= Magnitude::counts(MAGNITUDE_ENERGY)) break;

            for (auto& magnitude : _magnitudes) {
                if (MAGNITUDE_ENERGY != magnitude.type) continue;
                if (index != magnitude.index_global) continue;
                _sensorApiResetEnergy(magnitude, static_cast<const char*>(payload));
                break;
            }
            break;
        }
        case MQTT_CONNECT_EVENT: {
            for (auto& magnitude : _magnitudes) {
                if (MAGNITUDE_ENERGY == magnitude.type) {
                    const String topic = energy_topic + "/+";
                    mqttSubscribe(topic.c_str());
                    break;
                }
            }
            break;
        }
        case MQTT_DISCONNECT_EVENT:
            break;
    }
}

} // namespace

#endif // MQTT_SUPPORT == 1

#if TERMINAL_SUPPORT

namespace {

void _sensorInitCommands() {
    terminalRegisterCommand(F("MAGNITUDES"), [](::terminal::CommandContext&& ctx) {
        if (!_magnitudes.size()) {
            terminalError(ctx, F("No magnitudes"));
            return;
        }

        char last[64];
        char reported[64];

        size_t index = 0;
        for (const auto& magnitude : _magnitudes) {
            dtostrf(magnitude.last, 1, magnitude.decimals, last);
            dtostrf(magnitude.reported, 1, magnitude.decimals, reported);
            ctx.output.printf_P(PSTR("%2zu * %s @ %s (read:%s reported:%s units:%s)\n"),
                index++, _magnitudeTopicIndex(magnitude).c_str(),
                _magnitudeDescription(magnitude).c_str(), last, reported,
                _magnitudeUnits(magnitude.units).c_str());
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("EXPECTED"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() == 3) {
            const auto id = settings::internal::convert<size_t>(ctx.argv[1]);
            if (id < _magnitudes.size()) {
                const auto result = _sensorApiEmonExpectedValue(_magnitudes[id],
                    settings::internal::convert<double>(ctx.argv[2]));

                using namespace ::sensor::settings;
                const auto key = keys::get(_magnitudes[id], suffix::Ratio);

                ctx.output.printf("%s => %s\n", key.c_str(), String(result).c_str());
                terminalOK(ctx);
                return;
            }

            terminalError(ctx, F("Invalid magnitude ID"));
            return;
        }

        terminalError(ctx, F("EXPECTED <ID> <VALUE>"));
    });

    terminalRegisterCommand(F("RESET.RATIOS"), [](::terminal::CommandContext&& ctx) {
        _sensorApiEmonResetRatios();
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("ENERGY"), [](::terminal::CommandContext&& ctx) {
        using IndexType = decltype(Magnitude::index_global);

        if (ctx.argv.size() == 3) {
            const auto selected = settings::internal::convert<IndexType>(ctx.argv[1]);
            const auto energy = _sensorParseEnergy(ctx.argv[2]);

            if (!energy) {
                terminalError(ctx, F("Invalid energy string"));
                return;
            }

            for (auto& magnitude : _magnitudes) {
                if ((MAGNITUDE_ENERGY == magnitude.type) && (selected == magnitude.index_global) && _sensorIsEmon(magnitude.sensor)) {
                    static_cast<BaseEmonSensor*>(magnitude.sensor.get())->resetEnergy(magnitude.slot, energy.value());
                    terminalOK(ctx);
                    return;
                }
            }

            terminalError(ctx, F("Magnitude not found"));
            return;
        }

        terminalError(ctx, F("ENERGY <ID> <VALUE>"));
    });
}

} // namespace

#endif // TERMINAL_SUPPORT == 1

namespace {

void _sensorTick() {
    for (auto sensor : _sensors) {
        sensor->tick();
    }
}

void _sensorPre() {
    for (auto sensor : _sensors) {
        sensor->pre();
        if (!sensor->status()) {
            DEBUG_MSG_P(PSTR("[SENSOR] Could not read from %s (%s)\n"),
                sensor->description().c_str(), _sensorError(sensor->error()).c_str());
        }
    }
}

void _sensorPost() {
    for (auto sensor : _sensors) {
        sensor->post();
    }
}

} // namespace

// -----------------------------------------------------------------------------
// Sensor initialization
// -----------------------------------------------------------------------------

namespace {

void _sensorLoad() {

    /*

    This is temporal, in the future sensors will be initialized based on
    soft configuration (data stored in EEPROM config) so you will be able
    to define and configure new sensors on the fly

    At the time being, only enabled sensors (those with *_SUPPORT to 1) are being
    loaded and initialized here. If you want to add new sensors of the same type
    just duplicate the block and change the arguments for the set* methods.

    For example, how to add a second DHT sensor:

    #if DHT_SUPPORT
    {
        DHTSensor * sensor = new DHTSensor();
        sensor->setGPIO(DHT2_PIN);
        sensor->setType(DHT2_TYPE);
        _sensors.push_back(sensor);
    }
    #endif

    DHT2_PIN and DHT2_TYPE should be globally accessible:
    - as `build_src_flags = -DDHT2_PIN=... -DDHT2_TYPE=...`
    - in custom.h, as `#define ...`

    */

    #if AM2320_SUPPORT
    {
        AM2320Sensor * sensor = new AM2320Sensor();
        sensor->setAddress(AM2320_ADDRESS);
        _sensors.push_back(sensor);
    }
    #endif

    #if ANALOG_SUPPORT
    {
        AnalogSensor * sensor = new AnalogSensor();
        sensor->setSamples(ANALOG_SAMPLES);
        sensor->setDelay(ANALOG_DELAY);
        //CICM For analog scaling
        sensor->setFactor(ANALOG_FACTOR);
        sensor->setOffset(ANALOG_OFFSET);
        _sensors.push_back(sensor);
    }
    #endif

    #if BH1750_SUPPORT
    {
        BH1750Sensor * sensor = new BH1750Sensor();
        sensor->setAddress(BH1750_ADDRESS);
        sensor->setMode(BH1750_MODE);
        _sensors.push_back(sensor);
    }
    #endif

    #if BMP180_SUPPORT
    {
        BMP180Sensor * sensor = new BMP180Sensor();
        sensor->setAddress(BMP180_ADDRESS);
        _sensors.push_back(sensor);
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
            BMX280Sensor * sensor = new BMX280Sensor();
            sensor->setAddress(address_map[n]);
            _sensors.push_back(sensor);
        }
    }
    #endif

    #if BME680_SUPPORT
    {
        BME680Sensor * sensor = new BME680Sensor();
        sensor->setAddress(BME680_I2C_ADDRESS);
        _sensors.push_back(sensor);
    }
    #endif

    #if CSE7766_SUPPORT
    {
        CSE7766Sensor * sensor = new CSE7766Sensor();
        sensor->setRX(CSE7766_RX_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if DALLAS_SUPPORT
    {
        DallasSensor * sensor = new DallasSensor();
        sensor->setGPIO(DALLAS_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if DHT_SUPPORT
    {
        DHTSensor * sensor = new DHTSensor();
        sensor->setGPIO(DHT_PIN);
        sensor->setType(DHT_TYPE);
        _sensors.push_back(sensor);
    }
    #endif

    #if DIGITAL_SUPPORT
    {
        auto getPin = [](unsigned char index) -> int {
            switch (index) {
                case 0: return DIGITAL1_PIN;
                case 1: return DIGITAL2_PIN;
                case 2: return DIGITAL3_PIN;
                case 3: return DIGITAL4_PIN;
                case 4: return DIGITAL5_PIN;
                case 5: return DIGITAL6_PIN;
                case 6: return DIGITAL7_PIN;
                case 7: return DIGITAL8_PIN;
                default: return GPIO_NONE;
            }
        };

        auto getDefaultState = [](unsigned char index) -> int {
            switch (index) {
                case 0: return DIGITAL1_DEFAULT_STATE;
                case 1: return DIGITAL2_DEFAULT_STATE;
                case 2: return DIGITAL3_DEFAULT_STATE;
                case 3: return DIGITAL4_DEFAULT_STATE;
                case 4: return DIGITAL5_DEFAULT_STATE;
                case 5: return DIGITAL6_DEFAULT_STATE;
                case 6: return DIGITAL7_DEFAULT_STATE;
                case 7: return DIGITAL8_DEFAULT_STATE;
                default: return 1;
            }
        };

        auto getMode = [](unsigned char index) -> int {
            switch (index) {
                case 0: return DIGITAL1_PIN_MODE;
                case 1: return DIGITAL2_PIN_MODE;
                case 2: return DIGITAL3_PIN_MODE;
                case 3: return DIGITAL4_PIN_MODE;
                case 4: return DIGITAL5_PIN_MODE;
                case 5: return DIGITAL6_PIN_MODE;
                case 6: return DIGITAL7_PIN_MODE;
                case 7: return DIGITAL8_PIN_MODE;
                default: return INPUT_PULLUP;
            }
        };

        auto pins = gpioPins();
        for (unsigned char index = 0; index < pins; ++index) {
            const auto pin = getPin(index);
            if (pin == GPIO_NONE) break;

            DigitalSensor * sensor = new DigitalSensor();
            sensor->setGPIO(pin);
            sensor->setMode(getMode(index));
            sensor->setDefault(getDefaultState(index));

            _sensors.push_back(sensor);
        }
    }
    #endif

    #if DUMMY_SENSOR_SUPPORT
        _sensors.push_back(new DummySensor());
    #endif

    #if ECH1560_SUPPORT
    {
        ECH1560Sensor * sensor = new ECH1560Sensor();
        sensor->setCLK(ECH1560_CLK_PIN);
        sensor->setMISO(ECH1560_MISO_PIN);
        sensor->setInverted(ECH1560_INVERTED);
        _sensors.push_back(sensor);
    }
    #endif

    #if EMON_ADC121_SUPPORT
    {
        EmonADC121Sensor * sensor = new EmonADC121Sensor();
        sensor->setAddress(EMON_ADC121_I2C_ADDRESS);
        sensor->setVoltage(EMON_MAINS_VOLTAGE);
        sensor->setReferenceVoltage(EMON_REFERENCE_VOLTAGE);
        _sensors.push_back(sensor);
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
                _sensors.push_back(sensor);
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
        _sensors.push_back(sensor);
    }
    #endif

    #if EVENTS_SUPPORT
    {
        auto getPin = [](unsigned char index) -> unsigned char {
            return (index == 0) ? EVENTS1_PIN :
                (index == 1) ? EVENTS2_PIN :
                (index == 2) ? EVENTS3_PIN :
                (index == 3) ? EVENTS4_PIN :
                (index == 4) ? EVENTS5_PIN :
                (index == 5) ? EVENTS6_PIN :
                (index == 6) ? EVENTS7_PIN :
                (index == 7) ? EVENTS8_PIN : GPIO_NONE;
        };

        auto getMode = [](unsigned char index) -> int {
            return (index == 0) ? EVENTS1_PIN_MODE :
                (index == 1) ? EVENTS2_PIN_MODE :
                (index == 2) ? EVENTS3_PIN_MODE :
                (index == 3) ? EVENTS4_PIN_MODE :
                (index == 4) ? EVENTS5_PIN_MODE :
                (index == 5) ? EVENTS6_PIN_MODE :
                (index == 6) ? EVENTS7_PIN_MODE :
                (index == 7) ? EVENTS8_PIN_MODE : INPUT;
        };

        auto getDebounce = [](unsigned char index) -> espurna::duration::Milliseconds {
            return espurna::duration::Milliseconds(
                (index == 0) ? EVENTS1_DEBOUNCE :
                (index == 1) ? EVENTS2_DEBOUNCE :
                (index == 2) ? EVENTS3_DEBOUNCE :
                (index == 3) ? EVENTS4_DEBOUNCE :
                (index == 4) ? EVENTS5_DEBOUNCE :
                (index == 5) ? EVENTS6_DEBOUNCE :
                (index == 6) ? EVENTS7_DEBOUNCE :
                (index == 7) ? EVENTS8_DEBOUNCE : 50);
        };

        auto getIsrMode = [](unsigned char index) -> int {
            return (index == 0) ? EVENTS1_INTERRUPT_MODE :
                (index == 1) ? EVENTS2_INTERRUPT_MODE :
                (index == 2) ? EVENTS3_INTERRUPT_MODE :
                (index == 3) ? EVENTS4_INTERRUPT_MODE :
                (index == 4) ? EVENTS5_INTERRUPT_MODE :
                (index == 5) ? EVENTS6_INTERRUPT_MODE :
                (index == 6) ? EVENTS7_INTERRUPT_MODE :
                (index == 7) ? EVENTS8_INTERRUPT_MODE : RISING;
        };

        auto pins = gpioPins();
        for (unsigned char index = 0; index < pins; ++index) {
            const auto pin = getPin(index);
            if (pin == GPIO_NONE) break;

            EventSensor * sensor = new EventSensor();
            sensor->setGPIO(pin);
            sensor->setPinMode(getMode(index));
            sensor->setDebounceTime(getDebounce(index));
            sensor->setInterruptMode(getIsrMode(index));
            _sensors.push_back(sensor);
        }
    }
    #endif

    #if GEIGER_SUPPORT
    {
        GeigerSensor * sensor = new GeigerSensor();
        sensor->setGPIO(GEIGER_PIN);
        sensor->setMode(GEIGER_PIN_MODE);
        sensor->setDebounceTime(
            GeigerSensor::TimeSource::duration { GEIGER_DEBOUNCE });
        sensor->setInterruptMode(GEIGER_INTERRUPT_MODE);
        sensor->setCPM2SievertFactor(GEIGER_CPM2SIEVERT);
        _sensors.push_back(sensor);
    }
    #endif

    #if GUVAS12SD_SUPPORT
    {
        GUVAS12SDSensor * sensor = new GUVAS12SDSensor();
        sensor->setGPIO(GUVAS12SD_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if SONAR_SUPPORT
    {
        SonarSensor * sensor = new SonarSensor();
        sensor->setEcho(SONAR_ECHO);
        sensor->setIterations(SONAR_ITERATIONS);
        sensor->setMaxDistance(SONAR_MAX_DISTANCE);
        sensor->setTrigger(SONAR_TRIGGER);
        _sensors.push_back(sensor);
    }
    #endif

    #if HLW8012_SUPPORT
    {
        HLW8012Sensor * sensor = new HLW8012Sensor();
        sensor->setSEL(getSetting(F("hlw8012SEL"), HLW8012_SEL_PIN));
        sensor->setCF(getSetting(F("hlw8012CF"), HLW8012_CF_PIN));
        sensor->setCF1(getSetting(F("hlw8012CF1"), HLW8012_CF1_PIN));
        sensor->setSELCurrent(HLW8012_SEL_CURRENT);
        _sensors.push_back(sensor);
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
        _sensors.push_back(sensor);
    }
    #endif

    #if LDR_SUPPORT
    {
        LDRSensor * sensor = new LDRSensor();
        sensor->setSamples(LDR_SAMPLES);
        sensor->setDelay(LDR_DELAY);
        sensor->setType(LDR_TYPE);
        sensor->setPhotocellPositionOnGround(LDR_ON_GROUND);
        sensor->setResistor(LDR_RESISTOR);
        sensor->setPhotocellParameters(LDR_MULTIPLICATION, LDR_POWER);
        _sensors.push_back(sensor);
    }
    #endif

    #if MHZ19_SUPPORT
    {
        MHZ19Sensor * sensor = new MHZ19Sensor();
        sensor->setRX(MHZ19_RX_PIN);
        sensor->setTX(MHZ19_TX_PIN);
        sensor->setCalibrateAuto(getSetting("mhz19CalibrateAuto", false));
        _sensors.push_back(sensor);
    }
    #endif

    #if MICS2710_SUPPORT
    {
        MICS2710Sensor * sensor = new MICS2710Sensor();
        sensor->setPreHeatGPIO(MICS2710_PRE_PIN);
        sensor->setR0(MICS2710_R0);
        sensor->setRL(MICS2710_RL);
        sensor->setRS(0);
        _sensors.push_back(sensor);
    }
    #endif

    #if MICS5525_SUPPORT
    {
        MICS5525Sensor * sensor = new MICS5525Sensor();
        sensor->setR0(MICS5525_R0);
        sensor->setRL(MICS5525_RL);
        sensor->setRS(0);
        _sensors.push_back(sensor);
    }
    #endif

    #if NTC_SUPPORT
    {
        NTCSensor * sensor = new NTCSensor();
        sensor->setSamples(NTC_SAMPLES);
        sensor->setDelay(NTC_DELAY);
        sensor->setUpstreamResistor(NTC_R_UP);
        sensor->setDownstreamResistor(NTC_R_DOWN);
        sensor->setBeta(NTC_BETA);
        sensor->setR0(NTC_R0);
        sensor->setT0(NTC_T0);
        _sensors.push_back(sensor);
    }
    #endif

    #if PM1006_SUPPORT
    {
        PM1006Sensor * sensor = new PM1006Sensor();
        sensor->setRX(PM1006_RX_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if PMSX003_SUPPORT
    {
        PMSX003Sensor * sensor = new PMSX003Sensor();
        #if PMS_USE_SOFT
            sensor->setRX(PMS_RX_PIN);
            sensor->setTX(PMS_TX_PIN);
        #else
            sensor->setSerial(& PMS_HW_PORT);
        #endif
        sensor->setType(PMS_TYPE);
        _sensors.push_back(sensor);
    }
    #endif

    #if PULSEMETER_SUPPORT
    {

        PulseMeterSensor * sensor = new PulseMeterSensor();
        sensor->setGPIO(PULSEMETER_PIN);
        sensor->setInterruptMode(PULSEMETER_INTERRUPT_ON);
        sensor->setDebounceTime(
            PulseMeterSensor::TimeSource::duration{PULSEMETER_DEBOUNCE});
        _sensors.push_back(sensor);
    }
    #endif

    #if PZEM004T_SUPPORT
    {
        PZEM004TSensor::PortPtr port;

        auto rx = getSetting("pzemRX", PZEM004TSensor::RxPin);
        auto tx = getSetting("pzemTX", PZEM004TSensor::TxPin);

        if (getSetting("pzemSoft", PZEM004TSensor::useSoftwareSerial())) {
            port = PZEM004TSensor::makeSoftwarePort(rx, tx);
        } else {
            port = PZEM004TSensor::makeHardwarePort(
                PZEM004TSensor::defaultHardwarePort(), rx, tx);
        }

        if (!port) {
            return;
        }

        bool initialized { false };

#if !defined(PZEM004T_ADDRESSES)
        for (size_t index = 0; index < PZEM004TSensor::DevicesMax; ++index) {
            auto address = getSetting({"pzemAddr", index}, PZEM004TSensor::defaultAddress(index));
            if (!address.isSet()) {
                break;
            }

            auto* ptr = PZEM004TSensor::make(port, address);
            if (ptr) {
                _sensors.push_back(ptr);
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
                auto* ptr = PZEM004TSensor::make(port, address);
                if (ptr) {
                    _sensors.push_back(ptr);
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
        SenseAirSensor * sensor = new SenseAirSensor();
        sensor->setRX(SENSEAIR_RX_PIN);
        sensor->setTX(SENSEAIR_TX_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if SDS011_SUPPORT
    {
        SDS011Sensor * sensor = new SDS011Sensor();
        sensor->setRX(SDS011_RX_PIN);
        sensor->setTX(SDS011_TX_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if SHT3X_I2C_SUPPORT
    {
        SHT3XI2CSensor * sensor = new SHT3XI2CSensor();
        sensor->setAddress(SHT3X_I2C_ADDRESS);
        _sensors.push_back(sensor);
    }
    #endif

    #if SI7021_SUPPORT
    {
        SI7021Sensor * sensor = new SI7021Sensor();
        sensor->setAddress(SI7021_ADDRESS);
        _sensors.push_back(sensor);
    }
    #endif

    #if SM300D2_SUPPORT
    {
        SM300D2Sensor * sensor = new SM300D2Sensor();
        sensor->setRX(SM300D2_RX_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if T6613_SUPPORT
    {
        T6613Sensor * sensor = new T6613Sensor();
        sensor->setRX(T6613_RX_PIN);
        sensor->setTX(T6613_TX_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if TMP3X_SUPPORT
    {
        TMP3XSensor * sensor = new TMP3XSensor();
        sensor->setType(TMP3X_TYPE);
        _sensors.push_back(sensor);
    }
    #endif

    #if V9261F_SUPPORT
    {
        V9261FSensor * sensor = new V9261FSensor();
        sensor->setRX(V9261F_PIN);
        sensor->setInverted(V9261F_PIN_INVERSE);
        _sensors.push_back(sensor);
    }
    #endif

    #if MAX6675_SUPPORT
    {
        MAX6675Sensor * sensor = new MAX6675Sensor();
        sensor->setCS(MAX6675_CS_PIN);
        sensor->setSO(MAX6675_SO_PIN);
        sensor->setSCK(MAX6675_SCK_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if VEML6075_SUPPORT
    {
        VEML6075Sensor * sensor = new VEML6075Sensor();
        sensor->setIntegrationTime(VEML6075_INTEGRATION_TIME);
        sensor->setDynamicMode(VEML6075_DYNAMIC_MODE);
        _sensors.push_back(sensor);
    }
    #endif

    #if VL53L1X_SUPPORT
    {
        VL53L1XSensor * sensor = new VL53L1XSensor();
        sensor->setInterMeasurementPeriod(
            VL53L1XSensor::InterMeasurementPeriod{VL53L1X_INTER_MEASUREMENT_PERIOD});
        sensor->setMeasurementTimingBudget(
            VL53L1XSensor::MeasurementTimingBudget{VL53L1X_MEASUREMENT_TIMING_BUDGET});
        sensor->setDistanceMode(VL53L1X_DISTANCE_MODE);
        _sensors.push_back(sensor);
    }
    #endif

    #if EZOPH_SUPPORT
    {
        EZOPHSensor * sensor = new EZOPHSensor();
        sensor->setRX(EZOPH_RX_PIN);
        sensor->setTX(EZOPH_TX_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if ADE7953_SUPPORT
    {
        ADE7953Sensor * sensor = new ADE7953Sensor();
        sensor->setAddress(ADE7953_ADDRESS);
        _sensors.push_back(sensor);
    }
    #endif

    #if SI1145_SUPPORT
    {
        SI1145Sensor * sensor = new SI1145Sensor();
        sensor->setAddress(SI1145_ADDRESS);
        _sensors.push_back(sensor);
    }
    #endif

    #if HDC1080_SUPPORT
    {
        HDC1080Sensor * sensor = new HDC1080Sensor();
        sensor->setAddress(HDC1080_ADDRESS);
        _sensors.push_back(sensor);
    }
    #endif

    #if PZEM004TV30_SUPPORT
    {
        auto rx = getSetting("pzemv30RX", PZEM004TV30Sensor::RxPin);
        auto tx = getSetting("pzemv30TX", PZEM004TV30Sensor::TxPin);

        //TODO: getSetting("pzemv30*Cfg", (SW)SERIAL_8N1); ?
        //TODO: getSetting("serial*Cfg", ...); and attach index of the port ?
        //TODO: more than one sensor on port, like the v1
        PZEM004TV30Sensor::PortPtr port;
        if (getSetting("pzemSoft", PZEM004TV30Sensor::useSoftwareSerial())) {
            port = PZEM004TV30Sensor::makeSoftwarePort(rx, tx);
        } else {
            port = PZEM004TV30Sensor::makeHardwarePort(
                PZEM004TV30Sensor::defaultHardwarePort(), rx, tx);
        }

        if (!port) {
            return;
        }

        auto* sensor = PZEM004TV30Sensor::make(std::move(port),
            getSetting("pzemv30Addr", PZEM004TV30Sensor::DefaultAddress),
            getSetting("pzemv30ReadTimeout", PZEM004TV30Sensor::DefaultReadTimeout));
        sensor->setDebug(getSetting("pzemv30Debug", PZEM004TV30Sensor::DefaultDebug));

        _sensors.push_back(sensor);
    }
    #endif

}

String _magnitudeFormat(const Magnitude& magnitude, double value) {
    // XXX: dtostrf only handles basic floating point values and will never produce scientific notation
    //      ensure decimals is within some sane limit and the actual value never goes above this buffer size
    char buffer[64];
    dtostrf(value, 1, magnitude.decimals, buffer);

    return buffer;
}

sensor::Value _magnitudeValue(const Magnitude& magnitude, double value) {
    return sensor::Value {
        .type = magnitude.type,
        .index = magnitude.index_global,
        .units = magnitude.units,
        .decimals = magnitude.decimals,
        .value = value,
        .topic = _magnitudeTopicIndex(magnitude),
        .repr = _magnitudeFormat(magnitude, value),
    };
}

void _sensorReport(const Magnitude& magnitude, unsigned char index, double value) {
    const auto report = _magnitudeValue(magnitude, magnitude.reported);

    for (auto& handler : _magnitude_report_handlers) {
        handler(report);
    }

#if MQTT_SUPPORT
    {
        mqttSend(report.topic.c_str(), report.repr.c_str());

#if SENSOR_PUBLISH_ADDRESSES
        {
            static constexpr auto AddressTopic = STRING_VIEW(SENSOR_ADDRESS_TOPIC);

            String address_topic;
            address_topic.reserve(report.topic.length() + AddressTopic.length());
            address_topic.concat(AddressTopic.c_str(), AddressTopic.length());
            address_topic += '/';
            address_topic += report.topic;

            mqttSend(address_topic.c_str(), magnitude.sensor->address(magnitude.slot).c_str());
        }
#endif // SENSOR_PUBLISH_ADDRESSES

    }
#endif // MQTT_SUPPORT

    // TODO: both integrations depend on the absolute index instead of specific type
    //       so, we still need to pass / know the 'global' index inside of _magnitudes[]

#if THINGSPEAK_SUPPORT
    tspkEnqueueMeasurement(index, report.repr.c_str());
#endif // THINGSPEAK_SUPPORT

#if DOMOTICZ_SUPPORT
    domoticzSendMagnitude(index, report);
#endif // DOMOTICZ_SUPPORT
}

void _sensorInit() {
    _sensors_ready = true;

    for (auto sensor : _sensors) {

        // Do not process an already initialized sensor
        if (sensor->ready()) {
            continue;
        }

        // Force sensor to reload config
        DEBUG_MSG_P(PSTR("[SENSOR] Initializing %s\n"), sensor->description().c_str());
        sensor->begin();

        if (!sensor->ready()) {
            const auto error = sensor->error();
            if (error != SENSOR_ERROR_OK) {
                DEBUG_MSG_P(PSTR("[SENSOR]  -> ERROR %s (%hhu)\n"),
                    _sensorError(error).c_str(), error);
            }
            _sensors_ready = false;
            break;
        }

        // Initialize sensor magnitudes
        for (unsigned char magnitude_slot = 0; magnitude_slot < sensor->count(); ++magnitude_slot) {
            const auto magnitude_type = sensor->type(magnitude_slot);
            auto filter = _magnitudeCreateFilter(magnitude_type);
            _magnitudes.emplace_back(
                sensor,            // every magnitude is bound to it's sensor
                std::move(filter), // store, sum, average, etc. value for reporting
                magnitude_slot,    // id of the magnitude, unique to the sensor
                magnitude_type     // cache type as well, no need to call type(slot) again
            );
        }

        // Custom initializations for analog sensors
        // (but, notice that this is global across all sensors of this type!)
        if (_sensorIsAnalog(sensor)) {
            _sensorAnalogInit(static_cast<BaseAnalogSensor*>(sensor.get()));
        }
    }

    // Energy tracking is implemented by looking at the specific magnitude & it's index at read time
    // TODO: shuffle some functions around so that debug can be in the init func instead and still be inline?
    for (auto& magnitude : _magnitudes) {
        if (_sensorIsEmon(magnitude.sensor) && (MAGNITUDE_ENERGY == magnitude.type)) {
            _sensorTrackEnergyTotal(magnitude);
            DEBUG_MSG_P(PSTR("[ENERGY] Tracking %s/%u for %s\n"),
                    _magnitudeTopic(magnitude.type).c_str(),
                    magnitude.index_global,
                    magnitude.sensor->description().c_str());
        }
    }

    if (_sensors_ready) {
        DEBUG_MSG_P(PSTR("[SENSOR] Finished initialization for %zu sensor(s) and %zu magnitude(s)\n"),
            _sensors.size(), _magnitudes.size());
    }

}

void _sensorConfigure() {

    // Read interval is shared between every sensor
    // TODO: implement scheduling in the sensor itself.
    // allow reads faster than 1sec, not just internal ones via tick()
    // allow 'manual' sensors that may be triggered programatically
    _sensor_read_interval = sensor::settings::readInterval();
    _sensor_init_interval = sensor::settings::initInterval();
    _sensor_report_every = sensor::settings::reportEvery();

    // TODO: something more generic? energy is an accumulating value, only allow for similar ones?
    // TODO: move to an external module?
    _sensor_energy_tracker.every(sensor::settings::saveEvery());

    _sensor_real_time = sensor::settings::realTimeValues();

    // Update magnitude config, filter sizes and reset energy if needed
    {
        for (auto& magnitude : _magnitudes) {
            // Some filters must be able store up to a certain amount of readings.
            if (magnitude.filter->capacity() != _sensor_report_every) {
                magnitude.filter->resize(_sensor_report_every);
            }

            // process emon-specific settings first. ensure that settings use global index and we access sensor with the local one
            if (_sensorIsEmon(magnitude.sensor) && _magnitudeRatioSupported(magnitude.type)) {
                auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor.get());
                sensor->setRatio(magnitude.slot, getSetting(
                    ::sensor::settings::keys::get(magnitude, ::sensor::settings::suffix::Ratio),
                    sensor->defaultRatio(magnitude.slot)));
            }

            // analog variant of emon sensor has some additional settings
            if (_sensorIsAnalogEmon(magnitude.sensor) && (magnitude.type == MAGNITUDE_VOLTAGE)) {
                auto* sensor = static_cast<BaseAnalogEmonSensor*>(magnitude.sensor.get());
                sensor->setVoltage(getSetting(
                    ::sensor::settings::keys::get(magnitude, ::sensor::settings::suffix::Mains),
                    sensor->defaultVoltage()));
                sensor->setReferenceVoltage(getSetting(
                    ::sensor::settings::keys::get(magnitude, ::sensor::settings::suffix::Reference),
                    sensor->defaultReferenceVoltage()));
            }

            // adjust units based on magnitude's type
            magnitude.units = _magnitudeUnitFilter(magnitude,
                getSetting(
                    ::sensor::settings::keys::get(magnitude, ::sensor::settings::suffix::Units),
                    magnitude.sensor->units(magnitude.slot)));

            // adjust resulting value (simple plus or minus)
            // TODO: inject math or rpnlib expression?
            if (_magnitudeCorrectionSupported(magnitude.type)) {
                magnitude.correction = getSetting(
                    ::sensor::settings::keys::get(magnitude, ::sensor::settings::suffix::Correction),
                    _magnitudeCorrection(magnitude.type));
            }

            // pick decimal precision either from our (sane) defaults of from the sensor itself
            // (specifically, when sensor has more or less precision than we expect)
            {
                signed char decimals = magnitude.sensor->decimals(magnitude.units);
                magnitude.decimals = (decimals >= 0)
                    ? static_cast<unsigned char>(decimals)
                    : _sensorUnitDecimals(magnitude.units);
            }

            // Per-magnitude min & max delta settings for reporting the value
            // - ${prefix}DeltaMin${index} controls whether we report when report counter overflows
            //   (default is set to 0.0 aka value has changed from the last recorded one)
            // - ${prefix}DeltaMax${index} will trigger report as soon as read value is greater than the specified delta
            //   (default is 0.0 as well, but this needs to be >0 to actually do something)
            magnitude.min_delta = getSetting(
                ::sensor::settings::keys::get(magnitude, ::sensor::settings::suffix::MinDelta),
                sensor::build::DefaultMinDelta);
            magnitude.max_delta = getSetting(
                ::sensor::settings::keys::get(magnitude, ::sensor::settings::suffix::MaxDelta),
                sensor::build::DefaultMaxDelta);

            // Sometimes we want to ensure the value is above certain threshold before reporting
            magnitude.zero_threshold = getSetting(
                ::sensor::settings::keys::get(magnitude, ::sensor::settings::suffix::ZeroThreshold),
                sensor::Value::Unknown);

            // in case we don't save energy periodically, purge existing value in ram & settings
            if ((MAGNITUDE_ENERGY == magnitude.type) && (0 == _sensor_energy_tracker.every())) {
                _sensorResetEnergyTotal(magnitude.index_global);
            }

        }
    }

}

} // namespace

// -----------------------------------------------------------------------------
// Public
// -----------------------------------------------------------------------------

void sensorOnMagnitudeRead(MagnitudeReadHandler handler) {
    _magnitude_read_handlers.push_front(handler);
}

void sensorOnMagnitudeReport(MagnitudeReadHandler handler) {
    _magnitude_report_handlers.push_front(handler);
}

unsigned char sensorCount() {
    return _sensors.size();
}

unsigned char magnitudeCount() {
    return _magnitudes.size();
}

unsigned char magnitudeType(unsigned char index) {
    if (index < _magnitudes.size()) {
        return _magnitudes[index].type;
    }
    return MAGNITUDE_NONE;
}

String magnitudeTopic(unsigned char type) {
    return _magnitudeTopic(type);
}

sensor::Value::operator bool() const {
    return !std::isinf(value) && !std::isnan(value);
}

sensor::Value magnitudeValue(unsigned char index) {
    sensor::Value out;
    out.value = sensor::Value::Unknown;

    if (index < _magnitudes.size()) {
        const auto& magnitude = _magnitudes[index];
        out = _magnitudeValue(magnitude,
            _sensor_real_time ? magnitude.last : magnitude.reported);
    }

    return out;
}

unsigned char magnitudeIndex(unsigned char index) {
    if (index < _magnitudes.size()) {
        return _magnitudes[index].index_global;
    }
    return 0;
}

String magnitudeDescription(unsigned char index) {
    if (index < _magnitudes.size()) {
        return _magnitudeDescription(_magnitudes[index]);
    }
    return String();
}

String magnitudeTopicIndex(unsigned char index) {
    if (index < _magnitudes.size()) {
        return _magnitudeTopicIndex(_magnitudes[index]);
    }
    return String();
}

// -----------------------------------------------------------------------------

namespace {

void _sensorSettingsMigrate(int version) {
    using namespace ::sensor::settings;

    auto firstKey = [](unsigned char type, ::settings::StringView suffix) {
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
        moveSettings(F("pzEneTotal"), prefix::get(MAGNITUDE_ENERGY).toString() + FPSTR(suffix::Total));
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

} // namespace

void sensorSetup() {

    // Settings backwards compatibility
    migrateVersion(_sensorSettingsMigrate);

    // Load configured sensors and set up all of magnitudes
    _sensorLoad();
    _sensorInit();

    // Configure based on settings
    _sensorConfigure();

    // Allow us to query key default
    settingsRegisterQueryHandler({
        .check = _sensorCheckKeyPrefix,
        .get = _sensorQueryHandler
    });

    // Websockets integration, send sensor readings and configuration
    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_sensorWebSocketOnVisible)
            .onConnected(_sensorWebSocketOnConnectedInitial)
            .onConnected(_sensorWebSocketOnConnectedList)
            .onConnected(_sensorWebSocketOnConnectedSettings)
            .onData(_sensorWebSocketSendData)
            .onAction(_sensorWebSocketOnAction)
            .onKeyCheck(_sensorWebSocketOnKeyCheck);
    #endif

    // MQTT receive callback, atm only for energy reset
    #if MQTT_SUPPORT
        mqttRegister(_sensorMqttCallback);
    #endif

    // API
    #if API_SUPPORT
        _sensorApiSetup();
    #endif

    // Terminal
    #if TERMINAL_SUPPORT
        _sensorInitCommands();
    #endif

    // Main callbacks
    espurnaRegisterLoop(sensorLoop);
    espurnaRegisterReload(_sensorConfigure);

}

void sensorLoop() {

    // Continiously repeat initialization if there are still some un-initialized sensors after setup()
    using TimeSource = espurna::time::CoreClock;
    static auto last_init = TimeSource::now();

    auto timestamp = TimeSource::now();
    if (!_sensors_ready && (timestamp - last_init > _sensor_init_interval)) {
        last_init = timestamp;
        _sensorInit();
    }

    if (!_magnitudes.size()) {
        return;
    }

    // Tick hook, called every loop()
    _sensorTick();

    // But, the actual reading needs to happen at the specified interval
    static auto last_update = TimeSource::now();
    static size_t report_count { 0 };

    if (timestamp - last_update > _sensor_read_interval) {

        last_update = timestamp;
        report_count = (report_count + 1) % _sensor_report_every;

        sensor::ReadValue value {
            .raw = 0.0,         // as the sensor returns it
            .processed = 0.0,   // after applying units and decimals
            .filtered = 0.0     // after applying filters, units and decimals
        };

        // Pre-read hook, called every reading
        _sensorPre();

        // XXX: Filter out certain magnitude types when relay is turned OFF
#if RELAY_SUPPORT && SENSOR_POWER_CHECK_STATUS
        const bool relay_off = (relayCount() == 1) && (relayStatus(0) == 0);
#endif

        for (size_t magnitude_index = 0; magnitude_index < _magnitudes.size(); ++magnitude_index) {
            auto& magnitude = _magnitudes[magnitude_index];

            if (!magnitude.sensor->status()) {
                continue;
            }

            // -------------------------------------------------------------
            // Instant value
            // -------------------------------------------------------------

            value.raw = magnitude.sensor->value(magnitude.slot);

            // Completely remove spurious values if relay is OFF
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

            // In addition to that, we also check that value is above a certain threshold
            if ((!std::isnan(magnitude.zero_threshold)) && ((value.raw < magnitude.zero_threshold))) {
                value.raw = 0.0;
            }

            magnitude.last = value.raw;
            magnitude.filter->update(value.raw);

            // -------------------------------------------------------------
            // Procesing (units and decimals)
            // -------------------------------------------------------------

            value.processed = _magnitudeProcess(magnitude, value.raw);
            {
                const auto out = _magnitudeValue(magnitude, value.processed);
                for (auto& handler : _magnitude_read_handlers) {
                    handler(out);
                }
            }

            // -------------------------------------------------------------------
            // Reporting
            // -------------------------------------------------------------------

            // Initial status or after report counter overflows
            bool report { 0 == report_count };

            // In case magnitude was configured with ${name}MaxDelta, override report check
            // when the value change is greater than the delta
            if (!std::isnan(magnitude.reported) && (magnitude.max_delta > sensor::build::DefaultMaxDelta)) {
                report = std::abs(value.processed - magnitude.reported) >= magnitude.max_delta;
            }

            // Special case for energy, save readings to RAM and EEPROM
            if (MAGNITUDE_ENERGY == magnitude.type) {
                _magnitudeSaveEnergyTotal(magnitude, report);
            }

            if (report) {
                value.filtered = _magnitudeProcess(magnitude, magnitude.filter->value());

                // Make sure that report value is calculated using every read value before it
                magnitude.filter->reset();

                // Check ${name}MinDelta if there is a minimum change threshold to report
                if (std::isnan(magnitude.reported) || (std::abs(value.filtered - magnitude.reported) >= magnitude.min_delta)) {
                    _sensorReport(magnitude, magnitude_index, value.filtered);
                    magnitude.reported = value.filtered;
                }

            }

#if SENSOR_DEBUG
            {
                auto withUnits = [&](double value, sensor::Unit units) {
                    String out;
                    out += _magnitudeFormat(magnitude, value);
                    if (units != sensor::Unit::None) {
                        out += _magnitudeUnits(units);
                    }

                    return out;
                };

                DEBUG_MSG_P(PSTR("[SENSOR] %s -> raw %s processed %s filtered %s\n"),
                    _magnitudeTopic(magnitude.type).c_str(),
                    withUnits(value.raw, magnitude.sensor->units(magnitude.slot)).c_str(),
                    withUnits(value.processed, magnitude.units).c_str(),
                    withUnits(value.filtered, magnitude.units).c_str());
            }
#endif
        }

        // Post-read hook, called every reading
        _sensorPost();

        // And report data to modules that don't specifically track them
#if WEB_SUPPORT
        wsPost(_sensorWebSocketSendData);
#endif

#if THINGSPEAK_SUPPORT
        if (report_count == 0) tspkFlush();
#endif

    }

}

#endif // SENSOR_SUPPORT
