// -----------------------------------------------------------------------------
// VEML6075 Sensor over I2C
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && VEML6075_SUPPORT

#pragma once

#include <SparkFun_VEML6075_Arduino_Library.h>

#include "I2CSensor.h"

class VEML6075Sensor : public I2CSensor<> {

    public:

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_VEML6075_ID;
        }

        unsigned char count() const override {
            return 3;
        }

        void begin() override {
            if (_veml6075) {
                _veml6075.reset(nullptr);
            }

            _ready = false;
            _veml6075 = std::make_unique<VEML6075>();
            if (!_veml6075->begin()) {
                return;
            }

            _ready = true;
        }


        // Descriptive name of the sensor
        String description() const override {
            char buffer[25];
            snprintf_P(buffer, sizeof(buffer),
                PSTR("VEML6075 @ I2C (0x%02X)"), lockedAddress());
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) return MAGNITUDE_UVA;
            if (index == 1) return MAGNITUDE_UVB;
            if (index == 2) return MAGNITUDE_UVI;
            return MAGNITUDE_NONE;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() override {
            _error = SENSOR_ERROR_OK;
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) return _veml6075->a();
            if (index == 1) return _veml6075->b();
            if (index == 2) return _veml6075->index();

            return 0;
        }

        void setIntegrationTime(VEML6075::veml6075_uv_it_t integration_time) {
            _veml6075->setIntegrationTime(integration_time);
        }

        void setDynamicMode(VEML6075::veml6075_hd_t dynamic_mode) {
            _veml6075->setHighDynamic(dynamic_mode);
        }

    private:

        std::unique_ptr<VEML6075> _veml6075;

};

#endif // SENSOR_SUPPORT && VEML6075_SUPPORT
