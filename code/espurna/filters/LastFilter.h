// -----------------------------------------------------------------------------
// Last Filter
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "BaseFilter.h"

class LastFilter : public BaseFilter {
public:
    void update(double value) override {
        _status = true;
        _value = value;
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

