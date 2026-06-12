// -----------------------------------------------------------------------------
// Median Filter
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// Copyright (C) 2023-2024 by Maxim Prokhorov <prokhorov dot max at outlook dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "BaseFilter.h"

#include <algorithm>
#include <vector>

class MedianFilter : public BaseFilter {
public:
    void update(double value) override {
        // Inserted value is always with the last index
        if (!_size) {
            return;
        }

        const auto size = _values.size();
        auto pending =
            Value{
                .value = value,
                .index = size,
            };

        // Special case for initial state
        if (!_values.size()) {
            _values.push_back(pending);
            return;
        }

        // Pop first element by index, shift everything else down
        if (_values.size() == _size) {
            const auto it = std::find_if(
                _values.begin(),
                _values.end(),
                [&](const Value& value) {
                    return value.index == 0;
                });

            _values.erase(it);
            for (auto& entry : _values) {
                --entry.index;
            }

            pending.index -= 1;
        }

        // Defensively sort the values vector
        const auto upper = std::upper_bound(
            _values.begin(), _values.end(), pending,
            [](const Value& lhs, const Value& rhs) {
                return lhs.value < rhs.value;
            });
        _values.insert(upper, pending);
    }

    double value() const override {
        // Special case when early report triggers value read
        if (_values.size() == 1) {
            return _values.front().value;
        } else if (_values.size() == 2) {
            return (_values.front().value + _values.back().value) / 2.0;
        // Otherwise, pick out the middle section and average it
        } else if (0 == (_values.size() % 2)) {
            const auto lhs = _values.begin() + ((_values.size() / 2) - 1);
            const auto rhs = std::next(lhs);

            return ((*lhs).value + (*rhs).value) / 2.0;
        }

        // ...or, use the middle element as-is
        const auto it = _values.begin() + (_values.size() / 2);
        return (*it).value;
    }

    bool available() const override {
        return _values.size() > 0;
    }

    bool ready() const override {
        return (_size > 0)
            && (_values.size() == _size);
    }

    void resize(size_t size) override {
        _resize(size);
    }

    void reset() override {
        _values.clear();
    }

private:
    void _resize(size_t size) {
        if (!size) {
            _values.clear();
            _values.shrink_to_fit();
        } else if ((size < _size) && _values.size()) {
            _reset_offset(_size - size);
        } else if (size > _size) {
            _values.reserve(size);
        }

        _size = size;
    }

    void _reset_offset(size_t offset) {
        const auto it = std::remove_if(
            _values.begin(),
            _values.end(),
            [&](Value& value) {
                const auto remove = value.index < offset;
                if (!remove) {
                    value.index -= offset;
                }

                return remove;
            });

        if (it != _values.end()) {
            _values.erase(it, _values.end());
            _values.shrink_to_fit();
        }
    }

    void _reset(size_t size) {
        _values.clear();
        _resize(size);
    }

    void _reset() {
        _reset(_size);
    }

    // Track input index, since '_values' is sorted by 'value'
    struct Value {
        double value;
        size_t index;
    };

    std::vector<Value> _values {};
    size_t _size { 0 };
};

