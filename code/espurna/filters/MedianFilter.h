// -----------------------------------------------------------------------------
// Median Filter
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "BaseFilter.h"

#include <algorithm>
#include <vector>

class MedianFilter : public BaseFilter {
public:
    void update(double value) override {
        if (_values.size() < _values.capacity()) {
            _values.push_back(value);
        }
    }

    size_t capacity() const override {
        return _values.capacity();
    }

    void reset() override {
        if (_values.size() && _values.capacity() != 1) {
            _values[0] = _values.back();
            _values.resize(1);
        } else {
            _values.clear();
        }
    }

    double value() const override {
        double sum { 0.0 };

        if (_values.size() > 2) {
            // For each position,
            // we find the median with the previous and next value
            // and use that for the sum
            auto calculate = [](double previous, double current, double next) {
                if (previous > current) std::swap(previous, current);
                if (current > next) std::swap(current, next);
                if (previous > current) std::swap(previous, current);

                return current;
            };

            for (auto prev = _values.begin(); prev != (_values.end() - 2); ++prev) {
                const auto current = std::next(prev);
                const auto next = std::next(current);
                sum += calculate(*prev, *current, *next);
            }

            sum /= (_values.size() - 2);
        } else if (_values.size() > 0) {
            sum = _values.front();
        }

        return sum;
    }

    void resize(size_t capacity) override {
        if (_values.capacity() != capacity) {
            _values.clear();
            _values.reserve(capacity);
        }
    }

private:
    std::vector<double> _values;
};
