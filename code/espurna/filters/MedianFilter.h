// -----------------------------------------------------------------------------
// Median Filter
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT

#pragma once

#include "BaseFilter.h"

class MedianFilter : public BaseFilter {

    public:

        ~MedianFilter() {
            if (_data) delete _data;
        }

        void add(double value) {
            if (_pointer <= _size) {
                _data[_pointer] = value;
                _pointer++;
            }
        }

        unsigned char count() {
            return _pointer;
        }

        void reset() {
            if (_pointer > 0) {
                _data[0] = _data[_pointer-1];
                _pointer = 1;
            } else {
                _pointer = 0;
            }
        }

        double result() {

            double sum = 0;

            if (_pointer > 2) {

                for (unsigned char i = 1; i <= _pointer - 2; i++) {

                    // For each position,
                    // we find the median with the previous and next value
                    // and use that for the sum

                    double previous = _data[i-1];
                    double current = _data[i];
                    double next = _data[i+1];

                    if (previous > current) std::swap(previous, current);
                    if (current > next) std::swap(current, next);
                    if (previous > current) std::swap(previous, current);

                    sum += current;

                }

                sum /= (_pointer - 2);

            } else if (_pointer > 0) {

                sum = _data[0];

            }

            return sum;

        }

        void resize(unsigned char size) {
            if (_size == size) return;
            _size = size;
            if (_data) delete _data;
            _data = new double[_size+1];
            for (unsigned char i=0; i<=_size; i++) _data[i] = 0;
            _pointer = 0;
        }

    protected:

        unsigned char _pointer = 0;
        double * _data = NULL;

};

#endif // SENSOR_SUPPORT
