// -----------------------------------------------------------------------------
// Abstract sensor class (other sensor classes extend this class)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include <Arduino.h>

#include <cstddef>
#include <cstdint>
#include <functional>

class BaseSensor {
public:
    // Generic way to pass the sensor instance to the isr
    struct InterruptablePin {
        InterruptablePin() = default;
        ~InterruptablePin() {
            detach();
        }

        template <typename T, typename TypedCallback = void(*)(T*)>
        void attach(T* instance, TypedCallback callback, int mode) {
            _attach(static_cast<void*>(instance), reinterpret_cast<VoidCallback>(callback), mode);
        }

        InterruptablePin& operator=(unsigned char pin) {
            _pin = pin;
            return *this;
        }

        bool operator==(unsigned char other) const {
            return _pin == other;
        }

        explicit operator String() const {
            return String(_pin);
        }

        void detach() {
            gpioUnlock(_current);
            ::detachInterrupt(_current);
            _current = GPIO_NONE;
        }

        void pin(unsigned char value) {
            _pin = value;
        }

        unsigned char pin() const {
            return _pin;
        }

    private:
        using VoidCallback = void(*)(void*);

        void _attach(void* instance, VoidCallback callback, int mode) {
            if (_current != _pin) {
                if (!gpioLock(_pin)) {
                    return;
                }

                detach();
                ::attachInterruptArg(_pin, callback, instance, mode);

                _current = _pin;
            }
        }

        unsigned char _current { GPIO_NONE };
        unsigned char _pin { GPIO_NONE };
    };

    // Generic container for magnitude types used in the sensor
    struct Magnitude {
        unsigned char type;
#if __cplusplus <= 201103L
        constexpr Magnitude(unsigned char type_) :
            type(type_)
        {}
#endif
    };

    template <typename T, typename Callback>
    static void findMagnitudes(const T& container, unsigned char type, Callback&& callback) {
        auto begin = std::begin(container);
        auto end = std::end(container);

        for (auto it = begin; it != end; ++it) {
            if ((*it).type == type) {
                callback(std::distance(begin, it));
            }
        }
    }

    // Must implement as virtual.
    // Allows inhereting class correctly call it's own destructor through the ~BaseSensor()
    virtual ~BaseSensor() {
    }

    // Initialization method, must be idempotent
    virtual void begin() {
    }

    // Loop-like method, call it in your main loop
    virtual void tick() {
    }

    // Pre-read hook (usually to populate registers with up-to-date data)
    virtual void pre() {
    }

    // Post-read hook (usually to reset things)
    virtual void post() {
    }

    // Type of sensor
    virtual unsigned char type() const {
        return sensor::type::Base;
    }

    // Number of decimals for a unit (or -1 for default)
    virtual signed char decimals(sensor::Unit) const {
        return -1;
    }

    // Generic calibration
    virtual void calibrate() {
    };

    // Sensor ID
    virtual unsigned char getID() {
        return _sensor_id;
    };

    // Return status (true if no errors)
    bool status() {
        return 0 == _error;
    }

    // Return ready status (true for ready)
    bool ready() {
        return _ready;
    }

    // Return sensor last internal error
    int error() {
        return _error;
    }

    // Number of available slots
    unsigned char count() {
        return _count;
    }

    // Specify units attached to magnitudes
    virtual sensor::Unit units(unsigned char index) {
        switch (type(index)) {
        case MAGNITUDE_TEMPERATURE:
            return sensor::Unit::Celcius;
        case MAGNITUDE_HUMIDITY:
        case MAGNITUDE_POWER_FACTOR:
            return sensor::Unit::Percentage;
        case MAGNITUDE_PRESSURE:
            return sensor::Unit::Hectopascal;
        case MAGNITUDE_CURRENT:
            return sensor::Unit::Ampere;
        case MAGNITUDE_VOLTAGE:
            return sensor::Unit::Volt;
        case MAGNITUDE_POWER_ACTIVE:
            return sensor::Unit::Watt;
        case MAGNITUDE_POWER_APPARENT:
            return sensor::Unit::Voltampere;
        case MAGNITUDE_POWER_REACTIVE:
            return sensor::Unit::VoltampereReactive;
        case MAGNITUDE_ENERGY_DELTA:
            return sensor::Unit::Joule;
        case MAGNITUDE_ENERGY:
            return sensor::Unit::KilowattHour;
        case MAGNITUDE_PM1DOT0:
        case MAGNITUDE_PM2DOT5:
        case MAGNITUDE_PM10:
        case MAGNITUDE_TVOC:
        case MAGNITUDE_CH2O:
            return sensor::Unit::MicrogrammPerCubicMeter;
        case MAGNITUDE_CO:
        case MAGNITUDE_CO2:
        case MAGNITUDE_NO2:
        case MAGNITUDE_VOC:
            return sensor::Unit::PartsPerMillion;
        case MAGNITUDE_LUX:
            return sensor::Unit::Lux;
        case MAGNITUDE_RESISTANCE:
            return sensor::Unit::Ohm;
        case MAGNITUDE_HCHO:
            return sensor::Unit::MilligrammPerCubicMeter;
        case MAGNITUDE_GEIGER_CPM:
            return sensor::Unit::CountsPerMinute;
        case MAGNITUDE_GEIGER_SIEVERT:
            return sensor::Unit::MicrosievertPerHour;
        case MAGNITUDE_DISTANCE:
            return sensor::Unit::Meter;
        case MAGNITUDE_FREQUENCY:
            return sensor::Unit::Hertz;
        case MAGNITUDE_PH:
            return sensor::Unit::Ph;
        default:
            break;
        }

        return sensor::Unit::None;
    }

    // Descriptive name of the sensor
    virtual String description() = 0;

    // Descriptive name of the slot # index
    virtual String description(unsigned char index) = 0;

    // Address of the sensor (it could be the GPIO or I2C address)
    virtual String address(unsigned char index) = 0;

    // Type for slot # index
    virtual unsigned char type(unsigned char index) = 0;

    // Current value for slot # index
    virtual double value(unsigned char index) = 0;

protected:
    unsigned char _sensor_id = 0;
    int _error = 0;
    bool _dirty = true;
    unsigned char _count = 0;
    bool _ready = false;
};
