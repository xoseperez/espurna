// -----------------------------------------------------------------------------
// Max Filter
// Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT

#pragma once

#include <vector>
#include "BaseFilter.h"

class MaxFilter : public BaseFilter {

    public:

        double result() {
            return max();
        }

};

#endif // SENSOR_SUPPORT
