// -----------------------------------------------------------------------------
// SDS011 dust sensor
// Based on: https://github.com/ricki-z/SDS011
//
// Uses SoftwareSerial library
// Copyright (C) 2018 by Lucas Pleß <hello at lucas-pless dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && SDS011_SUPPORT

#pragma once

#include <Arduino.h>
#include "SdsDustSensor.h"

#include "BaseSensor.h"


class SDS011Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        SDS011Sensor() {
            _count = 2;
            _sensor_id = SENSOR_SDS011_ID;
        }

        ~SDS011Sensor() {
            if (_sensor) delete _sensor;
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

        // ---------------------------------------------------------------------

        unsigned char getRX() {
            return _pin_rx;
        }

        unsigned char getTX() {
            return _pin_tx;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;

            if (_sensor) delete _sensor;

            _sensor = new SdsDustSensor(_pin_rx, _pin_tx);
            _sensor->begin(9600);

            #if SDS011_REPORTING_MODE == 0
                _sensor->setQueryReportingMode();
            #elif SDS011_REPORTING_MODE == 1
                _sensor->setActiveReportingMode();
                _sensor->setContinuousWorkingPeriod();
                #if SDS011_CUSTOM_WORKING_PERIOD > 0
                _sensor->setCustomWorkingPeriod(SDS011_CUSTOM_WORKING_PERIOD / 60);
                #endif
            #endif

            DEBUG_MSG_P(PSTR("[SDS011] %s\n"), _sensor->queryFirmwareVersion().toString().c_str());
            DEBUG_MSG_P(PSTR("[SDS011] %s\n"), _sensor->queryReportingMode().toString().c_str());
            DEBUG_MSG_P(PSTR("[SDS011] %s\n"), _sensor->queryWorkingPeriod().toString().c_str());

            _ready = true;
            _dirty = false;
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[28];
            snprintf(buffer, sizeof(buffer), "SDS011 @ SwSerial(%u,%u)", _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String description(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            char buffer[6];
            snprintf(buffer, sizeof(buffer), "%u:%u", _pin_rx, _pin_tx);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_PM2dot5;
            if (index == 1) return MAGNITUDE_PM10;
            return MAGNITUDE_NONE;
        }

        void pre() {
            _read();
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _p2dot5;
            if (index == 1) return _p10;
            return 0;
        }



    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _read() {
            #if SDS011_REPORTING_MODE == 0
            PmResult pm = _sensor->queryPm();
            #elif SDS011_REPORTING_MODE == 1
            PmResult pm = _sensor->readPm();
            #endif

            if (!pm.isOk()) {
                switch (pm.status) {
                    case Status::NotAvailable:
                    #if SDS011_CUSTOM_WORKING_PERIOD != -1
                        if (_p2dot5 == 0 && _p10 == 0) {
                            _error = SENSOR_ERROR_WARM_UP;
                            return;
                        }

                        _error = SENSOR_ERROR_OK;
                        return;
                    #endif
                    case Status::InvalidChecksum:
                        _error = SENSOR_ERROR_CRC;
                        return;
                    default:
                        _error = SENSOR_ERROR_OTHER;
                        return;
                }
            }

            _error = SENSOR_ERROR_OK;

            _p10 = pm.pm10;
            _p2dot5 = pm.pm25;
        }

        double _p2dot5 = 0;
        double _p10 = 0;
        unsigned int _pin_rx;
        unsigned int _pin_tx;
        SdsDustSensor * _sensor = NULL;

};

#endif // SENSOR_SUPPORT && SDS011_SUPPORT
