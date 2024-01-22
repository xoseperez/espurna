// -----------------------------------------------------------------------------
// Max Filter
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "BaseFilter.h"

#include <algorithm>

class MaxFilter : public BaseFilter {
public:
    void update(double value) override {
        _value = std::max(value, _value);
    }

    size_t capacity() const override {
        return 1;
    }

    void reset() override {
        _value = 0;
    }

    double value() const {
        return _value;
    }

private:
    double _value = 0;
};
