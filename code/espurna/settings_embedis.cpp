/*

Part of the SETTINGS MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "settings_embedis.h"

namespace settings {
namespace embedis {

#define SETTINGS_EMBEDIS_TRACING 1

#if SETTINGS_EMBEDIS_TRACING
#define trace(...) do { printf("%s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__); } while(false)
#else
#define trace(...)
#endif

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

        uint16_t index = 0;
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

    // we can't save empty keys
    size_t key_len = key.length();
    if (!key_len) {
        return false;
    }

    size_t value_len = value.length();

    size_t available = _cursor_reset_end();
    trace("::set(%s,%s) starting available = %u @%u:%u\n", key.c_str(), value.c_str(), available, _cursor.begin, _cursor.end);

    do {
        auto kv = _read_kv();
        if (!kv) {
            break;
        }

        available = kv.value.cursor.begin;
        trace("::set available after kv %u, found key=%u:%u value=%u:%u\n",
            available,
            kv.key.cursor.begin,
            kv.key.cursor.end,
            kv.value.cursor.begin,
            kv.value.cursor.end
        );

        // trying to compare the existing keys with the ones we've got
        if ((kv.key.length == key_len) && (kv.key.read() == key)) {
            if (kv.value.length == value.length()) {
                if (kv.value.read() == value) {
                    return true;
                }
                available = kv.key.cursor.end;
                break;
            }
            // but we need remove it from the storage when changing contents
            del(key);
        }
    } while (_state != State::End);

    // ...but can save empty values as 0x00 0x00 0x00 0x00
    // total sum is:
    // - 2 bytes gap at the end (will be re-used by the next value length byte)
    // - 4 bytes to store length of 2 values
    // - byte-length of values themselves
    size_t need = 4 + key_len + ((value_len > 0) ? value_len : 2);

    trace("::set need=%u available=%u\n", need, available);

    // we should only insert when possition is still within possible size
    if (available && (available >= need)) {
        size_t pos = available - 1;

        // put the length of the value as 2 bytes and then write the data
        _source.write(pos--, (key_len & 0xff));
        _source.write(pos--, ((key_len >> 8) & 0xff));
        while (key_len--) {
            _source.write(pos--, key[key_len]);
        }

        _source.write(pos--, (value_len & 0xff));
        _source.write(pos--, ((value_len >> 8) & 0xff));

        if (value_len) {
            while (value_len--) {
                _source.write(pos--, value[value_len]);
            }
        } else {
            _source.write(pos--, 0);
            _source.write(pos--, 0);
        }

        // we also need to pad the space *after* the value
        // when we still have some space left
        if (pos >= 1) {
            auto next_kv = _read_kv();
            if (!next_kv) {
                _source.write(pos--, 0);
                _source.write(pos--, 0);
            }
        }

        return true;
    }

    trace("::set can't\n");

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
    size_t offset_pos = start_pos;

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
            offset = kv.key.cursor.size() + kv.value.cursor.size();
            offset_pos = kv.value.cursor.begin;
            trace("need to erase kv @%u:%u\n", offset_pos, offset_pos + offset);
        }
    } while (_state != State::End);

    if (!offset) {
        return false;
    }

    // we either end up to the left or to the right of the boundary
    if (start_pos < offset_pos) {
        trace("::del moving  @%u:%u to @%u:%u\n",
            start_pos,
            offset_pos,
            start_pos + offset,
            offset_pos + offset
        );

        auto from = Cursor::fromEnd(_source, start_pos, offset_pos);
        auto to = Cursor::fromEnd(_source, start_pos + offset, offset_pos + offset);

        do {
            to.write(from.read());
            from.write(0xff);
        } while (--from && --to);
    } else {
        trace("::del marking key as empty @%u:%u\n", offset_pos, offset);
        // just null the lenght, since we at the last key
       _source.write(offset_pos + offset - 1, 0);
       _source.write(offset_pos + offset - 2, 0);
    }

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

RawStorage::ReadResult RawStorage::_raw_read() {
    uint16_t len = 0;
    ReadResult out(_source);

    trace("::_raw_read()\n");

    do {
        //trace("read_raw pos=%u end=%u state=%s\n", _cursor.position, _cursor.end, _state_describe().c_str());

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
            trace("found blob @%u:%u (left=%u) datalength=%u\n", out.cursor.begin, out.cursor.end, left, out.length);

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

uint16_t RawStorage::_cursor_reset_end() {
    _cursor.resetEnd();
    _state = State::Begin;
    return _cursor.end;
}

String RawStorage::_state_describe() {
    switch (_state) {
    case State::Begin: return "Begin";
    case State::End: return "End";
    case State::LenByte1: return "LenByte1";
    case State::LenByte2: return "LenByte2";
    case State::EmptyValue: return "EmptyValue";
    case State::Value: return "Value";
    }
}

} // namespace embedis
} // namespace settings
