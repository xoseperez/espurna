/*

Part of WEBSERVER MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <ArduinoJson.h>
#include <sys/pgmspace.h>

#include "types.h"

namespace ArduinoJson {
namespace Internals {

// ArduinoJson v5 view adapter; we can't force variant to not
// buffer us until v6, but might as well make it aware of this type
// (internals depend on JsonVariant asString, which works through normal string functions)

template <>
struct StringTraits<::espurna::StringView, void> {
    // c/p from std stream / Stream adapters
    // allow to pass the view as-is to the parser
    struct Reader {
        Reader(espurna::StringView value) :
            _value(value),
            _it(_value.begin())
        {}

        void move() {
            _current = _next;
            _next = '\0';
        }

        char current() {
            if (!_current) {
                _current = read();
            }
            return _current;
        }

        char next() {
            if (!_next) {
                _next = read();
            }
            return _next;
        }

    private:
        char read() {
            if (_it != _value.end()) {
                const auto c = (*_it);
                ++_it;
                return c;
            }

            return '\0';
        }

        espurna::StringView _value;
        const char* _it;

        char _current = 0;
        char _next = 0;
    };

    template <typename T>
    static bool equals(::espurna::StringView lhs, T&& rhs) {
        return lhs == rhs;
    }

    static bool is_null(::espurna::StringView string) {
        return string.length() == 0;
    }

    using duplicate_t = const char*;

    template <typename Buffer>
    static const char* duplicate(::espurna::StringView string, Buffer* buffer) {
        if (string.length()) {
            const auto size = string.length();
            auto* dup = reinterpret_cast<char*>(buffer->alloc(size));
            if (dup) {
                memcpy_P(dup, string.begin(), size);
                dup[size] = '\0';
                return dup;
            }
        }

        return nullptr;
    }

    // technically, we could've had append w/ strings only
    // but, this also means append of char, which we could not do
    static const bool has_append = false;
    static const bool has_equals = true;
    static const bool should_duplicate = true;
};

template <>
struct ValueSaver<::espurna::StringView> {
    template <typename Destination>
    static bool save(JsonBuffer*, Destination& dst, ::espurna::StringView src) {
        dst = src;
        return true;
    }
};

} // namespace Internals
} // namespace ArduinoJson
