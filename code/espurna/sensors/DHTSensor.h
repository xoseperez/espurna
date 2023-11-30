// -----------------------------------------------------------------------------
// DHTXX Sensor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && DHT_SUPPORT

#pragma once


#include "../gpio.h"
#include "../utils.h"
#include "BaseSensor.h"

enum class DHTChipType {
    DHT11,
    DHT12,
    DHT21,
    DHT22,
    AM2301,
    SI7021
};

// Note: backwards compatibility for configuration headers
#define DHT_CHIP_DHT11              DHTChipType::DHT11
#define DHT_CHIP_DHT12              DHTChipType::DHT12
#define DHT_CHIP_DHT22              DHTChipType::DHT22
#define DHT_CHIP_DHT21              DHTChipType::DHT21
#define DHT_CHIP_AM2301             DHTChipType::AM2301
#define DHT_CHIP_SI7021             DHTChipType::SI7021

int dhtchip_to_number(DHTChipType chip) {
    switch (chip) {
    case DHTChipType::DHT11:
        return 11;
    case DHTChipType::DHT12:
        return 12;
    case DHTChipType::DHT21:
    case DHTChipType::AM2301:
        return 21;
    case DHTChipType::DHT22:
    case DHTChipType::SI7021:
        return 22;
    }

    return -1;
}

class DHTSensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        static constexpr double DummyValue = -255.0;
        static constexpr size_t MaxErrors = 5;

        using Data = std::array<uint8_t, 5>;

        using TimeSource = espurna::time::CoreClock;
        static constexpr auto MinInterval = espurna::duration::Milliseconds { 2000 };

        ~DHTSensor() {
            gpioUnlock(_gpio);
        }

        // ---------------------------------------------------------------------

        void setGPIO(unsigned char gpio) {
            _gpio = gpio;
        }

        void setType(DHTChipType type) {
            _type = type;
        }

        // ---------------------------------------------------------------------

        unsigned char getGPIO() const {
            return _gpio;
        }

        int getType() const {
            return dhtchip_to_number(_type);
        }

        DHTChipType getChipType() const {
            return _type;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        unsigned char id() const override {
            return SENSOR_DHTXX_ID;
        }

        unsigned char count() const override {
            return 2;
        }

        // Initialization method, must be idempotent
        void begin() override {

            // Manage GPIO lock (note that this only handles the basic *hw* I/O)
            if (_previous != GPIO_NONE) {
                gpioUnlock(_previous);
            }

            _previous = GPIO_NONE;
            if (!gpioLock(_gpio)) {
                _error = SENSOR_ERROR_GPIO_USED;
                return;
            }
            _previous = _gpio;

            // Set now to fail the check in _read at least once
            _last_ok = TimeSource::now();
            _ready = true;

        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() override {
            _error = SENSOR_ERROR_OK;
            _read();
        }

        // Descriptive name of the sensor
        String description() const override {
            char buffer[20];
            snprintf_P(buffer, sizeof(buffer),
                "DHT%d @ GPIO%hhu", dhtchip_to_number(_type), _gpio);
            return String(buffer);
        }

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char) const override {
            return String(_gpio, 10);
        }

        // Type for slot # index
        unsigned char type(unsigned char index) const override {
            if (index == 0) return MAGNITUDE_TEMPERATURE;
            if (index == 1) return MAGNITUDE_HUMIDITY;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) override {
            if (index == 0) return _temperature;
            if (index == 1) return _humidity;
            return 0;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _read() {

            if (TimeSource::now() - _last_ok < MinInterval) {
                if ((_temperature == DummyValue) && (_humidity == DummyValue)) {
                    _error = SENSOR_ERROR_WARM_UP;
                } else {
                    _error = SENSOR_ERROR_OK;
                }
                return;
            }

            unsigned long low = 0;
            unsigned long high = 0;

            Data dhtData{};

            unsigned char byteInx = 0;
            unsigned char bitInx = 7;

            pinMode(_gpio, OUTPUT);

            // Send start signal to DHT sensor
            if (++_errors > MaxErrors) {
                _errors = 0;
                digitalWrite(_gpio, HIGH);
                espurna::time::blockingDelay(
                    espurna::duration::Milliseconds(250));
            }

            noInterrupts();
            digitalWrite(_gpio, LOW);

            if ((_type == DHT_CHIP_DHT11) || (_type == DHT_CHIP_DHT12)) {
                espurna::time::blockingDelay(
                    espurna::duration::Milliseconds(20));
            } else if (_type == DHT_CHIP_SI7021) {
                espurna::time::critical::delay(
                    espurna::duration::critical::Microseconds(500));
            } else {
                espurna::time::critical::delay(
                    espurna::duration::critical::Microseconds(1100));
            }
            digitalWrite(_gpio, HIGH);
            espurna::time::critical::delay(
                espurna::duration::critical::Microseconds(40));
            pinMode(_gpio, INPUT_PULLUP);
            espurna::time::critical::delay(
                espurna::duration::critical::Microseconds(10));

            // No errors, read the 40 data bits
            for( int k = 0; k < 41; k++ ) {

                // Starts new data transmission with >50us low signal
                low = _signal(100, LOW);
                if (low == 0) {
                    _error = SENSOR_ERROR_TIMEOUT;
                    return;
                }

                // Check to see if after >70us rx data is a 0 or a 1
                high = _signal(100, HIGH);
                if (high == 0) {
                    _error = SENSOR_ERROR_TIMEOUT;
                    return;
                }

                // Skip the first bit
                if (k == 0) continue;

                // add the current read to the output data
                // since all dhtData array where set to 0 at the start,
                // only look for "1" (>28us us)
                if (high > low) {
                    dhtData[byteInx] |= (1 << bitInx);
                }

                // index to next byte
                if (bitInx == 0) {
                    bitInx = 7;
                    ++byteInx;
                } else {
                    --bitInx;
                }

            }

            interrupts();

            // Verify checksum
            if (dhtData[4] != ((dhtData[0] + dhtData[1] + dhtData[2] + dhtData[3]) & 0xFF)) {
                _error = SENSOR_ERROR_CRC;
                return;
            }

            // Get humidity from Data[0] and Data[1]
            if (_type == DHT_CHIP_DHT11) {
                _humidity = dhtData[0];
            } else if (_type == DHT_CHIP_DHT12) {
                _humidity = dhtData[0];
                _humidity += dhtData[1] * 0.1;
            } else {
                _humidity = dhtData[0] * 256 + dhtData[1];
                _humidity /= 10;
            }

            // Get temp from Data[2] and Data[3]
            if (_type == DHT_CHIP_DHT11) {
                _temperature = dhtData[2];
            } else if (_type == DHT_CHIP_DHT12) {
                _temperature = (dhtData[2] & 0x7F);
                _temperature += dhtData[3] * 0.1;
                if (dhtData[2] & 0x80) {
                    _temperature *= -1;
                }
            } else {
                _temperature = (dhtData[2] & 0x7F) * 256 + dhtData[3];
                _temperature /= 10;
                if (dhtData[2] & 0x80) {
                    _temperature *= -1;
                }
            }

            _last_ok = TimeSource::now();

            _errors = 0;
            _error = SENSOR_ERROR_OK;

        }

        unsigned long _signal(unsigned long maximum, bool state) {
            unsigned long ticks = 1;

            while (digitalRead(_gpio) == state) {
                if (++ticks > maximum) return 0;
                espurna::time::critical::delay(
                    espurna::duration::critical::Microseconds(1));
            }

            return ticks;
        }

        DHTChipType _type = DHT_CHIP_DHT22;

        unsigned char _gpio = GPIO_NONE;
        unsigned char _previous = GPIO_NONE;

        TimeSource::time_point _last_ok;
        size_t _errors = 0;

        bool _warmup = false;
        double _temperature = DummyValue;
        double _humidity = 0.0;

};

#endif // SENSOR_SUPPORT && DHT_SUPPORT
