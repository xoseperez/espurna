// -----------------------------------------------------------------------------
// VL53L1X Sensor over I2C
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && VL53L1X_SUPPORT

#pragma once

#include <Arduino.h>
#include <VL53L1X.h>

#include "I2CSensor.h"

class VL53L1XSensor : public I2CSensor<> {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        VL53L1XSensor() {
            _count = 1;
            _sensor_id = SENSOR_VL53L1X_ID;
            _vl53l1x = new VL53L1X();
        }

        ~VL53L1XSensor() {
          delete _vl53l1x;
        }

        // ---------------------------------------------------------------------

        void setDistanceMode(VL53L1X::DistanceMode mode) {
          _vl53l1x->setDistanceMode(mode);
        }

        void setMeasurementTimingBudget(uint32_t budget_us) {
          _vl53l1x->setMeasurementTimingBudget(budget_us);
        }

        void setInterMeasurementPeriod(unsigned int period) {
          if (_inter_measurement_period == period) return;
          _inter_measurement_period = period;
          _dirty = true;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        void begin() {
          if (!_dirty) {
            return;
          }

          // I2C auto-discover
          unsigned char addresses[] = {0x29};
          _address = _begin_i2c(_address, sizeof(addresses), addresses);
          if (_address == 0) return;

          _vl53l1x->setAddress(_address);

          if (!_vl53l1x->init()) {
            return;
          };

          _vl53l1x->startContinuous(_inter_measurement_period);

          _ready = true;
          _dirty = false;
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[21];
            snprintf(buffer, sizeof(buffer), "VL53L1X @ I2C (0x%02X)", _address);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        };

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_DISTANCE;
            return MAGNITUDE_NONE;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {
            if (!_vl53l1x->dataReady()) {
              return;
            }

            _distance = (double) _vl53l1x->read(false) / 1000.00;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index != 0) return 0;
            return _distance;
        }

    protected:

        VL53L1X * _vl53l1x = NULL;
        unsigned int _inter_measurement_period;
        double _distance = 0;

};

#endif // SENSOR_SUPPORT && VL53L1X_SUPPORT
