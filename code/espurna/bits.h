/*

Part of BITS MODULE

*/

#pragma once

#include <cstdint>
#include <limits>
#include <bitset>

#include "types_orch.h"

namespace espurna {
namespace bits {

#if __cplusplus > 201411L
#define CONSTEXPR17 constexpr
#else
#define CONSTEXPR17
#endif

// fill u32 or 64 [begin, end] with ones, keep the rest as zeroes

template <typename T>
inline CONSTEXPR17 T fill_generic(uint8_t begin, uint8_t end) {
    T mask = std::numeric_limits<T>::max();
    mask >>= (sizeof(T) * 8) - T(end - begin);
    mask <<= T(begin);

    return mask;
}

template <typename T>
inline CONSTEXPR17 T fill_generic_inverse(uint8_t begin, uint8_t end) {
    T high = std::numeric_limits<T>::max();
    high <<= T(begin);

    T low = std::numeric_limits<T>::max();
    low >>= (sizeof(T) * 8) - T(end);

    return high | low;
}

inline CONSTEXPR17 uint32_t fill_u32(uint8_t begin, uint8_t end) {
    return fill_generic<uint32_t>(begin, end);
}

inline CONSTEXPR17 uint32_t fill_u32_inverse(uint8_t begin, uint8_t end) {
    return fill_generic_inverse<uint32_t>(begin, end);
}

inline CONSTEXPR17 uint64_t fill_u64(uint8_t begin, uint8_t end) {
    return fill_generic<uint64_t>(begin, end);
}

inline CONSTEXPR17 uint32_t fill_u64_inverse(uint8_t begin, uint8_t end) {
    return fill_generic_inverse<uint64_t>(begin, end);
}

#undef CONSTEXPR17

// helper class for [begin, end] bit range

struct Range {
    Range() = delete;

    Range(const Range&) = default;
    Range& operator=(const Range&) = default;

    Range(Range&&) noexcept = default;
    Range& operator=(Range&&) noexcept = default;

    Range(uint8_t begin, uint8_t end) :
        _begin(begin),
        _end(end)
    {}

    bool valid(uint8_t value) {
        return (value >= _begin) && (value <= _end);
    }

    void fill(uint8_t begin, uint8_t end, uint8_t repeat) {
        if (begin > end) {
            _fill_inverse(begin, end, repeat);
        } else {
            _fill(begin, end, repeat);
        }
    }

    void fill(uint8_t begin, uint8_t end) {
        _fill(begin, end, 1);
    }

    void set() {
        _mask.set();
    }

    void set(uint8_t index) {
        _mask[index] = true;
    }

    void reset() {
        _mask.reset();
    }

    void reset(uint8_t index) {
        _mask[index] = false;
    }

    int begin() const {
        return _begin;
    }

    int end() const {
        return _end;
    }

    static constexpr int min() {
        return 0;
    }

    static constexpr int max() {
        return SizeMax;
    }

    uint32_t to_u32() const {
        return _mask.to_ulong();
    }

    uint64_t to_u64() const {
        return _mask.to_ullong();
    }

    String toString() const;

private:
    void _fill_inverse(uint8_t begin, uint8_t end, uint8_t repeat);
    void _fill(uint8_t begin, uint8_t end, uint8_t repeat);

    static constexpr auto SizeMax = size_t{ 64 };
    using Mask = std::bitset<SizeMax>;

    uint8_t _begin;
    uint8_t _end;

    Mask _mask{};
};

bool fill_range(Range& range, StringView);

// Returns one plus the index of the least significant 1-bit of x, or if x is zero, returns zero.
inline constexpr int first_set_u32(uint32_t value) {
    return __builtin_ffs(value);
}

// Returns one plus the index of the least significant 1-bit of x, or if x is zero, returns zero.
inline constexpr int first_set_u64(uint64_t value) {
    return __builtin_ffsll(value);
}

// Returns one plus the index of the most significant 1-bit of x, or if x is zero, returns zero.
inline constexpr int last_set_u32(uint32_t value) {
    return value ? (32 - __builtin_clz(value)) : 0;
}

// Returns one plus the index of the most significant 1-bit of x, or if x is zero, returns zero.
inline constexpr int last_set_u64(uint64_t value) {
    return value ? (64 - __builtin_clzll(value)) : 0;
}

} // namespace bits
} // namespace espurna

#undef CONSTEXPR17

