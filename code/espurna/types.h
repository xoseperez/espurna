/*

Part of the SYSTEM MODULE

Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/
#pragma once

#include <Arduino.h>
#include <sys/pgmspace.h>

// missing in our original header
extern "C" int memcmp_P(const void*, const void*, size_t);

namespace espurna {

struct StringView {
    StringView() = delete;
    ~StringView() = default;

    constexpr StringView(const StringView&) noexcept = default;
    constexpr StringView(StringView&&) noexcept = default;

#if __cplusplus > 201103L
    constexpr StringView& operator=(const StringView&) noexcept = default;
    constexpr StringView& operator=(StringView&&) noexcept = default;
#else
    StringView& operator=(const StringView&) noexcept = default;
    StringView& operator=(StringView&&) noexcept = default;
#endif

    constexpr StringView(std::nullptr_t) noexcept :
        _ptr(nullptr),
        _len(0)
    {}

    constexpr StringView(const char* ptr, size_t len) noexcept :
        _ptr(ptr),
        _len(len)
    {}

    constexpr StringView(const char* ptr) noexcept :
        StringView(ptr, __builtin_strlen(ptr))
    {}

    template <size_t Size>
    constexpr StringView(const char (&string)[Size]) noexcept :
        StringView(&string[0], Size - 1)
    {}

    constexpr StringView(const char* begin, const char* end) noexcept :
        StringView(begin, end - begin)
    {}

    explicit StringView(const __FlashStringHelper* ptr) noexcept :
        _ptr(reinterpret_cast<const char*>(ptr)),
        _len(strlen_P(_ptr))
    {}

    StringView(const String&&) = delete;
    StringView(const String& string) noexcept :
        StringView(string.c_str(), string.length())
    {}

    StringView& operator=(const String& string) noexcept {
        _ptr = string.c_str();
        _len = string.length();
        return *this;
    }

    template <size_t Size>
    constexpr StringView& operator=(const char (&string)[Size]) noexcept {
        _ptr = &string[0];
        _len = Size - 1;
        return *this;
    }

    constexpr const char* begin() const noexcept {
        return _ptr;
    }

    constexpr const char* end() const noexcept {
        return _ptr + _len;
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

    explicit operator String() const {
        return toString();
    }

    bool compare(StringView other) const;

private:
    static bool inFlash(const char* ptr) {
        // common comparison would use >=0x40000000
        // instead, slightly reduce the footprint by
        // checking *only* for numbers below it
        static constexpr uintptr_t Mask { 1 << 30 };
        return (reinterpret_cast<uintptr_t>(ptr) & Mask) > 0;
    }

    const char* _ptr;
    size_t _len;
};

inline bool operator==(StringView lhs, StringView rhs) {
    return lhs.compare(rhs);
}

inline String operator+(String&& lhs, StringView rhs) {
    lhs.concat(rhs.c_str(), rhs.length());
    return lhs;
}

#define STRING_VIEW(X) ({\
        alignas(4) static constexpr char __pstr__[] PROGMEM = (X);\
        ::espurna::StringView{__pstr__};\
    })

} // namespace espurna
