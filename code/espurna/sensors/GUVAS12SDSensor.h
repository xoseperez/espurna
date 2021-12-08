// -----------------------------------------------------------------------------
// GUVA-S12SD UV Sensor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
//                         by Mustafa Tufan
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && GUVAS12SD_SUPPORT

#pragma once


#include "BaseSensor.h"
#include "../utils.h"

// http://www.eoc-inc.com/genicom/GUVA-S12SD.pdf
//
// GUVA-S12D has a wide spectral range of 200nm-400nm
// The output voltage and the UV index is linear, illumination intensity = 307 * Vsig where: Vsig is the value of voltage measured from the SIG pin of the interface, unit V.
// illumination intensity unit: mW/m2 for the combination strength of UV light with wavelength range: 200nm-400nm
// UV Index = illumination intensity / 200
//
// UV Index   |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  10  |  10+
// -----------+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+------+--------
// mV         | <50 | 227 | 318 | 408 | 503 | 606 | 696 | 795 | 881 | 976 | 1079 | 1170+
// analog val | <10 |  46 |  65 |  83 | 103 | 124 | 142 | 162 | 180 | 200 |  221 |  240+
//

#define UV_SAMPLE_RATE  1

class GUVAS12SDSensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        GUVAS12SDSensor() {
            _count = 1;
            _sensor_id = SENSOR_GUVAS12SD_ID;
        }

        ~GUVAS12SDSensor() {
            gpioUnlock(_gpio);
        }

        // ---------------------------------------------------------------------

        void setGPIO(unsigned char gpio) {
            _gpio = gpio;
        }

        // ---------------------------------------------------------------------

        unsigned char getGPIO() {
            return _gpio;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            // Manage GPIO lock
            if (_previous != GPIO_NONE) {
                gpioUnlock(_previous);
            }

            _previous = GPIO_NONE;
            if (!gpioLock(_gpio)) {
                _error = SENSOR_ERROR_GPIO_USED;
                return;
            }
            _previous = _gpio;

            _ready = true;

        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {
            _error = SENSOR_ERROR_OK;
            _read();
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[18];
            snprintf(buffer, sizeof(buffer), "GUVAS12SD @ GPIO%hhu", _gpio);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String description(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            return String(_gpio);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index == 0) return MAGNITUDE_UVI;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _uvindex;
            return 0;
        }


    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _read() {

                int _average = 0;

            #if UV_SAMPLE_RATE == 1
                _average = analogRead(0);
            #else
                for (unsigned int i=0; i < UV_SAMPLE_RATE; i++) {
                    _average += analogRead(0);
                    espurna::time::blockingDelay(
                        espurna::duration::Milliseconds(2));
                }
                _average = (_average / UV_SAMPLE_RATE);
            #endif
                // _sensormV = _average / 1023*3.3;

                if (_average <  10) {
                    _uvindex = 0;
                } else if (_average <  46) {
                    _uvindex = (_average - 10) / (46-10);
                } else if (_average <  65) {
                    _uvindex = 1 + ((_average - 46) / (65-46));
                } else if (_average <  83) {
                    _uvindex = 2 + ((_average - 65) / (83-65));
                } else if (_average < 103) {
                    _uvindex = 3 + ((_average - 83) / (103- 83));
                } else if (_average < 124) {
                    _uvindex = 4 + ((_average - 103) / (124-103));
                } else if (_average < 142) {
                    _uvindex = 5 + ((_average - 124) / (142-124));
                } else if (_average < 162) {
                    _uvindex = 6 + ((_average - 142) / (162-142));
                } else if (_average < 180) {
                    _uvindex = 7 + ((_average - 162) / (180-162));
                } else if (_average < 200) {
                    _uvindex = 8 + ((_average - 180) / (200-180));
                } else if (_average < 221) {
                    _uvindex = 9 + ((_average - 200) / (221-200));
                } else {
                    _uvindex = 10;
                }

            }

        unsigned char _gpio = GPIO_NONE;
        unsigned char _previous = GPIO_NONE;

        double _uvindex = 0;

};

#endif // SENSOR_SUPPORT && GUVAS12SD_SUPPORT
