// -----------------------------------------------------------------------------
// EZO™ pH Circuit from Atlas Scientific
//
// Uses SoftwareSerial library
// Copyright (C) 2018 by Rui Marinho <ruipmarinho at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && EZOPH_SUPPORT

#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>

#include "BaseSensor.h"

class EZOPHSensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        EZOPHSensor() {
            _count = 1;
            _sensor_id = SENSOR_EZOPH_ID;
        }

        ~EZOPHSensor() {
            if (_serial) delete _serial;
        }

        // ---------------------------------------------------------------------

        void setRX(unsigned char pin_rx) {
            if (_pin_rx == pin_rx) return;
            _pin_rx = pin_rx;
            _dirty = true;
        }

        void setTX(unsigned char pin_tx) {
            if (_pin_tx == pin_tx) return;
            _pin_tx = pin_tx;
            _dirty = true;
        }

        // ---------------------------------------------------------------------

        unsigned char getRX() {
            return _pin_rx;
        }

        unsigned char getTX() {
            return _pin_tx;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {
            if (!_dirty) return;

            if (_serial) delete _serial;

            _serial = new SoftwareSerial(_pin_rx, _pin_tx);
            _serial->enableIntTx(false);
            _serial->begin(9600);

            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[28];
            snprintf(buffer, sizeof(buffer), "EZOPH @ SwSerial(%u,%u)", _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            char buffer[6];
            snprintf(buffer, sizeof(buffer), "%u:%u", _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_PH;
            return MAGNITUDE_NONE;
        }

        void tick() {
            _setup();
            _read();
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _ph;
            return 0;
        }



    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _setup() {
          if (_sync_responded) {
            return;
          }

          _error = SENSOR_ERROR_WARM_UP;

          String sync_serial = "";
          sync_serial.reserve(30);

          if (!_sync_requested) {
              _serial->write(67); // C
              _serial->write(44); // ,
              _serial->write(63); // ?
              _serial->write(13); // \r
              _serial->flush();

              _sync_requested = true;
          }

          while ((_serial->available() > 0)) {
              char sync_char = (char)_serial->read();
              sync_serial += sync_char;

              if (sync_char == '\r') {
                break;
              }
          }

          if (sync_serial.startsWith("?C,")) {
              _sync_interval = sync_serial.substring(sync_serial.indexOf(",") + 1).toInt() * 1000;

              if (_sync_interval == 0) {
                  _error = SENSOR_ERROR_OTHER;
                  return;
              }
          }

          if (sync_serial.startsWith("*OK")) {
              _sync_responded = true;
          }

          if (!_sync_responded) {
            return;
          }

          _error = SENSOR_ERROR_OK;
        }

        void _read() {
            if (_error != SENSOR_ERROR_OK) {
              return;
            }

            if (millis() - _ts <= _sync_interval) {
              return;
            }

            _ts = millis();

            String ph_serial = "";
            ph_serial.reserve(30);

            while ((_serial->available() > 0)) {
                char ph_char = (char)_serial->read();
                ph_serial += ph_char;

                if (ph_char == '\r') {
                  break;
                }
            }

            if (ph_serial == "*ER") {
              _error = SENSOR_ERROR_OTHER;
              return;
            }

            _ph = ph_serial.toFloat();

            _error = SENSOR_ERROR_OK;
        }

        bool _sync_requested = false;
        bool _sync_responded = false;
        unsigned long _sync_interval = 100000; // Maximum continuous reading interval allowed is 99000 milliseconds.
        unsigned long _ts = 0;
        double _ph = 0;
        unsigned int _pin_rx;
        unsigned int _pin_tx;
        SoftwareSerial * _serial = NULL;

};

#endif // SENSOR_SUPPORT && EZOPH_SUPPORT
