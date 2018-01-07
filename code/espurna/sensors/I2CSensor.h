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

    protected:

        // Specific for I2C sensors
        unsigned char _begin_i2c(unsigned char address, size_t size, unsigned char * addresses) {

            // Check if we should release a previously locked address
            if (_previous_address != address) {
                i2cReleaseLock(_previous_address);
            }

            // If we have already an address, check it is not locked
            if (address && !i2cGetLock(address)) {
                _error = SENSOR_ERROR_I2C;

            // If we don't have an address...
            } else {

                // Trigger auto-discover
                address = i2cFindAndLock(size, addresses);

                // If still nothing exit with error
                if (address == 0) _error = SENSOR_ERROR_I2C;

            }

            _previous_address = address;
            return address;

        }

        unsigned char _previous_address = 0;
        unsigned char _address = 0;

};

#endif // SENSOR_SUPPORT && I2C_SUPPORT
