// -----------------------------------------------------------------------------
// EZO™ pH Circuit from Atlas Scientific
//
// Uses SoftwareSerial library
// Copyright (C) 2018 by Rui Marinho <ruipmarinho at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && EZOPH_SUPPORT

#pragma once

#include <SoftwareSerial.h>

#include "BaseSensor.h"

class EZOPHSensor : public BaseSensor {

    public:

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

        unsigned char id() const override {
            return SENSOR_EZOPH_ID;
        }

        unsigned char count() const override {
            return 1;
        }

        // Initialization method, must be idempotent
        void begin() override {
            if (!_dirty) return;

            if (_serial) {
                _serial.reset(nullptr);
            }

            _serial = std::make_unique<SoftwareSerial>(_pin_rx, _pin_tx);
            _serial->enableIntTx(false);
            _serial->begin(9600);

            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[28];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("EZOPH @ SwSerial(%u,%u)"), _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) const override {
            char buffer[8];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("%hhu:%hhu"), _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) return MAGNITUDE_PH;
            return MAGNITUDE_NONE;
        }

        void tick() override {
            _setup();
            _read();
        }

        // Current value for slot # index
        double value(unsigned char index) override {
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

          String data;
          data.reserve(30);

          if (!_sync_requested) {
              _serial->write(67); // C
              _serial->write(44); // ,
              _serial->write(63); // ?
              _serial->write(13); // \r
              _serial->flush();

              _sync_requested = true;
          }

          data += _serial->readStringUntil('\r');
          if (data.startsWith("?C,")) {
              auto interval = TimeSource::duration(
                    data.substring(data.indexOf(",") + 1).toInt() * 1000);

              if (!interval.count()) {
                  _error = SENSOR_ERROR_OTHER;
                  return;
              }

              _sync_interval = interval;
          } else if (data.startsWith("*OK")) {
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

            const auto now = TimeSource::now();
            if (now - _timestamp < _sync_interval) {
              return;
            }

            _timestamp = now;

            String data;
            data.reserve(30);

            data += _serial->readStringUntil('\r');

            if (data == F("*ER")) {
              _error = SENSOR_ERROR_OTHER;
              return;
            }

            _error = SENSOR_ERROR_OK;
            _ph = data.toFloat();
        }

        bool _sync_requested = false;
        bool _sync_responded = false;

        // Maximum continuous reading interval allowed is 99000 milliseconds.
        using TimeSource = espurna::time::CoreClock;
        TimeSource::duration _sync_interval { espurna::duration::Milliseconds(100000) };
        TimeSource::time_point _timestamp;

        double _ph = 0;
        unsigned char _pin_rx;
        unsigned char _pin_tx;
        std::unique_ptr<SoftwareSerial> _serial;

};

#endif // SENSOR_SUPPORT && EZOPH_SUPPORT
