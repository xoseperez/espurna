// -----------------------------------------------------------------------------
// MICS-5525 (and MICS-4514) CO Analog Sensor
// Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && MICS5525_SUPPORT

#pragma once

#include "BaseAnalogSensor.h"

extern "C" {
#include "../libs/fs_math.h"
}

class MICS5525Sensor : public AnalogSensor {

    public:

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_MICS5525_ID;
        }

        unsigned char count() const override {
            return 2;
        }

        void calibrate() override {
            setR0(_getResistance());
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() override {
            _Rs = _getResistance();
            _ppm = _getPPM();
        }

        // Descriptive name of the sensor
        String description() const override {
            return F("MICS-5525 @ TOUT");
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (0 == index) return MAGNITUDE_RESISTANCE;
            if (1 == index) return MAGNITUDE_CO;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (0 == index) return _Rs;
            if (1 == index) return _ppm;
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

        // Calculate according to the datasheet (https://airqualityegg.wikispaces.com/file/view/mics-5525-CO.pdf)
        double _getPPM() const {
            return 764.2976 * fs_pow(2.71828, -7.6389 * ((float) _Rs / _R0));
        }

        double _ppm { 0.0 };

};

#endif // SENSOR_SUPPORT && MICS5525_SUPPORT
