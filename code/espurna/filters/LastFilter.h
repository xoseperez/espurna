// -----------------------------------------------------------------------------
// Last Filter
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT

#pragma once

#include "BaseFilter.h"

class LastFilter : public BaseFilter {

    public:

        void add(double value) {
            _value = value;
        }

        unsigned char count() {
            return 1;
        }

        void reset() {
            _value = 0;
        }

        double result() {
            return _value;
        }

        void resize(unsigned char size) {}

    protected:

        double _value = 0;

};

#endif // SENSOR_SUPPORT
