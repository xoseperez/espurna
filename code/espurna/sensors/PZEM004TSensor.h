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

        PZEM004TSensor(): BaseSensor() {
            _count = 4;
            _sensor_id = SENSOR_PZEM004T_ID;
            _ip = IPAddress(192,168,1,1);
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

        void setSerial(HardwareSerial * serial) {
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

        // ---------------------------------------------------------------------

        void resetEnergy(double value = 0) {
            if (_ready) {
                _energy_offset = value - (_pzem->energy(_ip) * 3600);
            } else {
                _energy_offset = value;
            }
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;

            if (_pzem) delete _pzem;
            if (_serial) {
                _pzem = new PZEM004T(_serial);
            } else {
                _pzem = new PZEM004T(_pin_rx, _pin_tx);
            }
            _pzem->setAddress(_ip);

            _ready = true;
            _dirty = false;

        }

        // Descriptive name of the sensor
        String description() {
            char buffer[28];
            if (_serial) {
                snprintf(buffer, sizeof(buffer), "PZEM004T @ HwSerial");
            } else {
                snprintf(buffer, sizeof(buffer), "PZEM004T @ SwSerial(%u,%u)", _pin_rx, _pin_tx);
            }
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
            double response = 0;
            if (index == 0) response = _pzem->current(_ip);
            if (index == 1) response = _pzem->voltage(_ip);
            if (index == 2) response = _pzem->power(_ip);
            if (index == 3) response = _energy_offset + (_pzem->energy(_ip) * 3600);
            if (response < 0) response = 0;
            return response;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        unsigned int _pin_rx = PZEM004T_RX_PIN;
        unsigned int _pin_tx = PZEM004T_TX_PIN;
        IPAddress _ip;
        HardwareSerial * _serial = NULL;
        PZEM004T * _pzem = NULL;
        double _energy_offset = 0;

};

#endif // SENSOR_SUPPORT && PZEM004T_SUPPORT
