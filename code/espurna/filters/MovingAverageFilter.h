// -----------------------------------------------------------------------------
// Moving Average Filter
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// Copyright (C) 2023-2024 by Maxim Prokhorov <prokhorov dot max at outlook dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "BaseFilter.h"

#include <cstdio>
#include <vector>
#include <numeric>

class MovingAverageFilter : public BaseFilter {
public:
    void update(double value) override {
        if (!_size) {
            return;
        }

        if (_size == _values.size()) {
            _values.erase(_values.begin());
        }

        _values.push_back(value);
    }

    bool available() const override {
        return _values.size() > 0;
    }

    bool ready() const override {
        return (_size > 0)
            && (_values.size() == _size);
    }

    double value() const override {
        if (!_values.size()) {
            return 0.0;
        }

        return std::accumulate(_values.begin(), _values.end(), 0.0)
             / _values.size();
    }

    void resize(size_t size) override {
        if (!size) {
            _values.clear();
            _values.shrink_to_fit();
        } else if (size < _size) {
            _values.erase(
                _values.begin(),
                _values.begin() + (_size - size));
            _values.shrink_to_fit();
        } else if (size > _size) {
            _values.reserve(size);
        }

        _size = size;
    }

    void reset() override {
        _values.clear();
    }

private:
    std::vector<double> _values {};
    size_t _size { 0 };
};

