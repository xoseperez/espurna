// -----------------------------------------------------------------------------
// Moving Average Filter
// Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT

#pragma once

#include <vector>
#include "BaseFilter.h"

class MovingAverageFilter : public BaseFilter {

    public:

        void add(double value) {

            // If we are at the end of the vector we add a new element
            if (_pointer >= _data.size()) {
                _sum = _sum + value;
                _data.push_back(value);

            // Else we substract the old value at the current poisiton and overwrite it
            } else {
                _sum = _sum + value - _data[_pointer];
                _data[_pointer] = value;
            }

            _pointer++;

        }

        void reset() {

            // I assume series length to be the number of data points since last reset,
            // so I zero-ed old data points from this point on
            for (unsigned char i=_pointer; i<_data.size(); i++) {
                _data[i] = 0;
            }

            _pointer = 0;

        }

        double result() {

            // At this point we want to return the sum since last request
            for (unsigned char i=_pointer; i<_data.size(); i++) {
                _sum = _sum - _data[i];
            }

            return _sum;

        }

    protected:

        unsigned char _pointer = 0;
        double _sum = 0;

};

#endif // SENSOR_SUPPORT
