// -----------------------------------------------------------------------------
// Base Filter (other filters inherit from this)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT

#pragma once

class BaseFilter {

    public:
        virtual void add(double value);
        virtual unsigned char count();
        virtual void reset();
        virtual double result();
        virtual void resize(unsigned char size);
        unsigned char size() { return _size; };

    protected:
        unsigned char _size;

};

#endif // SENSOR_SUPPORT
