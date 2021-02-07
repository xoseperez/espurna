/*

Helper class to set the boolean exactly when setting multiple times in a row
(as an alternative to checking input every time before setting)

*/

#pragma once

struct OnceFlag {
    OnceFlag() = default;
    OnceFlag(const OnceFlag&) = delete;
    OnceFlag(OnceFlag&&) = delete;

    explicit operator bool() const {
        return _value;
    }

    OnceFlag& operator=(bool value) {
        if (!_value) {
            _value = value;
        }
        return *this;
    }

    void set() {
        _value = true;
    }

    bool get() const {
        return _value;
    }

private:
    bool _value { false };
};

