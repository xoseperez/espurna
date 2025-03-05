// -----------------------------------------------------------------------------
// DHTXX Sensor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && DHT_SUPPORT

#pragma once

#include "../gpio.h"
#include "../system_time.h"
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

namespace {

int dht_chip_to_number(DHTChipType type) {
    switch (type) {
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

float dht_humidity(DHTChipType type, std::array<uint8_t, 2> pair) {
    // binary representation varies between the original chip and its copies
    // but, its never negative, so no reason to do any conversions for signed numbers
    float out;

    // based on ADSONG datasheet and various other libs implementing dht12
    // extract sign info from the decimal scale part instead of the integral
    constexpr auto MagnitudeMask = uint8_t{ 0b1111111 };

    switch (type) {
    case DHT_CHIP_DHT11:
        out = pair[0];
        break;

    case DHT_CHIP_DHT12:
        out = (pair[0] & MagnitudeMask);
        out += (pair[1] & MagnitudeMask) * 0.1f;
        break;

    case DHT_CHIP_DHT21:
    case DHT_CHIP_DHT22:
    case DHT_CHIP_AM2301:
    case DHT_CHIP_SI7021:
        out = ((pair[0] << 8) | pair[1]) * 0.1f;
        break;
    }

    return out;
}

float dht_temperature(DHTChipType type, std::array<uint8_t, 2> pair) {
    // binary representation varies between the original chip and its copies
    // by default, check for generic sign-magnitude
    constexpr auto MagnitudeMask = uint8_t{ 0b1111111 };
    constexpr auto SignMask = uint8_t{ 0b10000000 };

    // in case it is negative and looks like twos-complement, value can be c/p into memory as-is
    // it is enough to only check the sign bit neighbour; possible values are around [0...800]
    constexpr auto NegativeTwoComplementMask = uint8_t{ 0b11000000 };

    float out;

    switch (type) {
    // []pair for dht11 & dht12 contains integral and decimal scale
    // based on ADSONG datasheet and various other libs implementing dht12
    // try to extract sign info from both pairs, not just the integral one

    // original code prefers to drop decimal scale in favour of a integral reading due to poor precision
    case DHT_CHIP_DHT11:
        out = pair[0];
        if ((pair[0] & SignMask) || (pair[1] & SignMask)) {
            out = -out;
        }
        break;

    // added decimal place based on the decimal scale byte
    case DHT_CHIP_DHT12:
        out = (pair[0] & MagnitudeMask);
        out += (pair[1] & MagnitudeMask) * 0.1f;
        if ((pair[0] & SignMask) || (pair[1] & SignMask)) {
            out = -out;
        }

        break;

    // []pair on recent chips contains s16 data w/ sign included
    case DHT_CHIP_DHT21:
    case DHT_CHIP_DHT22:
    case DHT_CHIP_AM2301:
    case DHT_CHIP_SI7021:
        // special exception for negative numbers in twos-complement
        if ((pair[0] & NegativeTwoComplementMask) == NegativeTwoComplementMask) {
            int16_t tmp;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            std::swap(pair[0], pair[1]);
#endif
            std::memcpy(&tmp, pair.data(), sizeof(tmp));
            out = tmp;
        // fallback works both for the original chips and positive numbers
        } else {
            out = ((pair[0] & MagnitudeMask) << 8) | pair[1];
            if (pair[0] & SignMask) {
                out = -out;
            }
        }

        out *= 0.1f;
        break;
    }

    return out;
}

bool dht_checksum(const std::array<uint8_t, 5>& data) {
    return data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF);
}

} // namespace

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
            return dht_chip_to_number(_type);
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
                "DHT%d @ GPIO%hhu", dht_chip_to_number(_type), _gpio);
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

        void _read_critical(Data& dhtData) {
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

            unsigned long low = 0;
            unsigned long high = 0;

            unsigned char byteInx = 0;
            unsigned char bitInx = 7;

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
        }

        void _read() {
            if (TimeSource::now() - _last_ok < MinInterval) {
                if ((_temperature == DummyValue) && (_humidity == DummyValue)) {
                    _error = SENSOR_ERROR_WARM_UP;
                } else {
                    _error = SENSOR_ERROR_OK;
                }
                return;
            }

            pinMode(_gpio, OUTPUT);

            // Send start signal to DHT sensor
            if (++_errors > MaxErrors) {
                _errors = 0;
                digitalWrite(_gpio, HIGH);
                espurna::time::blockingDelay(
                    espurna::duration::Milliseconds(250));
            }

            Data data{};

            noInterrupts();
            _read_critical(data);
            interrupts();

            if (_error != SENSOR_ERROR_OK) {
                return;
            }

            if (!dht_checksum(data)) {
                _error = SENSOR_ERROR_CRC;
                return;
            }

            _humidity = dht_humidity(_type, {data[0], data[1]});
            _temperature = dht_temperature(_type, {data[2], data[3]});

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
