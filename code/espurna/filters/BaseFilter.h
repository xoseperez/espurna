// -----------------------------------------------------------------------------
// Base Filter (other filters inherit from this)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT

#pragma once

class BaseFilter {

    public:
        virtual void add(double value) = 0;
        virtual unsigned char count() = 0;
        virtual void reset() = 0;
        virtual double result() = 0;
        virtual void resize(unsigned char size) = 0;
        unsigned char size() { return _size; };

    protected:
        unsigned char _size;

};

#endif // SENSOR_SUPPORT
