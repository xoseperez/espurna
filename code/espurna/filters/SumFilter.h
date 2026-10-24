// -----------------------------------------------------------------------------
// Sum Filter
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "BaseFilter.h"

class SumFilter : public BaseFilter {
public:
    void update(double value) override {
        if (!_status) {
            _value = value;
        } else {
            _value += value;
        }

        _status = true;
    }

    bool ready() const override {
        return _status;
    }

    bool available() const override {
        return _status;
    }

    void reset() override {
        _reset();
    }

    void restart() override {
        _reset();
    }

    double value() const override {
        return _value;
    }

private:
    void _reset() {
        _status = false;
    }

    double _value { 0.0 };
    bool _status { false };
};

