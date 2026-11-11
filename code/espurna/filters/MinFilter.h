// -----------------------------------------------------------------------------
// Min Filter
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// Copyright (C) 2023-2024 by Maxim Prokhorov <prokhorov dot max at outlook dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "BaseFilter.h"

#include <algorithm>

class MinFilter : public BaseFilter {
public:
    void update(double value) override {
        if (!_status) {
            _value = value;
        } else {
            _value = std::min(value, _value);
        }
        _status = true;
    }

    bool available() const override {
        return _status;
    }

    bool ready() const override {
        return _status;
    }

    void reset() override {
        _status = false;
    }

    void restart() override {
        _status = false;
    }

    double value() const override {
        return _value;
    }

private:
    double _value { 0 };
    bool _status { false };
};

