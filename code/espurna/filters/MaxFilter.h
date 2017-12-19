// -----------------------------------------------------------------------------
// Max Filter
// Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include <vector>
#include "BaseFilter.h"

class MaxFilter : public BaseFilter {

    public:

        virtual double result() {
            return max();
        }

};
