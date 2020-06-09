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
        length(0),
        dataLength(0)
    {}

    ReadResult(SourceBase& source) :
        cursor(source),
        result(false),
        length(0),
        dataLength(0)
    {}

    operator bool() {
        return result;
    }

    String read() {
        String out;
        out.reserve(dataLength);
        if (!dataLength) {
            return out;
        }

        cursor.end -= 2;
        while (cursor) {
            out += static_cast<char>(cursor.read());
            ++cursor;
        }
        cursor.end += 2;

        return out;
    }

    Cursor cursor;
    bool result;
    uint16_t length;
    uint16_t dataLength;

};

struct RawStorage::KeyValueResult {
    operator bool() {
        return (key) && (value) && (key.dataLength);
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

    _cursor_rewind();

    do {
        auto kv = _read_kv();
        if (!kv) break;

        // no point in comparing keys when length does not match
        // (and we also don't want to allocate the string)
        if (kv.key.dataLength != len) {
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
    if (!key_len) return false;

    size_t value_len = value.length();

    // ...but can save empty values as 0x00 0x00 0x00 0x00
    // total sum is 4 length bytes for 2 values + byte-length of values themselves
    size_t need = key_len + ((value_len > 0) ? value_len : 2) + 4;

    size_t start = _cursor_rewind();
    trace("::set start default = %u @%u:%u\n", start, _cursor.position, _cursor.end);

    do {
        auto kv = _read_kv();
        if (!kv) {
            break;
        }

        start = kv.value.cursor.position - 1;
        trace("::set start is now %u, found key=%u:%u value=%u:%u\n",
            start,
            kv.key.cursor.position,
            kv.key.cursor.end,
            kv.value.cursor.position,
            kv.value.cursor.end
        );

        // in case we match with the existing key, remove it from the storage and place ours at the end
        if ((kv.key.dataLength == key_len) && (kv.key.read() == key)) {
            del(key);
        }
    } while (_state != State::End);

    trace("::set need=%u start=%u\n", need, start);

    // we should only insert when possition is still within possible size
    if (start > need) {
        size_t pos = start;

        // put the length of the value as 2 bytes and then write the string
        _source.write(pos--, (key_len & 0xf));
        _source.write(pos--, ((key_len >> 8) & 0xf));
        while (key_len--) {
            _source.write(pos--, key[key_len]);
        }

        _source.write(pos--, (value_len & 0xf));
        _source.write(pos--, ((value_len >> 8) & 0xf));

        // value is placed the same way as the key
        if (value_len) {
            while (value_len--) {
                _source.write(pos--, value[value_len]);
            }
        // when value is empty, we just store some padding
        } else {
            _source.write(pos--, 0);
            _source.write(pos--, 0);
        }

        return true;
    }

    return false;
}

bool RawStorage::del(const String& key) {
    size_t key_len = key.length();
    if (!key_len) return false;

    // Removes key from the storage by overwriting the key with left-most data
    Cursor to_erase(_source);

    size_t start = _cursor_rewind();

    do {
        auto kv = _read_kv();
        if (!kv) break;

        start = kv.value.cursor.position;

        // we should only compare strings of equal length.
        // when matching, record { value ... key } range + 4 bytes for length
        // continue searching for the leftmost boundary
        if ((kv.key.dataLength == key_len) && (kv.key.read() == key)) {
            to_erase.position = kv.value.cursor.position;
            to_erase.end = to_erase.position + kv.key.length + kv.value.length;
            trace("need to erase @blob between %u:%u\n", to_erase.position, to_erase.end);
            continue;
        }
    } while (_state != State::End);

    if (!to_erase) {
        return false;
    }

    // we either end up to the left or to the right of the boundary
    if (start < to_erase.position) {
        trace("::del moving available range  @%u:%u to @%u:%u\n",
            start,
            start + to_erase.length(),
            to_erase.position,
            to_erase.position + to_erase.length()
        );
        // overwrite key with data that is to the left of it
        size_t to = to_erase.end - 1;
        size_t from = to_erase.position - 1;
        do {
            _source.write(to, _source.read(from));
            _source.write(from--, 0);
        } while (to-- != start);
    } else {
        trace("::del invalidating blob  @%u:%u\n",
            to_erase.position, to_erase.end
        );
        // just null the lenght, since we at the last key
       _source.write(to_erase.end - 1, 0);
       _source.write(to_erase.end - 2, 0);
    }

    return true;
}

size_t RawStorage::keys() {
    size_t result = 0;

    _cursor_rewind();

    do {
        auto kv = _read_kv();
        if (!kv) break;
         ++result;
    } while (_state != State::End);

    return result;
}

RawStorage::KeyValueResult RawStorage::_read_kv() {
    KeyValueResult defaultResult(_source);

    auto key = _raw_read();
    if (!key) {
        return defaultResult;
    }
    if (!key.dataLength) {
        return defaultResult;
    }

    auto value = _raw_read();

    return {key, value};
};

RawStorage::ReadResult RawStorage::_raw_read() {
    uint16_t len = 0;
    ReadResult out(_source);

    do {
        //trace("read_raw pos=%u end=%u state=%s\n", _cursor.position, _cursor.end, _state_describe().c_str());

        // storage is written right-to-left, cursor is always decreasing
        switch (_state) {
        case State::Begin:
            --_cursor;
            _state = State::LenByte1;
            continue;
        case State::LenByte1:
            len = _cursor.read();
            --_cursor;
            _state = State::LenByte2;
            break;
        case State::LenByte2:
        {
            uint8_t len2 = _cursor.read();
            --_cursor;
            // special case when we encounter 0
            if (!len && !len2) {
                len = 2;
                _state = State::EmptyValue;
                // otherwise, merge both values
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

            // set cursor to the discovered value
            out.result = true;
            out.dataLength = (_state == State::EmptyValue) ? 0 : len;
            out.length = len + 2;
            out.cursor.reset(
                _cursor.position,
                _cursor.position + out.length
            );
            trace("found blob @%u:%u len=%u data=%u\n", out.cursor.position, out.cursor.end, out.length, out.dataLength);

            // probe that we can read next 2 bytes
            if (_cursor.position > 2) {
                _state = State::LenByte1;
            } else {
                _state = State::End;
            }
            --_cursor;

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

uint16_t RawStorage::_cursor_rewind() {
    _cursor.rewind();
    _state = State::Begin;
    return _cursor.end - 1;
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
