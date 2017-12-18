// -----------------------------------------------------------------------------
// Aggregator base class
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
