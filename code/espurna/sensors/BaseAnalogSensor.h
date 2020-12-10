// -----------------------------------------------------------------------------
// Abstract emon sensor class (other sensor classes extend this class)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "BaseSensor.h"

class BaseAnalogSensor : public BaseSensor {

    public:

        virtual unsigned long getR0() { return _R0; }
        virtual void setR0(unsigned long value) { _R0 = value; }

        virtual unsigned long getRL() { return _Rl; }
        virtual void setRL(unsigned long value) { _Rl = value; }

        virtual unsigned long getRS() { return _Rs; }
        virtual void setRS(unsigned long value) { _Rs = value; }

        virtual void calibrate() { }

        unsigned char type() { return sensor::type::Analog; }

    protected:

        unsigned long _R0;            // R0, calibration value at 25º
        unsigned long _Rl;            // RL, load resistance
        unsigned long _Rs;            // cached resistance

};
