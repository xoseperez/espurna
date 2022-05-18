// -----------------------------------------------------------------------------
// Abstract I2C sensor class (other sensor classes extend this class)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && I2C_SUPPORT

#if I2C_USE_BRZO
    #define I2C_TRANS_SUCCESS               0   // All i2c commands were executed without errors
    #define I2C_TRANS_ERROR_BUS_NOT_FREE    1   // Bus not free, i.e. either SDA or SCL is low
    #define I2C_TRANS_ERROR_NACK_WRITE      2   // Not ACK ("NACK") by slave during write:
                                                // Either the slave did not respond to the given slave address; or the slave did not ACK a byte transferred by the master.
    #define I2C_TRANS_ERROR_NACK_READ       4   // Not ACK ("NACK") by slave during read,
                                                // i.e. slave did not respond to the given slave address
    #define I2C_TRANS_ERROR_CLOCK           8   // Clock Stretching by slave exceeded maximum clock stretching time. Most probably, there is a bus stall now!
    #define I2C_TRANS_ERROR_READ_NULL       16  // Read was called with 0 bytes to be read by the master. Command not sent to the slave, since this could yield to a bus stall
    #define I2C_TRANS_ERROR_TIMEOUT         32  // ACK Polling timeout exceeded
#else // Wire
    #define I2C_TRANS_SUCCESS               0   // success
    #define I2C_TRANS_ERROR_BUFFER_OVERLOW  1   // data too long to fit in transmit buffer
    #define I2C_TRANS_ERROR_NACK_ADDRESS    2   // received NACK on transmit of address
    #define I2C_TRANS_ERROR_NACK_DATA       3   // received NACK on transmit of data
    #define I2C_TRANS_ERROR_OTHER           4   // other error
#endif

#pragma once

#include "BaseSensor.h"
#include "../i2c.h"

class I2CSensorAddress {
public:
    I2CSensorAddress() = default;

    uint8_t findAndLock(uint8_t address) {
        // If we have already locked this address for this sensor, do nothing
        if ((address > 0) && (address == _address)) {
            return address;
        }

        // Check if we should release a previously locked address
        if ((_address > 0) && (_address != address)) {
            i2cUnlock(_address);
            _address = 0;
        }

        // If requesting a specific address, try to ger a lock to it
        if ((address > 0) && i2cFind(address) && i2cLock(address)) {
            _address = address;
            return address;
        }

        return 0;
    }

    bool findAndLock(const uint8_t* begin, const uint8_t* end) {
        const auto address = i2cFindAndLock(begin, end);
        if (address) {
            _address = address;
            return true;
        }

        return false;
    }

    void unlock() {
        if (_address) {
            i2cUnlock(_address);
            _address = 0;
        }
    }

    bool locked() const {
        return _address != 0;
    }

    uint8_t address() const {
        return _address;
    }

private:
    uint8_t _address { 0 };
};

// TODO: Must inherit from I2CSensor<>, not just I2CSensor. Even with default value :(
//       Perhaps I2CSensor should be alias for I2CSensorBase<T> / BaseI2CSensor<T>?
// TODO: Should this depend on any T at all? Leave the address helpers, but drop _dirty and _error?

template <typename T = BaseSensor>
class I2CSensor : public T {
public:
    // Make sure to export base constructor, we don't have any
    using T::T;

    // Descriptive name of the slot # index
    String description(unsigned char) const override {
        return static_cast<const T*>(this)->description();
    };

    // Address of the sensor (it could be the GPIO or I2C address)
    String address(unsigned char) const override {
        char buffer[5];
        snprintf_P(buffer, sizeof(buffer),
            PSTR("0x%02X"), _sensor_address.address());
        return String(buffer);
    }

    void setAddress(uint8_t address) {
        if (_address != address) {
            _address = address;
            T::_dirty = true;
        }
    }

    uint8_t configuredAddress() const {
        return _address;
    }

    uint8_t lockedAddress() const {
        return _sensor_address.address();
    }

    uint8_t lock() {
        return _sensor_address.findAndLock(_address);
    }

    void unlock() {
        _sensor_address.unlock();
    }

    void resetUnknown() {
        _sensor_address.unlock();
        _unknownAddress();
    }

    uint8_t findAndLock(uint8_t address) {
        if (_sensor_address.findAndLock(address)) {
            return _sensor_address.address();
        }

        return _unknownAddress();
    }

    uint8_t findAndLock() {
        return findAndLock(_address);
    }

    uint8_t findAndLock(const uint8_t* begin, const uint8_t* end) {
        if (_address) {
            return findAndLock(_address);
        }


        if (_sensor_address.findAndLock(begin, end)) {
            return _sensor_address.address();
        }

        return _unknownAddress();
    }

    template <size_t Size>
    uint8_t findAndLock(const uint8_t (&addresses)[Size]) {
        return findAndLock(std::begin(addresses), std::end(addresses));
    }

private:
    uint8_t _unknownAddress() {
        T::_error = SENSOR_ERROR_UNKNOWN_ID;
        return 0;
    }

    I2CSensorAddress _sensor_address;
    uint8_t _address { 0x00 };
};

#endif // SENSOR_SUPPORT && I2C_SUPPORT
