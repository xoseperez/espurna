// -----------------------------------------------------------------------------
// Abstract I2C sensor class (other sensor classes extend this class)
// Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && ( I2C_SUPPORT || EMON_ANALOG_SUPPORT )

#pragma once

#include "BaseSensor.h"

class I2CSensor : public BaseSensor {

    public:

        void setAddress(unsigned char address) {
            if (_address == address) return;
            _address = address;
            _dirty = true;
        }

        unsigned char getAddress() {
            return _address;
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            char buffer[5];
            snprintf(buffer, sizeof(buffer), "0x%02X", _address);
            return String(buffer);
        }

    protected:

        // Specific for I2C sensors
        unsigned char _begin_i2c(unsigned char address, size_t size, unsigned char * addresses) {

            // If we have already locked this address for this sensor quit
            if ((address > 0) && (address == _previous_address)) {
                return _previous_address;
            }

            // Check if we should release a previously locked address
            if ((_previous_address > 0) && (_previous_address != address)) {
                i2cReleaseLock(_previous_address);
                _previous_address = 0;
            }

            // If requesting a specific address, try to ger a lock to it
            if ((0 < address) && i2cGetLock(address)) {
                _previous_address = address;
                return _previous_address;
            }

            // If everything else fails, perform an auto-discover
            _previous_address = i2cFindAndLock(size, addresses);

            // Flag error
            if (0 == _previous_address) {
                _error = SENSOR_ERROR_I2C;
            }

            return _previous_address;

        }

        unsigned char _previous_address = 0;
        unsigned char _address = 0;

};

#endif // SENSOR_SUPPORT && I2C_SUPPORT
