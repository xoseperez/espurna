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

    bool status() const override {
        return true;
    }

    void resize(size_t) override {
        _reset();
    }

    void reset() override {
        _reset();
    }

    double value() const {
        return _value;
    }

private:
    void _reset() {
        _value = 0;
    }

    double _value = 0;
};
