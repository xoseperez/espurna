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

    // Defaults to false aka filter is not initialized
    virtual bool status() const {
        return false;
    }

    // Resize the backing storage (when it is available) and reset internal state
    virtual void resize(size_t) {
    }

    // Store reading
    virtual void update(double value) = 0;

    // Return filtered value
    virtual double value() const = 0;
};
