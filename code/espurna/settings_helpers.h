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
    SettingsKey() = default;
    SettingsKey(const SettingsKey&) = default;
    SettingsKey(SettingsKey&&) = default;

    SettingsKey(const char* key) :
        _key(key)
    {}

    SettingsKey(const __FlashStringHelper* key) :
        _key(key)
    {}

    SettingsKey(const String& key) :
        _key(key)
    {}

    SettingsKey(String&& key) :
        _key(std::move(key))
    {}

    SettingsKey(const String& prefix, size_t index) :
        _key(prefix)
    {
        _key += index;
    }

    SettingsKey(String&& prefix, size_t index) :
        _key(std::move(prefix))
    {
        _key += index;
    }

    const char* c_str() const {
        return _key.c_str();
    }

    size_t length() const {
        return _key.length();
    }

    template <typename T>
    bool operator==(T&& other) const {
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
    constexpr BasicSetting(const char* key, Get get) :
        _key(key),
        _get(get)
    {}

    constexpr const char* key() const {
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
    constexpr IndexedSetting(const char* prefix, Get get) :
        _prefix(prefix),
        _get(get)
    {}

    constexpr const char* prefix() const {
        return _prefix;
    }

    String get(size_t index) const {
        return _get(index);
    }

private:
    const char* const _prefix;
    Get _get;
};

struct EnumOptionNumeric {
    static bool check(const String& value);
};

template <typename Value>
struct EnumOption {
    static_assert(std::is_enum<Value>::value, "");

    using ValueType = Value;
    using UnderlyingType = typename std::underlying_type<Value>::type;
    static_assert((alignof(UnderlyingType) % 4) == 0, "");

    struct Numeric {
        using Convert = UnderlyingType(*)(const String&);

        bool check(const String& value, Convert convert) {
            _checked = false;

            if (EnumOptionNumeric::check(value)) {
                _value = convert(value);
                _checked = true;
            }

            return _checked;
        }

        explicit operator bool() const {
            return _checked;
        }

        UnderlyingType value() const {
            return _value;
        }

    private:
        UnderlyingType _value{};
        bool _checked { false };
    };


    EnumOption() = delete;
    constexpr EnumOption(ValueType value, const char* string) noexcept :
        _value(value),
        _string(string)
    {}

    constexpr ValueType value() const {
        return _value;
    }

    constexpr UnderlyingType numeric() const {
        return static_cast<UnderlyingType>(_value);
    }

    constexpr const char* string() const {
        return _string;
    }

    bool operator==(const String& string) const {
        return strcmp_P(string.c_str(), _string) == 0;
    }

private:
    ValueType _value;
    const char* _string;
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
