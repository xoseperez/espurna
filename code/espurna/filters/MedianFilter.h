// -----------------------------------------------------------------------------
// Median Filter
// Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "BaseFilter.h"

class MedianFilter : public BaseFilter {

    public:

        virtual void reset() {
            double last = _data.empty() ? 0 : _data.back();
            _data.clear();
            add(last);
        }

        virtual double result() {

            double sum = 0;

            if (_data.size() > 2) {

                for (unsigned char i = 1; i <= _data.size() - 2; i++) {

                    double previous = _data.at(i-1);
                    double current = _data.at(i);
                    double next = _data.at(i+1);

                    if (previous > current) std::swap(previous, current);
                    if (current > next) std::swap(current, next);
                    if (previous > current) std::swap(previous, current);

                    sum += current;

                }

                sum /= (_data.size() - 2);

            } else if (_data.size() > 0) {

                sum = _data.front();

            }

            return sum;

        }

};
