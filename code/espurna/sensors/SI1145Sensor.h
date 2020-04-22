// -----------------------------------------------------------------------------
// SI1145 Sensor over I2C
// Copyright (C) 2020 by @HilverinkJ (https://github.com/HilverinkJ)
// Based on https://github.com/xoseperez/espurna/issues/2192#issuecomment-603430308
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && SI1145_SUPPORT

#pragma once

#include <Arduino.h>
#include <Adafruit_SI1145.h>

#include "I2CSensor.h"

class SI1145Sensor : public I2CSensor<> {

     public:

         SI1145Sensor() {
             _count = 1;
             _sensor_id = SENSOR_SI1145_ID;
             _si1145 = new Adafruit_SI1145();
         }

         void begin() { 
             static unsigned char addresses[1] = { SI1145_ADDRESS };
             _address = _begin_i2c(_address, sizeof(addresses), addresses);
             if (_address == 0) return;

             if (!_si1145->begin()) {
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
         String description() {
             char buffer[25];
             snprintf(buffer, sizeof(buffer), "SI1145 @ I2C (0x%02X)", _address);
             return String(buffer);
         }

         // Descriptive name of the slot # index
         String slot(unsigned char index) {
             return description();
         };

         // Type for slot # index
         unsigned char type(unsigned char index) {
             if (index == 0) return MAGNITUDE_UVI;
             return MAGNITUDE_NONE;
         }

         // Pre-read hook (usually to populate registers with up-to-date data)
         void pre() {
             _uvi = _si1145->readUV() / 100.0;
         }

         // Current value for slot # index
         double value(unsigned char index) {
             if (index == 0) return _uvi; 
             return 0.0;
         }

     protected: 
         Adafruit_SI1145 * _si1145 = nullptr;
         double _uvi = 0.0;
};
   
#endif // SENSOR_SUPPORT && SI1145_SUPPORT
