// -----------------------------------------------------------------------------
// Last Filter
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "BaseFilter.h"

class LastFilter : public BaseFilter {
public:
    void update(double value) override {
        _value = value;
    }

    size_t capacity() const override {
        return 1;
    }

    void reset() override {
        _reset();
    }

    void resize(size_t) override {
        _reset();
    }

    double value() const override {
        return _value;
    }

private:
    void _reset() {
        _value = 0;
    }

    double _value = 0;
    bool _status = false;
};
