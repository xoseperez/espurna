// -----------------------------------------------------------------------------
// Base Filter (other filters inherit from this)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include <cstddef>

class BaseFilter {
public:
    virtual ~BaseFilter() = default;

    // Reset internal state to default
    virtual void reset() {
    }

    // Notify filter that current value was used and should be updated for the next reading
    virtual void restart() {
    }

    // Resize the backing storage (when it is available) and reset internal state
    virtual void resize(size_t) {
    }

    // Whether filter value is *available* and *can* be used
    // For filters with size>=1, should mean that at least 1 value was processed
    virtual bool available() const {
        return false;
    }

    // Whether filter value is *ready* and *can* be used
    // For filters with size>=1, only true when reaching the specified size limit
    virtual bool ready() const {
        return false;
    }

    // Store reading
    virtual void update(double value) = 0;

    // Return filtered value
    virtual double value() const = 0;
};

