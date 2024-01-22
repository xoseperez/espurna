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
        if (_values.size() < _values.capacity()) {
            _values.push_back(value);
        }
    }

    size_t capacity() const override {
        return _values.capacity();
    }

    double value() const override {
        double out{ 0. };

        for (const auto& value : _values) {
            out += value;
        }

        if (_values.size()) {
            out /= _values.size();
        }

        return out;
    }

    void resize(size_t size) override {
        _values.clear();
        _values.reserve(size);
    }

    void reset() override {
        _values.clear();
    }

private:
    std::vector<double> _values{};
};
