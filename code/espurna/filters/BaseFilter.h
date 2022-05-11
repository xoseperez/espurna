// -----------------------------------------------------------------------------
// Base Filter (other filters inherit from this)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include <cstddef>

class BaseFilter {
public:
    virtual ~BaseFilter() = default;

    // Reset internal value to default and also erases internal storage
    virtual void reset() {
    }

    // Defaults to 0 aka no backing storage
    virtual size_t capacity() const {
        return 0;
    }

    // Resize the backing storage (when it is available)
    virtual void resize(size_t) {
    }

    // Store reading
    virtual void update(double value) = 0;

    // Return filtered value
    virtual double value() const = 0;
};
