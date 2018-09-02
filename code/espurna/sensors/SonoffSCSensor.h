// -----------------------------------------------------------------------------
// SonoffSC peudo-sensor
// Communicates with the ATMEGA328 onboard to retrieve
// humidity, temperature, light, sound and dust values
// Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && SONOFFSC_SUPPORT

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

#define SONOFFSC_TERMINATION_CHAR       0x1B

class SonoffSCSensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        SonoffSCSensor(): BaseSensor() {
            _count = 3;
            #if SONOFFSC_HAS_LIGHT_REL
                ++_count;
            #endif
            #if SONOFFSC_HAS_DUST_REL
                ++_count;
            #endif
            #if SONOFFSC_HAS_MOVEMENT
                ++_count;
            #endif
            #if SONOFFSC_HAS_LUX
                ++_count;
            #endif
            #if SONOFFSC_HAS_DUST
                ++_count;
            #endif
            #if SONOFFSC_HAS_CLAP
                ++_count;
            #endif
            _sensor_id = SENSOR_SONOFFSC_ID;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {
            Serial.begin(SONOFFSC_BAUDRATE);
            _send("AT+START");
            _sendConfig();
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() {
            return String("SonoffSC @ HwSerial");
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            return String("sc");
        }

        // Loop-like method, call it in your main loop
        void tick() {

            while (Serial.available()) {

                char ch = Serial.read();

                if (SONOFFSC_TERMINATION_CHAR == ch) {
                    _buffer[_index] = 0;
                    Serial.flush();
                    _parse();
                    _index = 0;
                } else {
                    _buffer[_index] = ch;
                    ++_index;
                    if (SONOFFSC_BUFFER_SIZE == _index) _index = 0;
                }

            }

        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_HUMIDITY;
            if (index == 1) return MAGNITUDE_TEMPERATURE;
            if (index == 2) return MAGNITUDE_NOISE_REL;
            unsigned char next = 3;
            #if SONOFFSC_HAS_LIGHT_REL
                if (index == next) return MAGNITUDE_LIGHT_REL;
                ++next;
            #endif
            #if SONOFFSC_HAS_DUST_REL
                if (index == next) return MAGNITUDE_DUST_REL;
                ++next;
            #endif
            #if SONOFFSC_HAS_MOVEMENT
                if (index == next) return MAGNITUDE_MOVEMENT;
                ++next;
            #endif
            #if SONOFFSC_HAS_LUX
                if (index == next) return MAGNITUDE_LUX;
                ++next;
            #endif
            #if SONOFFSC_HAS_DUST
                if (index == next) return MAGNITUDE_PM10;
                ++next;
            #endif
            #if SONOFFSC_HAS_CLAP
                if (index == next) return MAGNITUDE_EVENT;
                ++next;
            #endif
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            double response = 0;
            if (index == 0) return _humidity;
            if (index == 1) return _temperature;
            if (index == 2) return _noise_rel;
            unsigned char next = 3;
            #if SONOFFSC_HAS_LIGHT_REL
                if (index == next) return _light_rel;
                ++next;
            #endif
            #if SONOFFSC_HAS_DUST_REL
                if (index == next) return _dust_rel;
                ++next;
            #endif
            #if SONOFFSC_HAS_MOVEMENT
                if (index == next) return _movement;
                ++next;
            #endif
            #if SONOFFSC_HAS_LUX
                if (index == next) return _lux;
                ++next;
            #endif
            #if SONOFFSC_HAS_DUST
                if (index == next) return _dust;
                ++next;
            #endif
            #if SONOFFSC_HAS_CLAP
                if (index == next) return _clap;
                ++next;
            #endif
            return response;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _send(const char * message) {

            #if SENSOR_DEBUG
                DEBUG_MSG("[SONOFFSC] Sending: %s\n", message);
            #endif

            Serial.write(message);
            Serial.write(SONOFFSC_TERMINATION_CHAR);

        }

        void _sendConfig() {
            char buffer[64];
            snprintf(buffer, sizeof(buffer),
                "AT+DEVCONFIG=\"uploadFreq\":%lu,\"humiThreshold\":%d,\"tempThreshold\":%d",
                _upload_frequency, _hum_threshold, _tmp_threshold
            );
            _send(buffer);
        }

        void _parse() {

            #if SENSOR_DEBUG
                DEBUG_MSG("[SONOFFSC] Received: %s\n", _buffer);
            #endif

            if (strncmp(_buffer, "AT+UPDATE=", 10) == 0) {

                String haystack = String(_buffer);
                double value;
                unsigned char param_count = 0;

                // -------------------------------------------------------------

                if (_find(haystack, "humidity", value)) {
                    _humidity = value;
                    param_count++;
                }

                if (_find(haystack, "temperature", value)) {
                    _temperature = value;
                    param_count++;
                }

                if (_find(haystack, "noise", value)) {
                    _noise_rel = value * 10.0;
                    param_count++;
                }

                #if SONOFFSC_HAS_LIGHT_REL
                if (_find(haystack, "light", value)) {
                    _light_rel = constrain(10 - value, 0, 10) * 10.0;
                    param_count++;
                }
                #endif

                #if SONOFFSC_HAS_DUST_REL
                if (_find(haystack, "dusty", value)) {
                    _dust_rel = value * 10.0;
                    param_count++;
                }
                #endif
                #if SONOFFSC_HAS_LUX

                if (_find(haystack, "illuminance", value)) {
                    _lux = value;
                    param_count++;
                }
                #endif

                #if SONOFFSC_HAS_DUST
                if (_find(haystack, "dust", value)) {
                    _dust = value;
                    param_count++;
                }
                #endif

                #if SONOFFSC_HAS_MOVEMENT
                if (_find(haystack, "movement", value)) {
                    _movement = value;
                    param_count++;
                }
                #endif

                #if SONOFFSC_HAS_CLAP
                if (_find(haystack, "clap", value)) {
                    _clap = value;
                    if (_callback) _callback(MAGNITUDE_EVENT, _clap);
                    param_count++;
                }
                #endif

                // -------------------------------------------------------------

                if (param_count > 0) {
                    _send("AT+SEND=ok");
                } else {
                    _send("AT+SEND=fail");
                }

            } else if (strncmp(_buffer, "AT+STATUS?", 10) == 0) {
                _send("AT+STATUS=4");
            }


        }

        bool _find(String haystack, const char * key, double &value) {
            String k = String("\"") + String(key) + String("\"");
            unsigned char pos = haystack.indexOf(k);
            if (pos >= 0) {
                unsigned char ch = haystack.charAt(pos+strlen(key)+3);
                if ((ch >= '0' && ch <= '9') || (ch == '-')) {
                    value = haystack.substring(pos+strlen(key)+3).toFloat();
                    return true;
                }
            }
            return false;
        }

        // ---------------------------------------------------------------------

        char _buffer[128];
        unsigned char _index = 0;
        bool _ready_to_parse = false;

        double _humidity = 0;
        double _temperature = 0;
        unsigned char _noise_rel = 0;
        #if SONOFFSC_HAS_LIGHT_REL
            unsigned char _light_rel = 0;
        #endif
        #if SONOFFSC_HAS_DUST_REL
            unsigned char _dust_rel = 0;
        #endif
        #if SONOFFSC_HAS_MOVEMENT
            unsigned char _movement = 0;
        #endif
        #if SONOFFSC_HAS_LUX
            double _lux = 0;
        #endif
        #if SONOFFSC_HAS_DUST
            double _dust = 0;
        #endif
        #if SONOFFSC_HAS_CLAP
            unsigned char _clap = 0;
        #endif

        unsigned long _upload_frequency = 60;
        unsigned long _hum_threshold = 2;
        unsigned long _tmp_threshold = 1;

};

#endif // SENSOR_SUPPORT && SONOFFSC_SUPPORT
