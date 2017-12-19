// -----------------------------------------------------------------------------
// Base Filter (other filters inherit from this)
// Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include <vector>

class BaseFilter {

    public:

        BaseFilter() {
        }

        ~BaseFilter() {
        }

        virtual void add(double value) {
            _data.push_back(value);
        }

        virtual unsigned char count() {
            return _data.size();
        }

        virtual void reset() {
            _data.clear();
        }

        virtual double max() {
            double max = 0;
            for (unsigned char i = 1; i < _data.size(); i++) {
                if (max < _data.at(i)) max = _data.at(i);
            }
            return max;
        }

        virtual double result() {
            return 0;
        }

    protected:

        std::vector<double> _data;

};
