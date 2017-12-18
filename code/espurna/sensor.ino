/*

SENSOR MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <vector>
#include "filters/MedianFilter.h"
#include "filters/MovingAverageFilter.h"
#include "sensors/BaseSensor.h"

typedef struct {
    BaseSensor * sensor;
    unsigned char local;        // Local index in its provider
    magnitude_t type;           // Type of measurement
    unsigned char global;       // Global index in its type
    double current;             // Current (last) value, unfiltered
    double filtered;            // Filtered (averaged) value
    double reported;            // Last reported value
    double min_change;          // Minimum value change to report
    BaseFilter * filter;    // Filter object
} sensor_magnitude_t;

std::vector<BaseSensor *> _sensors;
std::vector<sensor_magnitude_t> _magnitudes;

unsigned char _counts[MAGNITUDE_MAX];
bool _sensor_realtime = API_REAL_TIME_VALUES;
unsigned char _sensor_temperature_units = SENSOR_TEMPERATURE_UNITS;
double _sensor_temperature_correction = SENSOR_TEMPERATURE_CORRECTION;
unsigned char _sensor_isr = 0xFF;

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

String _sensorTopic(magnitude_t type) {
    if (type == MAGNITUDE_TEMPERATURE) return String(SENSOR_TEMPERATURE_TOPIC);
    if (type == MAGNITUDE_HUMIDITY) return String(SENSOR_HUMIDITY_TOPIC);
    if (type == MAGNITUDE_PRESSURE) return String(SENSOR_PRESSURE_TOPIC);
    if (type == MAGNITUDE_CURRENT) return String(SENSOR_CURRENT_TOPIC);
    if (type == MAGNITUDE_VOLTAGE) return String(SENSOR_VOLTAGE_TOPIC);
    if (type == MAGNITUDE_POWER_ACTIVE) return String(SENSOR_ACTIVE_POWER_TOPIC);
    if (type == MAGNITUDE_POWER_APPARENT) return String(SENSOR_APPARENT_POWER_TOPIC);
    if (type == MAGNITUDE_POWER_REACTIVE) return String(SENSOR_REACTIVE_POWER_TOPIC);
    if (type == MAGNITUDE_POWER_FACTOR) return String(SENSOR_POWER_FACTOR_TOPIC);
    if (type == MAGNITUDE_ENERGY) return String(SENSOR_ENERGY_TOPIC);
    if (type == MAGNITUDE_ENERGY_DELTA) return String(SENSOR_ENERGY_DELTA_TOPIC);
    if (type == MAGNITUDE_ANALOG) return String(SENSOR_ANALOG_TOPIC);
    if (type == MAGNITUDE_EVENTS) return String(SENSOR_EVENTS_TOPIC);
    if (type == MAGNITUDE_PM1dot0) return String(SENSOR_PM1dot0_TOPIC);
    if (type == MAGNITUDE_PM2dot5) return String(SENSOR_PM2dot5_TOPIC);
    if (type == MAGNITUDE_PM10) return String(SENSOR_PM10_TOPIC);
    return String(SENSOR_UNKNOWN_TOPIC);
}

unsigned char _sensorDecimals(magnitude_t type) {
    if (type == MAGNITUDE_TEMPERATURE) return SENSOR_TEMPERATURE_DECIMALS;
    if (type == MAGNITUDE_HUMIDITY) return SENSOR_HUMIDITY_DECIMALS;
    if (type == MAGNITUDE_PRESSURE) return SENSOR_PRESSURE_DECIMALS;
    if (type == MAGNITUDE_CURRENT) return SENSOR_CURRENT_DECIMALS;
    if (type == MAGNITUDE_VOLTAGE) return SENSOR_VOLTAGE_DECIMALS;
    if (type == MAGNITUDE_POWER_ACTIVE) return SENSOR_POWER_DECIMALS;
    if (type == MAGNITUDE_POWER_APPARENT) return SENSOR_POWER_DECIMALS;
    if (type == MAGNITUDE_POWER_REACTIVE) return SENSOR_POWER_DECIMALS;
    if (type == MAGNITUDE_POWER_FACTOR) return SENSOR_POWER_FACTOR_DECIMALS;
    if (type == MAGNITUDE_ENERGY) return SENSOR_ENERGY_DECIMALS;
    if (type == MAGNITUDE_ENERGY_DELTA) return SENSOR_ENERGY_DECIMALS;
    if (type == MAGNITUDE_ANALOG) return SENSOR_ANALOG_DECIMALS;
    if (type == MAGNITUDE_EVENTS) return SENSOR_EVENTS_DECIMALS;
    if (type == MAGNITUDE_PM1dot0) return SENSOR_PM1dot0_DECIMALS;
    if (type == MAGNITUDE_PM2dot5) return SENSOR_PM2dot5_DECIMALS;
    if (type == MAGNITUDE_PM10) return SENSOR_PM10_DECIMALS;
    return 0;
}

String _sensorUnits(magnitude_t type) {
    if (type == MAGNITUDE_TEMPERATURE) return (_sensor_temperature_units == TMP_CELSIUS) ? String("C") : String("F");
    if (type == MAGNITUDE_HUMIDITY) return String("%");
    if (type == MAGNITUDE_PRESSURE) return String("hPa");
    if (type == MAGNITUDE_CURRENT) return String("A");
    if (type == MAGNITUDE_VOLTAGE) return String("V");
    if (type == MAGNITUDE_POWER_ACTIVE) return String("W");
    if (type == MAGNITUDE_POWER_APPARENT) return String("W");
    if (type == MAGNITUDE_POWER_REACTIVE) return String("W");
    if (type == MAGNITUDE_POWER_FACTOR) return String("%");
    if (type == MAGNITUDE_ENERGY) return String("J");
    if (type == MAGNITUDE_ENERGY_DELTA) return String("J");
    if (type == MAGNITUDE_EVENTS) return String("/min");
    if (type == MAGNITUDE_PM1dot0) return String("µg/m3");
    if (type == MAGNITUDE_PM2dot5) return String("µg/m3");
    if (type == MAGNITUDE_PM10) return String("µg/m3");
    return String();
}

double _sensorProcess(magnitude_t type, double value) {
    if (type == MAGNITUDE_TEMPERATURE) {
        if (_sensor_temperature_units == TMP_FAHRENHEIT) value = value * 1.8 + 32;
        value = value + _sensor_temperature_correction;
    }
    return roundTo(value, _sensorDecimals(type));
}

void _sensorConfigure() {
    _sensor_realtime = getSetting("apiRealTime", API_REAL_TIME_VALUES).toInt() == 1;
    _sensor_temperature_units = getSetting("tmpUnits", SENSOR_TEMPERATURE_UNITS).toInt();
    _sensor_temperature_correction = getSetting("tmpCorrection", SENSOR_TEMPERATURE_CORRECTION).toFloat();
}

#if WEB_SUPPORT

void _sensorWebSocketOnSend(JsonObject& root) {

    char buffer[10];
    bool hasTemperature = false;

    JsonArray& sensors = root.createNestedArray("sensors");
    for (unsigned char i=0; i<_magnitudes.size(); i++) {

        sensor_magnitude_t magnitude = _magnitudes[i];
        unsigned char decimals = _sensorDecimals(magnitude.type);
        dtostrf(magnitude.current, 1-sizeof(buffer), decimals, buffer);

        JsonObject& sensor = sensors.createNestedObject();
        sensor["type"] = int(magnitude.type);
        sensor["value"] = String(buffer);
        sensor["units"] = _sensorUnits(magnitude.type);
        sensor["description"] = magnitude.sensor->slot(magnitude.local);

        if (magnitude.type == MAGNITUDE_TEMPERATURE) hasTemperature = true;

    }

    //root["apiRealTime"] = _sensor_realtime;
    root["tmpUnits"] = _sensor_temperature_units;
    root["tmpCorrection"] = _sensor_temperature_correction;
    if (hasTemperature) root["temperatureVisible"] = 1;

}

void _sensorAPISetup() {

    for (unsigned char magnitude_id=0; magnitude_id<_magnitudes.size(); magnitude_id++) {

        sensor_magnitude_t magnitude = _magnitudes[magnitude_id];

        String topic = _sensorTopic(magnitude.type);
        if (SENSOR_USE_INDEX || (_counts[magnitude.type] > 1)) topic = topic + "/" + String(magnitude.global);

        apiRegister(topic.c_str(), topic.c_str(), [magnitude_id](char * buffer, size_t len) {
            sensor_magnitude_t magnitude = _magnitudes[magnitude_id];
            unsigned char decimals = _sensorDecimals(magnitude.type);
            double value = _sensor_realtime ? magnitude.current : magnitude.filtered;
            dtostrf(value, 1-len, decimals, buffer);
        });

    }

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
            DEBUG_MSG("[SENSOR] Error reading data from %s (error: %d)\n",
                _sensors[i]->name().c_str(),
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
// Interrupts
// -----------------------------------------------------------------------------
#if COUNTER_SUPPORT

unsigned char _event_sensor_id = 0;
void isrEventSensor() {
    _sensors[_event_sensor_id]->InterruptHandler();
}

#endif // COUNTER_SUPPORT

// -----------------------------------------------------------------------------
// Values
// -----------------------------------------------------------------------------

void sensorRegister(BaseSensor * sensor) {
    _sensors.push_back(sensor);
}

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

void sensorInit() {

    #if DHT_SUPPORT
        #include "sensors/DHTSensor.h"
        sensorRegister(new DHTSensor(DHT_PIN, DHT_TYPE, DHT_PULLUP));
    #endif

    #if DS18B20_SUPPORT
        #include "sensors/DallasSensor.h"
        sensorRegister(new DallasSensor(DS18B20_PIN, SENSOR_READ_INTERVAL, DS18B20_PULLUP));
    #endif

    #if SI7021_SUPPORT
        #include "sensors/SI7021Sensor.h"
        sensorRegister(new SI7021Sensor(SI7021_ADDRESS));
    #endif

    #if BME280_SUPPORT
        #include "sensors/BME280Sensor.h"
        sensorRegister(new BME280Sensor(BME280_ADDRESS));
    #endif

    #if ANALOG_SUPPORT
        #include "sensors/AnalogSensor.h"
        sensorRegister(new AnalogSensor(ANALOG_PIN));
    #endif

    #if EMON_ANALOG_SUPPORT
        #include "sensors/EmonAnalogSensor.h"
        sensorRegister(new EmonAnalogSensor(A0, EMON_MAINS_VOLTAGE, EMON_ANALOG_ADC_BITS, EMON_ANALOG_REFERENCE_VOLTAGE, EMON_ANALOG_CURRENT_RATIO));
    #endif

    #if EMON_ADC121_SUPPORT
        #include "sensors/EmonADC121Sensor.h"
        sensorRegister(new EmonADC121Sensor(EMON_ADC121_I2C_ADDRESS, EMON_MAINS_VOLTAGE, EMON_ADC121_ADC_BITS, EMON_ADC121_REFERENCE_VOLTAGE, EMON_ADC121_CURRENT_RATIO));
    #endif

    #if EMON_ADS1X15_SUPPORT
        #include "sensors/EmonADS1X15Sensor.h"
        sensorRegister(new EmonADS1X15Sensor(EMON_ADS1X15_I2C_ADDRESS, EMON_ADS1X15_ADS1115, EMON_ADS1X15_PORT_MASK, EMON_MAINS_VOLTAGE, EMON_ADS1X15_ADC_BITS, EMON_ADS1X15_REFERENCE_VOLTAGE, EMON_ADS1X15_CURRENT_RATIO));
    #endif

    #if PMSX003_SUPPORT
        #include "sensors/PMSX003Sensor.h"
        sensorRegister(new PMSX003Sensor(PMS_RX_PIN, PMS_TX_PIN));
    #endif

    #if COUNTER_SUPPORT
        #include "sensors/EventSensor.h"
        sensorRegister(new EventSensor(COUNTER_PIN, COUNTER_PIN_MODE, COUNTER_DEBOUNCE));
        _event_sensor_id = sensorCount() - 1;
        attachInterrupt(COUNTER_PIN, isrEventSensor, COUNTER_INTERRUPT_MODE);
    #endif

}

void sensorSetup() {

    // Load sensors
    sensorInit();

    // Load magnitudes
    for (unsigned char i=0; i<_sensors.size(); i++) {

        BaseSensor * sensor = _sensors[i];
        DEBUG_MSG("[SENSOR] %s\n", sensor->name().c_str());

        for (unsigned char k=0; k<sensor->count(); k++) {

            magnitude_t type = sensor->type(k);

            sensor_magnitude_t new_magnitude;
            new_magnitude.sensor = sensor;
            new_magnitude.local = k;
            new_magnitude.type = type;
            new_magnitude.global = _counts[type];
            new_magnitude.current = 0;
            new_magnitude.filtered = 0;
            new_magnitude.reported = 0;
            new_magnitude.min_change = 0;
            if (type == MAGNITUDE_EVENTS) {
                new_magnitude.filter = new MovingAverageFilter(SENSOR_REPORT_EVERY);
            } else {
                new_magnitude.filter = new MedianFilter();
            }
            _magnitudes.push_back(new_magnitude);

            DEBUG_MSG("[SENSOR]  -> %s:%d\n", _sensorTopic(type).c_str(), _counts[type]);

            _counts[type] = _counts[type] + 1;

        }

    }

    #if WEB_SUPPORT

        // Websockets
        wsOnSendRegister(_sensorWebSocketOnSend);
        wsOnAfterParseRegister(_sensorConfigure);

        // API
        _sensorAPISetup();

    #endif

}

void sensorLoop() {

    static unsigned long last_update = 0;
    static unsigned long report_count = 0;

    // Tick hook
    _sensorTick();

    // Check if we should read new data
    if (millis() - last_update > SENSOR_READ_INTERVAL) {

        last_update = millis();
        report_count = (report_count + 1) % SENSOR_REPORT_EVERY;

        double current;
        double filtered;
        char buffer[64];

        // Pre-read hook
        _sensorPre();

        // Get readings
        for (unsigned char i=0; i<_magnitudes.size(); i++) {

            sensor_magnitude_t magnitude = _magnitudes[i];

            if (magnitude.sensor->status()) {

                unsigned char decimals = _sensorDecimals(magnitude.type);

                current = magnitude.sensor->value(magnitude.local);
                magnitude.filter->add(current);

                // Special case
                if (magnitude.type == MAGNITUDE_EVENTS) current = magnitude.filter->result();

                current = _sensorProcess(magnitude.type, current);
                _magnitudes[i].current = current;

                // Debug
                #if true
                {
                    dtostrf(current, 1-sizeof(buffer), decimals, buffer);
                    DEBUG_MSG("[SENSOR] %s - %s: %s%s\n",
                        magnitude.sensor->slot(magnitude.local).c_str(),
                        _sensorTopic(magnitude.type).c_str(),
                        buffer,
                        _sensorUnits(magnitude.type).c_str()
                    );
                }
                #endif

                // Time to report (we do it every SENSOR_REPORT_EVERY readings)
                if (report_count == 0) {

                    filtered = magnitude.filter->result();
                    magnitude.filter->reset();
                    filtered = _sensorProcess(magnitude.type, filtered);
                    _magnitudes[i].filtered = filtered;

                    // Check if there is a minimum change threshold to report
                    if (fabs(filtered - magnitude.reported) >= magnitude.min_change) {

                        _magnitudes[i].reported = filtered;
                        dtostrf(filtered, 1-sizeof(buffer), decimals, buffer);

                        #if MQTT_SUPPORT
                            if (SENSOR_USE_INDEX || (_counts[magnitude.type] > 1)) {
                                mqttSend(_sensorTopic(magnitude.type).c_str(), magnitude.global, buffer);
                            } else {
                                mqttSend(_sensorTopic(magnitude.type).c_str(), buffer);
                            }
                        #endif

                        #if INFLUXDB_SUPPORT
                            if (SENSOR_USE_INDEX || (_counts[magnitude.type] > 1)) {
                                idbSend(_sensorTopic(magnitude.type).c_str(), magnitude.global, buffer);
                            } else {
                                idbSend(_sensorTopic(magnitude.type).c_str(), buffer);
                            }
                        #endif

                        #if DOMOTICZ_SUPPORT
                        {
                            char key[15];
                            snprintf_P(key, sizeof(key), PSTR("dczSensor%d"), i);
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
                        #endif

                    } // if (fabs(filtered - magnitude.reported) >= magnitude.min_change)
                } // if (report_count == 0)
            } // if (magnitude.sensor->status())
        } // for (unsigned char i=0; i<_magnitudes.size(); i++)

        // Post-read hook
        _sensorPost();

        #if WEB_SUPPORT
            wsSend(_sensorWebSocketOnSend);
        #endif

    }


}
