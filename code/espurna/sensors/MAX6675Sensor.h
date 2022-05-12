// -----------------------------------------------------------------------------
// MAX6675 Sensor
// Uses MAX6675_Thermocouple library
// Copyright (C) 2017-2019 by Xose Pérez <andrade dot luciano at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && MAX6675_SUPPORT

#pragma once

#include <MAX6675.h>

#include <vector>

#include "BaseSensor.h"

class MAX6675Sensor : public BaseSensor {

    public:

        void setCS(unsigned char pin_cs) {
            if (_pin_cs == pin_cs) return;
            _pin_cs = pin_cs;
            _dirty = true;
        }

        void setSO(unsigned char pin_so) {
            if (_pin_so == pin_so) return;
            _pin_so = pin_so;
            _dirty = true;
        }

        void setSCK(unsigned char pin_sck) {
            if (_pin_sck == pin_sck) return;
            _pin_sck = pin_sck;
            _dirty = true;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_MAX6675_ID;
        }

        unsigned char count() const override {
            return 1;
        }

        // ---------------------------------------------------------------------
        // Initialization method, must be idempotent
        void begin() override {

            if (!_dirty) return;

            if (_max) {
                _max.reset(nullptr);
            }

            // MAX6675 Units to readout temp (0 = raw, 1 = ˚C, 2 = ˚F)
            _max = std::make_unique<MAX6675>(_pin_cs, _pin_so, _pin_sck, 1);

            _ready = true;
            _dirty = false;

        }

        // Loop-like method, call it in your main loop
        void tick() override {
            const auto now = TimeSource::now();
            if (now - _last_reading > ReadInterval) {
                _last_reading = now;
                _value = _max->read_temp();
            }
        }

        // Descriptive name of the sensor
        String description() const override {
            return F("MAX6675");
        }

        String address(unsigned char) const override {
            char buffer[16] {0};
            snprintf_P(buffer, sizeof(buffer),
                PSTR("%hhu:%hhu:%hhu"), _pin_cs, _pin_so, _pin_sck);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) return MAGNITUDE_TEMPERATURE;
            return MAGNITUDE_NONE;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() override {
            _error = SENSOR_ERROR_OK;
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) {
                return _value;
            }

            return 0.0;
        }

    private:
        using TimeSource = espurna::time::CoreClock;
        static constexpr auto ReadInterval = TimeSource::duration { 3000 };
        TimeSource::time_point _last_reading = TimeSource::now();

        unsigned char _pin_cs = MAX6675_CS_PIN;
        unsigned char _pin_so = MAX6675_SO_PIN;
        unsigned char _pin_sck = MAX6675_SCK_PIN;
        bool _busy = false;
        double _value = 0;
        std::unique_ptr<MAX6675> _max;

};

#endif // SENSOR_SUPPORT && MAX6675_SUPPORT
