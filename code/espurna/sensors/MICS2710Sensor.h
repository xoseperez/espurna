// -----------------------------------------------------------------------------
// MICS-2710 (and MICS-4514) NO2 Analog Sensor
// Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && MICS2710_SUPPORT

#pragma once


#include "BaseAnalogSensor.h"
#include "../libs/fs_math.h"

class MICS2710Sensor : public AnalogSensor {

    public:

        void setPreHeatGPIO(unsigned char gpio) {
            _preGPIO = gpio;
        }

        unsigned char getPreHeatGPIO() const {
            return _preGPIO;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_MICS2710_ID;
        }

        unsigned char count() const override {
            return 2;
        }

        void calibrate() override {
            setR0(_getResistance());
        }

        // Initialization method, must be idempotent
        void begin() override {
            pinMode(_preGPIO, OUTPUT);
            digitalWrite(_preGPIO, HIGH);
            _heating = true;
            _heating_start = TimeSource::now();

            _ready = true;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() override {
            _error = SENSOR_ERROR_OK;

            static constexpr auto PreheatTime = TimeSource::duration { MICS2710_PREHEAT_TIME };
            if (_heating && (TimeSource::now() - _heating_start > PreheatTime)) {
                digitalWrite(_preGPIO, LOW);
                _heating = false;
            }

            if (_heating) {
                _error = SENSOR_ERROR_WARM_UP;
                return;
            }

            _Rs = _getResistance();
        }

        // Descriptive name of the sensor
        String description() const override {
            return F("MICS-2710 @ TOUT");
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (0 == index) return MAGNITUDE_RESISTANCE;
            if (1 == index) return MAGNITUDE_NO2;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (0 == index) return _Rs;
            if (1 == index) return _getPPM();
            return 0;
        }

    private:
        double _getResistance() const {

            // get voltage (1 == reference) from analog pin
            double voltage = AnalogSensor::analogRead() / 1024.0;

            // schematic: 3v3 - Rs - P - Rl - GND
            // V(P) = 3v3 * Rl / (Rs + Rl)
            // Rs = 3v3 * Rl / V(P) - Rl = Rl * ( 3v3 / V(P) - 1)
            // 3V3 voltage is cancelled
            double resistance = (voltage > 0) ? _Rl * ( 1 / voltage - 1 ) : 0;

            return resistance;

        }

        double _getPPM() const {

            // According to the datasheet (https://www.cdiweb.com/datasheets/e2v/mics-2710.pdf)
            // there is an almost linear relation between log(Rs/R0) and log(ppm).
            // Regression parameters have been calculated based on the graph
            // in the datasheet with these readings:
            //
            // Rs/R0    NO2(ppm)
            // 23       0.20
            // 42       0.30
            // 90       0.40
            // 120      0.50
            // 200      0.60
            // 410      0.90
            // 500      1.00
            // 1000     1.30
            // 10000	5.00

            return fs_pow(10, 0.5170 * fs_log10(_Rs / _R0) - 1.3954);

        }

        using TimeSource = espurna::time::CoreClock;

        TimeSource::time_point _heating_start;
        bool _heating = false;

        unsigned char _noxGPIO = MICS2710_PRE_PIN;
        unsigned char _preGPIO = MICS2710_NOX_PIN;

};

#endif // SENSOR_SUPPORT && MICS2710_SUPPORT
