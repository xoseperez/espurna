// -----------------------------------------------------------------------------
// Max Filter
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT

#pragma once

#include "BaseFilter.h"

class MaxFilter : public BaseFilter {

    public:

        void add(double value) {
            if (value > _max) _max = value;
        }

        unsigned char count() {
            return 1;
        }

        void reset() {
            _max = 0;
        }

        double result() {
            return _max;
        }

        void resize(unsigned char size) {}

    protected:

        double _max = 0;

};

#endif // SENSOR_SUPPORT
