/*

SENSOR MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "sensor.h"

#if SENSOR_SUPPORT

#include "api.h"
#include "broker.h"
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

// TODO: namespace { ... } ? sensor ctors need to work though

#include "filters/LastFilter.h"
#include "filters/MaxFilter.h"
#include "filters/MedianFilter.h"
#include "filters/MovingAverageFilter.h"
#include "filters/SumFilter.h"

#include "sensors/BaseSensor.h"
#include "sensors/BaseEmonSensor.h"
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

struct sensor_magnitude_t {

    private:

    constexpr static double _unset = std::numeric_limits<double>::quiet_NaN();
    static unsigned char _counts[MAGNITUDE_MAX];

    sensor_magnitude_t& operator=(const sensor_magnitude_t&) = default;

    void move(sensor_magnitude_t&& other) noexcept {
        *this = other;
        other.filter = nullptr;
    }

    public:

    static unsigned char counts(unsigned char type) {
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

    sensor_magnitude_t(unsigned char slot, unsigned char index_local, unsigned char type, sensor::Unit units, BaseSensor* sensor);

    BaseSensor * sensor { nullptr }; // Sensor object
    BaseFilter * filter { nullptr }; // Filter object

    unsigned char slot { 0u }; // Sensor slot # taken by the magnitude, used to access the measurement
    unsigned char type { MAGNITUDE_NONE }; // Type of measurement, returned by the BaseSensor::type(slot)

    unsigned char index_local { 0u };  // N'th magnitude of it's type, local to the sensor
    unsigned char index_global { 0u }; // ... and across all of the active sensors

    sensor::Unit units { sensor::Unit::None }; // Units of measurement
    unsigned char decimals { 0u }; // Number of decimals in textual representation

    double last { _unset };     // Last raw value from sensor (unfiltered)
    double reported { _unset }; // Last reported value
    double min_change { 0.0 };  // Minimum value change to report
    double max_change { 0.0 };  // Maximum value change to report
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

Energy::operator bool() {
    return (kwh.value > 0) && (ws.value > 0);
}

Ws Energy::asWs() {
    auto _kwh = kwh.value;
    while (_kwh >= KwhLimit) {
        _kwh -= KwhLimit;
    }

    return (_kwh * KwhMultiplier) + ws.value;
}

double Energy::asDouble() {
    return (double)kwh.value + ((double)ws.value / (double)KwhMultiplier);
}

void Energy::reset() {
    kwh.value = 0;
    ws.value = 0;
}

} // namespace sensor

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

constexpr bool _magnitudeCanUseCorrection(unsigned char type) {
    return (
        (MAGNITUDE_TEMPERATURE == type) ? (true) :
        (MAGNITUDE_HUMIDITY == type) ? (true) :
        (MAGNITUDE_LUX == type) ? (true) :
        (MAGNITUDE_PRESSURE == type) ? (true) :
        false
    );
}

// -----------------------------------------------------------------------------
// Energy persistence
// -----------------------------------------------------------------------------

std::vector<unsigned char> _sensor_save_count;
unsigned char _sensor_save_every = SENSOR_SAVE_EVERY;

bool _sensorIsEmon(BaseSensor* sensor) {
    return sensor->type() & sensor::type::Emon;
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

sensor::Energy _sensorParseEnergy(const String& value) {
    sensor::Energy result;

    const bool separator = value.indexOf('+') > 0;
    if (value.length() && (separator > 0)) {
        const String before = value.substring(0, separator);
        const String after = value.substring(separator + 1);
        result.kwh = strtoul(before.c_str(), nullptr, 10);
        result.ws = strtoul(after.c_str(), nullptr, 10);
    }

    return result;
}

void _sensorApiResetEnergy(const sensor_magnitude_t& magnitude, const char* payload) {
    if (!payload || !strlen(payload)) return;

    auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor);
    auto energy = _sensorParseEnergy(payload);

    sensor->resetEnergy(magnitude.index_local, energy);
}

sensor::Energy _sensorEnergyTotal(unsigned char index) {

    sensor::Energy result;

    if (rtcmemStatus() && (index < (sizeof(Rtcmem->energy) / sizeof(*Rtcmem->energy)))) {
        result = _sensorRtcmemLoadEnergy(index);
    } else if (_sensor_save_every > 0) {
        result = _sensorParseEnergy(getSetting({"eneTotal", index}));
    }

    return result;

}

sensor::Energy sensorEnergyTotal() {
    return _sensorEnergyTotal(0);
}

void _sensorResetEnergyTotal(unsigned char index) {
    delSetting({"eneTotal", index});
    delSetting({"eneTime", index});
    if (index < (sizeof(Rtcmem->energy) / sizeof(*Rtcmem->energy))) {
        Rtcmem->energy[index].kwh = 0;
        Rtcmem->energy[index].ws = 0;
    }
}

void _magnitudeSaveEnergyTotal(sensor_magnitude_t& magnitude, bool persistent) {
    if (magnitude.type != MAGNITUDE_ENERGY) return;

    auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor);

    const auto energy = sensor->totalEnergy();

    // Always save to RTCMEM
    if (magnitude.index_global < (sizeof(Rtcmem->energy) / sizeof(*Rtcmem->energy))) {
        _sensorRtcmemSaveEnergy(magnitude.index_global, energy);
    }

    // Save to EEPROM every '_sensor_save_every' readings
    // Format is `<kwh>+<ws>`, value without `+` is treated as `<ws>`
    if (persistent && _sensor_save_every) {
        _sensor_save_count[magnitude.index_global] =
            (_sensor_save_count[magnitude.index_global] + 1) % _sensor_save_every;

        if (0 == _sensor_save_count[magnitude.index_global]) {
            const String total = String(energy.kwh.value) + "+" + String(energy.ws.value);
            setSetting({"eneTotal", magnitude.index_global}, total);
            #if NTP_SUPPORT
                if (ntpSynced()) setSetting({"eneTime", magnitude.index_global}, ntpDateTime());
            #endif
        }
    }
}

// ---------------------------------------------------------------------------

BrokerBind(SensorReadBroker);
BrokerBind(SensorReportBroker);

std::vector<BaseSensor *> _sensors;
std::vector<sensor_magnitude_t> _magnitudes;
bool _sensors_ready = false;

bool _sensor_realtime = API_REAL_TIME_VALUES;
unsigned long _sensor_read_interval = 1000 * SENSOR_READ_INTERVAL;
unsigned char _sensor_report_every = SENSOR_REPORT_EVERY;

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

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

sensor_magnitude_t::sensor_magnitude_t(unsigned char slot_, unsigned char index_local_, unsigned char type_, sensor::Unit units_, BaseSensor* sensor_) :
    sensor(sensor_),
    filter(_magnitudeCreateFilter(type_, _sensor_report_every)),
    slot(slot_),
    type(type_),
    index_local(index_local_),
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

String magnitudeTopic(unsigned char type) {

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
        case MAGNITUDE_NONE:
        default:
            result = F("unknown");
            break;
    }

    return String(result);

}

String _magnitudeTopic(const sensor_magnitude_t& magnitude) {
    return magnitudeTopic(magnitude.type);
}

String _magnitudeUnits(const sensor_magnitude_t& magnitude) {

    const __FlashStringHelper* result = nullptr;

    switch (magnitude.units) {
        case sensor::Unit::Farenheit:
            result = F("°F");
            break;
        case sensor::Unit::Celcius:
            result = F("°C");
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
        case sensor::Unit::None:
        default:
            result = F("");
            break;
    }

    return String(result);

}

String magnitudeUnits(unsigned char index) {
    if (index >= magnitudeCount()) return String();
    return _magnitudeUnits(_magnitudes[index]);
}

// Choose unit based on type of magnitude we use

sensor::Unit _magnitudeUnitFilter(const sensor_magnitude_t& magnitude, sensor::Unit updated) {
    auto result = magnitude.units;

    switch (magnitude.type) {

    case MAGNITUDE_TEMPERATURE: {
        switch (updated) {
        case sensor::Unit::Celcius:
        case sensor::Unit::Farenheit:
        case sensor::Unit::Kelvin:
            result = updated;
            break;
        default:
            break;
        }
        break;
    }

    case MAGNITUDE_POWER_ACTIVE: {
        switch (updated) {
        case sensor::Unit::Kilowatt:
        case sensor::Unit::Watt:
            result = updated;
            break;
        default:
            break;
        }
        break;
    }

    case MAGNITUDE_ENERGY: {
       switch (updated) {
       case sensor::Unit::KilowattHour:
       case sensor::Unit::Joule:
           result = updated;
           break;
       default:
           break;
       }
       break;
    }

    default:
        result = updated;
        break;

    }

    return result;
};

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
template<typename T>
void _magnitudeForEachCounted(T callback) {
    for (unsigned char type = MAGNITUDE_NONE + 1; type < MAGNITUDE_MAX; ++type) {
        if (sensor_magnitude_t::counts(type)) {
            callback(type);
        }
    }
}

// check if `callback(type)` returns `true` at least once
template<typename T>
bool _magnitudeForEachCountedCheck(T callback) {
    for (unsigned char type = MAGNITUDE_NONE + 1; type < MAGNITUDE_MAX; ++type) {
        if (sensor_magnitude_t::counts(type) && callback(type)) {
            return true;
        }
    }

    return false;
}

// do `callback(type)` for each error type
template<typename T>
void _sensorForEachError(T callback) {
    for (unsigned char error = SENSOR_ERROR_OK; error < SENSOR_ERROR_MAX; ++error) {
        callback(error);
    }
}

const char * const _magnitudeSettingsPrefix(unsigned char type) {
    switch (type) {
    case MAGNITUDE_TEMPERATURE: return "tmp";
    case MAGNITUDE_HUMIDITY: return "hum";
    case MAGNITUDE_PRESSURE: return "press";
    case MAGNITUDE_CURRENT: return "curr";
    case MAGNITUDE_VOLTAGE: return "volt";
    case MAGNITUDE_POWER_ACTIVE: return "pwrP";
    case MAGNITUDE_POWER_APPARENT: return "pwrQ";
    case MAGNITUDE_POWER_REACTIVE: return "pwrModS";
    case MAGNITUDE_POWER_FACTOR: return "pwrPF";
    case MAGNITUDE_ENERGY: return "ene";
    case MAGNITUDE_ENERGY_DELTA: return "eneDelta";
    case MAGNITUDE_ANALOG: return "analog";
    case MAGNITUDE_DIGITAL: return "digital";
    case MAGNITUDE_EVENT: return "event";
    case MAGNITUDE_PM1dot0: return "pm1dot0";
    case MAGNITUDE_PM2dot5: return "pm1dot5";
    case MAGNITUDE_PM10: return "pm10";
    case MAGNITUDE_CO2: return "co2";
    case MAGNITUDE_VOC: return "voc";
    case MAGNITUDE_IAQ: return "iaq";
    case MAGNITUDE_IAQ_ACCURACY: return "iaqAccuracy";
    case MAGNITUDE_IAQ_STATIC: return "iaqStatic";
    case MAGNITUDE_LUX: return "lux";
    case MAGNITUDE_UVA: return "uva";
    case MAGNITUDE_UVB: return "uvb";
    case MAGNITUDE_UVI: return "uvi";
    case MAGNITUDE_DISTANCE: return "distance";
    case MAGNITUDE_HCHO: return "hcho";
    case MAGNITUDE_GEIGER_CPM: return "gcpm";
    case MAGNITUDE_GEIGER_SIEVERT: return "gsiev";
    case MAGNITUDE_COUNT: return "count";
    case MAGNITUDE_NO2: return "no2";
    case MAGNITUDE_CO: return "co";
    case MAGNITUDE_RESISTANCE: return "res";
    case MAGNITUDE_PH: return "ph";
    case MAGNITUDE_FREQUENCY: return "freq";
    default: return nullptr;
    }
}

template <typename T>
String _magnitudeSettingsKey(sensor_magnitude_t& magnitude, T&& suffix) {
    return String(_magnitudeSettingsPrefix(magnitude.type)) + suffix;
}

bool _sensorMatchKeyPrefix(const char * key) {

    if (strncmp(key, "sns", 3) == 0) return true;
    if (strncmp(key, "pwr", 3) == 0) return true;

    return _magnitudeForEachCountedCheck([key](unsigned char type) {
        const char* const prefix { _magnitudeSettingsPrefix(type) };
        return (strncmp(prefix, key, strlen(prefix)) == 0);
    });

}

const String _sensorQueryDefault(const String& key) {

    auto get_defaults = [](unsigned char type, BaseSensor* ptr) -> String {
        if (!ptr) return String();
        auto* sensor = static_cast<BaseEmonSensor*>(ptr);
        switch (type) {
        case MAGNITUDE_CURRENT:
            return String(sensor->defaultCurrentRatio());
        case MAGNITUDE_VOLTAGE:
            return String(sensor->defaultVoltageRatio());
        case MAGNITUDE_POWER_ACTIVE:
            return String(sensor->defaultPowerRatio());
        case MAGNITUDE_ENERGY:
            return String(sensor->defaultEnergyRatio());
        default:
            return String();
        }
    };

    auto magnitude_key = [](const sensor_magnitude_t& magnitude) -> settings_key_t {
        switch (magnitude.type) {
        case MAGNITUDE_CURRENT:
            return {"pwrRatioC", magnitude.index_global};
        case MAGNITUDE_VOLTAGE:
            return {"pwrRatioV", magnitude.index_global};
        case MAGNITUDE_POWER_ACTIVE:
            return {"pwrRatioP", magnitude.index_global};
        case MAGNITUDE_ENERGY:
            return {"pwrRatioE", magnitude.index_global};
        default:
            return {};
        }
    };

    unsigned char type = MAGNITUDE_NONE;
    BaseSensor* target = nullptr;

    for (auto& magnitude : _magnitudes) {
        switch (magnitude.type) {
            case MAGNITUDE_CURRENT:
            case MAGNITUDE_VOLTAGE:
            case MAGNITUDE_POWER_ACTIVE:
            case MAGNITUDE_ENERGY: {
                auto ratioKey(magnitude_key(magnitude));
                if (ratioKey.match(key)) {
                    target = magnitude.sensor;
                    type = magnitude.type;
                    goto return_defaults;
                }
                break;
            }
            default:
                break;
        }
    }

return_defaults:

    return get_defaults(type, target);

}

#if WEB_SUPPORT

bool _sensorWebSocketOnKeyCheck(const char* key, JsonVariant&) {
    return _sensorMatchKeyPrefix(key);
}

// Used by modules to generate magnitude_id<->module_id mapping for the WebUI

void sensorWebSocketMagnitudes(JsonObject& root, const String& prefix) {

    // ws produces flat list <prefix>Magnitudes
    const String ws_name = prefix + "Magnitudes";

    // config uses <prefix>Magnitude<index> (cut 's')
    const String conf_name = ws_name.substring(0, ws_name.length() - 1);

    JsonObject& list = root.createNestedObject(ws_name);
    list["size"] = magnitudeCount();

    JsonArray& type = list.createNestedArray("type");
    JsonArray& index = list.createNestedArray("index");
    JsonArray& idx = list.createNestedArray("idx");

    for (unsigned char i=0; i<magnitudeCount(); ++i) {
        type.add(magnitudeType(i));
        index.add(magnitudeIndex(i));
        idx.add(getSetting({conf_name, i}, 0));
    }
}

String sensorError(unsigned char error) {

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

String magnitudeName(unsigned char type) {

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
        case MAGNITUDE_NONE:
        default:
            break;
    }

    return String(result);
}

void _sensorWebSocketOnVisible(JsonObject& root) {

    root["snsVisible"] = 1;

    // prepare available magnitude types
    JsonArray& magnitudes = root.createNestedArray("snsMagnitudes");
    _magnitudeForEachCounted([&magnitudes](unsigned char type) {
        JsonArray& tuple = magnitudes.createNestedArray();
        tuple.add(type);
        tuple.add(_magnitudeSettingsPrefix(type));
        tuple.add(magnitudeName(type));
    });

    // and available error types
    JsonArray& errors = root.createNestedArray("snsErrors");
    _sensorForEachError([&errors](unsigned char error) {
        JsonArray& tuple = errors.createNestedArray();
        tuple.add(error);
        tuple.add(sensorError(error));
    });

}

void _sensorWebSocketMagnitudesConfig(JsonObject& root) {

    // retrieve per-type ...Correction settings, when available
    _magnitudeForEachCounted([&root](unsigned char type) {
        if (_magnitudeCanUseCorrection(type)) {
            auto key = String(_magnitudeSettingsPrefix(type)) + F("Correction");
            root[key] = getSetting(key, _magnitudeCorrection(type));
        }
    });

    JsonObject& magnitudes = root.createNestedObject("magnitudesConfig");
    uint8_t size = 0;

    JsonArray& index = magnitudes.createNestedArray("index");
    JsonArray& type = magnitudes.createNestedArray("type");
    JsonArray& units = magnitudes.createNestedArray("units");
    JsonArray& description = magnitudes.createNestedArray("description");

    for (auto& magnitude : _magnitudes) {

        // TODO: we don't display event for some reason?
        if (magnitude.type == MAGNITUDE_EVENT) continue;
        ++size;

        index.add<uint8_t>(magnitude.index_global);
        type.add<uint8_t>(magnitude.type);
        units.add(_magnitudeUnits(magnitude));
        description.add(_magnitudeDescription(magnitude));

    }

    magnitudes["size"] = size;

}

void _sensorWebSocketSendData(JsonObject& root) {

    char buffer[64];

    JsonObject& magnitudes = root.createNestedObject("magnitudes");
    uint8_t size = 0;

    JsonArray& value = magnitudes.createNestedArray("value");
    JsonArray& error = magnitudes.createNestedArray("error");
    #if NTP_SUPPORT
        JsonArray& info = magnitudes.createNestedArray("info");
    #endif

    for (auto& magnitude : _magnitudes) {
        if (magnitude.type == MAGNITUDE_EVENT) continue;
        ++size;

        dtostrf(_magnitudeProcess(magnitude, magnitude.last), 1, magnitude.decimals, buffer);

        value.add(buffer);
        error.add(magnitude.sensor->error());

        #if NTP_SUPPORT
            if ((_sensor_save_every > 0) && (magnitude.type == MAGNITUDE_ENERGY)) {
                String string = F("Last saved: ");
                string += getSetting({"eneTime", magnitude.index_global}, F("(unknown)"));
                info.add(string);
            } else {
                info.add((uint8_t)0);
            }
        #endif
    }

    magnitudes["size"] = size;

}

void _sensorWebSocketOnConnected(JsonObject& root) {

    for (auto* sensor [[gnu::unused]] : _sensors) {

        if (_sensorIsEmon(sensor)) {
            root["emonVisible"] = 1;
            root["pwrVisible"] = 1;
        }

        #if EMON_ANALOG_SUPPORT
            if (sensor->getID() == SENSOR_EMON_ANALOG_ID) {
                root["pwrVoltage"] = ((EmonAnalogSensor *) sensor)->getVoltage();
            }
        #endif

        #if HLW8012_SUPPORT
            if (sensor->getID() == SENSOR_HLW8012_ID) {
                root["hlwVisible"] = 1;
            }
        #endif

        #if CSE7766_SUPPORT
            if (sensor->getID() == SENSOR_CSE7766_ID) {
                root["cseVisible"] = 1;
            }
        #endif

        #if PZEM004T_SUPPORT || PZEM004TV30_SUPPORT
            switch (sensor->getID()) {
            case SENSOR_PZEM004T_ID:
            case SENSOR_PZEM004TV30_ID:
                root["pzemVisible"] = 1;
                break;
            default:
                break;
            }
        #endif

        #if PULSEMETER_SUPPORT
            if (sensor->getID() == SENSOR_PULSEMETER_ID) {
                root["pmVisible"] = 1;
                root["pwrRatioE"] = ((PulseMeterSensor *) sensor)->getEnergyRatio();
            }
        #endif

        #if MICS2710_SUPPORT || MICS5525_SUPPORT
            switch (sensor->getID()) {
            case SENSOR_MICS2710_ID:
            case SENSOR_MICS5525_ID:
                root["micsVisible"] = 1;
                break;
            default:
                break;
            }
        #endif

    }

    if (magnitudeCount()) {
        root["snsRead"] = _sensor_read_interval / 1000;
        root["snsReport"] = _sensor_report_every;
        root["snsSave"] = _sensor_save_every;
        _sensorWebSocketMagnitudesConfig(root);
    }

}

#endif // WEB_SUPPORT

#if API_SUPPORT

String _sensorApiMagnitudeName(sensor_magnitude_t& magnitude) {
    String name = magnitudeTopic(magnitude.type);
    if (SENSOR_USE_INDEX || (sensor_magnitude_t::counts(magnitude.type) > 1)) name = name + "/" + String(magnitude.index_global);

    return name;
}

void _sensorApiJsonCallback(const Api&, JsonObject& root) {
    JsonArray& magnitudes = root.createNestedArray("magnitudes");
    for (auto& magnitude : _magnitudes) {
        JsonArray& data = magnitudes.createNestedArray();
        data.add(_sensorApiMagnitudeName(magnitude));
        data.add(magnitude.last);
        data.add(magnitude.reported);
    }
}

void _sensorApiGetValue(const Api& api, ApiBuffer& buffer) {
    auto& magnitude = _magnitudes[api.arg];
    double value = _sensor_realtime ? magnitude.last : magnitude.reported;
    dtostrf(value, 1, magnitude.decimals, buffer.data);
}

void _sensorApiResetEnergyPutCallback(const Api& api, ApiBuffer& buffer) {
    _sensorApiResetEnergy(_magnitudes[api.arg], buffer.data);
}

void _sensorApiSetup() {

    apiReserve(
        _magnitudes.size() + sensor_magnitude_t::counts(MAGNITUDE_ENERGY) + 1u
    );

    apiRegister({"magnitudes", Api::Type::Json, ApiUnusedArg, _sensorApiJsonCallback});

    for (unsigned char id = 0; id < _magnitudes.size(); ++id) {
        apiRegister({
            _sensorApiMagnitudeName(_magnitudes[id]).c_str(),
            Api::Type::Basic, id,
            _sensorApiGetValue,
            (_magnitudes[id].type == MAGNITUDE_ENERGY)
                ? _sensorApiResetEnergyPutCallback
                : nullptr
        });
    }

}

#endif // API_SUPPORT == 1

#if MQTT_SUPPORT

void _sensorMqttCallback(unsigned int type, const char* topic, char* payload) {
    static const auto energy_topic = magnitudeTopic(MAGNITUDE_ENERGY);
    switch (type) {
        case MQTT_MESSAGE_EVENT: {
            String t = mqttMagnitude((char *) topic);
            if (!t.startsWith(energy_topic)) break;

            unsigned int index = t.substring(energy_topic.length() + 1).toInt();
            if (index >= sensor_magnitude_t::counts(MAGNITUDE_ENERGY)) break;

            for (auto& magnitude : _magnitudes) {
                if (MAGNITUDE_ENERGY != magnitude.type) continue;
                if (index != magnitude.index_global) continue;
                _sensorApiResetEnergy(magnitude, payload);
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

#endif // MQTT_SUPPORT == 1

#if TERMINAL_SUPPORT

void _sensorInitCommands() {
    terminalRegisterCommand(F("MAGNITUDES"), [](const terminal::CommandContext&) {
        char last[64];
        char reported[64];
        for (size_t index = 0; index < _magnitudes.size(); ++index) {
            auto& magnitude = _magnitudes.at(index);
            dtostrf(magnitude.last, 1, magnitude.decimals, last);
            dtostrf(magnitude.reported, 1, magnitude.decimals, reported);
            DEBUG_MSG_P(PSTR("[SENSOR] %2u * %s/%u @ %s (last:%s, reported:%s)\n"),
                index,
                magnitudeTopic(magnitude.type).c_str(),
                magnitude.index_global,
                _magnitudeDescription(magnitude).c_str(),
                last, reported
            );
        }
        terminalOK();
    });
}

#endif // TERMINAL_SUPPORT == 1

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

// -----------------------------------------------------------------------------
// Sensor initialization
// -----------------------------------------------------------------------------

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

        for (unsigned char index = 0; index < GpioPins; ++index) {
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
        sensor->setReference(EMON_REFERENCE_VOLTAGE);
        sensor->setCurrentRatio(0, EMON_CURRENT_RATIO);
        _sensors.push_back(sensor);
    }
    #endif

    #if EMON_ADS1X15_SUPPORT
    {
        EmonADS1X15Sensor * sensor = new EmonADS1X15Sensor();
        sensor->setAddress(EMON_ADS1X15_I2C_ADDRESS);
        sensor->setType(EMON_ADS1X15_TYPE);
        sensor->setMask(EMON_ADS1X15_MASK);
        sensor->setGain(EMON_ADS1X15_GAIN);
        sensor->setVoltage(EMON_MAINS_VOLTAGE);
        sensor->setCurrentRatio(0, EMON_CURRENT_RATIO);
        sensor->setCurrentRatio(1, EMON_CURRENT_RATIO);
        sensor->setCurrentRatio(2, EMON_CURRENT_RATIO);
        sensor->setCurrentRatio(3, EMON_CURRENT_RATIO);
        _sensors.push_back(sensor);
    }
    #endif

    #if EMON_ANALOG_SUPPORT
    {
        EmonAnalogSensor * sensor = new EmonAnalogSensor();
        sensor->setVoltage(EMON_MAINS_VOLTAGE);
        sensor->setReference(EMON_REFERENCE_VOLTAGE);
        sensor->setCurrentRatio(0, EMON_CURRENT_RATIO);
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

        for (unsigned char index = 0; index < GpioPins; ++index) {
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
        sensor->setSEL(getSetting("snsHlw8012SelGPIO", HLW8012_SEL_PIN));
        sensor->setCF(getSetting("snsHlw8012CfGPIO", HLW8012_CF_PIN));
        sensor->setCF1(getSetting("snsHlw8012Cf1GPIO", HLW8012_CF1_PIN));
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
        sensor->setEnergyRatio(PULSEMETER_ENERGY_RATIO);
        sensor->setInterruptMode(PULSEMETER_INTERRUPT_ON);
        sensor->setDebounceTime(PULSEMETER_DEBOUNCE);
        _sensors.push_back(sensor);
    }
    #endif

    #if PZEM004T_SUPPORT
    {
        String addresses = getSetting("pzemAddr", F(PZEM004T_ADDRESSES));
        if (!addresses.length()) {
            DEBUG_MSG_P(PSTR("[SENSOR] PZEM004T Error: no addresses are configured\n"));
            return;
        }

        PZEM004TSensor * sensor = PZEM004TSensor::create();
        sensor->setAddresses(addresses.c_str());
        sensor->setRX(getSetting("pzemRX", PZEM004T_RX_PIN));
        sensor->setTX(getSetting("pzemTX", PZEM004T_TX_PIN));

        if (!getSetting("pzemSoft", 1 == PZEM004T_USE_SOFT)) {
            sensor->setSerial(& PZEM004T_HW_PORT);
        }

        _sensors.push_back(sensor);

        #if TERMINAL_SUPPORT
            pzem004tInitCommands();
        #endif
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
        PZEM004TV30Sensor * sensor = PZEM004TV30Sensor::create();

        // TODO: we need an equivalent to the `pzem.address` command
        sensor->setAddress(getSetting("pzemv30Addr", PZEM004TV30Sensor::DefaultAddress));
        sensor->setReadTimeout(getSetting("pzemv30ReadTimeout", PZEM004TV30Sensor::DefaultReadTimeout));
        sensor->setDebug(getSetting("pzemv30Debug", 1 == PZEM004TV30_DEBUG));

        bool soft = getSetting("pzemv30Soft", 1 == PZEM004TV30_USE_SOFT);

        int tx = getSetting("pzemv30TX", PZEM004TV30_TX_PIN);
        int rx = getSetting("pzemv30RX", PZEM004TV30_RX_PIN);

        // we operate only with Serial, as Serial1 cannot not receive any data
        if (!soft) {
            sensor->setStream(&Serial);
            sensor->setDescription("HwSerial");
            Serial.begin(PZEM004TV30Sensor::Baudrate);
            // Core does not allow us to begin(baud, cfg, rx, tx) / pins(rx, tx) before begin(baud)
            // b/c internal UART handler does not exist yet
            // Also see https://github.com/esp8266/Arduino/issues/2380 as to why there is flush()
            if ((tx == 15) && (rx == 13)) {
                Serial.flush();
                Serial.swap();
            }
        } else {
            auto* ptr = new SoftwareSerial(rx, tx);
            sensor->setDescription("SwSerial");
            sensor->setStream(ptr); // we don't care about lifetime
            ptr->begin(PZEM004TV30Sensor::Baudrate);
        }

        //TODO: getSetting("pzemv30*Cfg", (SW)SERIAL_8N1); ?
        //      may not be relevant, but some sources claim we need 8N2

        _sensors.push_back(sensor);
    }
    #endif

}

String _magnitudeTopicIndex(const sensor_magnitude_t& magnitude) {
    char buffer[32] = {0};

    String topic { magnitudeTopic(magnitude.type) };
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

#if BROKER_SUPPORT
    SensorReportBroker::Publish(magnitudeTopic(magnitude.type), magnitude.index_global, magnitude.reported, buffer);
#endif

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
        if (sensor->ready()) continue;
        DEBUG_MSG_P(PSTR("[SENSOR] Initializing %s\n"), sensor->description().c_str());

        // Force sensor to reload config
        sensor->begin();
        if (!sensor->ready()) {
            if (0 != sensor->error()) {
                DEBUG_MSG_P(PSTR("[SENSOR]  -> ERROR %d\n"), sensor->error());
            }
            _sensors_ready = false;
            break;
        }

        // Initialize sensor magnitudes
        for (unsigned char magnitude_index = 0; magnitude_index < sensor->count(); ++magnitude_index) {

            const auto magnitude_type = sensor->type(magnitude_index);
            const auto magnitude_local = sensor->local(magnitude_type);
            _magnitudes.emplace_back(
                magnitude_index,     // id of the magnitude, unique to the sensor
                magnitude_local,     // index_local, # of the magnitude
                magnitude_type,      // specific type of the magnitude
                sensor::Unit::None,  // set up later, in configuration
                sensor               // bind the sensor to allow us to reference it later
            );

            if (_sensorIsEmon(sensor) && (MAGNITUDE_ENERGY == magnitude_type)) {
                const auto index_global = _magnitudes.back().index_global;
                auto* ptr = static_cast<BaseEmonSensor*>(sensor);
                ptr->resetEnergy(magnitude_local, _sensorEnergyTotal(index_global));
                _sensor_save_count.push_back(0);
            }

            DEBUG_MSG_P(PSTR("[SENSOR]  -> %s:%u\n"),
                magnitudeTopic(magnitude_type).c_str(),
                sensor_magnitude_t::counts(magnitude_type)
            );

        }

        // Custom initializations are based on IDs

        switch (sensor->getID()) {
        case SENSOR_MICS2710_ID:
        case SENSOR_MICS5525_ID: {
            auto* ptr = static_cast<BaseAnalogSensor*>(sensor);
            ptr->setR0(getSetting("snsR0", ptr->getR0()));
            ptr->setRS(getSetting("snsRS", ptr->getRS()));
            ptr->setRL(getSetting("snsRL", ptr->getRL()));
            break;
        }
        default:
            break;
        }

    }

}

namespace settings {
namespace internal {

template <>
sensor::Unit convert(const String& string) {
    const int value = string.toInt();
    if ((value > static_cast<int>(sensor::Unit::Min_)) && (value < static_cast<int>(sensor::Unit::Max_))) {
        return static_cast<sensor::Unit>(value);
    }

    return sensor::Unit::None;
}

template <>
String serialize(const sensor::Unit& unit) {
    return String(static_cast<int>(unit));
}

} // ns settings::internal
} // ns settings

void _sensorConfigure() {

    // General sensor settings for reporting and saving
    _sensor_read_interval = 1000 * constrain(getSetting("snsRead", SENSOR_READ_INTERVAL), SENSOR_READ_MIN_INTERVAL, SENSOR_READ_MAX_INTERVAL);
    _sensor_report_every = constrain(getSetting("snsReport", SENSOR_REPORT_EVERY), SENSOR_REPORT_MIN_EVERY, SENSOR_REPORT_MAX_EVERY);
    _sensor_save_every = getSetting("snsSave", SENSOR_SAVE_EVERY);

    _sensor_realtime = getSetting("apiRealTime", 1 == API_REAL_TIME_VALUES);

    // pre-load some settings that are controlled via old build flags
    const auto tmp_min_delta = getSetting("tmpMinDelta", TEMPERATURE_MIN_CHANGE);
    const auto hum_min_delta = getSetting("humMinDelta", HUMIDITY_MIN_CHANGE);
    const auto ene_max_delta = getSetting("eneMaxDelta", ENERGY_MAX_CHANGE);

    // Apply settings based on sensor type
    for (unsigned char index = 0; index < _sensors.size(); ++index) {

        #if MICS2710_SUPPORT || MICS5525_SUPPORT
        {
            if (getSetting("snsResetCalibration", false)) {
                switch (_sensors[index]->getID()) {
                case SENSOR_MICS2710_ID:
                case SENSOR_MICS5525_ID: {
                    auto* sensor = static_cast<BaseAnalogSensor*>(_sensors[index]);
                    sensor->calibrate();
                    setSetting("snsR0", sensor->getR0());
                    break;
                }
                default:
                    break;
                }
            }
        }
        #endif // MICS2710_SUPPORT || MICS5525_SUPPORT

        if (_sensorIsEmon(_sensors[index])) {

            // TODO: ::isEmon() ?
            double value;
            auto* sensor = static_cast<BaseEmonSensor*>(_sensors[index]);

            if ((value = getSetting("pwrExpectedC", 0.0))) {
                sensor->expectedCurrent(value);
                delSetting("pwrExpectedC");
                setSetting("pwrRatioC", sensor->getCurrentRatio());
            }

            if ((value = getSetting("pwrExpectedV", 0.0))) {
                delSetting("pwrExpectedV");
                sensor->expectedVoltage(value);
                setSetting("pwrRatioV", sensor->getVoltageRatio());
            }

            if ((value = getSetting("pwrExpectedP", 0.0))) {
                delSetting("pwrExpectedP");
                sensor->expectedPower(value);
                setSetting("pwrRatioP", sensor->getPowerRatio());
            }

            if (getSetting("pwrResetE", false)) {
                delSetting("pwrResetE");
                for (size_t index = 0; index < sensor->countDevices(); ++index) {
                    sensor->resetEnergy(index);
                    _sensorResetEnergyTotal(index);
                }
            }

            if (getSetting("pwrResetCalibration", false)) {
                delSetting("pwrResetCalibration");
                delSetting("pwrRatioC");
                delSetting("pwrRatioV");
                delSetting("pwrRatioP");
                sensor->resetRatios();
            }

        } // is emon?

    }

    // Update magnitude config, filter sizes and reset energy if needed
    {
        for (unsigned char index = 0; index < _magnitudes.size(); ++index) {

            auto& magnitude = _magnitudes.at(index);

            // process emon-specific settings first. ensure that settings use global index and we access sensor with the local one
            if (_sensorIsEmon(magnitude.sensor)) {
                // TODO: compatibility proxy, fetch global key before indexed
                auto get_ratio = [](const char* key, unsigned char index, double default_value) -> double {
                    return getSetting({key, index}, getSetting(key, default_value));
                };

                auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor);

                switch (magnitude.type) {
                case MAGNITUDE_CURRENT:
                    sensor->setCurrentRatio(
                        magnitude.index_local, get_ratio("pwrRatioC", magnitude.index_global, sensor->defaultCurrentRatio())
                    );
                    break;
                case MAGNITUDE_POWER_ACTIVE:
                    sensor->setPowerRatio(
                        magnitude.index_local, get_ratio("pwrRatioP", magnitude.index_global, sensor->defaultPowerRatio())
                    );
                    break;
                case MAGNITUDE_VOLTAGE:
                    sensor->setVoltageRatio(
                        magnitude.index_local, get_ratio("pwrRatioV", magnitude.index_global, sensor->defaultVoltageRatio())
                    );
                    sensor->setVoltage(
                        magnitude.index_local, get_ratio("pwrVoltage", magnitude.index_global, sensor->defaultVoltage())
                    );
                    break;
                case MAGNITUDE_ENERGY:
                    sensor->setEnergyRatio(
                        magnitude.index_local, get_ratio("pwrRatioE", magnitude.index_global, sensor->defaultEnergyRatio())
                    );
                    break;
                default:
                    break;
                }
            }

            // adjust type-specific units
            {
                const sensor::Unit default_unit { magnitude.sensor->units(magnitude.slot) };
                const settings_key_t key {
                    String(_magnitudeSettingsPrefix(magnitude.type)) + F("Units") + String(magnitude.index_global, 10) };

                magnitude.units = _magnitudeUnitFilter(
                    magnitude,
                    getSetting(key, default_unit)
                );
            }

            // some magnitudes allow to be corrected with an offset
            {
                if (_magnitudeCanUseCorrection(magnitude.type)) {
                    auto key = String(_magnitudeSettingsPrefix(magnitude.type)) + F("Correction");
                    magnitude.correction = getSetting({key, magnitude.index_global}, getSetting(key, _magnitudeCorrection(magnitude.type)));
                }
            }

            // some sensors can override decimal values if sensor has more precision than default
            {
                signed char decimals = magnitude.sensor->decimals(magnitude.units);
                if (decimals < 0) decimals = _sensorUnitDecimals(magnitude.units);
                magnitude.decimals = (unsigned char) decimals;
            }

            // Per-magnitude min & max delta settings
            // - min controls whether we report at all when report_count overflows
            // - max will trigger report as soon as read value is greater than the specified delta
            //   (atm this works best for accumulated magnitudes, like energy)
            {
                auto min_default = 0.0;
                auto max_default = 0.0;

                switch (magnitude.type) {
                    case MAGNITUDE_TEMPERATURE:
                        min_default = tmp_min_delta;
                        break;
                    case MAGNITUDE_HUMIDITY:
                        min_default = hum_min_delta;
                        break;
                    case MAGNITUDE_ENERGY:
                        max_default = ene_max_delta;
                        break;
                    default:
                        break;
                }

                magnitude.min_change = getSetting(
                    {_magnitudeSettingsKey(magnitude, F("MinDelta")), magnitude.index_global},
                    min_default
                );
                magnitude.max_change = getSetting(
                    {_magnitudeSettingsKey(magnitude, F("MaxDelta")), magnitude.index_global},
                    max_default
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
            if ((MAGNITUDE_ENERGY == magnitude.type) && (0 == _sensor_save_every)) {
                _sensorResetEnergyTotal(magnitude.index_global);
            }

        }
    }

    saveSettings();

}

// -----------------------------------------------------------------------------
// Public
// -----------------------------------------------------------------------------

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

double sensor::Value::get() {
    return _sensor_realtime ? last : reported;
}

sensor::Value magnitudeValue(unsigned char index) {
    sensor::Value result;

    if (index >= _magnitudes.size()) {
        result.last = std::numeric_limits<double>::quiet_NaN(),
        result.reported = std::numeric_limits<double>::quiet_NaN(),
        result.decimals = 0u;
        return result;
    }

    auto& magnitude = _magnitudes[index];
    result.last = magnitude.last;
    result.reported = magnitude.reported;
    result.decimals = magnitude.decimals;

    return result;
}

void magnitudeFormat(const sensor::Value& value, char* out, size_t) {
    // TODO: 'size' does not do anything, since dtostrf used here is expected to be 'sane', but
    //       it does not allow any size arguments besides for digits after the decimal point
    dtostrf(
        _sensor_realtime ? value.last : value.reported,
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

void _sensorBackwards() {

    // Some keys from older versions were longer
    moveSetting("powerUnits", "pwrUnits");
    moveSetting("energyUnits", "eneUnits");

    // Energy is now indexed (based on magnitude.index_global)
    moveSetting("eneTotal", "eneTotal0");

	// Update PZEM004T energy total across multiple devices
    moveSettings("pzEneTotal", "eneTotal");

    // Unit ID is no longer shared, drop when equal to Min_ or None
    const char *keys[3] = {
        "pwrUnits", "eneUnits", "tmpUnits"
    };

    for (auto* key : keys) {
        const auto units = getSetting(key);
        if (units.length() && (units.equals("0") || units.equals("1"))) {
            delSetting(key);
        }
    }

}

void sensorSetup() {

    // Settings backwards compatibility
    _sensorBackwards();

    // Load configured sensors and set up all of magnitudes
    _sensorLoad();
    _sensorInit();

    // Configure based on settings
    _sensorConfigure();

    // Allow us to query key default
    settingsRegisterDefaults({
        [](const char* key) -> bool {
            if (strncmp(key, "pwr", 3) == 0) return true;
            return false;
        },
        _sensorQueryDefault
    });

    // Websockets integration, send sensor readings and configuration
    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_sensorWebSocketOnVisible)
            .onConnected(_sensorWebSocketOnConnected)
            .onData(_sensorWebSocketSendData)
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

    // Check if we still have uninitialized sensors
    static unsigned long last_init = 0;
    if (!_sensors_ready) {
        if (millis() - last_init > SENSOR_INIT_INTERVAL) {
            last_init = millis();
            _sensorInit();
        }
    }

    if (_magnitudes.size() == 0) return;

    // Tick hook, called every loop()
    _sensorTick();

    // Check if we should read new data
    static unsigned long last_update = 0;
    static unsigned long report_count = 0;
    if (millis() - last_update > _sensor_read_interval) {

        last_update = millis();
        report_count = (report_count + 1) % _sensor_report_every;

        double value_raw;       // holds the raw value as the sensor returns it
        double value_show;      // holds the processed value applying units and decimals
        double value_filtered;  // holds the processed value applying filters, and the units and decimals

        // Pre-read hook, called every reading
        _sensorPre();

        // Get the first relay state
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

            value_raw = magnitude.sensor->value(magnitude.slot);

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
                    value_raw = 0.0;
                }
                break;
            default:
                break;
            }
#endif

            // In addition to that, we also check that value is above a certain threshold
            if ((!std::isnan(magnitude.zero_threshold)) && ((value_raw < magnitude.zero_threshold))) {
                value_raw = 0.0;
            }

            magnitude.last = value_raw;
            magnitude.filter->add(value_raw);

            // -------------------------------------------------------------
            // Procesing (units and decimals)
            // -------------------------------------------------------------

            value_show = _magnitudeProcess(magnitude, value_raw);
#if BROKER_SUPPORT
            {
                char buffer[64];
                dtostrf(value_show, 1, magnitude.decimals, buffer);
                SensorReadBroker::Publish(magnitudeTopic(magnitude.type), magnitude.index_global, value_show, buffer);
            }
#endif

            // -------------------------------------------------------------
            // Debug
            // -------------------------------------------------------------

#if SENSOR_DEBUG
            {
                char buffer[64];
                dtostrf(value_show, 1, magnitude.decimals, buffer);
                DEBUG_MSG_P(PSTR("[SENSOR] %s - %s: %s%s\n"),
                    _magnitudeDescription(magnitude).c_str(),
                    magnitudeTopic(magnitude.type).c_str(),
                    buffer,
                    _magnitudeUnits(magnitude).c_str()
                );
            }
#endif

            // -------------------------------------------------------------------
            // Report when
            // - report_count overflows after reaching _sensor_report_every
            // - when magnitude specifies max_change and we greater or equal to it
            // -------------------------------------------------------------------

            bool report = (0 == report_count);

            if (!std::isnan(magnitude.reported) && (magnitude.max_change > 0)) {
                report = (std::abs(value_show - magnitude.reported) >= magnitude.max_change);
            }

            // Special case for energy, save readings to RAM and EEPROM
            if (MAGNITUDE_ENERGY == magnitude.type) {
                _magnitudeSaveEnergyTotal(magnitude, report);
            }

            if (report) {
                value_filtered = _magnitudeProcess(magnitude, magnitude.filter->result());

                magnitude.filter->reset();
                if (magnitude.filter->size() != _sensor_report_every) {
                    magnitude.filter->resize(_sensor_report_every);
                }

                // Check if there is a minimum change threshold to report
                if (std::isnan(magnitude.reported) || (std::abs(value_filtered - magnitude.reported) >= magnitude.min_change)) {
                    magnitude.reported = value_filtered;
                    _sensorReport(magnitude_index, magnitude);
                }

            } // if (report_count == 0)

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
