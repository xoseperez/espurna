// -----------------------------------------------------------------------------
// SI1145 Sensor over I2C
// Copyright (C) 2020 by @HilverinkJ (https://github.com/HilverinkJ)
// Based on https://github.com/xoseperez/espurna/issues/2192#issuecomment-603430308
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && SI1145_SUPPORT

#pragma once

#include <Adafruit_SI1145.h>

#include "I2CSensor.h"

class SI1145Sensor : public I2CSensor<> {

     public:

         unsigned char id() const override {
            return SENSOR_SI1145_ID;
         }

         unsigned char count() const override {
            return 1;
         }

         void begin() override {
             const auto address = findAndLock();
             if (address == 0) {
                 return;
             }

             if (!_si1145.begin()) {
                 _ready = false;
                 return;
             }

             // Adafruit library never sets any errors
             _error = SENSOR_ERROR_OK;
             _ready = true;
         }

         // ---------------------------------------------------------------------
         // Sensor API
         // ---------------------------------------------------------------------

         // Descriptive name of the sensor
         String description() const override {
             char buffer[32];
             snprintf_P(buffer, sizeof(buffer),
                PSTR("SI1145 @ I2C (0x%02X)"), lockedAddress());
             return String(buffer);
         }

         // Descriptive name of the slot # index
         String description(unsigned char index) const override {
             return description();
         };

         // Type for slot # index
         unsigned char type(unsigned char index) const override {
             if (index == 0) return MAGNITUDE_UVI;
             return MAGNITUDE_NONE;
         }

         // Pre-read hook (usually to populate registers with up-to-date data)
         void pre() override {
             _uvi = _si1145.readUV() / 100.0;
         }

         // Current value for slot # index
         double value(unsigned char index) override {
             if (index == 0) return _uvi;
             return 0.0;
         }

     private:
         Adafruit_SI1145 _si1145;
         double _uvi = 0.0;
};

#endif // SENSOR_SUPPORT && SI1145_SUPPORT
