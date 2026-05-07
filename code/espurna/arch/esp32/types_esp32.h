/*

Part of the SYSTEM MODULE FOR ESP32

Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/
#pragma once

#include <Arduino.h>
#include <pgmspace.h>

#include <chrono>
#include <memory>
#include <vector>
#include <array>
#include <functional>
#include <type_traits>

#include "compat.h"
#if defined(ESP32)
#include "compat_esp32.h"
#endif

namespace espurna {

// -----------------------------------------------------------------------------
// SOURCE LOCATION
// -----------------------------------------------------------------------------

struct SourceLocation {
    int line;
    const char* file;
    const char* func;
};

inline bool operator==(SourceLocation lhs, SourceLocation rhs) {
    return lhs.line == rhs.line && lhs.file == rhs.file && lhs.func == rhs.func;
}

inline SourceLocation trim_source_location(SourceLocation value) {
    for (auto* ptr = value.file; *ptr != '\0'; ++ptr) {
        if ((*ptr == '/') || (*ptr == '\\')) { value.file = ptr + 1; }
    }
    return value;
}

inline constexpr SourceLocation make_source_location(
        int line = __builtin_LINE(),
        const char* file = __builtin_FILE(),
        const char* func = __builtin_FUNCTION())
{
    return SourceLocation{ .line = line, .file = file, .func = func };
}

// -----------------------------------------------------------------------------
// REENTRY LOCK
// -----------------------------------------------------------------------------

struct ReentryLock {
    ReentryLock() = delete;
    ReentryLock(const ReentryLock&) = delete;
    ReentryLock& operator=(const ReentryLock&) = delete;
    ReentryLock(ReentryLock&&) = default;
    ReentryLock& operator=(ReentryLock&&) = delete;
    explicit ReentryLock(bool& handle) : _initialized(!handle), _handle(handle) { lock(); }
    ~ReentryLock() { unlock(); }
    explicit operator bool() const { return initialized(); }
    bool initialized() const { return _initialized; }
    void lock() { if (initialized()) { _handle = true; } }
    void unlock() { if (initialized()) { _handle = false; } }
private:
    bool _initialized;
    bool& _handle;
};

// -----------------------------------------------------------------------------
// DURATION & TIME
// -----------------------------------------------------------------------------

namespace duration {

using Microseconds = std::chrono::duration<uint64_t, std::micro>;
using Milliseconds = std::chrono::duration<uint32_t, std::milli>;
using rep_type = uint32_t;

using Seconds = std::chrono::duration<rep_type, std::ratio<1> >;
using Minutes = std::chrono::duration<rep_type, std::ratio<60> >;
using Hours = std::chrono::duration<rep_type, std::ratio<Minutes::period::num * 60> >;
using Days = std::chrono::duration<rep_type, std::ratio<Hours::period::num * 24> >;
using Weeks = std::chrono::duration<rep_type, std::ratio<Days::period::num * 7> >;

using ClockCycles = std::chrono::duration<uint32_t, std::ratio<1, 240000000>>;

} // namespace duration

namespace time {

struct SystemClock {
    using rep = uint32_t;
    using period = std::milli;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<SystemClock>;
    static constexpr bool is_steady = true;

    static time_point now() noexcept {
        return time_point(duration(millis()));
    }
};

struct CoreClock {
    using rep = uint32_t;
    using period = std::milli;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<CoreClock>;
    static constexpr bool is_steady = true;

    static time_point now() noexcept {
        return time_point(duration(millis()));
    }
};

using CpuClock = CoreClock;

bool blockingDelay(CoreClock::duration timeout, CoreClock::duration interval, std::function<bool()> callback);
bool blockingDelay(CoreClock::duration timeout, CoreClock::duration interval);
bool blockingDelay(CoreClock::duration timeout);

inline void delay(CoreClock::duration duration) {
    ::delay(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

inline CoreClock::time_point millis() {
    return CoreClock::now();
}

} // namespace time

namespace sleep {
    using Microseconds = duration::Microseconds;
}

// -----------------------------------------------------------------------------
// POLLED FLAG
// -----------------------------------------------------------------------------

template <typename TimeSource>
struct PolledFlag {
    using time_point = typename TimeSource::time_point;
    using duration = typename TimeSource::duration;

    void reset() {
        _last = TimeSource::now();
    }

    bool wait(duration timeout) {
        auto now = TimeSource::now();
        if (now - _last >= timeout) {
            _last = now;
            return true;
        }
        return false;
    }

private:
    time_point _last {};
};

// -----------------------------------------------------------------------------
// CALLBACKS
// -----------------------------------------------------------------------------

struct Callback {
    using Type = void (*)();
    using WrapperType = std::function<void()>;

    Callback() = default;
    Callback(const Callback& other) : _storage(nullptr), _type(other._type) { copy(other); }
    Callback& operator=(const Callback& other) { reset(); copy(other); return *this; }
    Callback(const Callback&&) = delete;
    Callback(Callback&& other) noexcept : _storage(nullptr), _type(other._type) { move(other); }
    Callback& operator=(Callback&& other) noexcept;

    template <typename T> using is_callback = std::is_same<typename std::remove_cv<typename std::remove_reference<T>::type>::type, Callback>;
    template <typename T> using is_type = std::is_same<T, Type>;
    template <typename T> using type_convertible = std::is_convertible<T, Type>;
    template <typename T> using wrapper_convertible = std::is_convertible<T, WrapperType>;

    template <typename T,
              typename = typename std::enable_if<
                  is_type<T>::value
               || type_convertible<T>::value>::type>
    constexpr Callback(T callback) noexcept :
        _storage(Type(callback)),
        _type(StorageType::Simple)
    {}

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

    ~Callback() { reset(); }
    bool isEmpty() const { return (_type == StorageType::Empty); }
    bool isSimple() const { return (_type == StorageType::Simple); }
    bool isWrapped() const { return (_type == StorageType::Wrapper) || (_type == StorageType::Simple && _storage.simple != nullptr); }
    bool operator==(Type callback) const { return isSimple() && (_storage.simple == callback); }

    void reset();
    void swap(Callback&) noexcept;
    void operator()() const;

private:
    union Storage {
        WrapperType wrapper;
        Type simple;
        ~Storage() {}
        explicit Storage(WrapperType callback) : wrapper(std::move(callback)) {}
        constexpr explicit Storage(Type callback) : simple(callback) {}
        constexpr explicit Storage(std::nullptr_t) : simple(nullptr) {}
    };

    enum class StorageType { Empty, Simple, Wrapper };
    void copy(const Callback&);
    void move(Callback&) noexcept;
    Storage _storage { nullptr };
    StorageType _type { StorageType::Empty };
};

// -----------------------------------------------------------------------------
// STRING VIEW
// -----------------------------------------------------------------------------

struct StringView {
    constexpr StringView() noexcept : _ptr(nullptr), _len(0) {}
    ~StringView() = default;
    StringView(std::nullptr_t) = delete;
    constexpr StringView(const StringView&) noexcept = default;
    constexpr StringView(StringView&&) noexcept = default;

    StringView& operator=(const StringView&) noexcept = default;
    StringView& operator=(StringView&&) noexcept = default;

    constexpr StringView(const char* ptr, size_t len) noexcept : _ptr(ptr), _len(len) {}

    template <typename T, typename = typename std::enable_if<std::is_pointer<T>::value>::type>
    constexpr StringView(T ptr) noexcept : StringView(ptr, __builtin_strlen(ptr)) {}

    template <size_t Size>
    constexpr StringView(const char (&string)[Size]) noexcept : StringView(&string[0], Size - 1) {}

    constexpr StringView(const char* begin, const char* end) noexcept : StringView(begin, end - begin) {}

    explicit StringView(const __FlashStringHelper* ptr) noexcept :
        _ptr(reinterpret_cast<const char*>(ptr)),
        _len(strlen(reinterpret_cast<const char*>(ptr)))
    {}

    StringView(const String& string) noexcept :
        _ptr(string.c_str()), _len(string.length())
    {}

    StringView& operator=(const String& string) noexcept {
        _ptr = string.c_str();
        _len = string.length();
        return *this;
    }

    template <size_t Size>
    StringView& operator=(const char (&string)[Size]) noexcept {
        _ptr = &string[0];
        _len = Size - 1;
        return *this;
    }

    constexpr const char* begin() const noexcept { return _ptr; }
    constexpr const char* end() const noexcept { return _ptr + _len; }
    constexpr const char* c_str() const { return _ptr; }
    constexpr const char* data() const { return _ptr; }
    constexpr const char& operator[](size_t offset) const { return *(_ptr + offset); }
    constexpr size_t length() const { return _len; }

    String toString() const {
        String out;
        if (_len) {
            out.reserve(_len);
            for (size_t n = 0; n < _len; ++n) { out += _ptr[n]; }
        }
        return out;
    }

    operator String() const { return toString(); }

    bool equals(StringView) const;
    bool equalsIgnoreCase(StringView) const;
    bool startsWith(StringView) const;
    bool endsWith(StringView) const;
    StringView slice(size_t index, size_t end) const;
    StringView slice(size_t index) const;

    static bool inFlash(const char* ptr) {
        static constexpr uintptr_t Mask { 1 << 30 };
        return (reinterpret_cast<uintptr_t>(ptr) & Mask) > 0;
    }

private:
    const char* _ptr;
    size_t _len;
};

// Internal-use non-member inFlash for types.cpp
inline bool inFlash(const char* ptr) {
    return StringView::inFlash(ptr);
}

// -----------------------------------------------------------------------------
// OPERATORS
// -----------------------------------------------------------------------------

template <typename T, typename U>
inline typename std::enable_if<
    std::is_same<typename std::decay<T>::type, StringView>::value ||
    std::is_same<typename std::decay<U>::type, StringView>::value,
    bool>::type
operator==(const T& lhs, const U& rhs) {
    return StringView(lhs).equals(StringView(rhs));
}

template <typename T, typename U>
inline typename std::enable_if<
    std::is_same<typename std::decay<T>::type, StringView>::value ||
    std::is_same<typename std::decay<U>::type, StringView>::value,
    bool>::type
operator!=(const T& lhs, const U& rhs) {
    return !StringView(lhs).equals(StringView(rhs));
}

template <typename T, typename U>
inline typename std::enable_if<
    (std::is_same<typename std::decay<T>::type, StringView>::value && !std::is_same<typename std::decay<U>::type, StringView>::value) ||
    (!std::is_same<typename std::decay<T>::type, StringView>::value && std::is_same<typename std::decay<U>::type, StringView>::value) ||
    (std::is_same<typename std::decay<T>::type, StringView>::value && std::is_same<typename std::decay<U>::type, StringView>::value),
    String>::type
operator+(const T& lhs, const U& rhs) {
    StringView sv_lhs(lhs);
    StringView sv_rhs(rhs);
    String out;
    out.reserve(sv_lhs.length() + sv_rhs.length());
    out.concat(sv_lhs.data(), sv_lhs.length());
    out.concat(sv_rhs.data(), sv_rhs.length());
    return out;
}

} // namespace espurna

using espurna::StringView;
using espurna::SourceLocation;
using espurna::make_source_location;
using espurna::ReentryLock;
using espurna::PolledFlag;

// Global settings helpers
String getSetting(StringView key);
bool delSetting(StringView key);
bool hasSetting(StringView key);

namespace espurna {

#define PROGMEM_STRING_ATTR
#define PROGMEM_STRING(NAME, X) static constexpr char NAME[] = (X)
#define STRING_VIEW(X) ::espurna::StringView(X)
#define STRING_VIEW_INLINE(NAME, X) \
    static constexpr char __pstr__ ## NAME ##  __ [] = (X); \
    static constexpr auto NAME = ::espurna::StringView(__pstr__ ## NAME ## __)

#define STRING_VIEW_SETTING(X) ((__builtin_strlen(X) > 0) ? STRING_VIEW(X) : StringView())

template <typename T>
struct Span {
    constexpr explicit Span(std::nullptr_t) : _data(nullptr), _size(0) {}
    constexpr Span() noexcept : Span(nullptr) {}
    constexpr Span(T* data, size_t size) : _data(data), _size(size) {}
    constexpr Span(T* begin, T* end) : _data(begin), _size(end - begin) {}
    constexpr T* data() const { return _data; }
    constexpr bool is_null() const { return _data == nullptr; }
    constexpr bool empty() const { return is_null() || _size == 0; }
    constexpr size_t size() const { return _size; }
    constexpr T* begin() const { return _data; }
    constexpr T* end() const { return _data + _size; }
    constexpr T& operator[](size_t index) const { return _data[index]; }
    constexpr T& front() const { return _data[0]; }
    constexpr T& back() const { return _data[_size - 1]; }
    constexpr Span<T> slice(size_t index, size_t size) const {
        return Span<T>(_data + (index < _size ? index : _size), (index + size < _size ? size : _size - (index < _size ? index : _size)));
    }
    constexpr Span<T> slice(size_t index) const { return slice(index, _size - (index < _size ? index : _size)); }
    Span<T>& advance(size_t index) { return *this = slice(index); }
private:
    T* _data;
    size_t _size;
};

template <typename T, size_t Size> constexpr inline Span<T> make_span(T (&data)[Size]) { return Span<T>(&data[0], Size); }
template <size_t Size> inline Span<uint8_t> make_span(std::array<uint8_t, Size>& data) { return Span<uint8_t>(data.data(), data.size()); }
template <size_t Size> inline Span<const uint8_t> make_span(const std::array<uint8_t, Size>& data) { return Span<const uint8_t>(data.data(), data.size()); }
template <typename T> inline Span<T> make_span(std::vector<T>& data) { return Span<T>(data.data(), data.size()); }
template <typename T> inline Span<const T> make_span(const std::vector<T>& data) { return Span<const T>(data.data(), data.size()); }

namespace duration {
struct Pair { Seconds seconds{}; Microseconds microseconds{}; };
constexpr bool operator==(const Pair& lhs, const Pair& rhs) { return lhs.seconds == rhs.seconds && lhs.microseconds == rhs.microseconds; }
template <typename T, typename Rep = typename T::rep, typename Period = typename T::period>
std::chrono::duration<Rep, Period> to_chrono(Pair result) {
    using Type = std::chrono::duration<Rep, Period>;
    return std::chrono::duration_cast<Type>(result.seconds) + std::chrono::duration_cast<Type>(result.microseconds);
}
struct PairResult { Pair value; bool ok { false }; };
PairResult parse(StringView, int num, int den);
template <intmax_t Num, intmax_t Den> PairResult parse(StringView view, std::ratio<Num, Den>) { return parse(view, Num, Den); }
template <typename T> T unchecked_parse(StringView view) {
    const auto result = parse(view, typename T::period{});
    if (result.ok) { return to_chrono<T>(result.value); }
    return T{}.min();
}
} // namespace duration
} // namespace espurna
