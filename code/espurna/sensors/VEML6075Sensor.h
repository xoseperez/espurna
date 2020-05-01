// -----------------------------------------------------------------------------
// VEML6075 Sensor over I2C
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && VEML6075_SUPPORT

#pragma once

#include <Arduino.h>
#include <SparkFun_VEML6075_Arduino_Library.h>

#include "I2CSensor.h"

class VEML6075Sensor : public I2CSensor<> {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        VEML6075Sensor() {
            _count = 3;
            _sensor_id = SENSOR_VEML6075_ID;
            _veml6075 = new VEML6075();
        }

        ~VEML6075Sensor() {
          delete _veml6075;
        }

        void begin() {
          if (!_veml6075->begin()) {
            return;
          };

          _ready = true;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Descriptive name of the sensor
        String description() {
            char buffer[25];
            snprintf(buffer, sizeof(buffer), "VEML6075 @ I2C (0x%02X)", _address);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        };

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_UVA;
            if (index == 1) return MAGNITUDE_UVB;
            if (index == 2) return MAGNITUDE_UVI;
            return MAGNITUDE_NONE;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {
            _error = SENSOR_ERROR_OK;
        }

        // Current value for slot # index
        double value(unsigned char index) {
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

    protected:

        VEML6075 * _veml6075 = NULL;

};

#endif // SENSOR_SUPPORT && VEML6075_SUPPORT
