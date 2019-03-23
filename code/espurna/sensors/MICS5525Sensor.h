// -----------------------------------------------------------------------------
// MICS-5525 (and MICS-4514) CO Analog Sensor
// Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && MICS5525_SUPPORT

#pragma once

// Set ADC to TOUT pin
#undef ADC_MODE_VALUE
#define ADC_MODE_VALUE ADC_TOUT

#include "Arduino.h"
#include "BaseSensor.h"

extern "C" {
    #include "../libs/fs_math.h"
}

class MICS5525Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        MICS5525Sensor(): BaseSensor() {
            _count = 2;
            _sensor_id = SENSOR_MICS5525_ID;
        }

        void calibrate() {
            setR0(_getResistance());
        }

        // ---------------------------------------------------------------------

        void setAnalogGPIO(unsigned char gpio) {
            _redGPIO = gpio;
        }

        unsigned char getAnalogGPIO() {
            return _redGPIO;
        }

        void setRL(unsigned long Rl) {
            if (Rl > 0) _Rl = Rl;
        }

        unsigned long getRL() {
            return _Rl;
        }

        void setR0(unsigned long R0) {
            if (R0 > 0) _R0 = R0;
        }

        unsigned long getR0() {
            return _R0;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {
            pinMode(_redGPIO, INPUT);
            _ready = true;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {
            _Rs = _getResistance();
        }

        // Descriptive name of the sensor
        String description() {
            return String("MICS-5525 @ TOUT");
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
            if (0 == index) return MAGNITUDE_RESISTANCE;
            if (1 == index) return MAGNITUDE_CO;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (0 == index) return _Rs;
            if (1 == index) return _getPPM();
            return 0;
        }

    private:

        unsigned long _getReading() {
            return analogRead(_redGPIO);
        }

        double _getResistance() {

            // get voltage (1 == reference) from analog pin
            double voltage = (float) _getReading() / 1024.0;

            // schematic: 3v3 - Rs - P - Rl - GND
            // V(P) = 3v3 * Rl / (Rs + Rl)
            // Rs = 3v3 * Rl / V(P) - Rl = Rl * ( 3v3 / V(P) - 1)
            // 3V3 voltage is cancelled
            double resistance = (voltage > 0) ? _Rl * ( 1 / voltage - 1 ) : 0;

            return resistance;

        }

        double _getPPM() {

            // According to the datasheet (https://airqualityegg.wikispaces.com/file/view/mics-5525-CO.pdf)

            return 764.2976 * fs_pow(2.71828, -7.6389 * ((float) _Rs / _R0));

        }

        unsigned long _R0 = MICS5525_R0;            // R0, calibration value at 25º on air
        unsigned long _Rl = MICS5525_RL;            // RL, load resistance
        unsigned long _Rs = 0;                      // cached resistance
        unsigned char _redGPIO = MICS5525_RED_PIN;

};

#endif // SENSOR_SUPPORT && MICS5525_SUPPORT
