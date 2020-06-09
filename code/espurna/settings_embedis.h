#pragma once

#include <Arduino.h>

#include <algorithm>
#include <vector>
#include <memory>

#include "libs/TypeChecks.h"

namespace settings {
namespace embedis {

enum class Result {
    Success,
    None
};

struct ValueResult {
    operator bool() {
        return result;
    }
    bool result;
    String value;
};

class RawStorage {

    public:

    struct SourceBase {
        virtual void write(size_t index, uint8_t value) = 0;
        virtual uint8_t read(size_t index) = 0;
        virtual size_t size() = 0;
    };

    enum class State {
        Begin,
        End,
        LenByte1,
        LenByte2,
        EmptyValue,
        Value
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

    // remove key from the storage. ensure 'key' isn't empty
    bool del(const String& key);

    // Simply count key-value pairs that we could parse
    size_t keys();

    protected:

    // Pointer to the region of data that we are using
    // Note: Implementation is here b/c c++ won't allow us
    //       to have a plain member (not a ptr or ref) of unknown size
    struct Cursor {
        Cursor(SourceBase& source, uint16_t start, uint16_t end) :
            source(source),
            position(start),
            end(end)
        {}

        explicit Cursor(SourceBase& source) :
            Cursor(source, source.size(), source.size())
        {}

        void rewind() {
            position = end;
        }

        uint8_t read() {
            return source.read(position);
        }

        size_t length() {
            return (end - position);
        }

        void reset(uint16_t start, uint16_t end) {
            this->position = start;
            this->end = end;
        }

        Cursor& operator =(const Cursor& other) {
            if (&source == &other.source) {
                position = other.position;
                end = other.end;
            }
            return *this;
        }

        uint8_t operator[](size_t pos) const {
            return source.read(pos);
        }

        Cursor& operator++() {
            ++position;
            return *this;
        }

        Cursor& operator--() {
            --position;
            return *this;
        }

        operator bool() {
            return end != position;
        }

        SourceBase& source;
        uint16_t position;
        uint16_t end;
    };

    // Store value location in a more reasonable forward-iterator-style manner
    // Allows us to skip string creation when just searching for specific values
    // XXX: be cautious that cursor **will** break when underlying storage changes
    struct ReadResult;

    // we goind to end up using this pattern all the time here.
    // - read possible key, bail when not found or empty
    // - read possible value, bail when not found
    // - when both conditions pass, read the value and store in the output buffer
    struct KeyValueResult;

    // Return key-value pair cursors as a single object.
    // This way we can validate that both exist in a single call.
    KeyValueResult _read_kv();
        
    // Place cursor at the `end` and reset parser to expect length byte
    // Returns position at `end - 1`
    uint16_t _cursor_rewind();

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

    // explain the internal state value
    String _state_describe();

    SourceBase& _source;
    Cursor _cursor;
    State _state { State::Begin };

};

} // namespace embedis
} // namespace settings
