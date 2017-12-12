// -----------------------------------------------------------------------------
// DHT Sensor
// -----------------------------------------------------------------------------

#pragma once

#include "Arduino.h"
#include "SensorBase.h"

#define DHT_MAX_DATA                5
#define DHT_MAX_ERRORS              5
#define DHT_MIN_INTERVAL            2000
#define DHT_OK                      0
#define DHT_CHECKSUM_ERROR          -1
#define DHT_TIMEOUT_ERROR           -2

#define DHT11                       11
#define DHT22                       22
#define DHT21                       21
#define AM2301                      21

class SensorDHT : public SensorBase {

    public:

        SensorDHT(unsigned char gpio, unsigned char type): SensorBase() {
            _gpio = gpio;
            _type = type;
        }

        // Pre-read hook (usually to populate registers with up-to-date data)
        void pre() {

            if ((_last_ok > 0) && (millis() - _last_ok < DHT_MIN_INTERVAL)) {
                _error = 0;
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
                delay(250);
            }
            pinMode(_gpio, OUTPUT);
            noInterrupts();
        	digitalWrite(_gpio, LOW);
            delayMicroseconds(500);
            digitalWrite(_gpio, HIGH);
            delayMicroseconds(40);
            pinMode(_gpio, INPUT_PULLUP);

        	// No errors, read the 40 data bits
        	for( int k = 0; k < 41; k++ ) {

        		// Starts new data transmission with >50us low signal
        		low = _signal(56, LOW);
        		if (low == 0) {
                    _error = DHT_TIMEOUT_ERROR;
                    return;
                }

        		// Check to see if after >70us rx data is a 0 or a 1
        		high = _signal(75, HIGH);
                if (high == 0) {
                    _error = DHT_TIMEOUT_ERROR;
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
                _error = DHT_CHECKSUM_ERROR;
                return;
            }

        	// Get humidity from Data[0] and Data[1]
            if (_type == DHT11) {
                _humidity = dhtData[0];
            } else {
        	    _humidity = dhtData[0] * 256 + dhtData[1];
        	    _humidity /= 10;
            }

        	// Get temp from Data[2] and Data[3]
            if (_type == DHT11) {
                _temperature = dhtData[2];
            } else {
                _temperature = (dhtData[2] & 0x7F) * 256 + dhtData[3];
                _temperature /= 10;
                if (dhtData[2] & 0x80) _temperature *= -1;
            }

            _last_ok = millis();
            _errors = 0;
            _error = 0;

        }

        // Post-read hook (usually to reset things)
        void post() {

        }

        // Return sensor status (true for ready)
        bool status() {
            return (_last_ok > 0) & (_error == 0);
        }

        // Return sensor last internal error
        int error() {
            return _error;
        }

        // Number of available slots
        unsigned char count() {
            return 2;
        }

        // Descriptive name of the sensor
        String name() {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "DHT%d @ GPIO%d", _type, _gpio);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return name();
        }

        // Type for slot # index
        magnitude_t type(unsigned char index) {
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


    private:

        unsigned long _signal(int usTimeOut, bool state) {
        	unsigned long uSec = 1;
        	while (digitalRead(_gpio) == state) {
                if (++uSec > usTimeOut) return 0;
                delayMicroseconds(1);
        	}
        	return uSec;
        }

        unsigned char _gpio;
        unsigned char _type;
        int _error;

        unsigned long _last_ok = 0;
        unsigned char _errors = 0;

        double _temperature;
        unsigned int _humidity;

};
