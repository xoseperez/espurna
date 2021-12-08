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

class sensor_magnitude_t {
private:
    constexpr static double _unset = std::numeric_limits<double>::quiet_NaN();
    static unsigned char _counts[MAGNITUDE_MAX];

    sensor_magnitude_t& operator=(const sensor_magnitude_t&) = default;

    void move(sensor_magnitude_t&& other) noexcept {
        *this = other;
        other.filter = nullptr;
    }

public:
    static size_t counts(unsigned char type) {
        return _counts[type];
    }

    sensor_magnitude_t() = delete;
    sensor_magnitude_t(const sensor_magnitude_t&) = delete;
    sensor_magnitude_t(sensor_magnitude_t&& other) noexcept {
        *this = other;
        other.filter = nullptr;
    }

    sensor_magnitude_t& operator=(sensor_magnitude_t&& other) noexcept {
        move(std::move(other));
        return *this;
    }

    ~sensor_magnitude_t() noexcept {
        delete filter;
    }

    sensor_magnitude_t(unsigned char slot, unsigned char type, sensor::Unit units, BaseSensor* sensor);

    BaseSensor * sensor { nullptr }; // Sensor object
    BaseFilter * filter { nullptr }; // Filter object

    unsigned char slot { 0u }; // Sensor slot # taken by the magnitude, used to access the measurement
    unsigned char type { MAGNITUDE_NONE }; // Type of measurement, returned by the BaseSensor::type(slot)

    unsigned char index_global { 0u }; // N'th magnitude of it's type, across all of the active sensors

    sensor::Unit units { sensor::Unit::None }; // Units of measurement
    unsigned char decimals { 0u }; // Number of decimals in textual representation

    double last { _unset };     // Last raw value from sensor (unfiltered)
    double reported { _unset }; // Last reported value
    double min_delta { 0.0 };   // Minimum value change to report
    double max_delta { 0.0 };   // Maximum value change to report
    double correction { 0.0 };  // Value correction (applied when processing)

    double zero_threshold { _unset }; // Reset value to zero when below threshold (applied when reading)
};

static_assert(
    std::is_nothrow_move_constructible<sensor_magnitude_t>::value,
    "std::vector<sensor_magnitude_t> should be able to work with resize()"
);

static_assert(
    !std::is_copy_constructible<sensor_magnitude_t>::value,
    "std::vector<sensor_magnitude_t> should only use move ctor"
);

unsigned char sensor_magnitude_t::_counts[MAGNITUDE_MAX] = {0};

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

namespace {
namespace build {

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

constexpr int ReportEveryMin { SENSOR_REPORT_MIN_EVERY };
constexpr int ReportEveryMax { SENSOR_REPORT_MAX_EVERY };

constexpr int reportEvery() {
    return SENSOR_REPORT_EVERY;
}

constexpr int saveEvery() {
    return SENSOR_SAVE_EVERY;
}

constexpr bool realTimeValues() {
    return SENSOR_REAL_TIME_VALUES;
}

} // namespace build

namespace settings {

espurna::duration::Seconds readInterval() {
    return std::clamp(getSetting("snsRead", build::readInterval()),
            build::ReadIntervalMin, build::ReadIntervalMax);
}

espurna::duration::Seconds initInterval() {
    return std::clamp(getSetting("snsInit", build::initInterval()),
            build::ReadIntervalMin, build::ReadIntervalMax);
}

int reportEvery() {
    return std::clamp(getSetting("snsReport", build::reportEvery()),
            build::ReportEveryMin, build::ReportEveryMax);
}

int saveEvery() {
    return getSetting("snsSave", build::saveEvery());
}

bool realTimeValues() {
    return getSetting("snsRealTime", build::realTimeValues());
}

} // namespace settings
} // namespace
} // namespace sensor

namespace settings {
namespace internal {

template <>
sensor::Unit convert(const String& value) {
    auto len = value.length();
    if (len && isNumber(value)) {
        constexpr int Min { static_cast<int>(sensor::Unit::Min_) };
        constexpr int Max { static_cast<int>(sensor::Unit::Max_) };
        auto num = convert<int>(value);
        if ((Min < num) && (num < Max)) {
            return static_cast<sensor::Unit>(num);
        }
    }

    return sensor::Unit::None;
}

String serialize(sensor::Unit unit) {
    return serialize(static_cast<int>(unit));
}

} // namespace internal
} // namespace settings

namespace {

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

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
    return (
        (MAGNITUDE_TEMPERATURE == type) ? (true) :
        (MAGNITUDE_HUMIDITY == type) ? (true) :
        (MAGNITUDE_LUX == type) ? (true) :
        (MAGNITUDE_PRESSURE == type) ? (true) :
        false
    );
}

} // namespace

// -----------------------------------------------------------------------------
// Energy persistence
// -----------------------------------------------------------------------------

namespace {

struct SensorEnergyTracker {
    static constexpr int Every { SENSOR_SAVE_EVERY };
    using Magnitude = std::reference_wrapper<sensor_magnitude_t>;

    struct Counter {
        Magnitude magnitude;
        int value;
    };
    using Counters = std::vector<Counter>;

    explicit operator bool() const {
        return _every > 0;
    }

    int every() const {
        return _every;
    }

    void add(sensor_magnitude_t& magnitude) {
        _count.push_back({magnitude, 0});
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

bool _sensorIsEmon(BaseSensor* sensor) {
    return sensor->type() & (sensor::type::Emon | sensor::type::AnalogEmon);
}

bool _sensorIsAnalogEmon(BaseSensor* sensor) {
    return sensor->type() & sensor::type::AnalogEmon;
}

bool _sensorIsAnalog(BaseSensor* sensor) {
    return sensor->type() & sensor::type::Analog;
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

void _sensorApiResetEnergy(const sensor_magnitude_t& magnitude, const String& payload) {
    if (!payload.length()) {
        return;
    }

    auto energy = _sensorParseEnergy(payload);
    if (!energy) {
        return;
    }

    auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor);
    sensor->resetEnergy(magnitude.slot, energy.value());
}

void _sensorApiResetEnergy(const sensor_magnitude_t& magnitude, const char* payload) {
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
        result = _sensorParseEnergy(getSetting({"eneTotal", index})).value();
    }

    return result;
}

void _sensorResetEnergyTotal(unsigned char index) {
    delSetting({"eneTotal", index});
    delSetting({"eneTime", index});
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
        setSetting({"eneTotal", _index}, _energy.asString());
#if NTP_SUPPORT
        if (ntpSynced()) {
            setSetting({"eneTime", _index}, ntpDateTime());
        }
#endif
    }

private:
    size_t _index;
    sensor::Energy _energy;
};

void _magnitudeSaveEnergyTotal(sensor_magnitude_t& magnitude, bool persistent) {
    if (magnitude.type != MAGNITUDE_ENERGY) return;

    auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor);
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

void _sensorTrackEnergyTotal(sensor_magnitude_t& magnitude) {
    const auto index_global = magnitude.index_global;
    auto* ptr = static_cast<BaseEmonSensor*>(magnitude.sensor);
    ptr->resetEnergy(magnitude.slot, _sensorEnergyTotal(index_global));
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
std::vector<BaseSensor*> _sensors;

bool _sensor_real_time { sensor::build::realTimeValues() };
int _sensor_report_every { sensor::build::reportEvery() };

espurna::duration::Seconds _sensor_read_interval { sensor::build::readInterval() };
espurna::duration::Seconds _sensor_init_interval { sensor::build::initInterval() };

std::vector<sensor_magnitude_t> _magnitudes;

using MagnitudeReadHandlers = std::forward_list<MagnitudeReadHandler>;
MagnitudeReadHandlers _magnitude_read_handlers;
MagnitudeReadHandlers _magnitude_report_handlers;

BaseFilter* _magnitudeCreateFilter(unsigned char type, size_t size) {
    BaseFilter* filter { nullptr };

    switch (type) {
    case MAGNITUDE_IAQ:
    case MAGNITUDE_IAQ_STATIC:
    case MAGNITUDE_ENERGY:
        filter = new LastFilter();
        break;
    case MAGNITUDE_COUNT:
    case MAGNITUDE_GEIGER_CPM:
    case MAGNITUDE_GEIGER_SIEVERT:
    case MAGNITUDE_ENERGY_DELTA:
        filter = new SumFilter();
        break;
    case MAGNITUDE_EVENT:
    case MAGNITUDE_DIGITAL:
        filter = new MaxFilter();
        break;
    default:
        filter = new MedianFilter();
        break;
    }

    filter->resize(size);

    return filter;
}

sensor_magnitude_t::sensor_magnitude_t(unsigned char slot_, unsigned char type_, sensor::Unit units_, BaseSensor* sensor_) :
    sensor(sensor_),
    filter(_magnitudeCreateFilter(type_, _sensor_report_every)),
    slot(slot_),
    type(type_),
    index_global(_counts[type]),
    units(units_)
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
        case MAGNITUDE_PM1dot0:
            result = F("pm1dot0");
            break;
        case MAGNITUDE_PM2dot5:
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

String _magnitudeUnits(sensor::Unit unit) {
    const __FlashStringHelper* result { F("") };

    switch (unit) {
    case sensor::Unit::Farenheit:
        result = F("°F");
        break;
    case sensor::Unit::Celcius:
        result = F("°C");
        break;
    case sensor::Unit::Kelvin:
        result = F("K");
        break;
    case sensor::Unit::Percentage:
        result = F("%");
        break;
    case sensor::Unit::Hectopascal:
        result = F("hPa");
        break;
    case sensor::Unit::Ampere:
        result = F("A");
        break;
    case sensor::Unit::Volt:
        result = F("V");
        break;
    case sensor::Unit::Watt:
        result = F("W");
        break;
    case sensor::Unit::Kilowatt:
        result = F("kW");
        break;
    case sensor::Unit::Voltampere:
        result = F("VA");
        break;
    case sensor::Unit::Kilovoltampere:
        result = F("kVA");
        break;
    case sensor::Unit::VoltampereReactive:
        result = F("VAR");
        break;
    case sensor::Unit::KilovoltampereReactive:
        result = F("kVAR");
        break;
    case sensor::Unit::Joule:
    //aka case sensor::Unit::WattSecond:
        result = F("J");
        break;
    case sensor::Unit::KilowattHour:
        result = F("kWh");
        break;
    case sensor::Unit::MicrogrammPerCubicMeter:
        result = F("µg/m³");
        break;
    case sensor::Unit::PartsPerMillion:
        result = F("ppm");
        break;
    case sensor::Unit::Lux:
        result = F("lux");
        break;
    case sensor::Unit::UltravioletIndex:
        break;
    case sensor::Unit::Ohm:
        result = F("ohm");
        break;
    case sensor::Unit::MilligrammPerCubicMeter:
        result = F("mg/m³");
        break;
    case sensor::Unit::CountsPerMinute:
        result = F("cpm");
        break;
    case sensor::Unit::MicrosievertPerHour:
        result = F("µSv/h");
        break;
    case sensor::Unit::Meter:
        result = F("m");
        break;
    case sensor::Unit::Hertz:
        result = F("Hz");
        break;
    case sensor::Unit::Ph:
        result = F("pH");
        break;
    case sensor::Unit::Min_:
    case sensor::Unit::Max_:
    case sensor::Unit::None:
        break;
    }

    return String(result);
}

String _magnitudeUnits(const sensor_magnitude_t& magnitude) {
    return _magnitudeUnits(magnitude.units);
}

} // namespace

String magnitudeUnits(unsigned char index) {
    if (index < _magnitudes.size()) {
        return _magnitudeUnits(_magnitudes[index]);
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

    case MAGNITUDE_PM1dot0:
    case MAGNITUDE_PM2dot5:
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

bool _magnitudeUnitSupported(const sensor_magnitude_t& magnitude, sensor::Unit unit) {
    const auto range = _magnitudeUnitsRange(magnitude.type);
    return std::any_of(range.begin(), range.end(), [&](sensor::Unit supported) {
        return (unit == supported);
    });
}

sensor::Unit _magnitudeUnitFilter(const sensor_magnitude_t& magnitude, sensor::Unit unit) {
    return _magnitudeUnitSupported(magnitude, unit) ? unit : magnitude.units;
}

double _magnitudeProcess(const sensor_magnitude_t& magnitude, double value) {

    // Process input (sensor) units and convert to the ones that magnitude specifies as output
    switch (magnitude.sensor->units(magnitude.slot)) {
        case sensor::Unit::Celcius:
            if (magnitude.units == sensor::Unit::Farenheit) {
                value = (value * 1.8) + 32.0;
            } else if (magnitude.units == sensor::Unit::Kelvin) {
                value = value + 273.15;
            }
            break;
        case sensor::Unit::Percentage:
            value = constrain(value, 0.0, 100.0);
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

String _magnitudeDescription(const sensor_magnitude_t& magnitude) {
    return magnitude.sensor->description(magnitude.slot);
}

// -----------------------------------------------------------------------------

// do `callback(type)` for each present magnitude
template <typename T>
void _magnitudeForEachCounted(T&& callback) {
    for (unsigned char type = MAGNITUDE_NONE + 1; type < MAGNITUDE_MAX; ++type) {
        if (sensor_magnitude_t::counts(type)) {
            callback(type);
        }
    }
}

// check if `callback(type)` returns `true` at least once
template <typename T>
bool _magnitudeForEachCountedCheck(T&& callback) {
    for (unsigned char type = MAGNITUDE_NONE + 1; type < MAGNITUDE_MAX; ++type) {
        if (sensor_magnitude_t::counts(type) && callback(type)) {
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

const __FlashStringHelper* _magnitudeSettingsPrefix(unsigned char type) {
    switch (type) {
    case MAGNITUDE_TEMPERATURE:
        return F("tmp");
    case MAGNITUDE_HUMIDITY:
        return F("hum");
    case MAGNITUDE_PRESSURE:
        return F("press");
    case MAGNITUDE_CURRENT:
        return F("curr");
    case MAGNITUDE_VOLTAGE:
        return F("volt");
    case MAGNITUDE_POWER_ACTIVE:
        return F("pwrP");
    case MAGNITUDE_POWER_APPARENT:
        return F("pwrQ");
    case MAGNITUDE_POWER_REACTIVE:
        return F("pwrModS");
    case MAGNITUDE_POWER_FACTOR:
        return F("pwrPF");
    case MAGNITUDE_ENERGY:
        return F("ene");
    case MAGNITUDE_ENERGY_DELTA:
        return F("eneDelta");
    case MAGNITUDE_ANALOG:
        return F("analog");
    case MAGNITUDE_DIGITAL:
        return F("digital");
    case MAGNITUDE_EVENT:
        return F("event");
    case MAGNITUDE_PM1dot0:
        return F("pm1dot0");
    case MAGNITUDE_PM2dot5:
        return F("pm1dot5");
    case MAGNITUDE_PM10:
        return F("pm10");
    case MAGNITUDE_CO2:
        return F("co2");
    case MAGNITUDE_VOC:
        return F("voc");
    case MAGNITUDE_IAQ:
        return F("iaq");
    case MAGNITUDE_IAQ_ACCURACY:
        return F("iaqAccuracy");
    case MAGNITUDE_IAQ_STATIC:
        return F("iaqStatic");
    case MAGNITUDE_LUX:
        return F("lux");
    case MAGNITUDE_UVA:
        return F("uva");
    case MAGNITUDE_UVB:
        return F("uvb");
    case MAGNITUDE_UVI:
        return F("uvi");
    case MAGNITUDE_DISTANCE:
        return F("distance");
    case MAGNITUDE_HCHO:
        return F("hcho");
    case MAGNITUDE_GEIGER_CPM:
        return F("gcpm");
    case MAGNITUDE_GEIGER_SIEVERT:
        return F("gsiev");
    case MAGNITUDE_COUNT:
        return F("count");
    case MAGNITUDE_NO2:
        return F("no2");
    case MAGNITUDE_CO:
        return F("co");
    case MAGNITUDE_RESISTANCE:
        return F("res");
    case MAGNITUDE_PH:
        return F("ph");
    case MAGNITUDE_FREQUENCY:
        return F("freq");
    case MAGNITUDE_TVOC:
        return F("tvoc");
    case MAGNITUDE_CH2O:
        return F("ch2o");
    }

    return nullptr;
}

template <typename T>
String _magnitudeSettingsKey(unsigned char type, T&& suffix) {
    return String(_magnitudeSettingsPrefix(type)) + suffix;
}

template <typename T>
String _magnitudeSettingsKey(sensor_magnitude_t& magnitude, T&& suffix) {
    return _magnitudeSettingsKey(magnitude.type, std::forward<T>(suffix));
}

const char RatioKey[] PROGMEM = "Ratio";

bool _sensorMatchKeyPrefix(const char* key) {
    if (strncmp_P(key, PSTR("sns"), 3) == 0) {
        return true;
    }

    if (strncmp_P(key, PSTR("pwr"), 3) == 0) {
        return true;
    }

    const String _key(key);
    return _magnitudeForEachCountedCheck([&](unsigned char type) {
        return _key.startsWith(_magnitudeSettingsPrefix(type));
    });
}

SettingsKey _magnitudeSettingsRatioKey(unsigned char type, size_t index) {
    return {_magnitudeSettingsKey(type, RatioKey), index};
}

SettingsKey _magnitudeSettingsRatioKey(const sensor_magnitude_t& magnitude) {
    return _magnitudeSettingsRatioKey(magnitude.type, magnitude.index_global);
}

double _magnitudeSettingsRatio(const sensor_magnitude_t& magnitude, double defaultValue) {
    return getSetting(_magnitudeSettingsRatioKey(magnitude), defaultValue);
};

constexpr bool _magnitudeRatioSupported(unsigned char type) {
    return (type == MAGNITUDE_CURRENT)
        || (type == MAGNITUDE_VOLTAGE)
        || (type == MAGNITUDE_POWER_ACTIVE)
        || (type == MAGNITUDE_ENERGY);
}

String _sensorQueryDefault(const String& key) {
    auto get_defaults = [](sensor_magnitude_t* magnitude) -> String {
        if (magnitude && _sensorIsEmon(magnitude->sensor)) {
            auto* sensor = static_cast<BaseEmonSensor*>(magnitude->sensor);
            if (_magnitudeRatioSupported(magnitude->type)) {
                return String(sensor->defaultRatio(magnitude->slot));
            }
        }

        return String();
    };

    auto magnitude_key = [](const sensor_magnitude_t& magnitude) -> SettingsKey {
        if (_magnitudeRatioSupported(magnitude.type)) {
            return _magnitudeSettingsRatioKey(magnitude);
        }

        return "";
    };

    sensor_magnitude_t* target { nullptr };
    for (auto& magnitude : _magnitudes) {
        if (_magnitudeRatioSupported(magnitude.type)) {
            auto ratioKey(magnitude_key(magnitude));
            if (ratioKey == key) {
                target = &magnitude;
                break;
            }
        }
    }

    return get_defaults(target);
}

} // namespace

// -----------------------------------------------------------------------------
// Sensor calibration & emon ratios
// -----------------------------------------------------------------------------

namespace {

void _sensorAnalogInit(BaseAnalogSensor* sensor) {
    sensor->setR0(getSetting("snsR0", sensor->getR0()));
    sensor->setRS(getSetting("snsRS", sensor->getRS()));
    sensor->setRL(getSetting("snsRL", sensor->getRL()));
}

void _sensorApiAnalogCalibrate() {
    for (auto& ptr : _sensors) {
        if (_sensorIsAnalog(ptr)) {
            DEBUG_MSG_P(PSTR("[ANALOG] Calibrating %s\n"), ptr->description().c_str());

            auto* sensor = static_cast<BaseAnalogSensor*>(ptr);
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

    for (const auto& type : types) {
        for (size_t index = 0; index < sensor_magnitude_t::counts(type); ++index) {
            delSetting(_magnitudeSettingsRatioKey(type, index));
        }
    }

    for (auto& ptr : _sensors) {
        if (_sensorIsEmon(ptr)) {
            DEBUG_MSG_P(PSTR("[EMON] Resetting %s\n"), ptr->description().c_str());
            static_cast<BaseEmonSensor*>(ptr)->resetRatios();
        }
    }
}

double _sensorApiEmonExpectedValue(const sensor_magnitude_t& magnitude, double expected) {
    if (!_sensorIsEmon(magnitude.sensor)) {
        return BaseEmonSensor::DefaultRatio;
    }

    auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor);
    return sensor->ratioFromValue(magnitude.slot, sensor->value(magnitude.slot), expected);
}

} // namespace

// -----------------------------------------------------------------------------
// WebUI Websockets API
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

namespace {

bool _sensorWebSocketOnKeyCheck(const char* key, JsonVariant&) {
    return _sensorMatchKeyPrefix(key);
}

String _sensorError(unsigned char error) {

    const __FlashStringHelper* result = nullptr;

    switch (error) {
        case SENSOR_ERROR_OK:
            result = F("OK");
            break;
        case SENSOR_ERROR_OUT_OF_RANGE:
            result = F("Out of Range");
            break;
        case SENSOR_ERROR_WARM_UP:
            result = F("Warming Up");
            break;
        case SENSOR_ERROR_TIMEOUT:
            result = F("Timeout");
            break;
        case SENSOR_ERROR_UNKNOWN_ID:
            result = F("Unknown ID");
            break;
        case SENSOR_ERROR_CRC:
            result = F("CRC / Data Error");
            break;
        case SENSOR_ERROR_I2C:
            result = F("I2C Error");
            break;
        case SENSOR_ERROR_GPIO_USED:
            result = F("GPIO Already Used");
            break;
        case SENSOR_ERROR_CALIBRATION:
            result = F("Calibration Error");
            break;
        default:
        case SENSOR_ERROR_OTHER:
            result = F("Other / Unknown Error");
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
        case MAGNITUDE_PM1dot0:
            result = F("PM1.0");
            break;
        case MAGNITUDE_PM2dot5:
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
    ::web::ws::EnumerableConfig config{root, F("types")};
    config(F("values"), {MAGNITUDE_NONE + 1, MAGNITUDE_MAX},
        [](size_t type) {
            return sensor_magnitude_t::counts(type) > 0;
        },
        {
            {F("type"), [](JsonArray& out, size_t index) {
                out.add(index);
            }},
            {F("prefix"), [](JsonArray& out, size_t index) {
                out.add(_magnitudeSettingsPrefix(index));
            }},
            {F("name"), [](JsonArray& out, size_t index) {
                out.add(_magnitudeName(index));
            }}
        });
}

void _sensorWebSocketErrors(JsonObject& root) {
    ::web::ws::EnumerableConfig config{root, F("errors")};
    config(F("values"), SENSOR_ERROR_MAX, {
        {F("type"), [](JsonArray& out, size_t index) {
            out.add(index);
        }},
        {F("name"), [](JsonArray& out, size_t index) {
            out.add(_sensorError(index));
        }}
    });
}

void _sensorWebSocketUnits(JsonObject& root) {
    ::web::ws::EnumerableConfig config{root, F("units")};
    config(F("values"), _magnitudes.size(), {
        {F("supported"), [](JsonArray& out, size_t index) {
            JsonArray& units = out.createNestedArray();
            const auto range = _magnitudeUnitsRange(_magnitudes[index].type);
            for (auto it = range.begin(); it != range.end(); ++it) {
                JsonArray& unit = units.createNestedArray();
                unit.add(static_cast<int>(*it));
                unit.add(_magnitudeUnits(*it));
            }
        }}
    });
}

void _sensorWebSocketList(JsonObject& root) {
    ::web::ws::EnumerableConfig config{root, F("magnitudes-list")};
    config(F("values"), _magnitudes.size(), {
        {F("index_global"), [](JsonArray& out, size_t index) {
            out.add(_magnitudes[index].index_global);
        }},
        {F("type"), [](JsonArray& out, size_t index) {
            out.add(_magnitudes[index].type);
        }},
        {F("description"), [](JsonArray& out, size_t index) {
            out.add(_magnitudeDescription(_magnitudes[index]));
        }},
        {F("units"), [](JsonArray& out, size_t index) {
            out.add(static_cast<int>(_magnitudes[index].units));
        }}
    });
}

void _sensorWebSocketSettings(JsonObject& root) {
    // XXX: inject 'null' in the output. need this for optional fields, since the current
    // version of serializer only does this for char ptr and even makes NaN serialized as
    // NaN, instead of more commonly used null (but, expect this to be fixed after switching to v6+)
    static const char* const NullSymbol { nullptr };

    ::web::ws::EnumerableConfig config{root, F("magnitudes-settings")};
    config(F("values"), _magnitudes.size(), {
        {F("Correction"), [](JsonArray& out, size_t index) {
            const auto& magnitude = _magnitudes[index];
            if (_magnitudeCorrectionSupported(magnitude.type)) {
                out.add(magnitude.correction);
            } else {
                out.add(NullSymbol);
            }
        }},
        {F("Ratio"), [](JsonArray& out, size_t index) {
            const auto& magnitude = _magnitudes[index];
            if (_magnitudeRatioSupported(magnitude.type)) {
                out.add(static_cast<BaseEmonSensor*>(magnitude.sensor)->getRatio(magnitude.slot));
            } else {
                out.add(NullSymbol);
            }
        }},
        {F("ZeroThreshold"), [](JsonArray& out, size_t index) {
            const auto threshold = _magnitudes[index].zero_threshold;
            if (!std::isnan(threshold)) {
                out.add(threshold);
            } else {
                out.add(NullSymbol);
            }
        }},
        {F("MinDelta"), [](JsonArray& out, size_t index) {
            out.add(_magnitudes[index].min_delta);
        }},
        {F("MaxDelta"), [](JsonArray& out, size_t index) {
            out.add(_magnitudes[index].max_delta);
        }}
    });

    root["snsRead"] = _sensor_read_interval.count();
    root["snsInit"] = _sensor_init_interval.count();
    root["snsSave"] = _sensor_energy_tracker.every();
    root["snsReport"] = _sensor_report_every;
    root["snsRealTime"] = _sensor_real_time;
}

void _sensorWebSocketSendData(JsonObject& root) {
    ::web::ws::EnumerableConfig config{root, F("magnitudes")};
    config(F("values"), _magnitudes.size(), {
        {F("value"), [](JsonArray& out, size_t index) {
            char buffer[64];
            dtostrf(_magnitudeProcess(
                _magnitudes[index], _magnitudes[index].last),
                1, _magnitudes[index].decimals, buffer);
            out.add(buffer);
        }},
        {F("error"), [](JsonArray& out, size_t index) {
            out.add(_magnitudes[index].sensor->error());
        }},
        {F("info"), [](JsonArray& out, size_t index) {
#if NTP_SUPPORT
            if ((_magnitudes[index].type == MAGNITUDE_ENERGY) && (_sensor_energy_tracker)) {
                out.add(String(F("Last saved: "))
                    + getSetting({"eneTime", _magnitudes[index].index_global},
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

void _sensorWebSocketOnAction(uint32_t client_id, const char* action, JsonObject& data) {
    if (strcmp(action, "emon-expected") == 0) {
        auto id = data["id"].as<size_t>();
        if (id < _magnitudes.size()) {
            auto expected = data["expected"].as<float>();
            wsPost([client_id, id, expected](JsonObject& root) {
                const auto& magnitude = _magnitudes[id];

                String key { F("result:") };
                key += _magnitudeSettingsRatioKey(magnitude).value();

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
    for (auto* sensor [[gnu::unused]] : _sensors) {
        if (_sensorIsEmon(sensor)) {
            wsPayloadModule(root, "emon");
        }

        switch (sensor->getID()) {
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

// Entries specific to the sensor_magnitude_t; type, info, description

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
    ::web::ws::EnumerableConfig config{root, F("magnitudes-module")};

    auto& container = config.root();
    container[F("prefix")] = prefix;

    config(F("values"), _magnitudes.size(), {
        {F("type"), [](JsonArray& out, size_t index) {
            out.add(_magnitudes[index].type);
        }},
        {F("index_global"), [](JsonArray& out, size_t index) {
            out.add(_magnitudes[index].index_global);
        }},
        {F("index_module"), callback}
    });
}

#endif // WEB_SUPPORT

#if API_SUPPORT

namespace {

String _sensorApiMagnitudeName(sensor_magnitude_t& magnitude) {
    String name = _magnitudeTopic(magnitude.type);
    if (SENSOR_USE_INDEX || (sensor_magnitude_t::counts(magnitude.type) > 1)) name = name + "/" + String(magnitude.index_global);

    return name;
}

bool _sensorApiTryParseMagnitudeIndex(const char* p, unsigned char type, unsigned char& magnitude_index) {
    char* endp { nullptr };
    const unsigned long result { strtoul(p, &endp, 10) };
    if ((endp == p) || (*endp != '\0') || (result >= sensor_magnitude_t::counts(type))) {
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
                data.add(_sensorApiMagnitudeName(magnitude));
                data.add(magnitude.last);
                data.add(magnitude.reported);
            }
            return true;
        },
        nullptr
    );

    _magnitudeForEachCounted([](unsigned char type) {
        String pattern = _magnitudeTopic(type);
        if (SENSOR_USE_INDEX || (sensor_magnitude_t::counts(type) > 1)) {
            pattern += "/+";
        }

        ApiBasicHandler get {
            [type](ApiRequest& request) {
                return _sensorApiTryHandle(request, type, [&](const sensor_magnitude_t& magnitude) {
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
                return _sensorApiTryHandle(request, MAGNITUDE_ENERGY, [&](const sensor_magnitude_t& magnitude) {
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
            if (index >= sensor_magnitude_t::counts(MAGNITUDE_ENERGY)) break;

            for (auto& magnitude : _magnitudes) {
                if (MAGNITUDE_ENERGY != magnitude.type) continue;
                if (index != magnitude.index_global) continue;
                _sensorApiResetEnergy(magnitude, static_cast<const char*>(payload));
                break;
            }
        }
        case MQTT_CONNECT_EVENT: {
            for (auto& magnitude : _magnitudes) {
                if (MAGNITUDE_ENERGY == magnitude.type) {
                    const String topic = energy_topic + "/+";
                    mqttSubscribe(topic.c_str());
                    break;
                }
            }
        }
        case MQTT_DISCONNECT_EVENT:
        default:
            break;
    }
}

} // namespace

#endif // MQTT_SUPPORT == 1

#if TERMINAL_SUPPORT

namespace {

void _sensorInitCommands() {
    terminalRegisterCommand(F("MAGNITUDES"), [](::terminal::CommandContext&& ctx) {
        char last[64];
        char reported[64];
        for (size_t index = 0; index < _magnitudes.size(); ++index) {
            auto& magnitude = _magnitudes.at(index);
            dtostrf(magnitude.last, 1, magnitude.decimals, last);
            dtostrf(magnitude.reported, 1, magnitude.decimals, reported);
            ctx.output.printf_P(PSTR("%u * %s/%u @ %s (last:%s, reported:%s)\n"),
                index, _magnitudeTopic(magnitude.type).c_str(), magnitude.index_global,
                _magnitudeDescription(magnitude).c_str(), last, reported);
        }
        terminalOK();
    });

    terminalRegisterCommand(F("EXPECTED"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() == 3) {
            const auto id = settings::internal::convert<size_t>(ctx.argv[1]);
            if (id < _magnitudes.size()) {
                const auto result = _sensorApiEmonExpectedValue(_magnitudes[id],
                    settings::internal::convert<double>(ctx.argv[2]));
                const auto key = _magnitudeSettingsRatioKey(_magnitudes[id]);
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
        using IndexType = decltype(sensor_magnitude_t::index_global);

        if (ctx.argv.size() == 3) {
            const auto selected = settings::internal::convert<IndexType>(ctx.argv[1]);
            const auto energy = _sensorParseEnergy(ctx.argv[2]);

            if (!energy) {
                terminalError(ctx, F("Invalid energy string"));
                return;
            }

            for (auto& magnitude : _magnitudes) {
                if ((MAGNITUDE_ENERGY == magnitude.type) && (selected == magnitude.index_global) && _sensorIsEmon(magnitude.sensor)) {
                    static_cast<BaseEmonSensor*>(magnitude.sensor)->resetEnergy(magnitude.slot, energy.value());
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
    for (auto* sensor : _sensors) {
        sensor->tick();
    }
}

void _sensorPre() {
    for (auto* sensor : _sensors) {
        sensor->pre();
        if (!sensor->status()) {
            DEBUG_MSG_P(PSTR("[SENSOR] Error reading data from %s (error: %d)\n"),
                sensor->description().c_str(),
                sensor->error()
            );
        }
    }
}

void _sensorPost() {
    for (auto* sensor : _sensors) {
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
    - as `src_build_flags = -DDHT2_PIN=... -DDHT2_TYPE=...`
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
        // Support up to two sensors with full auto-discovery.
        const unsigned char number = constrain(getSetting("bmx280Number", BMX280_NUMBER), 1, 2);

        // For second sensor, if BMX280_ADDRESS is 0x00 then auto-discover
        // otherwise choose the other unnamed sensor address
        const auto first = getSetting("bmx280Address", BMX280_ADDRESS);
        const auto second = (first == 0x00) ? 0x00 : (0x76 + 0x77 - first);

        const decltype(first) address_map[2] { first, second };

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
        auto getPin = [](unsigned char index) -> int {
            switch (index) {
            case 0: return EVENTS1_PIN;
            case 1: return EVENTS2_PIN;
            case 2: return EVENTS3_PIN;
            case 3: return EVENTS4_PIN;
            case 4: return EVENTS5_PIN;
            case 5: return EVENTS6_PIN;
            case 6: return EVENTS7_PIN;
            case 7: return EVENTS8_PIN;
            default: return GPIO_NONE;
            }
        };

        auto getMode = [](unsigned char index) -> int {
            switch (index) {
            case 0: return EVENTS1_PIN_MODE;
            case 1: return EVENTS2_PIN_MODE;
            case 2: return EVENTS3_PIN_MODE;
            case 3: return EVENTS4_PIN_MODE;
            case 4: return EVENTS5_PIN_MODE;
            case 5: return EVENTS6_PIN_MODE;
            case 6: return EVENTS7_PIN_MODE;
            case 7: return EVENTS8_PIN_MODE;
            default: return INPUT;
            }
        };

        auto getDebounce = [](unsigned char index) -> unsigned long {
            switch (index) {
            case 0: return EVENTS1_DEBOUNCE;
            case 1: return EVENTS2_DEBOUNCE;
            case 2: return EVENTS3_DEBOUNCE;
            case 3: return EVENTS4_DEBOUNCE;
            case 4: return EVENTS5_DEBOUNCE;
            case 5: return EVENTS6_DEBOUNCE;
            case 6: return EVENTS7_DEBOUNCE;
            case 7: return EVENTS8_DEBOUNCE;
            default: return 50;
            }
        };

        auto getIsrMode = [](unsigned char index) -> int {
            switch (index) {
            case 0: return EVENTS1_INTERRUPT_MODE;
            case 1: return EVENTS2_INTERRUPT_MODE;
            case 2: return EVENTS3_INTERRUPT_MODE;
            case 3: return EVENTS4_INTERRUPT_MODE;
            case 4: return EVENTS5_INTERRUPT_MODE;
            case 5: return EVENTS6_INTERRUPT_MODE;
            case 6: return EVENTS7_INTERRUPT_MODE;
            case 7: return EVENTS8_INTERRUPT_MODE;
            default: return RISING;
            }
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
        GeigerSensor * sensor = new GeigerSensor();        // Create instance of thr Geiger module.
        sensor->setGPIO(GEIGER_PIN);                       // Interrupt pin of the attached geiger counter board.
        sensor->setMode(GEIGER_PIN_MODE);                  // This pin is an input.
        sensor->setDebounceTime(GEIGER_DEBOUNCE);          // Debounce time 25ms, because https://github.com/Trickx/espurna/wiki/Geiger-counter
        sensor->setInterruptMode(GEIGER_INTERRUPT_MODE);   // Interrupt triggering: edge detection rising.
        sensor->setCPM2SievertFactor(GEIGER_CPM2SIEVERT);  // Conversion factor from counts per minute to µSv/h
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
        sensor->setAnalogGPIO(MICS2710_NOX_PIN);
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
        sensor->setAnalogGPIO(MICS5525_RED_PIN);
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
        sensor->setDebounceTime(PULSEMETER_DEBOUNCE);
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
        sensor->setInterMeasurementPeriod(VL53L1X_INTER_MEASUREMENT_PERIOD);
        sensor->setDistanceMode(VL53L1X_DISTANCE_MODE);
        sensor->setMeasurementTimingBudget(VL53L1X_MEASUREMENT_TIMING_BUDGET);
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

String _magnitudeTopicIndex(const sensor_magnitude_t& magnitude) {
    char buffer[32] = {0};

    String topic { _magnitudeTopic(magnitude.type) };
    if (SENSOR_USE_INDEX || (sensor_magnitude_t::counts(magnitude.type) > 1)) {
        snprintf(buffer, sizeof(buffer), "%s/%u", topic.c_str(), magnitude.index_global);
    } else {
        snprintf(buffer, sizeof(buffer), "%s", topic.c_str());
    }

    return String(buffer);
}

void _sensorReport(unsigned char index, const sensor_magnitude_t& magnitude) {

    // XXX: dtostrf only handles basic floating point values and will never produce scientific notation
    //      ensure decimals is within some sane limit and the actual value never goes above this buffer size
    char buffer[64];
    dtostrf(magnitude.reported, 1, magnitude.decimals, buffer);

    for (auto& handler : _magnitude_report_handlers) {
        handler(_magnitudeTopic(magnitude.type), magnitude.index_global, magnitude.reported, buffer);
    }

#if MQTT_SUPPORT
    {
        const String topic(_magnitudeTopicIndex(magnitude));
        mqttSend(topic.c_str(), buffer);

#if SENSOR_PUBLISH_ADDRESSES
        String address_topic;
        address_topic.reserve(topic.length() + 1 + strlen(SENSOR_ADDRESS_TOPIC));

        address_topic += F(SENSOR_ADDRESS_TOPIC);
        address_topic += '/';
        address_topic += topic;

        mqttSend(address_topic.c_str(), magnitude.sensor->address(magnitude.slot).c_str());
#endif // SENSOR_PUBLISH_ADDRESSES

    }
#endif // MQTT_SUPPORT

    // TODO: both integrations depend on the absolute index instead of specific type
    //       so, we still need to pass / know the 'global' index inside of _magnitudes[]

#if THINGSPEAK_SUPPORT
    tspkEnqueueMeasurement(index, buffer);
#endif // THINGSPEAK_SUPPORT

#if DOMOTICZ_SUPPORT
    domoticzSendMagnitude(magnitude.type, index, magnitude.reported, buffer);
#endif // DOMOTICZ_SUPPORT

}

void _sensorInit() {
    _sensors_ready = true;

    for (auto& sensor : _sensors) {

        // Do not process an already initialized sensor
        if (sensor->ready()) {
            continue;
        }

        // Force sensor to reload config
        DEBUG_MSG_P(PSTR("[SENSOR] Initializing %s\n"), sensor->description().c_str());
        sensor->begin();

        if (!sensor->ready()) {
            if (0 != sensor->error()) {
                DEBUG_MSG_P(PSTR("[SENSOR]  -> ERROR %d\n"), sensor->error());
            }
            _sensors_ready = false;
            break;
        }

        // Initialize sensor magnitudes
        for (unsigned char magnitude_slot = 0; magnitude_slot < sensor->count(); ++magnitude_slot) {
            const auto magnitude_type = sensor->type(magnitude_slot);
            _magnitudes.emplace_back(
                magnitude_slot,      // id of the magnitude, unique to the sensor
                magnitude_type,      // cache type as well, no need to call type(slot) again
                sensor::Unit::None,  // set by configuration, default for now
                sensor               // bind the sensor to allow us to reference it later
            );
        }

        // Custom initializations for analog sensors
        // (but, notice that this is global across all sensors of this type!)
        if (_sensorIsAnalog(sensor)) {
            _sensorAnalogInit(static_cast<BaseAnalogSensor*>(sensor));
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
            // process emon-specific settings first. ensure that settings use global index and we access sensor with the local one
            if (_sensorIsEmon(magnitude.sensor) && _magnitudeRatioSupported(magnitude.type)) {
                auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor);
                sensor->setRatio(magnitude.slot, _magnitudeSettingsRatio(magnitude, sensor->defaultRatio(magnitude.slot)));
            }

            // analog variant of emon sensor has some additional settings
            if (_sensorIsAnalogEmon(magnitude.sensor) && (magnitude.type == MAGNITUDE_VOLTAGE)) {
                auto* sensor = static_cast<BaseAnalogEmonSensor*>(magnitude.sensor);
                sensor->setVoltage(
                    getSetting({_magnitudeSettingsKey(magnitude, F("Mains")), magnitude.index_global},
                    sensor->defaultVoltage()));
                sensor->setReferenceVoltage(
                    getSetting({_magnitudeSettingsKey(magnitude, F("Reference")), magnitude.index_global},
                    sensor->defaultReferenceVoltage()));
            }

            // adjust units based on magnitude's type
            {
                const sensor::Unit default_unit { magnitude.sensor->units(magnitude.slot) };
                const String key {
                    String(_magnitudeSettingsPrefix(magnitude.type)) + F("Units") + String(magnitude.index_global, 10) };

                magnitude.units = _magnitudeUnitFilter(magnitude, getSetting(key, default_unit));
            }

            // adjust resulting value (simple plus or minus)
            // TODO: inject math or rpnlib expression?
            {
                if (_magnitudeCorrectionSupported(magnitude.type)) {
                    auto key = String(_magnitudeSettingsPrefix(magnitude.type)) + F("Correction");
                    magnitude.correction = getSetting({key, magnitude.index_global}, getSetting(key, _magnitudeCorrection(magnitude.type)));
                }
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
            {
                magnitude.min_delta = getSetting(
                    {_magnitudeSettingsKey(magnitude, F("MinDelta")), magnitude.index_global},
                    sensor::build::DefaultMinDelta
                );
                magnitude.max_delta = getSetting(
                    {_magnitudeSettingsKey(magnitude, F("MaxDelta")), magnitude.index_global},
                    sensor::build::DefaultMaxDelta
                );
            }

            // Sometimes we want to ensure the value is above certain threshold before reporting
            {
                magnitude.zero_threshold = getSetting(
                    {_magnitudeSettingsKey(magnitude, F("ZeroThreshold")), magnitude.index_global},
                    std::numeric_limits<double>::quiet_NaN()
                );
            }

            // in case we don't save energy periodically, purge existing value in ram & settings
            if ((MAGNITUDE_ENERGY == magnitude.type) && (0 == _sensor_energy_tracker.every())) {
                _sensorResetEnergyTotal(magnitude.index_global);
            }

        }
    }

}

#if SENSOR_DEBUG
void _sensorDebugSetup() {
    _magnitude_read_handlers.push_back([](const String& topic, unsigned char index, double, const char* repr) {
        DEBUG_MSG_P(PSTR("[SENSOR] %s/%hhu -> %s (%s)\n"),
            topic.c_str(), index, repr, _magnitudeUnits(_magnitudes[index]));
    });
}
#endif

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

double sensor::Value::get() {
    return real_time ? last : reported;
}

sensor::Value magnitudeValue(unsigned char index) {
    sensor::Value result;

    if (index < _magnitudes.size()) {
        const auto& magnitude = _magnitudes[index];
        result.real_time = _sensor_real_time;
        result.last = magnitude.last;
        result.reported = magnitude.reported;
        result.decimals = magnitude.decimals;
    } else {
        result.real_time = false;
        result.last = std::numeric_limits<double>::quiet_NaN(),
        result.reported = std::numeric_limits<double>::quiet_NaN(),
        result.decimals = 0u;
    }

    return result;
}

void magnitudeFormat(const sensor::Value& value, char* out, size_t) {
    // TODO: 'size' does not do anything, since dtostrf used here is expected to be 'sane', but
    //       it does not allow any size arguments besides for digits after the decimal point
    dtostrf(
        _sensor_real_time ? value.last : value.reported,
        1, value.decimals,
        out
    );
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
    // Some keys from older versions were longer
    if (version < 3) {
        moveSetting("powerUnits", "pwrUnits");
        moveSetting("energyUnits", "eneUnits");
    }

    // Energy is now indexed (based on magnitude.index_global)
	// Also update PZEM004T energy total across multiple devices
    if (version < 5) {
        moveSetting("eneTotal", "eneTotal0");
        moveSettings("pzEneTotal", "eneTotal");
    }

    // Unit ID is no longer shared, drop when equal to Min_ or None
    if (version < 5) {
        delSetting("pwrUnits");
        delSetting("eneUnits");
        delSetting("tmpUnits");
    }

    // Generic pwr settings now have type-specific prefixes
    // (index 0, assuming there's only one emon sensor)
    if (version < 7) {
        moveSetting(F("pwrVoltage"), _magnitudeSettingsKey(MAGNITUDE_VOLTAGE, F("Mains0")));
        moveSetting(F("pwrRatioC"), _magnitudeSettingsRatioKey(MAGNITUDE_CURRENT, 0).value());
        moveSetting(F("pwrRatioV"), _magnitudeSettingsRatioKey(MAGNITUDE_VOLTAGE, 0).value());
        moveSetting(F("pwrRatioP"), _magnitudeSettingsRatioKey(MAGNITUDE_POWER_ACTIVE, 0).value());
        moveSetting(F("pwrRatioE"), _magnitudeSettingsRatioKey(MAGNITUDE_ENERGY, 0).value());
    }

#if HLW8012_SUPPORT
    if (version < 9) {
        moveSetting(F("snsHlw8012SelGPIO"), F("hlw8012SEL"));
        moveSetting(F("snsHlw8012CfGPIO"), F("hlw8012CF"));
        moveSetting(F("snsHlw8012Cf1GPIO"), F("hlw8012CF1"));
    }
#endif

    if (version < 11) {
        moveSetting(F("apiRealTime"), F("snsRealTime"));
        moveSetting(F("tmpMinDelta"), _magnitudeSettingsKey(MAGNITUDE_TEMPERATURE, F("MinDelta0")));
        moveSetting(F("humMinDelta"), _magnitudeSettingsKey(MAGNITUDE_HUMIDITY, F("MinDelta0")));
        moveSetting(F("eneMaxDelta"), _magnitudeSettingsKey(MAGNITUDE_ENERGY, F("MaxDelta0")));
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
    _magnitudeForEachCounted([](unsigned char type) {
        if (_magnitudeRatioSupported(type)) {
            settingsRegisterDefaults(_magnitudeSettingsPrefix(type), _sensorQueryDefault);
        }
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

    #if SENSOR_DEBUG
        _sensorDebugSetup();
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
    static int report_count { 0 };

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

        // Get readings
        for (unsigned char magnitude_index = 0; magnitude_index < _magnitudes.size(); ++magnitude_index) {

            auto& magnitude = _magnitudes[magnitude_index];

            if (!magnitude.sensor->status()) continue;

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
            magnitude.filter->add(value.raw);

            // -------------------------------------------------------------
            // Procesing (units and decimals)
            // -------------------------------------------------------------

            value.processed = _magnitudeProcess(magnitude, value.raw);
            {
                char buffer[64];
                dtostrf(value.processed, 1, magnitude.decimals, buffer);
                for (auto& handler : _magnitude_read_handlers) {
                    handler(_magnitudeTopic(magnitude.type), magnitude.index_global, value.processed, buffer);
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
                value.filtered = _magnitudeProcess(magnitude, magnitude.filter->result());

                // Make sure that report value is calculated using every read value before it
                magnitude.filter->reset();
                if (magnitude.filter->size() != _sensor_report_every) {
                    magnitude.filter->resize(_sensor_report_every);
                }

                // Check ${name}MinDelta if there is a minimum change threshold to report
                if (std::isnan(magnitude.reported) || (std::abs(value.filtered - magnitude.reported) >= magnitude.min_delta)) {
                    magnitude.reported = value.filtered;
                    _sensorReport(magnitude_index, magnitude);
                }

            }

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
