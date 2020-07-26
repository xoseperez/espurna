/*

Part of the SETTINGS MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

Reimplementation of the Embedis storage format:
- https://github.com/thingSoC/embedis

*/

#pragma once

#include <Arduino.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "libs/TypeChecks.h"

namespace settings {
namespace embedis {

// 'optional' type for byte range
struct ValueResult {
    operator bool() {
        return result;
    }
    bool result { false };
    String value;
};

// Sum total is calculated from:
// - 4 bytes to store length of 2 values (stored as big-endian)
// - N bytes of values themselves
// We can't save empty keys but can save empty values as just 2 length bytes
inline size_t estimate(const String& key, const String& value) {
    if (!key.length()) {
        return 0;
    }

    return (4 + key.length() + value.length());
}

// Note:  KeyValueStore is templated to avoid having to provide RawStorageBase via virtual inheritance.

template <typename RawStorageBase>
class KeyValueStore {

    // -----------------------------------------------------------------------------------
    // Notice: we can only use sfinae checks with the current compiler version

    // TODO: provide actual benchmark comparison with 'lambda'-list-as-vtable (old Embedis style)
    //       and vtable approach (write(), read() and commit() as pure virtual)
    // TODO: consider overrides for bulk operations like move (see ::del method)

    template <typename T>
    using storage_can_write_t = decltype(std::declval<T>().write(
        std::declval<uint16_t>(), std::declval<uint8_t>()));
    template <typename T>
    using storage_can_write = is_detected<storage_can_write_t, T>;

    template <typename T>
    using storage_can_read_t = decltype(std::declval<T>().read(std::declval<uint16_t>()));
    template <typename T>
    using storage_can_read = is_detected<storage_can_read_t, T>;

    template <typename T>
    using storage_can_commit_t = decltype(std::declval<T>().commit());
    template <typename T>
    using storage_can_commit = is_detected<storage_can_commit_t, T>;

    static_assert(
        (storage_can_commit<RawStorageBase>{} &&
        storage_can_read<RawStorageBase>{} &&
        storage_can_write<RawStorageBase>{}),
        "Storage class must implement read(index), write(index, byte) and commit()"
    );

    // -----------------------------------------------------------------------------------

    protected:

    // Tracking state of the parser inside of _raw_read()
    enum class State {
        Begin,
        End,
        LenByte1,
        LenByte2,
        Value
    };

    // Pointer to the region of data that we are using
    //
    // XXX:  It does not matter right now, but we **will** overflow position when using sizes >= (2^16) - 1
    // Note: Implementation is also in the header b/c c++ won't allow us
    //       to have a plain member (not a ptr or ref) of unknown size.
    // Note: There was a considiration to implement this as 'stashing iterator' to be compatible with stl algorithms.
    //       In such implementation, we would store intermediate index and allow the user to receive a `value_proxy`,
    //       temporary returned by `value_proxy& operator*()' that is bound to Cursor instance.
    //       This **will** cause problems with 'reverse_iterator' or anything like it, as it expects reference to
    //       outlive the iterator object (specifically, result of `return *--tmp`, where `tmp` is created inside of a function block)
    struct Cursor {

        Cursor(RawStorageBase& storage, uint16_t position_, uint16_t begin_, uint16_t end_) :
            position(position_),
            begin(begin_),
            end(end_),
            _storage(storage)
        {}

        Cursor(RawStorageBase& storage, uint16_t begin_, uint16_t end_) :
            Cursor(storage, 0, begin_, end_)
        {}

        explicit Cursor(RawStorageBase& storage) :
            Cursor(storage, 0, 0, 0)
        {}

        static Cursor merge(RawStorageBase& storage, const Cursor& key, const Cursor& value) {
            return Cursor(storage, key.begin, value.end);
        }

        static Cursor fromEnd(RawStorageBase& storage, uint16_t begin, uint16_t end) {
            return Cursor(storage, end - begin, begin, end);
        }

        Cursor() = delete;

        void reset(uint16_t begin_, uint16_t end_) {
            position = 0;
            begin = begin_;
            end = end_;
        }

        uint8_t read() {
            return _storage.read(begin + position);
        }

        void write(uint8_t value) {
            _storage.write(begin + position, value);
        }

        void resetBeginning() {
            position = 0;
        }

        void resetEnd() {
            position = end - begin;
        }

        size_t size() {
            return (end - begin);
        }

        bool inRange(uint16_t position_) {
            return (position_ < (end - begin));
        }

        operator bool() {
            return inRange(position);
        }

        uint8_t operator[](size_t position_) const {
            return _storage.read(begin + position_);
        }

        bool operator ==(const Cursor& other) {
            return (begin == other.begin) && (end == other.end);
        }

        bool operator !=(const Cursor& other) {
            return !(*this == other);
        }

        Cursor& operator++() {
            ++position;
            return *this;
        }

        Cursor operator++(int) {
            Cursor other(*this);
            ++*this;
            return other;
        }

        Cursor& operator--() {
            --position;
            return *this;
        }

        Cursor operator--(int) {
            Cursor other(*this);
            --*this;
            return other;
        }

        uint16_t position;
        uint16_t begin;
        uint16_t end;

        private:

        RawStorageBase& _storage;

    };

    public:

    // Store value location in a more reasonable forward-iterator-style manner
    // Allows us to skip string creation when just searching for specific values
    // XXX: be cautious that cursor positions **will** break when underlying storage changes
    struct ReadResult {

        friend class KeyValueStore<RawStorageBase>;

        ReadResult(const Cursor& cursor_) :
            length(0),
            cursor(cursor_),
            result(false)
        {}

        ReadResult(RawStorageBase& storage) :
            length(0),
            cursor(storage),
            result(false)
        {}

        operator bool() {
            return result;
        }

        String read() {
            String out;
            out.reserve(length);
            if (!length) {
                return out;
            }

            decltype(length) index = 0;
            cursor.resetBeginning();
            while (index < length) {
                out += static_cast<char>(cursor.read());
                ++cursor;
                ++index;
            }

            return out;
        }

        uint16_t length;

        private:

        Cursor cursor;
        bool result;

    };

    // Internal storage consists of sequences of <byte-range><length>
    struct KeyValueResult {
        operator bool() {
            return (key) && (value) && (key.length > 0);
        }

        bool operator !() {
            return !(static_cast<bool>(*this));
        }

        template <typename T = ReadResult>
        KeyValueResult(T&& key_, T&& value_) :
            key(std::forward<T>(key_)),
            value(std::forward<T>(value_))
        {}

        KeyValueResult(RawStorageBase& storage) :
            key(storage),
            value(storage)
        {}

        ReadResult key;
        ReadResult value;
    };

    // one and only possible constructor, simply move the class object into the
    // member variable to avoid forcing the user of the API to keep 2 objects alive.
    KeyValueStore(RawStorageBase&& storage, uint16_t begin, uint16_t end) :
        _storage(std::move(storage)),
        _cursor(_storage, begin, end),
        _state(State::Begin)
    {}

    // Try to find the matching key. Datastructure that we use does not specify
    // any value 'type' inside of it. We expect 'key' to be the first non-empty string,
    // 'value' can be empty.
    ValueResult get(const String& key) {
        return _get(key, true);
    }

    bool has(const String& key) {
        return static_cast<bool>(_get(key, false));
    }

    // We going be using this pattern all the time here, because we need 2 consecutive **valid** ranges
    // TODO: expose _read_kv() and _cursor_reset_end() so we can have 'break' here?
    //       perhaps as a wrapper object, allow something like next() and seekBegin()
    template <typename CallbackType>
    void foreach(CallbackType callback) {
        _cursor_reset_end();
        do {
            auto kv = _read_kv();
            if (!kv) {
                break;
            }
            callback(std::move(kv));
        } while (_state != State::End);
    }

    // read every key into a vector
    std::vector<String> keys() {
        std::vector<String> out;
        out.reserve(count());

        foreach([&](KeyValueResult&& kv) {
            out.push_back(kv.key.read());
        });

        return out;
    }

    // set or update key with value contents. ensure 'key' isn't empty, 'value' can be empty
    bool set(const String& key, const String& value) {

        // ref. 'estimate()' implementation in regards to the storage calculation
        auto need = estimate(key, value);
        if (!need) {
            return false;
        }

        auto key_len = key.length();
        auto value_len = value.length();

        Cursor to_erase(_storage);
        bool need_erase = false;

        // we need the position at the 'end' of the free space
        auto start_pos = _cursor_reset_end();

        do {
            auto kv = _read_kv();
            if (!kv) {
                break;
            }

            start_pos = kv.value.cursor.begin;

            // in the very special case we can match the existing key, we either
            if ((kv.key.length == key_len) && (kv.key.read() == key)) {
                if (kv.value.length == value.length()) {
                    // - do nothing, as the value is already set
                    if (kv.value.read() == value) {
                        return true;
                    }
                    // - overwrite the space again, with the new kv of the same length
                    start_pos = kv.key.cursor.end;
                    break;
                }
                // - or, erase the existing kv and place new kv at the end
                to_erase.reset(kv.value.cursor.begin, kv.key.cursor.end);
                need_erase = true;
            }

        } while (_state != State::End);

        if (need_erase) {
            _raw_erase(start_pos, to_erase);
            start_pos += to_erase.size();
        }

        // we should only insert when possition is still within possible size
        if (start_pos && (start_pos >= need)) {
            auto writer = Cursor::fromEnd(_storage, start_pos - need, start_pos);

            // put the length of the value as 2 bytes and then write the data
            (--writer).write(key_len & 0xff);
            (--writer).write((key_len >> 8) & 0xff);
            while (key_len--) {
                (--writer).write(key[key_len]);
            }

            (--writer).write(value_len & 0xff);
            (--writer).write((value_len >> 8) & 0xff);

            while (value_len--) {
                (--writer).write(value[value_len]);
            }

            // we also need to add an empty key *after* the value
            // but, only when we still have some space left
            if (writer.begin >= 2) {
                _cursor_set_position(writer.begin - _cursor.begin);
                auto next_kv = _read_kv();
                if (!next_kv) {
                    auto empty = Cursor::fromEnd(_storage, writer.begin - 2, writer.begin);
                    (--empty).write(0);
                    (--empty).write(0);
                }
            }

            _storage.commit();

            return true;
        }

        return false;
    }

    // remove key from the storage. will check that 'key' argument isn't empty
    bool del(const String& key) {
        size_t key_len = key.length();
        if (!key_len) {
            return false;
        }

        // we should only compare strings of equal length.
        // when matching, record { value ... key } range + 4 bytes for length
        // continue searching for available keys and set start_pos and the 'end' of the free space
        size_t start_pos = _cursor_reset_end();
        auto to_erase = Cursor::fromEnd(_storage, _cursor.begin, _cursor.end);

        foreach([&](KeyValueResult&& kv) {
            start_pos = kv.value.cursor.begin;
            if (!to_erase && (kv.key.length == key_len) && (kv.key.read() == key)) {
                to_erase.reset(kv.value.cursor.begin, kv.key.cursor.end);
            }
        });

        if (to_erase) {
            _raw_erase(start_pos, to_erase);
            return true;
        }

        return false;
    }

    // Simply count key-value pairs that we could parse
    size_t count() {
        size_t result = 0;
        foreach([&result](KeyValueResult&&) {
            ++result;
        });

        return result;
    }

    // Do exactly the same thing as 'keys' does, but return the amount
    // of bytes to the left of the last kv
    size_t available() {
        size_t result = _cursor.size();
        foreach([&result](KeyValueResult&& kv) {
            result -= kv.key.cursor.size();
            result -= kv.value.cursor.size();
        });

        return result;
    }

    // How much bytes can be used is directly read from the cursor, based on begin and end values
    size_t size() {
        return _cursor.size();
    }

    protected:

    // Try to find the matching key. Datastructure that we use does not specify
    // any value 'type' inside of it. We expect 'key' to be the first non-empty string,
    // 'value' can be empty.
    // To implement has(), allow to skip reading the value
    ValueResult _get(const String& key, bool read_value) {
        ValueResult out;
        auto len = key.length();

        _cursor_reset_end();

        do {
            auto kv = _read_kv();
            if (!kv) {
                break;
            }

            // no point in comparing keys when length does not match
            // (and we also don't want to allocate the string)
            if (kv.key.length != len) {
                continue;
            }

            auto key_result = kv.key.read();
            if (key_result == key) {
                if (read_value) {
                    out.value = kv.value.read();
                }
                out.result = true;
                break;
            }
        } while (_state != State::End);

        return out;
    }

    // Place cursor at the `end` and resets the parser to expect length byte
    uint16_t _cursor_reset_end() {
        _cursor.resetEnd();
        _state = State::Begin;
        return _cursor.end;
    }

    uint16_t _cursor_set_position(uint16_t position) {
        _state = State::Begin;
        _cursor.position = position;
        return _cursor.position;
    }

    // implementation quirk is that `Cursor::operator=` won't work because of the `RawStorageBase&` member
    // right now, just construct in place and assume that compiler will inline things
    // on one hand, we can implement it. but, we can't specifically 
    KeyValueResult _read_kv() {
        auto key = _raw_read();
        if (!key || !key.length) {
            return {_storage};
        }

        auto value = _raw_read();

        return {key, value};
    };

    void _raw_erase(size_t start_pos, Cursor& to_erase) {
        // we either end up to the left or to the right of the boundary

        size_t new_pos = (start_pos < to_erase.begin)
            ? (start_pos + to_erase.size())
            : (to_erase.end);

        if (start_pos < to_erase.begin) {
            // shift storage to the right, overwriting over the now empty space
            auto from = Cursor::fromEnd(_storage, start_pos, to_erase.begin);
            auto to = Cursor::fromEnd(_storage, start_pos + to_erase.size(), to_erase.end);

            while (--from && --to) {
                to.write(from.read());
                from.write(0xff);
            }
        } else {
            // overwrite the now empty space with 0xff
            to_erase.resetEnd();
            while (--to_erase) {
                to_erase.write(0xff);
            }
        }

        // same as set(), add empty key as padding
        auto empty = Cursor::fromEnd(_storage, new_pos - 2, new_pos);
        (--empty).write(0);
        (--empty).write(0);

        _storage.commit();
    }

    // Returns Cursor to the region that holds the data
    // Result object itself does not contain any data, we need to explicitly request it by calling read()
    //
    // Cursor object is always expected to point to something, e.g. minimum:
    //     0x00 0x00
    //     len2 len1
    // Position will be 0, end will be 2. Total length is 2, data length is 0
    //
    // Or, non-empty value:
    //     0x01 0x00 0x01
    //     data len2 len1
    // Position will be 0, end will be 3. Total length is 3, data length is 1
    ReadResult _raw_read() {
        uint16_t len = 0;
        ReadResult out(_storage);

        do {
            // storage is written right-to-left, cursor is always decreasing
            switch (_state) {
            case State::Begin:
                if (_cursor.position >= 2) {
                    --_cursor;
                    _state = State::LenByte1;
                } else {
                    _state = State::End;
                }
                break;
            // len is 16 bit uint (bigendian)
            // special case is 0, which is valid and should be returned when encountered
            // another special case is 0xffff, meaning we just hit an empty space
            case State::LenByte1:
                len = _cursor.read();
                _state = State::LenByte2;
                break;
            case State::LenByte2:
            {
                uint8_t len2 = (--_cursor).read();
                if ((0xff == len) && (0xff == len2)) {
                    _state = State::End;
                } else {
                    len |= len2 << 8;
                    _state = State::Value;
                }
                break;
            }
            case State::Value: {
                // ensure we don't go out-of-bounds
                if (len && _cursor.position < len) {
                    _state = State::End;
                    break;
                }

                // and point at the beginning of the value
                _cursor.position -= len;
                auto value_start = (_cursor.begin + _cursor.position);

                out.cursor.reset(value_start, value_start + len + 2);
                out.length = len;
                out.result = true;

                _state = State::Begin;

                goto return_result;
            }
            case State::End:
            default:
                break;
        }

        } while (_state != State::End);

    return_result:

        return out;
    }

    RawStorageBase _storage;
    Cursor _cursor;
    State _state { State::Begin };
};

} // namespace embedis
} // namespace settings
