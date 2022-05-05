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

    SettingsKey(const char* prefix, size_t index) :
        _key(prefix)
    {
        _key += index;
    }

    SettingsKey(const __FlashStringHelper* prefix, size_t index) :
        SettingsKey(reinterpret_cast<const char*>(prefix), index)
    {}

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

// 'optional' type for a byte range (...from the settings storage)
struct ValueResult {
    ValueResult() = default;
    ValueResult(const ValueResult&) = default;
    ValueResult(ValueResult&&) = default;

    explicit ValueResult(const String& value) :
        _value(value),
        _result(true)
    {}

    explicit ValueResult(String&& value) :
        _value(std::move(value)),
        _result(true)
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
    String _value;
    bool _result { false };
};

// generic number range
struct Iota {
    Iota() = default;
    constexpr explicit Iota(size_t end) noexcept :
        _it(0),
        _end(end)
    {}

    constexpr Iota(size_t begin, size_t end) noexcept :
        _it(begin),
        _end(end)
    {}

    constexpr Iota(size_t begin, size_t end, size_t step) noexcept :
        _it(begin),
        _end(end),
        _step(step)
    {}

#if __cplusplus >= 201703L
    constexpr
#endif
    Iota& operator++() {
        if (_it != _end) {
            _it = ((_it + _step) > _end)
                ? _end : (_it + _step);
        }

        return *this;
    }

#if __cplusplus >= 201703L
    constexpr
#endif
    Iota operator++(int) {
        Iota out(*this);
        ++out;
        return out;
    }

    constexpr explicit operator bool() const {
        return _it != _end;
    }

    constexpr size_t operator*() const {
        return _it;
    }

    constexpr size_t end() const {
        return _end;
    }

private:
    size_t _it { 0 };
    size_t _end { 0 };
    size_t _step { 1 };
};

struct StringView {
    StringView() = delete;

    template <size_t Size>
    constexpr StringView(const char (&string)[Size]) noexcept :
        _ptr(&string[0]),
        _len(Size - 1)
    {}

    constexpr StringView(const char* ptr, size_t len) noexcept :
        _ptr(ptr),
        _len(len)
    {}

    explicit StringView(const __FlashStringHelper* ptr) noexcept :
        _ptr(reinterpret_cast<const char*>(ptr)),
        _len(strlen_P(_ptr))
    {}

    explicit StringView(const SettingsKey& key) noexcept :
        _ptr(key.c_str()),
        _len(key.length())
    {}

    StringView(const String&&) = delete;
    StringView(const String& string) noexcept :
        StringView(string.c_str(), string.length())
    {}

    constexpr StringView(const char* ptr) noexcept :
        StringView(ptr, __builtin_strlen(ptr))
    {}

    bool compareRam(const StringView& other) const {
        return (other._len == _len)
            && (strncmp(other._ptr, _ptr, _len) == 0);
    }

    bool compareFlash(const StringView& other) const {
        return (other._len == _len)
            && (strncmp_P(other._ptr, _ptr, _len) == 0);
    }

    constexpr const char* c_str() const {
        return _ptr;
    }

    constexpr size_t length() const {
        return _len;
    }

    String toString() const {
        String out;
        out.concat(_ptr, _len);
        return out;
    }

private:
    const char* _ptr;
    size_t _len;
};

inline bool operator==(const StringView& lhs, const char* rhs) {
    return lhs.compareFlash(rhs);
}

inline bool operator==(const StringView& lhs, const String& rhs) {
    return lhs.compareFlash(rhs);
}

inline bool operator==(const StringView& lhs, const SettingsKey& rhs) {
    return lhs.compareFlash(StringView{rhs.c_str(), rhs.length()});
}

#define STRING_VIEW(X) ({\
        alignas(4) static constexpr char __pstr__[] PROGMEM = (X);\
        ::settings::StringView{__pstr__};\
    })

namespace options {

struct EnumerationNumericHelper {
    static bool check(const String& value);
};

template <typename Value>
struct alignas(8) Enumeration {
    static_assert(std::is_enum<Value>::value, "");

    using ValueType = Value;
    using UnderlyingType = typename std::underlying_type<Value>::type;
    static_assert((alignof(UnderlyingType) % 4) == 0, "");

    struct Numeric {
        using Convert = UnderlyingType(*)(const String&);

        bool check(const String& value, Convert convert) {
            _checked = false;

            if (EnumerationNumericHelper::check(value)) {
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


    Enumeration() = delete;
    constexpr Enumeration(ValueType value, const char* string) noexcept :
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

} // namespace options

namespace query {

inline bool samePrefix(StringView key, StringView prefix) {
    if (key.length() > prefix.length()) {
        return strncmp_P(key.c_str(), prefix.c_str(), prefix.length()) == 0;
    }

    return false;
}

struct StringViewIterator {
    using Element = const StringView*;

    StringViewIterator() = delete;
    StringViewIterator(const StringViewIterator&) = default;
    StringViewIterator(StringViewIterator&&) = default;

    constexpr StringViewIterator(Element begin, Element end) noexcept :
        _it(begin),
        _begin(begin),
        _end(end)
    {}

    StringViewIterator(std::initializer_list<StringView>&& values) noexcept :
        _it(std::begin(values)),
        _begin(_it),
        _end(std::end(values))
    {}

    StringViewIterator& operator++() {
        if (_it != _end) {
            ++_it;
        }

        return *this;
    }

    StringViewIterator operator++(int) {
        StringViewIterator out(*this);
        ++out;
        return out;
    }

    constexpr Element operator*() const {
        return _it;
    }

    constexpr Element begin() const {
        return _begin;
    }

    constexpr Element end() const {
        return _end;
    }

private:
    Element _it;
    Element _begin;
    Element _end;
};

// static container to allow queries on an array of key<->serialized value pairs
// both require a linear iterators as input e.g. most commonly - a static array
// (either a plain [], or an std::array. template should treat both equally)

// single key variant, exact match of the provided string
struct alignas(8) Setting {
    using ValueFunc = String(*)();

    Setting() = delete;
    constexpr Setting(StringView key, ValueFunc func) noexcept :
        _key(key),
        _func(func)
    {}

    constexpr StringView key() const {
        return _key;
    }

    String value() const {
        return _func();
    }

    bool operator==(const String& key) const {
        return _key == key;
    }

    bool operator==(const StringView& key) const {
        return _key.compareFlash(key);
    }

    static String findValueFrom(const Setting* begin, const Setting* end, StringView key);

    template <typename T>
    static String findValueFrom(const T& settings, StringView key) {
        return findValueFrom(std::begin(settings), std::end(settings), key);
    }

private:
    StringView _key;
    ValueFunc _func;
};

// 'indexed' variant, for a group of objects under the same prefix with a numeric suffix
// (suffix range depends on the external 'iota', only known at runtime)
struct alignas(8) IndexedSetting {
    using ValueFunc = String(*)(size_t);

    IndexedSetting() = delete;
    constexpr IndexedSetting(StringView prefix, ValueFunc func) noexcept :
        _prefix(prefix),
        _func(func)
    {}

    String value(size_t id) const {
        return _func(id);
    }

    constexpr ValueFunc func() const {
        return _func;
    }

    constexpr StringView prefix() const {
        return _prefix;
    }

    static bool findSamePrefix(const IndexedSetting* begin, const IndexedSetting* end, StringView key);

    template <typename T>
    static bool findSamePrefix(const T& settings, StringView key) {
        return findSamePrefix(std::begin(settings), std::end(settings), key);
    }

    static String findValueFrom(Iota iota, const IndexedSetting* begin, const IndexedSetting* end, StringView key);

    template <typename T>
    static String findValueFrom(Iota iota, const T& settings, StringView key) {
        return findValueFrom(iota, std::begin(settings), std::end(settings), key);
    }

    template <typename T>
    static String findValueFrom(size_t size, const T& settings, StringView key) {
        return findValueFrom(Iota{size}, settings, key);
    }

private:
    StringView _prefix;
    ValueFunc _func;
};


} // namespace query
} // namespace settings
