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

    bool status() const override {
        return _values.capacity() > 0;
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
        _values.reserve(size);
        _reset();
    }

    void reset() override {
        _reset();
    }

private:
    void _reset() {
        if (_values.size()) {
            _values.erase(_values.begin(), _values.end() - 1);
        } else {
            _values.clear();
        }
    }

    std::vector<double> _values{};
};
