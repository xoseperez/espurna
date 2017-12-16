// -----------------------------------------------------------------------------
// PMSX003 Dust sensor
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

#include <PMS.h>
#include <SoftwareSerial.h>

class PMSX003Sensor : public BaseSensor {

    public:

        PMSX003Sensor(int pin_rx = PMS_RX_PIN, int pin_tx = PMS_TX_PIN): BaseSensor() {
            _pmsSerial = new SoftwareSerial(pin_rx, pin_tx, false, 256);
            _pmsSerial->begin(9600);
            _pms = new PMS(* _pmsSerial);
            // _pmsSerial.begin(9600);
            _pin_rx = pin_rx;
            _pin_tx = pin_tx;
            _count = 3;
        }

        // Descriptive name of the sensor
        String name() {
            char buffer[36];
            snprintf(buffer, sizeof(buffer), "PMSX003 @ SwSerial RX: %i TX: %i", _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            if (index < _count) {
                _error = SENSOR_ERROR_OK;
                if (index == 0) return String("PM 1.0");
                if (index == 1) return String("PM 2.5");
                if (index == 2) return String("PM 10");
            }
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return String();
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
            if (index < _count) {
                _error = SENSOR_ERROR_OK;
                if (index == 0) return MAGNITUDE_PM1dot0;
                if (index == 1) return MAGNITUDE_PM2dot5;
                if (index == 2) return MAGNITUDE_PM10;
            }
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index < _count) {
                _error = SENSOR_ERROR_OK;
                if(index == 0) return _pm1dot0;
                if(index == 1) return _pm2dot5;
                if(index == 2) return _pm10;
            }
            _error = SENSOR_ERROR_OUT_OF_RANGE;
            return 0;
        }

        void tick() {
            if(_pms->read(_data, 1000)) {
            // if (_pms.read(_data)) {
                _pm1dot0 = _data.PM_AE_UG_1_0;
                _pm2dot5 = _data.PM_AE_UG_2_5;
                _pm10 = _data.PM_AE_UG_10_0;
            }
        }
    
    protected:
        unsigned int _pm1dot0;
        unsigned int _pm2dot5;
        unsigned int _pm10;
        unsigned int _pin_rx;
        unsigned int _pin_tx;
        SoftwareSerial * _pmsSerial;
        PMS * _pms;
        // SoftwareSerial _pmsSerial;
        // PMS _pms;
        PMS::DATA _data;

};
