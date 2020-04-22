// -----------------------------------------------------------------------------
// Analog Sensor (maps to an analogRead)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && (ANALOG_SUPPORT || NTC_SUPPORT || LDR_SUPPORT)

#pragma once

#include <Arduino.h>

#include "../debug.h"

#include "BaseSensor.h"
#include "BaseAnalogSensor.h"

class AnalogSensor : public BaseAnalogSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        AnalogSensor() {
            _count = 1;
            _sensor_id = SENSOR_ANALOG_ID;
        }

        void setSamples(unsigned int samples) {
            if (_samples > 0) _samples = samples;
        }

        void setDelay(unsigned long micros) {
            _micros = micros;
        }

        void setFactor(double factor) {
            //DEBUG_MSG(("[ANALOG_SENSOR] Factor set to: %s \n"), String(factor,6).c_str());
            _factor = factor;
        }

        void setOffset(double offset) {
          //DEBUG_MSG(("[ANALOG_SENSOR] Factor set to: %s \n"), String(offset,6).c_str());
            _offset = offset;
        }

        // ---------------------------------------------------------------------

        unsigned int getSamples() {
            return _samples;
        }

        unsigned long getDelay() {
            return _micros;
        }

        double getFactor() {
            return _factor;
        }

        double getOffset() {
            return _offset;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {            
            _ready = true;
        }

        // Descriptive name of the sensor
        String description() {
            return String("ANALOG @ TOUT");
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            return String("0");
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_ANALOG;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        // Changed return type as moving to scaled value
        double value(unsigned char index) {
            if (index == 0) return _read();
            return 0;
        }

    protected:

        //CICM: this should be for raw values
        // renaming protected function "_read" to "_rawRead"
        unsigned int _rawRead() {
            if (1 == _samples) return analogRead(0);
            unsigned long sum = 0;
            for (unsigned int i=0; i<_samples; i++) {
                if (i>0) delayMicroseconds(_micros);
                sum += analogRead(0);
            }
            return sum / _samples;
        }

        //CICM: and proper read should be scalable and thus needs sign
        //and decimal part
        double _read() {
          //Raw measure could also be a class variable with getter so that can
          //be reported through MQTT, ...
          unsigned int rawValue;
          double scaledValue;
          // Debugging doubles to string
          //DEBUG_MSG(("[ANALOG_SENSOR] Started standard read, factor: %s , offset: %s, decimals: %d \n"), String(_factor).c_str(), String(_offset).c_str(), ANALOG_DECIMALS);
          rawValue = _rawRead();
          //DEBUG_MSG(("[ANALOG_SENSOR] Raw read received: %d \n"), rawValue);
          scaledValue = _factor*rawValue  + _offset;
          //DEBUG_MSG(("[ANALOG_SENSOR] Scaled value result: %s \n"), String(scaledValue).c_str());
          return scaledValue;
        }


        unsigned int _samples = 1;
        unsigned long _micros = 0;
        //CICM: for scaling and offset, also with getters and setters
        double _factor = 1.0;
        double _offset = 0.0;

};

#endif // SENSOR_SUPPORT && ANALOG_SUPPORT
