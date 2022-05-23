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

    void reset() override {
        if (_values.size() > 2) {
            _values[0] = _values.back();
            _values.resize(1);
        } else {
            _values.clear();
        }
    }

    double value() const override {
        double sum { 0.0 };

        if (_values.size() > 2) {
            auto median = [](double previous, double current, double next) {
                if (previous < current) {
                    if (current < next) {
                        return current;
                    } else if (previous < next) {
                        return next;
                    } else {
                        return previous;
                    }
                } else if (previous < next) {
                    return previous;
                } else if (current < next) {
                    return next;
                }

                return current;
            };

            for (auto prev = _values.begin(); prev != (_values.end() - 2); ++prev) {
                sum += median(*prev, *std::next(prev, 1), *std::next(prev, 2));
            }

            sum /= (_values.size() - 2);
        } else if (_values.size() > 0) {
            sum = _values.front();
        }

        return sum;
    }

    size_t capacity() const override {
        return _capacity;
    }

    void resize(size_t capacity) override {
        if (_capacity != capacity) {
            _capacity = capacity;
            _values.clear();
            _values.reserve(_capacity + 1);
        }
    }

private:
    std::vector<double> _values;
    size_t _capacity = 0;
};
