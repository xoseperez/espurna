/*

Part of the SETTINGS MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <algorithm>
#include <vector>
#include <memory>

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

class RawStorage {

    public:

    // IO source, functional redirects for raw byte storage.
    // TODO: provide actual benchmark comparison with 'lambda'-list-as-vtable (old Embedis dict style)
    // TODO: consider overrides for bulk operations like move (see ::del method)
    // TODO: implementation without virtual calls would be nice...
    //       we need to move things into a header (all the things, if we want to keep nested classes readable)
    struct SourceBase {
        virtual void write(size_t index, uint8_t value) = 0;
        virtual uint8_t read(size_t index) = 0;
        virtual size_t size() = 0;
    };

    RawStorage(SourceBase& source) :
        _source(source),
        _cursor(source)
    {}

    // Try to find the matching key. Datastructure that we use does not specify
    // any value 'type' inside of it. We expect 'key' to be the first non-empty string,
    // 'value' can be empty.
    ValueResult get(const String& key);

    // set or update key with value contents. ensure 'key' isn't empty, 'value' can be empty
    bool set(const String& key, const String& value);

    // remove key from the storage. will check that 'key' argument isn't empty
    bool del(const String& key);

    // Simply count key-value pairs that we could parse
    size_t keys();

    // Internal storage info, to allow us to know the kv size requirements
    size_t estimate(const String& key, const String& value);
    size_t available();

    protected:

    // Pointer to the region of data that we are using
    //
    // XXX:  It does not matter right now, but we **will** overflow position when using sizes >= (2^16) - 1
    // Note: Implementation is in the header b/c c++ won't allow us
    //       to have a plain member (not a ptr or ref) of unknown size.
    // Note: There was a considiration to implement this as 'stashing iterator' to be compatible with stl algorithms.
    //       In such implementation, we would store intermediate index and allow the user to receive a `value_proxy`,
    //       temporary returned by `value_proxy& operator*()' that is bound to Cursor instance.
    //       This **will** cause problems with 'reverse_iterator' or anything like it, as it expects reference to
    //       outlive the iterator object (specifically, result of `return *--tmp`, where `tmp` is created inside of a function block)

    struct Cursor {

        Cursor(SourceBase& source, uint16_t position_, uint16_t begin_, uint16_t end_) :
            position(position_),
            begin(begin_),
            end(end_),
            _source(source)
        {}

        Cursor(SourceBase& source, uint16_t begin_, uint16_t end_) :
            Cursor(source, 0, begin_, end_)
        {}

        explicit Cursor(SourceBase& source) :
            Cursor(source, 0, 0, source.size())
        {}

        static Cursor fromEnd(SourceBase& source, uint16_t begin, uint16_t end) {
            return Cursor(source, end - begin, begin, end);
        }

        Cursor() = delete;

        void reset(uint16_t begin_, uint16_t end_) {
            position = 0;
            begin = begin_;
            end = end_;
        }

        uint8_t read() {
            return _source.read(begin + position);
        }

        void write(uint8_t value) {
            _source.write(begin + position, value);
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
            return _source.read(begin + position_);
        }

        bool operator ==(const Cursor& other) {
            return ((begin == other.begin)
                   && (end == other.end)
                   && (position == other.position));
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

        SourceBase& _source;

    };

    // Store value location in a more reasonable forward-iterator-style manner
    // Allows us to skip string creation when just searching for specific values
    // XXX: be cautious that cursor positions **will** break when underlying storage changes
    struct ReadResult;

    // Returns Cursor to the region that holds the data
    // Result object does not hold any data, we need to explicitly request read()
    //
    // Cursor object is always expected to point to something, e.g. minimum:
    //     0x01 0x00 0x01
    //     data len2 len1
    // Position will be 0, end will be 4. Total length is 3, data length is 1
    //
    // Note the distinction between real length and data length. For example,
    // special-case for empty 'value' (as 'key' can never be empty and will be rejected):
    //     0x00 0x00 0x00 0x00
    //     data data len2 len1
    // Position will be 0, end will be 5. Total length is 4, data length is 0
    ReadResult _raw_read();

    // Internal storage consists of sequences of <byte-range><length>
    // We going be using this pattern all the time here, because we need 2 consecutive **valid** ranges
    struct KeyValueResult;

    // Return key-value pair cursors as a single object.
    // This way we can validate that both exist in a single call.
    KeyValueResult _read_kv();

    // Place cursor at the `end` and resets the parser to expect length byte
    uint16_t _cursor_reset_end();

    // explain the internal state value
    String _state_describe();

    enum class State {
        Begin,
        End,
        LenByte1,
        LenByte2,
        EmptyValue,
        Value
    };

    SourceBase& _source;
    Cursor _cursor;
    State _state { State::Begin };

};

} // namespace embedis
} // namespace settings
