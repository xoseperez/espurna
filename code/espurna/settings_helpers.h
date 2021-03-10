/*

Part of the SETTINGS module

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <utility>

// --------------------------------------------------------------------------

class SettingsKey {
public:
    SettingsKey(const char* key) :
        _key(key)
    {}

    SettingsKey(const String& key) :
        _key(key)
    {}

    SettingsKey(String&& key) :
        _key(std::move(key))
    {}

    SettingsKey(const String& prefix, size_t index) {
        _key.reserve(prefix.length() + 4);
        _key += prefix;
        _key += index;
    }

    SettingsKey(String&& prefix, size_t index) :
        _key(std::move(prefix))
    {
        _key += index;
    }

    SettingsKey(const char* prefix, size_t index) :
        _key(prefix)
    {
        _key += index;
    }

    const char* c_str() const {
        return _key.c_str();
    }

    size_t length() const {
        return _key.length();
    }

    bool operator==(const char* other) const {
        return _key == other;
    }

    bool operator==(const String& other) const {
        return _key == other;
    }

    const String& value() const {
        return _key;
    }

    explicit operator String() const & {
        return _key;
    }

    explicit operator String() && {
        return std::move(_key);
    }

private:
    String _key;
};

// --------------------------------------------------------------------------

namespace settings {
namespace internal {

struct BasicSetting {
    using Get = String(*)();

    BasicSetting() = delete;
    constexpr BasicSetting(const char* const key, Get get) :
        _key(key),
        _get(get)
    {}

    constexpr const char* const key() const {
        return _key;
    }

    String get() const {
        return _get();
    }

private:
    const char* const _key;
    Get _get;
};

struct IndexedSetting {
    using Get = String(*)(size_t);

    IndexedSetting() = delete;
    constexpr IndexedSetting(const char* const prefix, Get get) :
        _prefix(prefix),
        _get(get)
    {}

    constexpr const char* const prefix() const {
        return _prefix;
    }

    String get(size_t index) const {
        return _get(index);
    }

private:
    const char* const _prefix;
    Get _get;
};

} // namespace internal

// 'optional' type for byte range
struct ValueResult {
    ValueResult() = default;
    ValueResult(const ValueResult&) = default;
    ValueResult(ValueResult&&) = default;

    explicit ValueResult(const String& value) :
        _result(true),
        _value(value)
    {}

    explicit ValueResult(String&& value) :
        _result(true),
        _value(std::move(value))
    {}

    template <typename T>
    ValueResult& operator=(T&& value) {
        if (!_result) {
            _result = true;
            _value = std::forward<T>(value);
        }

        return *this;
    }

    explicit operator bool() const {
        return _result;
    }

    String get() && {
        auto moved = std::move(_value);
        _result = false;
        return moved;
    }

    const String& ref() const {
        return _value;
    }

    const char* c_str() const {
        return _value.c_str();
    }

    size_t length() const {
        return _value.length();
    }

private:
    bool _result { false };
    String _value;
};

} // namespace settings
