/*

SENSOR MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <vector>
#include "libs/AggregatorMedian.h"
#include "libs/AggregatorMovingAverage.h"
#include "sensors/SensorBase.h"

typedef struct {
    SensorBase * sensor;
    unsigned char local;        // Local index in its provider
    magnitude_t type;           // Type of measurement
    unsigned char global;       // Global index in its type
    double current;             // Current (last) value, unfiltered
    double filtered;            // Filtered (averaged) value
    AggregatorBase * filter;    // Filter object
} sensor_magnitude_t;

std::vector<SensorBase *> _sensors;
std::vector<sensor_magnitude_t> _magnitudes;

unsigned char _counts[MAGNITUDE_MAX];
bool _sensor_realtime = API_REAL_TIME_VALUES;
unsigned char _sensor_temperature_units = SENSOR_TEMPERATURE_UNITS;
double _sensor_temperature_correction = SENSOR_TEMPERATURE_CORRECTION;

#if DHT_SUPPORT
    #include "sensors/SensorDHT.h"
#endif

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

String _sensorTopic(magnitude_t type) {
    if (type == MAGNITUDE_TEMPERATURE) {
        return String(SENSOR_TEMPERATURE_TOPIC);
    } else if (type == MAGNITUDE_HUMIDITY) {
        return String(SENSOR_HUMIDITY_TOPIC);
    }
    return String(SENSOR_UNKNOWN_TOPIC);
}

unsigned char _sensorDecimals(magnitude_t type) {
    if (type == MAGNITUDE_TEMPERATURE) {
        return SENSOR_TEMPERATURE_DECIMALS;
    } else if (type == MAGNITUDE_HUMIDITY) {
        return SENSOR_HUMIDITY_DECIMALS;
    }
    return 0;
}

String _sensorUnits(magnitude_t type) {
    if (type == MAGNITUDE_TEMPERATURE) {
        if (_sensor_temperature_units == TMP_CELSIUS) {
            return String("C");
        } else {
            return String("F");
        }
    } else if (type == MAGNITUDE_HUMIDITY) {
        return String("%");
    }
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

    bool hasTemperature = false;

    JsonArray& sensors = root.createNestedArray("sensors");
    for (unsigned char i=0; i<_magnitudes.size(); i++) {

        sensor_magnitude_t magnitude = _magnitudes[i];
        JsonObject& sensor = sensors.createNestedObject();
        sensor["type"] = int(magnitude.type);
        sensor["value"] = magnitude.current;
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

// -----------------------------------------------------------------------------
// Values
// -----------------------------------------------------------------------------

void sensorSetup() {

    // Load sensors
    #if DHT_SUPPORT
    {
        _sensors.push_back(new SensorDHT(DHT_PIN, DHT_TYPE));
        #if DHT_PULLUP
            pinMode(DHT_PIN, INPUT_PULLUP);
        #endif
    }
    #endif

    // Read magnitudes
    for (unsigned char i=0; i<_sensors.size(); i++) {

        SensorBase * sensor = _sensors[i];
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
            if (type == MAGNITUDE_EVENTS) {
                new_magnitude.filter = new AggregatorMovingAverage(SENSOR_REPORT_EVERY);
            } else {
                new_magnitude.filter = new AggregatorMedian();
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

    // Check if we should read new data
    if ((millis() - last_update > SENSOR_READ_INTERVAL) || (last_update == 0)) {

        last_update = millis();
        report_count = (report_count + 1) % SENSOR_REPORT_EVERY;

        double value;
        char buffer[64];

        // Pre-read hook
        for (unsigned char i=0; i<_sensors.size(); i++) {
            _sensors[i]->pre();
            if (!_sensors[i]->status()) {
                DEBUG_MSG("[SENSOR] Error reading data from %s (error: %d)\n",
                    _sensors[i]->name().c_str(),
                    _sensors[i]->error()
                );
            }
        }

        // Get readings
        for (unsigned char i=0; i<_magnitudes.size(); i++) {

            sensor_magnitude_t magnitude = _magnitudes[i];

            if (magnitude.sensor->status()) {

                unsigned char decimals = _sensorDecimals(magnitude.type);

                value = magnitude.sensor->value(magnitude.local);
                magnitude.filter->add(value);
                value = _sensorProcess(magnitude.type, value);
                _magnitudes[i].current = value;

                // Debug
                /*
                {
                    dtostrf(value, 1-sizeof(buffer), decimals, buffer);
                    DEBUG_MSG("[SENSOR] %s - %s: %s%s\n",
                        magnitude.sensor->name().c_str(),
                        _sensorTopic(magnitude.type).c_str(),
                        buffer,
                        _sensorUnits(magnitude.type).c_str()
                    );
                }
                */

                if (report_count == 0) {

                    double value = magnitude.filter->result();
                    value = _sensorProcess(magnitude.type, value);
                    _magnitudes[i].filtered = value;
                    magnitude.filter->reset();
                    dtostrf(value, 1-sizeof(buffer), decimals, buffer);

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
                        // TODO
                    #endif

                }
            }
        }

        // Post-read hook
        for (unsigned char i=0; i<_sensors.size(); i++) {
            _sensors[i]->post();
        }

        #if WEB_SUPPORT
            wsSend(_sensorWebSocketOnSend);
        #endif

    }


}
