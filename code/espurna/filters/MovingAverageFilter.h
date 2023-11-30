// -----------------------------------------------------------------------------
// Moving Average Filter
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "BaseFilter.h"

#include <vector>

class MovingAverageFilter : public BaseFilter {
public:
    void update(double value) override {
        _sum = _sum + value - _values[_sample];
        _values[_sample] = value;
        _sample = (_sample + 1) % _values.capacity();
    }

    size_t capacity() const override {
        return _values.capacity();
    }

    double value() const override {
        return _sum;
    }

    void resize(size_t size) override {
        _sum = 0.0;
        _sample = 0;
        _values.clear();
        _values.resize(size, 0.0);
    }

private:
    std::vector<double> _values {{0.0}};
    size_t _sample = 0;
    double _sum = 0;
};
