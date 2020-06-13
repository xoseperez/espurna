/*

Part of the SETTINGS MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

Reimplementation of the Embedis storage format:
- https://github.com/thingSoC/embedis

*/

#include "settings_embedis.h"

namespace settings {
namespace embedis {

struct RawStorage::ReadResult {
    ReadResult(const Cursor& cursor_) :
        cursor(cursor_),
        result(false),
        length(0)
    {}

    ReadResult(SourceBase& source) :
        cursor(source),
        result(false),
        length(0)
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

    Cursor cursor;
    bool result;
    uint16_t length;

};

struct RawStorage::KeyValueResult {
    operator bool() {
        return (key) && (value) && (key.length > 0);
    }

    bool operator !() {
        return !(bool(*this));
    }

    template <typename T = ReadResult>
    KeyValueResult(T&& key_, T&& value_) :
        key(std::forward<T>(key_)),
        value(std::forward<T>(value_))
    {}

    KeyValueResult(SourceBase& source) :
        key(source),
        value(source)
    {}

    ReadResult key;
    ReadResult value;
};

ValueResult RawStorage::get(const String& key) {
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
            out.value = kv.value.read();
            out.result = true;
            break;
        }
    } while (_state != State::End);

    return out;
}

bool RawStorage::set(const String& key, const String& value) {
    bool result = false;

    // ref. 'estimate()' implementation in regards to the storage calculation
    auto need = estimate(key, value);
    if (!need) {
        return false;
    }

    auto start_pos = _cursor_reset_end();

    auto key_len = key.length();
    auto value_len = value.length();

    Cursor to_erase(_source);
    bool need_erase = false;

    do {
        auto kv = _read_kv();
        if (!kv) {
            break;
        }

        start_pos = kv.value.cursor.begin;

        // in the very special case we match the existing key, we either
        if ((kv.key.length == key_len) && (kv.key.read() == key)) {
            if (kv.value.length == value.length()) {
                if (kv.value.read() == value) {
                    return true;
                }
                start_pos = kv.key.cursor.end;
                break;
            }
            to_erase.reset(kv.value.cursor.begin, kv.key.cursor.end);
            need_erase = true;
            // but we need remove it from the storage when changing contents
        }
    } while (_state != State::End);

    if (need_erase) {
        _raw_erase(start_pos, to_erase);
        start_pos += to_erase.size();
    }

    // we should only insert when possition is still within possible size
    if (start_pos && (start_pos >= need)) {
        auto writer = Cursor::fromEnd(_source, start_pos - need, start_pos);

        // put the length of the value as 2 bytes and then write the data
        (--writer).write(key_len & 0xff);
        (--writer).write((key_len >> 8) & 0xff);
        while (key_len--) {
            (--writer).write(key[key_len]);
        }

        (--writer).write(value_len & 0xff);
        (--writer).write((value_len >> 8) & 0xff);

        if (value_len) {
            while (value_len--) {
                (--writer).write(value[value_len]);
            }
        } else {
            (--writer).write(0);
            (--writer).write(0);
        }

        // we also need to pad the space *after* the value
        // when we still have some space left
        if (writer.position > 1) {
            auto next_kv = _read_kv();
            if (!next_kv) {
                (--writer).write(0);
                (--writer).write(0);
            }
        }

        return true;
    }

    return false;
}

bool RawStorage::del(const String& key) {
    size_t key_len = key.length();
    if (!key_len) {
        return false;
    }

    // Removes key from the storage by overwriting the key with left-most data
    uint16_t offset = 0;

    size_t start_pos = _cursor_reset_end() - 1;
    auto to_erase = Cursor::fromEnd(_source, _source.size(), _source.size());

    do {
        auto kv = _read_kv();
        if (!kv) {
            break;
        }

        start_pos = kv.value.cursor.begin;

        // we should only compare strings of equal length.
        // when matching, record { value ... key } range + 4 bytes for length
        // continue searching for the leftmost boundary
        if (!offset && (kv.key.length == key_len) && (kv.key.read() == key)) {
            to_erase.reset(kv.value.cursor.begin, kv.key.cursor.end);
        }
    } while (_state != State::End);

    if (!to_erase) {
        return false;
    }

    _raw_erase(start_pos, to_erase);

    return true;
}

size_t RawStorage::keys() {
    size_t result = 0;

    _cursor_reset_end();
    do {
        auto kv = _read_kv();
        if (!kv) {
            break;
        }
         ++result;
    } while (_state != State::End);

    return result;
}

// We can't save empty keys but can save empty values as 0x00 0x00 0x00 0x00
// total sum is:
// - 2 bytes gap at the end (will be re-used by the next value length byte)
// - 4 bytes to store length of 2 values (stored as big-endian)
// - N bytes of values themselves
size_t RawStorage::estimate(const String& key, const String& value) {
    if (!key.length()) {
        return 0;
    }

    const auto key_len = key.length();
    const auto value_len = value.length();

    return (4 + key_len + ((value_len > 0) ? value_len : 2));
}

// Do exactly the same thing as 'keys' does, but return the amount
// of bytes to the left of the last kv
size_t RawStorage::available() {
    _cursor_reset_end();

    size_t result = _cursor.end;
    do {
        auto kv = _read_kv();
        if (!kv) {
            break;
        }
        result = kv.value.cursor.begin;
    } while (_state != State::End);

    return result;
}

// Place cursor at the `end` and resets the parser to expect length byte
uint16_t RawStorage::_cursor_reset_end() {
    _cursor.resetEnd();
    _state = State::Begin;
    return _cursor.end;
}


// implementation quirk is that `Cursor::operator=` won't work because of the `SourceBase&` member
// right now, just construct in place and assume that compiler will inline things
RawStorage::KeyValueResult RawStorage::_read_kv() {
    auto key = _raw_read();
    if (!key || !key.length) {
        return {_source};
    }

    auto value = _raw_read();

    return {key, value};
};

void RawStorage::_raw_erase(size_t start_pos, Cursor& to_erase) {
    // we either end up to the left or to the right of the boundary
    if (start_pos < to_erase.begin) {
        auto from = Cursor::fromEnd(_source, start_pos, to_erase.begin);
        auto to = Cursor::fromEnd(_source, start_pos + to_erase.size(), to_erase.end);

        while (--from && --to) {
            to.write(from.read());
            from.write(0xff);
        };
    } else {
        // just null the length bytes, since we at the last key
        to_erase.resetEnd();
        (--to_erase).write(0);
        (--to_erase).write(0);
    }
}

RawStorage::ReadResult RawStorage::_raw_read() {
    uint16_t len = 0;
    ReadResult out(_source);

    do {
        // storage is written right-to-left, cursor is always decreasing
        switch (_state) {
        case State::Begin:
            if (_cursor.position > 2) {
                --_cursor;
                _state = State::LenByte1;
            } else {
                _state = State::End;
            }
            continue;
        // len is 16 bit uint (bigendian)
        // special case is 0, which is valid and should be returned when encountered
        // another special case is 0xffff, meaning we just hit an empty space
        case State::LenByte1:
            len = _cursor.read();
            --_cursor;
            _state = State::LenByte2;
            break;
        case State::LenByte2:
        {
            uint8_t len2 = _cursor.read();
            --_cursor;
            if ((0 == len) && (0 == len2)) {
                len = 2;
                _state = State::EmptyValue;
            } else if ((0xff == len) && (0xff == len2)) {
                _state = State::End;
            } else {
                len |= len2 << 8;
                _state = State::Value;
            }
            break;
        }
        case State::EmptyValue:
        case State::Value: {
            uint16_t left = len;

            // ensure we don't go out-of-bounds
            switch (_state) {
            case State::Value:
                while (_cursor && --left) {
                    --_cursor;
                }
                break;
            // ...and only read 0's
            case State::EmptyValue:
                while (_cursor && (_cursor.read() == 0) && --left) {
                    --_cursor;
                }
                break;
            default:
                break;
            }

            if (left) {
                _state = State::End;
                break;
            }

            // set the resulting cursor as [pos:len+2)
            out.result = true;
            out.length = (_state == State::EmptyValue) ? 0 : len;
            out.cursor.reset(
                _cursor.position,
                _cursor.position + len + 2
            );

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

} // namespace embedis
} // namespace settings
