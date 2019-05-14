// -----------------------------------------------------------------------------
// Moving Average Filter
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT

#pragma once

#include <vector>
#include "BaseFilter.h"

class MovingAverageFilter : public BaseFilter {

    public:

        void add(double value) {
            _sum = _sum + value - _data[_pointer];
            _data[_pointer] = value;
            _pointer = (_pointer + 1) % _size;
        }

        unsigned char count() {
            return _pointer;
        }

        void reset() {}

        double result() {
            return _sum;
        }

        void resize(unsigned char size) {
            if (_size == size) return;
            _size = size;
            if (_data) delete _data;
            _data = new double[_size];
            for (unsigned char i=0; i<_size; i++) _data[i] = 0;
            _pointer = 0;
            _sum = 0;
        }

    protected:

        unsigned char _pointer = 0;
        double _sum = 0;
        double * _data = NULL;

};

#endif // SENSOR_SUPPORT
