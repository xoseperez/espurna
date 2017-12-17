// -----------------------------------------------------------------------------
// Aggregator Moving Average
// -----------------------------------------------------------------------------

#pragma once

#include <vector>
#include "BaseFilter.h"

class MovingAverageFilter : public BaseFilter {

    public:

        MovingAverageFilter(unsigned char size) {
            _size = size;
            for (unsigned char i=0; i<size; i++) {
                _data.push_back(0);
            }
        }

        virtual void add(double value) {
            _sum = _sum + value - _data.at(_pointer);
            _data.at(_pointer) = value;
            _pointer = (_pointer + 1) % _size;
        }

        virtual void reset() {
            // Nothing to do
        }

        virtual double result() {
            return _sum;
        }

    protected:

        unsigned char _size = 0;
        unsigned char _pointer = 0;
        double _sum = 0;

};
