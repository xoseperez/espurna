// -----------------------------------------------------------------------------
// ECH1560 based power monitor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && ECH1560_SUPPORT

#pragma once

#include "BaseSensor.h"
#include "BaseEmonSensor.h"

class ECH1560Sensor : public BaseEmonSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        static constexpr Magnitude Magnitudes[] {
            MAGNITUDE_CURRENT,
            MAGNITUDE_VOLTAGE,
            MAGNITUDE_POWER_APPARENT,
            MAGNITUDE_ENERGY
        };

        ECH1560Sensor() {
            _sensor_id = SENSOR_ECH1560_ID;
            _count = std::size(Magnitudes);
            findAndAddEnergy(Magnitudes);
        }

        // ---------------------------------------------------------------------

        void setCLK(unsigned char clk) {
            if (_clk == clk) return;
            _clk = clk;
            _dirty = true;
        }

        void setMISO(unsigned char miso) {
            if (_miso == miso) return;
            _miso = miso;
            _dirty = true;
        }

        void setInverted(bool inverted) {
            _inverted = inverted;
        }

        // ---------------------------------------------------------------------

        unsigned char getCLK() {
            return _clk.pin();
        }

        unsigned char getMISO() {
            return _miso;
        }

        bool getInverted() {
            return _inverted;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            if (!_dirty) return;

            pinMode(_clk.pin(), INPUT);
            pinMode(_miso, INPUT);
            _clk.attach(this, handleInterrupt, RISING);

            _dirty = false;
            _ready = true;

        }

        // Loop-like method, call it in your main loop
        void tick() {
            if (_dosync) _sync();
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[35];
            snprintf(buffer, sizeof(buffer), "ECH1560 (CLK,SDO) @ GPIO(%hhu,%hhu)", _clk.pin(), _miso);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String description(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            char buffer[6];
            snprintf(buffer, sizeof(buffer), "%hhu:%hhu", _clk.pin(), _miso);
            return String(buffer);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            if (index < std::size(Magnitudes)) {
                return Magnitudes[index].type;
            }

            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _current;
            if (index == 1) return _voltage;
            if (index == 2) return _apparent;
            if (index == 3) return _energy[0].asDouble();
            return 0;
        }

        static void IRAM_ATTR handleInterrupt(ECH1560Sensor* instance) {
            instance->interrupt();
        }

    private:
        void IRAM_ATTR interrupt() {
            // if we are trying to find the sync-time (CLK goes high for 1-2ms)
            if (!_dosync) {

                _clk_count = 0;

                // register how long the ClkHigh is high to evaluate if we are at the part where clk goes high for 1-2 ms
                while (digitalRead(_clk.pin()) == HIGH) {
                    _clk_count += 1;
                    delayMicroseconds(30);  //can only use delayMicroseconds in an interrupt.
                }

                // if the Clk was high between 1 and 2 ms than, its a start of a SPI-transmission
                if (_clk_count >= 33 && _clk_count <= 67) {
                    _dosync = true;
                }

            // we are in sync and logging CLK-highs
            } else {

                // increment an integer to keep track of how many bits we have read.
                _bits_count += 1;
                _nextbit = true;

            }
        }

    protected:
        void _sync() {

            unsigned int byte1 = 0;
            unsigned int byte2 = 0;
            unsigned int byte3 = 0;

            _bits_count = 0;
            while (_bits_count < 40); // skip the uninteresting 5 first bytes
            _bits_count = 0;

            while (_bits_count < 24) { // loop through the next 3 Bytes (6-8) and save byte 6 and 7 in byte1 and byte2

                if (_nextbit) {

                    if (_bits_count < 9) { // first Byte/8 bits in byte1

                        byte1 = byte1 << 1;
                        if (digitalRead(_miso) == HIGH) byte1 |= 1;
                        _nextbit = false;

                    } else if (_bits_count < 17) { // bit 9-16 is byte 7, store in byte2

                        byte2 = byte2 << 1;
                        if (digitalRead(_miso) == HIGH) byte2 |= 1;
                        _nextbit = false;

                    }

                }

            }

            if (byte2 != 3) { // if bit byte2 is not 3, we have reached the important part, U is allready in byte1 and byte2 and next 8 Bytes will give us the Power.

                // voltage = 2 * (byte1 + byte2 / 255)
                _voltage = 2.0 * ((float) byte1 + (float) byte2 / 255.0);

                // power:
                _bits_count = 0;
                while (_bits_count < 40); // skip the uninteresting 5 first bytes
                _bits_count = 0;

                byte1 = 0;
                byte2 = 0;
                byte3 = 0;

                while (_bits_count < 24) { //store byte 6, 7 and 8 in byte1 and byte2 & byte3.

                    if (_nextbit) {

                        if (_bits_count < 9) {

                            byte1 = byte1 << 1;
                            if (digitalRead(_miso) == HIGH) byte1 |= 1;
                            _nextbit = false;

                        } else if (_bits_count < 17) {

                            byte2 = byte2 << 1;
                            if (digitalRead(_miso) == HIGH) byte2 |= 1;
                            _nextbit = false;

                        } else {

                            byte3 = byte3 << 1;
                            if (digitalRead(_miso) == HIGH) byte3 |= 1;
                            _nextbit = false;

                        }
                    }
                }

                if (_inverted) {
                    byte1 = 255 - byte1;
                    byte2 = 255 - byte2;
                    byte3 = 255 - byte3;
                }

                // power = (byte1*255+byte2+byte3/255)/2
                _apparent = ( (float) byte1 * 255 + (float) byte2 + (float) byte3 / 255.0) / 2;
                _current = _apparent / _voltage;

                static unsigned long last = 0;
                if (last > 0) {
                    _energy[0] += sensor::Ws {
                        static_cast<uint32_t>(_apparent * (millis() - last) / 1000)
                    };
                }
                last = millis();

                _dosync = false;

            }

            // If byte2 is not 3 or something else than 0, something is wrong!
            if (byte2 == 0) {
                _dosync = false;
            #if SENSOR_DEBUG
                DEBUG_MSG_P(PSTR("Nothing connected, or out of sync!\n"));
            #endif
            }
        }

        // ---------------------------------------------------------------------

        InterruptablePin _clk{};
        unsigned char _miso = GPIO_NONE;
        bool _inverted = false;

        volatile long _bits_count = 0;
        volatile long _clk_count = 0;
        volatile bool _dosync = false;
        volatile bool _nextbit = true;

        double _apparent = 0;
        double _voltage = 0;
        double _current = 0;

        unsigned char _data[24] {0};

};

#if __cplusplus < 201703L
constexpr BaseEmonSensor::Magnitude ECH1560Sensor::Magnitudes[];
#endif

#endif // SENSOR_SUPPORT && ECH1560_SUPPORT
