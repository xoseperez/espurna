// -----------------------------------------------------------------------------
// DHTXX Sensor
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && DHT_SUPPORT

#pragma once

#include "Arduino.h"
#include "BaseSensor.h"

#define DHT_MAX_DATA                5
#define DHT_MAX_ERRORS              5
#define DHT_MIN_INTERVAL            2000

#define DHT_CHIP_DHT11              11
#define DHT_CHIP_DHT12              12
#define DHT_CHIP_DHT22              22
#define DHT_CHIP_DHT21              21
#define DHT_CHIP_AM2301             21

class DHTSensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        DHTSensor(): BaseSensor() {
            _count = 2;
            _sensor_id = SENSOR_DHTXX_ID;
        }

        ~DHTSensor() {
            if (_previous != GPIO_NONE) gpioReleaseLock(_previous);
        }

        // ---------------------------------------------------------------------

        void setGPIO(unsigned char gpio) {
            _gpio = gpio;
        }

        void setType(unsigned char type) {
            _type = type;
        }

        // ---------------------------------------------------------------------

        unsigned char getGPIO() {
            return _gpio;
        }

        unsigned char getType() {
            return _type;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {

            _count = 0;

            // Manage GPIO lock
            if (_previous != GPIO_NONE) gpioReleaseLock(_previous);
            _previous = GPIO_NONE;
            if (!gpioGetLock(_gpio)) {
                _error = SENSOR_ERROR_GPIO_USED;
                return;
            }
            _previous = _gpio;

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
            snprintf(buffer, sizeof(buffer), "DHT%d @ GPIO%d", _type, _gpio);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
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
                _error = SENSOR_ERROR_OK;
                return;
            }

            unsigned long low = 0;
            unsigned long high = 0;

            unsigned char dhtData[DHT_MAX_DATA] = {0};
            unsigned char byteInx = 0;
            unsigned char bitInx = 7;

        	// Send start signal to DHT sensor
        	if (++_errors > DHT_MAX_ERRORS) {
                _errors = 0;
                digitalWrite(_gpio, HIGH);
                nice_delay(250);
            }
            pinMode(_gpio, OUTPUT);
            noInterrupts();
        	digitalWrite(_gpio, LOW);
            if ((_type == DHT_CHIP_DHT11) || (_type == DHT_CHIP_DHT12)) {
                nice_delay(20);
            } else {
                delayMicroseconds(500);
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

        unsigned char _gpio = GPIO_NONE;
        unsigned char _previous = GPIO_NONE;
        unsigned char _type = DHT_CHIP_DHT22;

        unsigned long _last_ok = 0;
        unsigned char _errors = 0;

        double _temperature = 0;
        double _humidity = 0;

};

#endif // SENSOR_SUPPORT && DHT_SUPPORT
