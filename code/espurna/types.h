/*

Part of the SYSTEM MODULE

Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/
#pragma once

#include <Arduino.h>
#include <sys/pgmspace.h>

#include <memory>

#include "compat.h"

// missing in our original header
extern "C" int memcmp_P(const void*, const void*, size_t);

namespace espurna {

// base class for loop / oneshot / generic callbacks that do not need arguments
// *not expected* to be used instead of std function at all times.
// main purpose of this special class is to circumvent the need for rtti in
// our gcc stl implementation and retrieve the 'target function' pointer
// (*should* be different in gcc 11 / 12 though, target() became constexpr)
struct Callback {
    using Type = void (*)();
    using WrapperType = std::function<void()>;

    Callback() = default;

    Callback(const Callback& other) :
        _storage(nullptr),
        _type(other._type)
    {
        copy(other);
    }

    Callback& operator=(const Callback& other) {
        reset();
        copy(other);
        return *this;
    }

    Callback(const Callback&&) = delete;
    Callback(Callback&& other) noexcept :
        _storage(nullptr),
        _type(other._type)
    {
        move(other);
    }

    Callback& operator=(Callback&& other) noexcept;

    template <typename T>
    using is_callback = std::is_same<std::remove_cvref<T>, Callback>;

    template <typename T>
    using is_type = std::is_same<T, Type>;

    template <typename T>
    using type_convertible = std::is_convertible<T, Type>;

    template <typename T>
    using wrapper_convertible = std::is_convertible<T, WrapperType>;

    // when T *can* be converted into Callback::Type
    // usually, function pointer *or* lambda without capture list
    template <typename T, 
              typename = typename std::enable_if<
                  is_type<T>::value
               || type_convertible<T>::value>::type>
    constexpr Callback(T callback) noexcept :
        _storage(Type(callback)),
        _type(StorageType::Simple)
    {}

    // anything else convertible into std function
    template <typename T, 
              typename = typename std::enable_if<
                !is_callback<T>::value>::type,
              typename = typename std::enable_if<
                wrapper_convertible<T>::value>::type,
              typename = typename std::enable_if<
                !type_convertible<T>::value>::type>
    Callback(T callback) :
        _storage(WrapperType(std::move(callback))),
        _type(StorageType::Wrapper)
    {
        static_assert(!is_callback<T>::value, "");
    }

    ~Callback() {
        reset();
    }

    bool isEmpty() const {
        return (_type == StorageType::Empty);
    }

    bool isSimple() const {
        return (_type == StorageType::Simple);
    }

    bool isWrapped() const {
        return (_type == StorageType::Wrapper);
    }

    bool operator==(Type callback) const {
        return isSimple() && (_storage.simple == callback);
    }

    void swap(Callback&) noexcept;
    void operator()() const;

private:
    union Storage {
        WrapperType wrapper;
        Type simple;

        ~Storage() {
        }

        explicit Storage(WrapperType callback) :
            wrapper(std::move(callback))
        {}

        constexpr explicit Storage(Type callback) :
            simple(callback)
        {}

        constexpr explicit Storage(std::nullptr_t) :
            simple(nullptr)
        {}
    };

    enum class StorageType {
        Empty,
        Simple,
        Wrapper,
    };

    void copy(const Callback&);
    void move(Callback&) noexcept;
    void reset();

    Storage _storage { nullptr };
    StorageType _type { StorageType::Empty };
};

// aka `std::source_location`
struct SourceLocation {
    int line;
    const char* file;
    const char* func;
};

inline constexpr SourceLocation make_source_location(
        int line = __builtin_LINE(),
        const char* file = __builtin_FILE(),
        const char* func = __builtin_FUNCTION())
{
    return SourceLocation{
        .line = line,
        .file = file,
        .func = func
    };
}

// disallow re-locking, tracking external `bool`
struct ReentryLock {
    ReentryLock() = delete;

    ReentryLock(const ReentryLock&) = delete;
    ReentryLock& operator=(const ReentryLock&) = delete;

    ReentryLock(ReentryLock&&) = default;
    ReentryLock& operator=(ReentryLock&&) = delete;

    ReentryLock(bool& handle) :
        _initialized(!handle),
        _handle(handle)
    {}

    ~ReentryLock() {
        unlock();
    }

    bool initialized() const {
        return _initialized;
    }

    void lock() {
        if (initialized()) {
            _handle = true;
        }
    }

    void unlock() {
        if (initialized()) {
            _handle = false;
        }
    }
private:
    bool _initialized;
    bool& _handle;
};

struct StringView {
    constexpr StringView() noexcept :
        _ptr(nullptr),
        _len(0)
    {}

    ~StringView() = default;

    StringView(std::nullptr_t) = delete;

    constexpr StringView(const StringView&) noexcept = default;
    constexpr StringView(StringView&&) noexcept = default;

#if __cplusplus > 201103L
    constexpr StringView& operator=(const StringView&) noexcept = default;
    constexpr StringView& operator=(StringView&&) noexcept = default;
#else
    StringView& operator=(const StringView&) noexcept = default;
    StringView& operator=(StringView&&) noexcept = default;
#endif

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

    constexpr const char& operator[](size_t offset) const {
        return *(_ptr + offset);
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

    bool equals(StringView) const;
    bool equalsIgnoreCase(StringView) const;

    bool startsWith(StringView) const;
    bool endsWith(StringView) const;

private:
#if defined(HOST_MOCK)
    constexpr static bool inFlash(const char*) {
        return false;
    }
#else
    static bool inFlash(const char* ptr) {
        // common comparison would use >=0x40000000
        // instead, slightly reduce the footprint by
        // checking *only* for numbers below it
        static constexpr uintptr_t Mask { 1 << 30 };
        return (reinterpret_cast<uintptr_t>(ptr) & Mask) > 0;
    }
#endif

    const char* _ptr;
    size_t _len;
};

inline bool operator==(StringView lhs, StringView rhs) {
    return lhs.equals(rhs);
}

inline bool operator!=(StringView lhs, StringView rhs) {
    return !lhs.equals(rhs);
}

inline String operator+(String&& lhs, StringView rhs) {
    lhs.concat(rhs.c_str(), rhs.length());
    return lhs;
}

inline String operator+=(String& lhs, StringView rhs) {
    lhs.concat(rhs.c_str(), rhs.length());
    return lhs;
}

#define STRING_VIEW(X) ({\
        alignas(4) static constexpr char __pstr__[] PROGMEM = (X);\
        ::espurna::StringView{__pstr__};\
    })

#define STRING_VIEW_INLINE(NAME, X)\
        alignas(4) static constexpr char __pstr__ ## NAME ##  __ [] PROGMEM = (X);\
        constexpr auto NAME = ::espurna::StringView(__pstr__ ## NAME ## __)

} // namespace espurna
