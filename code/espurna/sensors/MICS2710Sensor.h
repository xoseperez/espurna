// -----------------------------------------------------------------------------
// MICS-2710 (and MICS-4514) NO2 Analog Sensor
// Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && MICS2710_SUPPORT

#pragma once

#include <Arduino.h>

#include "BaseAnalogSensor.h"
extern "C" {
    #include "../libs/fs_math.h"
}

class MICS2710Sensor : public BaseAnalogSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        MICS2710Sensor() {
            _count = 2;
            _sensor_id = SENSOR_MICS2710_ID;
        }

        void calibrate() {
            setR0(_getResistance());
        }

        // ---------------------------------------------------------------------

        void setAnalogGPIO(unsigned char gpio) {
            _noxGPIO = gpio;
        }

        unsigned char getAnalogGPIO() {
            return _noxGPIO;
        }

        void setPreHeatGPIO(unsigned char gpio) {
            _preGPIO = gpio;
        }

        unsigned char getPreHeatGPIO() {
            return _preGPIO;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            // Set NOX as analog input
            pinMode(_noxGPIO, INPUT);

            // Start pre-heating
            pinMode(_preGPIO, OUTPUT);
            digitalWrite(_preGPIO, HIGH);
            _heating = true;
            _start = millis();

            _ready = true;

        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {

            // Check pre-heat time
            if (_heating && (millis() - _start > MICS2710_PREHEAT_TIME)) {
                digitalWrite(_preGPIO, LOW);
                _heating = false;
            }

            if (_ready) {
                _Rs = _getResistance();
            }

        }

        // Descriptive name of the sensor
        String description() {
            return String("MICS-2710 @ TOUT");
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
            if (1 == index) return MAGNITUDE_NO2;
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
            return analogRead(_noxGPIO);
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

        bool _heating = false;
        unsigned long _start = 0;                   // monitors the pre-heating time
        unsigned char _noxGPIO = MICS2710_PRE_PIN;
        unsigned char _preGPIO = MICS2710_NOX_PIN;

};

#endif // SENSOR_SUPPORT && MICS2710_SUPPORT
