/*

SENSOR MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if SENSOR_SUPPORT

#include <vector>
#include "filters/MaxFilter.h"
#include "filters/MedianFilter.h"
#include "filters/MovingAverageFilter.h"
#include "sensors/BaseSensor.h"

typedef struct {
    BaseSensor * sensor;        // Sensor object
    BaseFilter * filter;        // Filter object
    unsigned char local;        // Local index in its provider
    unsigned char type;         // Type of measurement
    unsigned char global;       // Global index in its type
    double current;             // Current (last) value, unfiltered
    double filtered;            // Filtered (averaged) value
    double reported;            // Last reported value
    double min_change;          // Minimum value change to report
} sensor_magnitude_t;

std::vector<BaseSensor *> _sensors;
std::vector<sensor_magnitude_t> _magnitudes;
bool _sensors_ready = false;

unsigned char _counts[MAGNITUDE_MAX];
bool _sensor_realtime = API_REAL_TIME_VALUES;
unsigned long _sensor_read_interval = 1000 * SENSOR_READ_INTERVAL;
unsigned char _sensor_report_every = SENSOR_REPORT_EVERY;
unsigned char _sensor_power_units = SENSOR_POWER_UNITS;
unsigned char _sensor_energy_units = SENSOR_ENERGY_UNITS;
unsigned char _sensor_temperature_units = SENSOR_TEMPERATURE_UNITS;
double _sensor_temperature_correction = SENSOR_TEMPERATURE_CORRECTION;
double _sensor_humidity_correction = SENSOR_HUMIDITY_CORRECTION;

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

unsigned char _magnitudeDecimals(unsigned char type) {

    // Hardcoded decimals (these should be linked to the unit, instead of the magnitude)

    if (type == MAGNITUDE_ENERGY ||
        type == MAGNITUDE_ENERGY_DELTA) {
        if (_sensor_energy_units == ENERGY_KWH) return 3;
    }
    if (type == MAGNITUDE_POWER_ACTIVE ||
        type == MAGNITUDE_POWER_APPARENT ||
        type == MAGNITUDE_POWER_REACTIVE) {
        if (_sensor_power_units == POWER_KILOWATTS) return 3;
    }
    if (type < MAGNITUDE_MAX) return pgm_read_byte(magnitude_decimals + type);
    return 0;

}

double _magnitudeProcess(unsigned char type, double value) {

    // Hardcoded conversions (these should be linked to the unit, instead of the magnitude)

    if (type == MAGNITUDE_TEMPERATURE) {
        if (_sensor_temperature_units == TMP_FAHRENHEIT) value = value * 1.8 + 32;
        value = value + _sensor_temperature_correction;
    }

    if (type == MAGNITUDE_HUMIDITY) {
        value = constrain(value + _sensor_humidity_correction, 0, 100);
    }

    if (type == MAGNITUDE_ENERGY ||
        type == MAGNITUDE_ENERGY_DELTA) {
        if (_sensor_energy_units == ENERGY_KWH) value = value  / 3600000;
    }
    if (type == MAGNITUDE_POWER_ACTIVE ||
        type == MAGNITUDE_POWER_APPARENT ||
        type == MAGNITUDE_POWER_REACTIVE) {
        if (_sensor_power_units == POWER_KILOWATTS) value = value  / 1000;
    }

    return roundTo(value, _magnitudeDecimals(type));

}

// -----------------------------------------------------------------------------

#if WEB_SUPPORT

bool _sensorWebSocketOnReceive(const char * key, JsonVariant& value) {
    if (strncmp(key, "pwr", 3) == 0) return true;
    if (strncmp(key, "sns", 3) == 0) return true;
    if (strncmp(key, "tmp", 3) == 0) return true;
    if (strncmp(key, "hum", 3) == 0) return true;
    if (strncmp(key, "energy", 6) == 0) return true;
    return false;
}

void _sensorWebSocketSendData(JsonObject& root) {

    char buffer[10];
    bool hasTemperature = false;
    bool hasHumidity = false;

    JsonArray& list = root.createNestedArray("magnitudes");
    for (unsigned char i=0; i<_magnitudes.size(); i++) {

        sensor_magnitude_t magnitude = _magnitudes[i];
        unsigned char decimals = _magnitudeDecimals(magnitude.type);
        dtostrf(magnitude.current, 1-sizeof(buffer), decimals, buffer);

        JsonObject& element = list.createNestedObject();
        element["index"] = int(magnitude.global);
        element["type"] = int(magnitude.type);
        element["value"] = String(buffer);
        element["units"] = magnitudeUnits(magnitude.type);
        element["description"] = magnitude.sensor->slot(magnitude.local);
        element["error"] = magnitude.sensor->error();

        if (magnitude.type == MAGNITUDE_TEMPERATURE) hasTemperature = true;
        if (magnitude.type == MAGNITUDE_HUMIDITY) hasHumidity = true;

    }

    if (hasTemperature) root["temperatureVisible"] = 1;
    if (hasHumidity) root["humidityVisible"] = 1;

}

void _sensorWebSocketStart(JsonObject& root) {

    for (unsigned char i=0; i<_sensors.size(); i++) {

        BaseSensor * sensor = _sensors[i];

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
                root["pwrVisible"] = 1;
            }
        #endif

    }

    if (_magnitudes.size() > 0) {
        root["sensorsVisible"] = 1;
        //root["apiRealTime"] = _sensor_realtime;
        root["pwrUnits"] = _sensor_power_units;
        root["energyUnits"] = _sensor_energy_units;
        root["tmpUnits"] = _sensor_temperature_units;
        root["tmpCorrection"] = _sensor_temperature_correction;
        root["humCorrection"] = _sensor_humidity_correction;
        root["snsRead"] = _sensor_read_interval / 1000;
        root["snsReport"] = _sensor_report_every;
    }

    /*
    // Sensors manifest
    JsonArray& manifest = root.createNestedArray("manifest");
    #if BMX280_SUPPORT
        BMX280Sensor::manifest(manifest);
    #endif

    // Sensors configuration
    JsonArray& sensors = root.createNestedArray("sensors");
    for (unsigned char i; i<_sensors.size(); i++) {
        JsonObject& sensor = sensors.createNestedObject();
        sensor["index"] = i;
        sensor["id"] = _sensors[i]->getID();
        _sensors[i]->getConfig(sensor);
    }
    */

}

void _sensorAPISetup() {

    for (unsigned char magnitude_id=0; magnitude_id<_magnitudes.size(); magnitude_id++) {

        sensor_magnitude_t magnitude = _magnitudes[magnitude_id];

        String topic = magnitudeTopic(magnitude.type);
        if (SENSOR_USE_INDEX || (_counts[magnitude.type] > 1)) topic = topic + "/" + String(magnitude.global);

        apiRegister(topic.c_str(), [magnitude_id](char * buffer, size_t len) {
            sensor_magnitude_t magnitude = _magnitudes[magnitude_id];
            unsigned char decimals = _magnitudeDecimals(magnitude.type);
            double value = _sensor_realtime ? magnitude.current : magnitude.filtered;
            dtostrf(value, 1-len, decimals, buffer);
        });

    }

}
#endif

#if TERMINAL_SUPPORT

void _sensorInitCommands() {
    settingsRegisterCommand(F("MAGNITUDES"), [](Embedis* e) {
        for (unsigned char i=0; i<_magnitudes.size(); i++) {
            sensor_magnitude_t magnitude = _magnitudes[i];
            DEBUG_MSG_P(PSTR("[SENSOR] * %2d: %s @ %s (%s/%d)\n"),
                i,
                magnitudeTopic(magnitude.type).c_str(),
                magnitude.sensor->slot(magnitude.local).c_str(),
                magnitudeTopic(magnitude.type).c_str(),
                magnitude.global
            );
        }
        DEBUG_MSG_P(PSTR("+OK\n"));
    });
}

#endif

void _sensorTick() {
    for (unsigned char i=0; i<_sensors.size(); i++) {
        _sensors[i]->tick();
    }
}

void _sensorPre() {
    for (unsigned char i=0; i<_sensors.size(); i++) {
        _sensors[i]->pre();
        if (!_sensors[i]->status()) {
            DEBUG_MSG_P(PSTR("[SENSOR] Error reading data from %s (error: %d)\n"),
                _sensors[i]->description().c_str(),
                _sensors[i]->error()
            );
        }
    }
}

void _sensorPost() {
    for (unsigned char i=0; i<_sensors.size(); i++) {
        _sensors[i]->post();
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
    Check the DHT block below for an example

     */

    #if ANALOG_SUPPORT
    {
        AnalogSensor * sensor = new AnalogSensor();
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

    #if BMX280_SUPPORT
    {
        BMX280Sensor * sensor = new BMX280Sensor();
        sensor->setAddress(BMX280_ADDRESS);
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

    /*
    // Example on how to add a second DHT sensor
    // DHT2_PIN and DHT2_TYPE should be defined in sensors.h file
    #if DHT_SUPPORT
    {
        DHTSensor * sensor = new DHTSensor();
        sensor->setGPIO(DHT2_PIN);
        sensor->setType(DHT2_TYPE);
        _sensors.push_back(sensor);
    }
    #endif
    */

    #if DIGITAL_SUPPORT
    {
        DigitalSensor * sensor = new DigitalSensor();
        sensor->setGPIO(DIGITAL_PIN);
        sensor->setMode(DIGITAL_PIN_MODE);
        sensor->setDefault(DIGITAL_DEFAULT_STATE);
        _sensors.push_back(sensor);
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
        EventSensor * sensor = new EventSensor();
        sensor->setGPIO(EVENTS_PIN);
        sensor->setMode(EVENTS_PIN_MODE);
        sensor->setDebounceTime(EVENTS_DEBOUNCE);
        sensor->setInterruptMode(EVENTS_INTERRUPT_MODE);
        _sensors.push_back(sensor);
    }
    #endif

    #if HLW8012_SUPPORT
    {
        HLW8012Sensor * sensor = new HLW8012Sensor();
        sensor->setSEL(HLW8012_SEL_PIN);
        sensor->setCF(HLW8012_CF_PIN);
        sensor->setCF1(HLW8012_CF1_PIN);
        sensor->setSELCurrent(HLW8012_SEL_CURRENT);
        _sensors.push_back(sensor);
    }
    #endif

    #if MHZ19_SUPPORT
    {
        MHZ19Sensor * sensor = new MHZ19Sensor();
        sensor->setRX(MHZ19_RX_PIN);
        sensor->setTX(MHZ19_TX_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if PMSX003_SUPPORT
    {
        PMSX003Sensor * sensor = new PMSX003Sensor();
        sensor->setRX(PMS_RX_PIN);
        sensor->setTX(PMS_TX_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if PZEM004T_SUPPORT
    {
        PZEM004TSensor * sensor = new PZEM004TSensor();
        #if PZEM004T_USE_SOFT
            sensor->setRX(PZEM004T_RX_PIN);
            sensor->setTX(PZEM004T_TX_PIN);
        #else
            sensor->setSerial(& PZEM004T_HW_PORT);
        #endif
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

    #if V9261F_SUPPORT
    {
        V9261FSensor * sensor = new V9261FSensor();
        sensor->setRX(V9261F_PIN);
        sensor->setInverted(V9261F_PIN_INVERSE);
        _sensors.push_back(sensor);
    }
    #endif

    #if AM2320_SUPPORT
    {
        AM2320Sensor * sensor = new AM2320Sensor();
        sensor->setAddress(AM2320_ADDRESS);
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

}

void _sensorCallback(unsigned char i, unsigned char type, const char * payload) {
    DEBUG_MSG_P(PSTR("[SENSOR] Sensor #%u callback, type %u, payload: '%s'\n"), i, type, payload);
}

void _sensorInit() {

    _sensors_ready = true;

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

        // Initialize magnitudes
        for (unsigned char k=0; k<_sensors[i]->count(); k++) {

            unsigned char type = _sensors[i]->type(k);

            sensor_magnitude_t new_magnitude;
            new_magnitude.sensor = _sensors[i];
            new_magnitude.local = k;
            new_magnitude.type = type;
            new_magnitude.global = _counts[type];
            new_magnitude.current = 0;
            new_magnitude.filtered = 0;
            new_magnitude.reported = 0;
            new_magnitude.min_change = 0;
            if (type == MAGNITUDE_DIGITAL) {
                new_magnitude.filter = new MaxFilter();
            } else if (type == MAGNITUDE_EVENTS) {
                new_magnitude.filter = new MovingAverageFilter();
            } else {
                new_magnitude.filter = new MedianFilter();
            }
            new_magnitude.filter->resize(_sensor_report_every);
            _magnitudes.push_back(new_magnitude);

            DEBUG_MSG_P(PSTR("[SENSOR]  -> %s:%d\n"), magnitudeTopic(type).c_str(), _counts[type]);

            _counts[type] = _counts[type] + 1;

        }

        // Hook callback
        _sensors[i]->onEvent([i](unsigned char type, const char * payload) {
            _sensorCallback(i, type, payload);
        });

        // Custom initializations

        #if EMON_ANALOG_SUPPORT

            if (_sensors[i]->getID() == SENSOR_EMON_ANALOG_ID) {
                EmonAnalogSensor * sensor = (EmonAnalogSensor *) _sensors[i];
                sensor->setCurrentRatio(0, getSetting("pwrRatioC", EMON_CURRENT_RATIO).toFloat());
                sensor->setVoltage(getSetting("pwrVoltage", EMON_MAINS_VOLTAGE).toInt());
            }

        #endif // EMON_ANALOG_SUPPORT

        #if HLW8012_SUPPORT

            if (_sensors[i]->getID() == SENSOR_HLW8012_ID) {

                HLW8012Sensor * sensor = (HLW8012Sensor *) _sensors[i];

                double value;

                value = getSetting("pwrRatioC", 0).toFloat();
                if (value > 0) sensor->setCurrentRatio(value);

                value = getSetting("pwrRatioV", 0).toFloat();
                if (value > 0) sensor->setVoltageRatio(value);

                value = getSetting("pwrRatioP", 0).toFloat();
                if (value > 0) sensor->setPowerRatio(value);

            }

        #endif // HLW8012_SUPPORT

    }

}

void _sensorConfigure() {

    // General sensor settings
    _sensor_read_interval = 1000 * constrain(getSetting("snsRead", SENSOR_READ_INTERVAL).toInt(), SENSOR_READ_MIN_INTERVAL, SENSOR_READ_MAX_INTERVAL);
    _sensor_report_every = constrain(getSetting("snsReport", SENSOR_REPORT_EVERY).toInt(), SENSOR_REPORT_MIN_EVERY, SENSOR_REPORT_MAX_EVERY);
    _sensor_realtime = getSetting("apiRealTime", API_REAL_TIME_VALUES).toInt() == 1;
    _sensor_power_units = getSetting("pwrUnits", SENSOR_POWER_UNITS).toInt();
    _sensor_energy_units = getSetting("energyUnits", SENSOR_ENERGY_UNITS).toInt();
    _sensor_temperature_units = getSetting("tmpUnits", SENSOR_TEMPERATURE_UNITS).toInt();
    _sensor_temperature_correction = getSetting("tmpCorrection", SENSOR_TEMPERATURE_CORRECTION).toFloat();
    _sensor_humidity_correction = getSetting("humCorrection", SENSOR_HUMIDITY_CORRECTION).toFloat();

    // Specific sensor settings
    for (unsigned char i=0; i<_sensors.size(); i++) {

        #if EMON_ANALOG_SUPPORT

            if (_sensors[i]->getID() == SENSOR_EMON_ANALOG_ID) {

                double value;
                EmonAnalogSensor * sensor = (EmonAnalogSensor *) _sensors[i];

                if (value = getSetting("pwrExpectedP", 0).toInt()) {
                    sensor->expectedPower(0, value);
                    setSetting("pwrRatioC", sensor->getCurrentRatio(0));
                }

                if (getSetting("pwrResetCalibration", 0).toInt() == 1) {
                    sensor->setCurrentRatio(0, EMON_CURRENT_RATIO);
                    delSetting("pwrRatioC");
                }

                if (getSetting("pwrResetE", 0).toInt() == 1) {
                    sensor->resetEnergy();
                }

                sensor->setVoltage(getSetting("pwrVoltage", EMON_MAINS_VOLTAGE).toInt());

            }

        #endif // EMON_ANALOG_SUPPORT

        #if EMON_ADC121_SUPPORT
            if (_sensors[i]->getID() == SENSOR_EMON_ADC121_ID) {
                EmonADC121Sensor * sensor = (EmonADC121Sensor *) _sensors[i];
                if (getSetting("pwrResetE", 0).toInt() == 1) {
                    sensor->resetEnergy();
                }
            }
        #endif

        #if EMON_ADS1X15_SUPPORT
            if (_sensors[i]->getID() == SENSOR_EMON_ADS1X15_ID) {
                EmonADS1X15Sensor * sensor = (EmonADS1X15Sensor *) _sensors[i];
                if (getSetting("pwrResetE", 0).toInt() == 1) {
                    sensor->resetEnergy();
                }
            }
        #endif

        #if HLW8012_SUPPORT


            if (_sensors[i]->getID() == SENSOR_HLW8012_ID) {

                double value;
                HLW8012Sensor * sensor = (HLW8012Sensor *) _sensors[i];

                if (value = getSetting("pwrExpectedC", 0).toFloat()) {
                    sensor->expectedCurrent(value);
                    setSetting("pwrRatioC", sensor->getCurrentRatio());
                }

                if (value = getSetting("pwrExpectedV", 0).toInt()) {
                    sensor->expectedVoltage(value);
                    setSetting("pwrRatioV", sensor->getVoltageRatio());
                }

                if (value = getSetting("pwrExpectedP", 0).toInt()) {
                    sensor->expectedPower(value);
                    setSetting("pwrRatioP", sensor->getPowerRatio());
                }

                if (getSetting("pwrResetE", 0).toInt() == 1) {
                    sensor->resetEnergy();
                }

                if (getSetting("pwrResetCalibration", 0).toInt() == 1) {
                    sensor->resetRatios();
                    delSetting("pwrRatioC");
                    delSetting("pwrRatioV");
                    delSetting("pwrRatioP");
                }

            }

        #endif // HLW8012_SUPPORT

    }

    // Update filter sizes
    for (unsigned char i=0; i<_magnitudes.size(); i++) {
        _magnitudes[i].filter->resize(_sensor_report_every);
    }

    // Save settings
    delSetting("pwrExpectedP");
    delSetting("pwrExpectedC");
    delSetting("pwrExpectedV");
    delSetting("pwrResetCalibration");
    delSetting("pwrResetE");
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

unsigned char magnitudeIndex(unsigned char index) {
    if (index < _magnitudes.size()) {
        return int(_magnitudes[index].global);
    }
    return 0;
}

String magnitudeTopic(unsigned char type) {
    char buffer[16] = {0};
    if (type < MAGNITUDE_MAX) strncpy_P(buffer, magnitude_topics[type], sizeof(buffer));
    return String(buffer);
}

String magnitudeTopicIndex(unsigned char index) {
    char topic[32] = {0};
    if (index < _magnitudes.size()) {
        sensor_magnitude_t magnitude = _magnitudes[index];
        if (SENSOR_USE_INDEX || (_counts[magnitude.type] > 1)) {
            snprintf(topic, sizeof(topic), "%s/%u", magnitudeTopic(magnitude.type).c_str(), magnitude.global);
        } else {
            snprintf(topic, sizeof(topic), "%s", magnitudeTopic(magnitude.type).c_str());
        }
    }
    return String(topic);
}


String magnitudeUnits(unsigned char type) {
    char buffer[8] = {0};
    if (type < MAGNITUDE_MAX) {
        if ((type == MAGNITUDE_TEMPERATURE) && (_sensor_temperature_units == TMP_FAHRENHEIT)) {
            strncpy_P(buffer, magnitude_fahrenheit, sizeof(buffer));
        } else if (
            (type == MAGNITUDE_ENERGY || type == MAGNITUDE_ENERGY_DELTA) &&
            (_sensor_energy_units == ENERGY_KWH)) {
            strncpy_P(buffer, magnitude_kwh, sizeof(buffer));
        } else if (
            (type == MAGNITUDE_POWER_ACTIVE || type == MAGNITUDE_POWER_APPARENT || type == MAGNITUDE_POWER_REACTIVE) &&
            (_sensor_power_units == POWER_KILOWATTS)) {
            strncpy_P(buffer, magnitude_kw, sizeof(buffer));
        } else {
            strncpy_P(buffer, magnitude_units[type], sizeof(buffer));
        }
    }
    return String(buffer);
}

// -----------------------------------------------------------------------------

void sensorSetup() {

    // Backwards compatibility
    moveSetting("powerUnits", "pwrUnits");

    // Load sensors
    _sensorLoad();
    _sensorInit();

    // Configure stored values
    _sensorConfigure();

    #if WEB_SUPPORT

        // Websockets
        wsOnSendRegister(_sensorWebSocketStart);
        wsOnReceiveRegister(_sensorWebSocketOnReceive);
        wsOnSendRegister(_sensorWebSocketSendData);
        wsOnAfterParseRegister(_sensorConfigure);

        // API
        _sensorAPISetup();

    #endif

    #if TERMINAL_SUPPORT
        _sensorInitCommands();
    #endif

    // Register loop
    espurnaRegisterLoop(sensorLoop);

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

    // Tick hook
    _sensorTick();

    // Check if we should read new data
    static unsigned long last_update = 0;
    static unsigned long report_count = 0;
    if (millis() - last_update > _sensor_read_interval) {

        last_update = millis();
        report_count = (report_count + 1) % _sensor_report_every;

        double current;
        double filtered;
        char buffer[64];

        // Pre-read hook
        _sensorPre();

        // Get the first relay state
        #if SENSOR_POWER_CHECK_STATUS
            bool relay_off = (relayCount() > 0) && (relayStatus(0) == 0);
        #endif

        // Get readings
        for (unsigned char i=0; i<_magnitudes.size(); i++) {

            sensor_magnitude_t magnitude = _magnitudes[i];

            if (magnitude.sensor->status()) {

                current = magnitude.sensor->value(magnitude.local);

                // Completely remove spurious values if relay is OFF
                #if SENSOR_POWER_CHECK_STATUS
                    if (relay_off) {
                        if (magnitude.type == MAGNITUDE_POWER_ACTIVE ||
                            magnitude.type == MAGNITUDE_POWER_REACTIVE ||
                            magnitude.type == MAGNITUDE_POWER_APPARENT ||
                            magnitude.type == MAGNITUDE_CURRENT ||
                            magnitude.type == MAGNITUDE_ENERGY_DELTA
                        ) {
                            current = 0;
                        }
                    }
                #endif

                magnitude.filter->add(current);

                // Special case
                if (magnitude.type == MAGNITUDE_EVENTS) {
                    current = magnitude.filter->result();
                }

                current = _magnitudeProcess(magnitude.type, current);
                _magnitudes[i].current = current;

                unsigned char decimals = _magnitudeDecimals(magnitude.type);

                // Debug
                #if SENSOR_DEBUG
                {
                    dtostrf(current, 1-sizeof(buffer), decimals, buffer);
                    DEBUG_MSG_P(PSTR("[SENSOR] %s - %s: %s%s\n"),
                        magnitude.sensor->slot(magnitude.local).c_str(),
                        magnitudeTopic(magnitude.type).c_str(),
                        buffer,
                        magnitudeUnits(magnitude.type).c_str()
                    );
                }
                #endif // SENSOR_DEBUG

                // Time to report (we do it every _sensor_report_every readings)
                if (report_count == 0) {

                    filtered = magnitude.filter->result();
                    magnitude.filter->reset();
                    filtered = _magnitudeProcess(magnitude.type, filtered);
                    _magnitudes[i].filtered = filtered;

                    // Check if there is a minimum change threshold to report
                    if (fabs(filtered - magnitude.reported) >= magnitude.min_change) {

                        _magnitudes[i].reported = filtered;
                        dtostrf(filtered, 1-sizeof(buffer), decimals, buffer);

                        #if BROKER_SUPPORT
                            brokerPublish(magnitudeTopic(magnitude.type).c_str(), magnitude.local, buffer);
                        #endif

                        #if MQTT_SUPPORT

                            mqttSend(magnitudeTopicIndex(i).c_str(), buffer);

                            #if SENSOR_PUBLISH_ADDRESSES
                                char topic[32];
                                snprintf(topic, sizeof(topic), "%s/%s", SENSOR_ADDRESS_TOPIC, magnitudeTopic(magnitude.type).c_str());
                                if (SENSOR_USE_INDEX || (_counts[magnitude.type] > 1)) {
                                    mqttSend(topic, magnitude.global, magnitude.sensor->address(magnitude.local).c_str());
                                } else {
                                    mqttSend(topic, magnitude.sensor->address(magnitude.local).c_str());
                                }
                            #endif // SENSOR_PUBLISH_ADDRESSES

                        #endif // MQTT_SUPPORT

                        #if INFLUXDB_SUPPORT
                            if (SENSOR_USE_INDEX || (_counts[magnitude.type] > 1)) {
                                idbSend(magnitudeTopic(magnitude.type).c_str(), magnitude.global, buffer);
                            } else {
                                idbSend(magnitudeTopic(magnitude.type).c_str(), buffer);
                            }
                        #endif // INFLUXDB_SUPPORT

                        #if THINGSPEAK_SUPPORT
                            tspkEnqueueMeasurement(i, buffer);
                        #endif

                        #if DOMOTICZ_SUPPORT
                        {
                            char key[15];
                            snprintf_P(key, sizeof(key), PSTR("dczMagnitude%d"), i);
                            if (magnitude.type == MAGNITUDE_HUMIDITY) {
                                int status;
                                if (filtered > 70) {
                                    status = HUMIDITY_WET;
                                } else if (filtered > 45) {
                                    status = HUMIDITY_COMFORTABLE;
                                } else if (filtered > 30) {
                                    status = HUMIDITY_NORMAL;
                                } else {
                                    status = HUMIDITY_DRY;
                                }
                                char status_buf[5];
                                itoa(status, status_buf, 10);
                                domoticzSend(key, buffer, status_buf);
                            } else {
                                domoticzSend(key, 0, buffer);
                            }
                        }
                        #endif // DOMOTICZ_SUPPORT

                    } // if (fabs(filtered - magnitude.reported) >= magnitude.min_change)
                } // if (report_count == 0)
            } // if (magnitude.sensor->status())
        } // for (unsigned char i=0; i<_magnitudes.size(); i++)

        // Post-read hook
        _sensorPost();

        #if WEB_SUPPORT
            wsSend(_sensorWebSocketSendData);
        #endif

        #if THINGSPEAK_SUPPORT
            if (report_count == 0) tspkFlush();
        #endif

    }

}

#endif // SENSOR_SUPPORT
