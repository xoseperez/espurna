// -----------------------------------------------------------------------------
// PZEM004T based power monitor
// Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && PZEM004T_SUPPORT

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

#include <PZEM004T.h>

class PZEM004TSensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        PZEM004TSensor(): BaseSensor(), _data() {
            _count = 4;
            _sensor_id = SENSOR_PZEM004T_ID;
        }

        ~PZEM004TSensor() {
            if (_pzem) delete _pzem;
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

        void setSerial(Stream & serial) {
            _serial = serial;
            _dirty = true;
        }

        // ---------------------------------------------------------------------

        unsigned char getRX() {
            return _pin_rx;
        }

        unsigned char getTX() {
            return _pin_tx;
        }

        Stream & getSerial() {
            return _serial;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;

            if (_pzem) delete _pzem;
            if (_serial == NULL) {
                _pzem = PZEM004T(_pin_rx, _pin_tx);
            } else {
                _pzem = PZEM004T(_serial);
            }
            _pzem->setAddress(_ip);

            _ready = true;
            _dirty = false;

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[28];
            snprintf(buffer, sizeof(buffer), "PZEM004T @ SwSerial(%u,%u)", _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            return _ip.toString();
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_CURRENT;
            if (index == 1) return MAGNITUDE_VOLTAGE;
            if (index == 2) return MAGNITUDE_POWER_ACTIVE;
            if (index == 3) return MAGNITUDE_ENERGY;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _pzem->current(_ip);
            if (index == 1) return _pzem->voltage(_ip);
            if (index == 2) return _pzem->power(_ip);
            if (index == 3) return _pzem->energy(_ip);
            return 0;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        unsigned int _pin_rx = PZEM004T_RX_PIN;
        unsigned int _pin_tx = PZEM004T_TX_PIN;
        Stream & _serial = NULL;
        IPAddress _ip(192,168,1,1);
        PZEM004T * _pzem = NULL;

};

#endif // SENSOR_SUPPORT && PZEM004T_SUPPORT
