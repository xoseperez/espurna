/*

SENSOR MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "sensor.h"

#if SENSOR_SUPPORT

#include <vector>
#include <float.h>

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

//--------------------------------------------------------------------------------


struct sensor_magnitude_t {

    private:

    static unsigned char _counts[MAGNITUDE_MAX];

    public:

    static unsigned char counts(unsigned char type) {
        return _counts[type];
    }

    sensor_magnitude_t();
    sensor_magnitude_t(unsigned char type, unsigned char local, sensor::Unit units, BaseSensor* sensor);

    BaseSensor * sensor;        // Sensor object
    BaseFilter * filter;        // Filter object

    unsigned char type;         // Type of measurement
    unsigned char local;        // Local index in its provider
    unsigned char global;       // Global index in its type
    unsigned char decimals;     // Number of decimals in textual representation

    sensor::Unit units;         // Units of measurement

    double last;                // Last raw value from sensor (unfiltered)
    double reported;            // Last reported value
    double min_change;          // Minimum value change to report
    double max_change;          // Maximum value change to report
    double correction;          // Value correction (applied when processing)

};

unsigned char sensor_magnitude_t::_counts[MAGNITUDE_MAX];

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
    if (payload[0] != '0') return;

    auto* sensor = static_cast<BaseEmonSensor*>(magnitude.sensor);
    auto energy = _sensorParseEnergy(payload);

    sensor->resetEnergy(magnitude.global, energy);
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
    if (magnitude.global < (sizeof(Rtcmem->energy) / sizeof(*Rtcmem->energy))) {
        _sensorRtcmemSaveEnergy(magnitude.global, energy);
    }

    // Save to EEPROM every '_sensor_save_every' readings
    // Format is `<kwh>+<ws>`, value without `+` is treated as `<ws>`
    if (persistent && _sensor_save_every) {
        _sensor_save_count[magnitude.global] =
            (_sensor_save_count[magnitude.global] + 1) % _sensor_save_every;

        if (0 == _sensor_save_count[magnitude.global]) {
            const String total = String(energy.kwh.value) + "+" + String(energy.ws.value);
            setSetting({"eneTotal", magnitude.global}, total);
            #if NTP_SUPPORT
                if (ntpSynced()) setSetting({"eneTime", magnitude.global}, ntpDateTime());
            #endif
        }
    }
}

// ---------------------------------------------------------------------------

std::vector<BaseSensor *> _sensors;
std::vector<sensor_magnitude_t> _magnitudes;
bool _sensors_ready = false;

bool _sensor_realtime = API_REAL_TIME_VALUES;
unsigned long _sensor_read_interval = 1000 * SENSOR_READ_INTERVAL;
unsigned char _sensor_report_every = SENSOR_REPORT_EVERY;

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

sensor_magnitude_t::sensor_magnitude_t() :
    sensor(nullptr),
    filter(nullptr),
    type(0),
    local(0),
    global(0),
    decimals(0),
    units(sensor::Unit::None),
    last(0.0),
    reported(0.0),
    min_change(0.0),
    max_change(0.0),
    correction(0.0)
{}

sensor_magnitude_t::sensor_magnitude_t(unsigned char type, unsigned char local, sensor::Unit units, BaseSensor* sensor) :
    sensor(sensor),
    filter(nullptr),
    type(type),
    local(local),
    global(_counts[type]),
    decimals(0),
    units(units),
    last(0.0),
    reported(0.0),
    min_change(0.0),
    max_change(0.0),
    correction(0.0)
{
    ++_counts[type];

    switch (type) {
        case MAGNITUDE_ENERGY:
            filter = new LastFilter();
        case MAGNITUDE_ENERGY_DELTA:
            filter = new SumFilter();
        case MAGNITUDE_DIGITAL:
            filter = new MaxFilter();
        // For geiger counting moving average filter is the most appropriate if needed at all.
        case MAGNITUDE_COUNT:
        case MAGNITUDE_GEIGER_CPM:
        case MAGNITUDE_GEIGER_SIEVERT:
            filter = new MovingAverageFilter();
        default:
            filter = new MedianFilter();
    }

    filter->resize(_sensor_report_every);
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
        case sensor::Unit::UltravioletIndex:
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
    }

    return result;
};

double _magnitudeProcess(const sensor_magnitude_t& magnitude, double value) {

    // Process input (sensor) units and convert to the ones that magnitude specifies as output
    switch (magnitude.sensor->units(magnitude.local)) {
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

// -----------------------------------------------------------------------------

#if WEB_SUPPORT

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

bool _sensorWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    if (strncmp(key, "pwr", 3) == 0) return true;
    if (strncmp(key, "sns", 3) == 0) return true;
    if (strncmp(key, "tmp", 3) == 0) return true;
    if (strncmp(key, "hum", 3) == 0) return true;
    if (strncmp(key, "ene", 3) == 0) return true;
    if (strncmp(key, "lux", 3) == 0) return true;
    return false;
}

void _sensorWebSocketOnVisible(JsonObject& root) {

    root["snsVisible"] = 1;

    for (auto& magnitude : _magnitudes) {
        if (magnitude.type == MAGNITUDE_TEMPERATURE) root["temperatureVisible"] = 1;
        if (magnitude.type == MAGNITUDE_HUMIDITY) root["humidityVisible"] = 1;
        #if MICS2710_SUPPORT || MICS5525_SUPPORT
            if (magnitude.type == MAGNITUDE_CO || magnitude.type == MAGNITUDE_NO2) root["micsVisible"] = 1;
        #endif
    }

}

void _sensorWebSocketMagnitudesConfig(JsonObject& root) {

    JsonObject& magnitudes = root.createNestedObject("magnitudesConfig");
    uint8_t size = 0;

    JsonArray& index = magnitudes.createNestedArray("index");
    JsonArray& type = magnitudes.createNestedArray("type");
    JsonArray& units = magnitudes.createNestedArray("units");
    JsonArray& description = magnitudes.createNestedArray("description");

    for (unsigned char i=0; i<magnitudeCount(); i++) {

        auto& magnitude = _magnitudes[i];
        if (magnitude.type == MAGNITUDE_EVENT) continue;
        ++size;

        index.add<uint8_t>(magnitude.global);
        type.add<uint8_t>(magnitude.type);
        units.add(_magnitudeUnits(magnitude));

        {
            String sensor_desc = magnitude.sensor->slot(magnitude.local);
            description.add(sensor_desc);
        }

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
                string += getSetting({"eneTime", magnitude.global}, F("(unknown)"));
                info.add(string);
            } else {
                info.add((uint8_t)0);
            }
        #endif
    }

    magnitudes["size"] = size;

}

void _sensorWebSocketOnConnected(JsonObject& root) {

    for (unsigned char i=0; i<_sensors.size(); i++) {

        BaseSensor * sensor = _sensors[i];
        UNUSED(sensor);

        #if EMON_ANALOG_SUPPORT
            if (sensor->getID() == SENSOR_EMON_ANALOG_ID) {
                root["emonVisible"] = 1;
                root["pwrVisible"] = 1;
                root["pwrVoltage"] = ((EmonAnalogSensor *) sensor)->getVoltage();
            }
        #endif

        #if HLW8012_SUPPORT
            if (sensor->getID() == SENSOR_HLW8012_ID) {
                root["hlwVisible"] = 1;
                root["pwrVisible"] = 1;
            }
        #endif

        #if CSE7766_SUPPORT
            if (sensor->getID() == SENSOR_CSE7766_ID) {
                root["cseVisible"] = 1;
                root["pwrVisible"] = 1;
            }
        #endif

        #if V9261F_SUPPORT
            if (sensor->getID() == SENSOR_V9261F_ID) {
                root["pwrVisible"] = 1;
            }
        #endif

        #if ECH1560_SUPPORT
            if (sensor->getID() == SENSOR_ECH1560_ID) {
                root["pwrVisible"] = 1;
            }
        #endif

        #if PZEM004T_SUPPORT
            if (sensor->getID() == SENSOR_PZEM004T_ID) {
                root["pzemVisible"] = 1;
                root["pwrVisible"] = 1;
            }
        #endif

        #if PULSEMETER_SUPPORT
            if (sensor->getID() == SENSOR_PULSEMETER_ID) {
                root["pmVisible"] = 1;
                root["pwrRatioE"] = ((PulseMeterSensor *) sensor)->getEnergyRatio();
            }
        #endif

    }

    if (sensor_magnitude_t::counts(MAGNITUDE_TEMPERATURE)) {
        root["tmpCorrection"] = getSetting("tmpCorrection", SENSOR_TEMPERATURE_CORRECTION);
    }

    if (sensor_magnitude_t::counts(MAGNITUDE_HUMIDITY)) {
        root["humCorrection"] = getSetting("humCorrection", SENSOR_HUMIDITY_CORRECTION);
    }

    if (sensor_magnitude_t::counts(MAGNITUDE_LUX)) {
        root["luxCorrection"] = getSetting("luxCorrection", SENSOR_LUX_CORRECTION);
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

void _sensorAPISetup() {

    for (unsigned char magnitude_id=0; magnitude_id<_magnitudes.size(); magnitude_id++) {

        auto& magnitude = _magnitudes.at(magnitude_id);

        String topic = magnitudeTopic(magnitude.type);
        if (SENSOR_USE_INDEX || (sensor_magnitude_t::counts(magnitude.type) > 1)) topic = topic + "/" + String(magnitude.global);

        api_get_callback_f get_cb = [&magnitude](char * buffer, size_t len) {
            double value = _sensor_realtime ? magnitude.last : magnitude.reported;
            dtostrf(value, 1, magnitude.decimals, buffer);
        };
        api_put_callback_f put_cb = nullptr;

        if (magnitude.type == MAGNITUDE_ENERGY) {
            put_cb = [&magnitude](const char* payload) {
                _sensorApiResetEnergy(magnitude, payload);
            };
        }

        apiRegister(topic.c_str(), get_cb, put_cb);

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
                if (index != magnitude.global) continue;
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
    terminalRegisterCommand(F("MAGNITUDES"), [](Embedis* e) {
        char last[64];
        char reported[64];
        for (size_t index = 0; index < _magnitudes.size(); ++index) {
            auto& magnitude = _magnitudes.at(index);
            dtostrf(magnitude.last, 1, magnitude.decimals, last);
            dtostrf(magnitude.reported, 1, magnitude.decimals, reported);
            DEBUG_MSG_P(PSTR("[SENSOR] %2u * %s/%u @ %s (last:%s, reported:%s)\n"),
                index,
                magnitudeTopic(magnitude.type).c_str(), magnitude.global,
                magnitude.sensor->slot(magnitude.local).c_str(),
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
        const unsigned char number = constrain(getSetting<int>("bmx280Number", BMX280_NUMBER), 1, 2);

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

    #if CSE7766_SUPPORT
    {
        CSE7766Sensor * sensor = new CSE7766Sensor();
        sensor->setRX(CSE7766_PIN);
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
        #if (EVENTS1_PIN != GPIO_NONE)
        {
            EventSensor * sensor = new EventSensor();
            sensor->setGPIO(EVENTS1_PIN);
            sensor->setTrigger(EVENTS1_TRIGGER);
            sensor->setPinMode(EVENTS1_PIN_MODE);
            sensor->setDebounceTime(EVENTS1_DEBOUNCE);
            sensor->setInterruptMode(EVENTS1_INTERRUPT_MODE);
            _sensors.push_back(sensor);
        }
        #endif

        #if (EVENTS2_PIN != GPIO_NONE)
        {
            EventSensor * sensor = new EventSensor();
            sensor->setGPIO(EVENTS2_PIN);
            sensor->setTrigger(EVENTS2_TRIGGER);
            sensor->setPinMode(EVENTS2_PIN_MODE);
            sensor->setDebounceTime(EVENTS2_DEBOUNCE);
            sensor->setInterruptMode(EVENTS2_INTERRUPT_MODE);
            _sensors.push_back(sensor);
        }
        #endif

        #if (EVENTS3_PIN != GPIO_NONE)
        {
            EventSensor * sensor = new EventSensor();
            sensor->setGPIO(EVENTS3_PIN);
            sensor->setTrigger(EVENTS3_TRIGGER);
            sensor->setPinMode(EVENTS3_PIN_MODE);
            sensor->setDebounceTime(EVENTS3_DEBOUNCE);
            sensor->setInterruptMode(EVENTS3_INTERRUPT_MODE);
            _sensors.push_back(sensor);
        }
        #endif

        #if (EVENTS4_PIN != GPIO_NONE)
        {
            EventSensor * sensor = new EventSensor();
            sensor->setGPIO(EVENTS4_PIN);
            sensor->setTrigger(EVENTS4_TRIGGER);
            sensor->setPinMode(EVENTS4_PIN_MODE);
            sensor->setDebounceTime(EVENTS4_DEBOUNCE);
            sensor->setInterruptMode(EVENTS4_INTERRUPT_MODE);
            _sensors.push_back(sensor);
        }
        #endif

        #if (EVENTS5_PIN != GPIO_NONE)
        {
            EventSensor * sensor = new EventSensor();
            sensor->setGPIO(EVENTS5_PIN);
            sensor->setTrigger(EVENTS5_TRIGGER);
            sensor->setPinMode(EVENTS5_PIN_MODE);
            sensor->setDebounceTime(EVENTS5_DEBOUNCE);
            sensor->setInterruptMode(EVENTS5_INTERRUPT_MODE);
            _sensors.push_back(sensor);
        }
        #endif

        #if (EVENTS6_PIN != GPIO_NONE)
        {
            EventSensor * sensor = new EventSensor();
            sensor->setGPIO(EVENTS6_PIN);
            sensor->setTrigger(EVENTS6_TRIGGER);
            sensor->setPinMode(EVENTS6_PIN_MODE);
            sensor->setDebounceTime(EVENTS6_DEBOUNCE);
            sensor->setInterruptMode(EVENTS6_INTERRUPT_MODE);
            _sensors.push_back(sensor);
        }
        #endif

        #if (EVENTS7_PIN != GPIO_NONE)
        {
            EventSensor * sensor = new EventSensor();
            sensor->setGPIO(EVENTS7_PIN);
            sensor->setTrigger(EVENTS7_TRIGGER);
            sensor->setPinMode(EVENTS7_PIN_MODE);
            sensor->setDebounceTime(EVENTS7_DEBOUNCE);
            sensor->setInterruptMode(EVENTS7_INTERRUPT_MODE);
            _sensors.push_back(sensor);
        }
        #endif

        #if (EVENTS8_PIN != GPIO_NONE)
        {
            EventSensor * sensor = new EventSensor();
            sensor->setGPIO(EVENTS8_PIN);
            sensor->setTrigger(EVENTS8_TRIGGER);
            sensor->setPinMode(EVENTS8_PIN_MODE);
            sensor->setDebounceTime(EVENTS8_DEBOUNCE);
            sensor->setInterruptMode(EVENTS8_INTERRUPT_MODE);
            _sensors.push_back(sensor);
        }
        #endif
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

        if (getSetting("pzemSoft", 1 == PZEM004T_USE_SOFT)) {
            sensor->setRX(getSetting("pzemRX", PZEM004T_RX_PIN));
            sensor->setTX(getSetting("pzemTX", PZEM004T_TX_PIN));
        } else {
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
}

void _sensorReport(unsigned char index, double value) {

    const auto& magnitude = _magnitudes.at(index);

    // XXX: ensure that the received 'value' will fit here
    // dtostrf 2nd arg only controls leading zeroes and the
    // 3rd is only for the part after the dot
    char buffer[64];
    dtostrf(value, 1, magnitude.decimals, buffer);

    #if BROKER_SUPPORT
        SensorReportBroker::Publish(magnitudeTopic(magnitude.type), magnitude.global, value, buffer);
    #endif

    #if MQTT_SUPPORT

        mqttSend(magnitudeTopicIndex(index).c_str(), buffer);

        #if SENSOR_PUBLISH_ADDRESSES
            char topic[32];
            snprintf(topic, sizeof(topic), "%s/%s", SENSOR_ADDRESS_TOPIC, magnitudeTopic(magnitude.type).c_str());
            if (SENSOR_USE_INDEX || (sensor_magnitude_t::counts(magnitude.type) > 1)) {
                mqttSend(topic, magnitude.global, magnitude.sensor->address(magnitude.local).c_str());
            } else {
                mqttSend(topic, magnitude.sensor->address(magnitude.local).c_str());
            }
        #endif // SENSOR_PUBLISH_ADDRESSES

    #endif // MQTT_SUPPORT

    #if THINGSPEAK_SUPPORT
        tspkEnqueueMeasurement(index, buffer);
    #endif // THINGSPEAK_SUPPORT

    #if DOMOTICZ_SUPPORT
        domoticzSendMagnitude(magnitude.type, index, value, buffer);
    #endif // DOMOTICZ_SUPPORT

}


void _sensorCallback(unsigned char i, unsigned char type, double value) {

    DEBUG_MSG_P(PSTR("[SENSOR] Sensor #%u callback, type %u, payload: '%s'\n"), i, type, String(value).c_str());

    for (unsigned char k=0; k<_magnitudes.size(); k++) {
        if ((_sensors[i] == _magnitudes[k].sensor) && (type == _magnitudes[k].type)) {
            _sensorReport(k, value);
            return;
        }
    }

}

void _sensorInit() {

    _sensors_ready = true;
    _sensor_save_every = 0;

    for (unsigned char i=0; i<_sensors.size(); i++) {

        // Do not process an already initialized sensor
        if (_sensors[i]->ready()) continue;
        DEBUG_MSG_P(PSTR("[SENSOR] Initializing %s\n"), _sensors[i]->description().c_str());

        // Force sensor to reload config
        _sensors[i]->begin();
        if (!_sensors[i]->ready()) {
            if (_sensors[i]->error() != 0) DEBUG_MSG_P(PSTR("[SENSOR]  -> ERROR %d\n"), _sensors[i]->error());
            _sensors_ready = false;
            continue;
        }

        // Initialize sensor magnitudes
        for (unsigned char magnitude_index = 0; magnitude_index < _sensors[i]->count(); ++magnitude_index) {

            const auto magnitude_type = _sensors[i]->type(magnitude_index);
            _magnitudes.emplace_back(
                magnitude_type,               // specific type of the magnitude
                magnitude_index,              // index local to the sensor
                sensor::Unit::None,           // set up later, in configuration
                _sensors[i]                   // bind the sensor to allow us to reference it later
            );

            if (MAGNITUDE_ENERGY == magnitude_type) {
                _sensor_save_count.push_back(0);
            }

            DEBUG_MSG_P(PSTR("[SENSOR]  -> %s:%u\n"),
                magnitudeTopic(magnitude_type).c_str(),
                sensor_magnitude_t::counts(magnitude_type)
            );

        }

        // Hook callback
        _sensors[i]->onEvent([i](unsigned char type, double value) {
            _sensorCallback(i, type, value);
        });

        // Custom initializations, based on IDs

        switch (_sensors[i]->getID()) {
        case SENSOR_MICS2710_ID:
        case SENSOR_MICS5525_ID: {
            auto* sensor = static_cast<BaseAnalogSensor*>(_sensors[i]);
            sensor->setR0(getSetting("snsR0", sensor->getR0()));
            sensor->setRS(getSetting("snsRS", sensor->getRS()));
            sensor->setRL(getSetting("snsRL", sensor->getRL()));
            break;
        }
        default:
            break;
        }

        // TODO: compatibility proxy, fetch global key before indexed
        auto get_ratio = [](const char* key, unsigned char index, double default_value) -> double {
            return getSetting({key, index}, getSetting(key, default_value));
        };

        if (_sensorIsEmon(_sensors[i])) {

            auto* sensor = static_cast<BaseEmonSensor*>(_sensors[i]);

            for (size_t index = 0; index < sensor->countDevices(); ++index) {
                sensor->resetEnergy(index, _sensorEnergyTotal(index));
                sensor->setCurrentRatio(
                    index, get_ratio("pwrRatioC", index, sensor->getCurrentRatio(index))
                );
                sensor->setVoltageRatio(
                    index, get_ratio("pwrRatioV", index, sensor->getVoltageRatio(index))
                );
                sensor->setPowerRatio(
                    index, get_ratio("pwrRatioP", index, sensor->getPowerRatio(index))
                );
                sensor->setEnergyRatio(
                    index, get_ratio("pwrRatioE", index, sensor->getEnergyRatio(index))
                );
                sensor->setVoltage(
                    index, get_ratio("pwrVoltage", index, sensor->getVoltage(index))
                );
            }

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

} // ns settings::internal
} // ns settings

void _sensorConfigure() {

    // General sensor settings for reporting and saving
    _sensor_read_interval = 1000 * constrain(getSetting("snsRead", SENSOR_READ_INTERVAL), SENSOR_READ_MIN_INTERVAL, SENSOR_READ_MAX_INTERVAL);
    _sensor_report_every = constrain(getSetting("snsReport", SENSOR_REPORT_EVERY), SENSOR_REPORT_MIN_EVERY, SENSOR_REPORT_MAX_EVERY);
    _sensor_save_every = getSetting("snsSave", SENSOR_SAVE_EVERY);

    _sensor_realtime = getSetting("apiRealTime", 1 == API_REAL_TIME_VALUES);

    // Per-magnitude min & max delta settings
    // - min controls whether we report at all when report_count overflows
    // - max will trigger report as soon as read value is greater than the specified delta
    //   (atm this works best for accumulated magnitudes, like energy)
    const auto tmp_min_delta = getSetting("tmpMinDelta", TEMPERATURE_MIN_CHANGE);
    const auto hum_min_delta = getSetting("humMinDelta", HUMIDITY_MIN_CHANGE);
    const auto ene_max_delta = getSetting("eneMaxDelta", ENERGY_MAX_CHANGE);

    // Specific sensor settings
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

            sensor->setEnergyRatio(getSetting("pwrRatioE", sensor->getEnergyRatio()));

        } // is emon?

    }

    // Update magnitude config, filter sizes and reset energy if needed
    {

        // TODO: instead of using global enum, have a local mapping?
        const auto tmpUnits = getSetting("tmpUnits", SENSOR_TEMPERATURE_UNITS);
        const auto pwrUnits = getSetting("pwrUnits", SENSOR_POWER_UNITS);
        const auto eneUnits = getSetting("eneUnits", SENSOR_ENERGY_UNITS);

        // TODO: map MAGNITUDE_... type to a specific string? nvm the preprocessor flags, just focus on settings
        const auto tmpCorrection = getSetting("tmpCorrection", SENSOR_TEMPERATURE_CORRECTION);
        const auto humCorrection = getSetting("humCorrection", SENSOR_HUMIDITY_CORRECTION);
        const auto luxCorrection = getSetting("luxCorrection", SENSOR_LUX_CORRECTION);

        for (unsigned char index = 0; index < _magnitudes.size(); ++index) {

            auto& magnitude = _magnitudes.at(index);

            switch (magnitude.type) {
                case MAGNITUDE_TEMPERATURE:
                    magnitude.units = _magnitudeUnitFilter(
                        magnitude,
                        getSetting({"tmpUnits", magnitude.global}, tmpUnits)
                    );
                    magnitude.correction = getSetting({"tmpCorrection", magnitude.global}, tmpCorrection);
                    break;
                case MAGNITUDE_HUMIDITY:
                    magnitude.correction = getSetting({"humCorrection", magnitude.global}, humCorrection);
                    break;
                case MAGNITUDE_POWER_ACTIVE:
                    magnitude.units = _magnitudeUnitFilter(
                        magnitude,
                        getSetting({"pwrUnits", magnitude.global}, pwrUnits)
                    );
                    break;
                case MAGNITUDE_ENERGY:
                    magnitude.units = _magnitudeUnitFilter(
                        magnitude,
                        getSetting({"eneUnits", magnitude.global}, eneUnits)
                    );
                    break;
                case MAGNITUDE_LUX:
                    magnitude.correction = getSetting({"luxCorrection", magnitude.global}, luxCorrection);
                    break;
                default:
                    magnitude.units = magnitude.sensor->units(magnitude.local);
                    break;
            }

            // some sensors can override decimal values if sensor has more precision than default
            {
                signed char decimals = magnitude.sensor->decimals(magnitude.units);
                if (decimals < 0) decimals = _sensorUnitDecimals(magnitude.units);
                magnitude.decimals = (unsigned char) decimals;
            }

            // adjust min & max change delta value to trigger report
            // TODO: find a proper way to extend this to min/max of any magnitude
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

                magnitude.min_change = getSetting({"snsMinDelta", index}, min_default);
                magnitude.max_change = getSetting({"snsMaxDelta", index}, max_default);
            }

            // in case we don't save energy periodically, purge existing value in ram & settings
            if ((MAGNITUDE_ENERGY == magnitude.type) && (0 == _sensor_save_every)) {
                _sensorResetEnergyTotal(magnitude.global);
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

String magnitudeName(unsigned char index) {
    if (index < _magnitudes.size()) {
        sensor_magnitude_t magnitude = _magnitudes[index];
        return magnitude.sensor->slot(magnitude.local);
    }
    return String();
}

unsigned char magnitudeType(unsigned char index) {
    if (index < _magnitudes.size()) {
        return int(_magnitudes[index].type);
    }
    return MAGNITUDE_NONE;
}

double magnitudeValue(unsigned char index) {
    if (index < _magnitudes.size()) {
        return _sensor_realtime ? _magnitudes[index].last : _magnitudes[index].reported;
    }
    return DBL_MIN;
}

unsigned char magnitudeIndex(unsigned char index) {
    if (index < _magnitudes.size()) {
        return int(_magnitudes[index].global);
    }
    return 0;
}

String magnitudeTopicIndex(unsigned char index) {
    char topic[32] = {0};
    if (index < _magnitudes.size()) {
        sensor_magnitude_t magnitude = _magnitudes[index];
        if (SENSOR_USE_INDEX || (sensor_magnitude_t::counts(magnitude.type) > 1)) {
            snprintf(topic, sizeof(topic), "%s/%u", magnitudeTopic(magnitude.type).c_str(), magnitude.global);
        } else {
            snprintf(topic, sizeof(topic), "%s", magnitudeTopic(magnitude.type).c_str());
        }
    }
    return String(topic);
}

// -----------------------------------------------------------------------------

void _sensorBackwards() {

    // Some keys from older versions were longer
    moveSetting("powerUnits", "pwrUnits");
    moveSetting("energyUnits", "eneUnits");

    // Energy is now indexed (based on magnitude.global)
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
        _sensorAPISetup();
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
        for (unsigned char i=0; i<_magnitudes.size(); i++) {

            sensor_magnitude_t magnitude = _magnitudes[i];

            if (magnitude.sensor->status()) {

                // -------------------------------------------------------------
                // Instant value
                // -------------------------------------------------------------

                value_raw = magnitude.sensor->value(magnitude.local);

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

                _magnitudes[i].last = value_raw;

                // -------------------------------------------------------------
                // Processing (filters)
                // -------------------------------------------------------------

                magnitude.filter->add(value_raw);

                // Special case for MovingAverageFilter
                switch (magnitude.type) {
                    case MAGNITUDE_COUNT:
                    case MAGNITUDE_GEIGER_CPM:
                    case MAGNITUDE_GEIGER_SIEVERT:
                        value_raw = magnitude.filter->result();
                        break;
                    default:
                        break;
                }

                // -------------------------------------------------------------
                // Procesing (units and decimals)
                // -------------------------------------------------------------

                value_show = _magnitudeProcess(magnitude, value_raw);
                #if BROKER_SUPPORT
                {
                    char buffer[64];
                    dtostrf(value_show, 1, magnitude.decimals, buffer);
                    SensorReadBroker::Publish(magnitudeTopic(magnitude.type), magnitude.global, value_show, buffer);
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
                        magnitude.sensor->slot(magnitude.local).c_str(),
                        magnitudeTopic(magnitude.type).c_str(),
                        buffer,
                        _magnitudeUnits(magnitude).c_str()
                    );
                }
                #endif // SENSOR_DEBUG

                // -------------------------------------------------------------------
                // Report when
                // - report_count overflows after reaching _sensor_report_every
                // - when magnitude specifies max_change and we greater or equal to it
                // -------------------------------------------------------------------

                bool report = (0 == report_count);

                if (magnitude.max_change > 0) {
                    report = (fabs(value_show - magnitude.reported) >= magnitude.max_change);
                }

                // Special case for energy, save readings to RAM and EEPROM
                if (MAGNITUDE_ENERGY == magnitude.type) {
                    _magnitudeSaveEnergyTotal(magnitude, report);
                }

                if (report) {

                    value_filtered = magnitude.filter->result();
                    value_filtered = _magnitudeProcess(magnitude, value_filtered);

                    magnitude.filter->reset();
                    if (magnitude.filter->size() != _sensor_report_every) {
                        magnitude.filter->resize(_sensor_report_every);
                    }

                    // Check if there is a minimum change threshold to report
                    if (fabs(value_filtered - magnitude.reported) >= magnitude.min_change) {
                        _magnitudes[i].reported = value_filtered;
                        _sensorReport(i, value_filtered);
                    } // if (fabs(value_filtered - magnitude.reported) >= magnitude.min_change)

                } // if (report_count == 0)

            } // if (magnitude.sensor->status())
        } // for (unsigned char i=0; i<_magnitudes.size(); i++)

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
