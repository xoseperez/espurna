// -----------------------------------------------------------------------------
// DHTXX Sensor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && DHT_SUPPORT

#pragma once


#include "../gpio.h"
#include "../utils.h"
#include "BaseSensor.h"

constexpr const double DHT_DUMMY_VALUE = -255;
constexpr const size_t DHT_MAX_DATA = 5;
constexpr const size_t DHT_MAX_ERRORS = 5;
constexpr const uint32_t DHT_MIN_INTERVAL = 2000;

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
        default:
            return -1;
    }
}

class DHTSensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        DHTSensor() {
            _count = 2;
            _sensor_id = SENSOR_DHTXX_ID;
        }

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

        unsigned char getGPIO() {
            return _gpio;
        }

        int getType() {
            return dhtchip_to_number(_type);
        }

        DHTChipType getChipType() {
            return _type;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            _count = 0;

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
            _last_ok = millis();

            _count = 2;
            _ready = true;

        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {
            _error = SENSOR_ERROR_OK;
            _read();
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "DHT%d @ GPIO%d", dhtchip_to_number(_type), _gpio);
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
            if (index == 0) return MAGNITUDE_TEMPERATURE;
            if (index == 1) return MAGNITUDE_HUMIDITY;
            return MAGNITUDE_NONE;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            if (index == 0) return _temperature;
            if (index == 1) return _humidity;
            return 0;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        void _read() {

            if ((_last_ok > 0) && (millis() - _last_ok < DHT_MIN_INTERVAL)) {
                if ((_temperature == DHT_DUMMY_VALUE) && (_humidity == DHT_DUMMY_VALUE)) {
                    _error = SENSOR_ERROR_WARM_UP;
                } else {
                    _error = SENSOR_ERROR_OK;
                }
                return;
            }

            unsigned long low = 0;
            unsigned long high = 0;

            unsigned char dhtData[DHT_MAX_DATA] = {0};
            unsigned char byteInx = 0;
            unsigned char bitInx = 7;

            pinMode(_gpio, OUTPUT);

        	// Send start signal to DHT sensor
        	if (++_errors > DHT_MAX_ERRORS) {
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
                delayMicroseconds(500);
            } else {
                delayMicroseconds(1100);
            }
            digitalWrite(_gpio, HIGH);
            delayMicroseconds(40);
            pinMode(_gpio, INPUT_PULLUP);
            delayMicroseconds(10);

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
        		if (high > low) dhtData[byteInx] |= (1 << bitInx);

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
				if (dhtData[2] & 0x80) _temperature *= -1;
            } else {
                _temperature = (dhtData[2] & 0x7F) * 256 + dhtData[3];
                _temperature /= 10;
                if (dhtData[2] & 0x80) _temperature *= -1;
            }

            _last_ok = millis();
            _errors = 0;
            _error = SENSOR_ERROR_OK;

        }

        unsigned long _signal(unsigned long usTimeOut, bool state) {
        	unsigned long uSec = 1;
        	while (digitalRead(_gpio) == state) {
                if (++uSec > usTimeOut) return 0;
                delayMicroseconds(1);
        	}
        	return uSec;
        }

        DHTChipType _type = DHT_CHIP_DHT22;

        unsigned char _gpio = GPIO_NONE;
        unsigned char _previous = GPIO_NONE;

        unsigned long _last_ok = 0;
        unsigned char _errors = 0;

        double _temperature = DHT_DUMMY_VALUE;
        double _humidity = DHT_DUMMY_VALUE;

};

#endif // SENSOR_SUPPORT && DHT_SUPPORT
