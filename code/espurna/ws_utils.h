/*

Part of the WEBSOCKET MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

// Generic payload for indexed aka enumerable entries
// For the specific root container, add 'name'ed key and the function that will generate entry from index / id
// Each key is appended to the 'schema' list that will be used by the webui to know the actual settings key
//
// For example
// ```
// {
//   name:
//   {
//     schema: ["one", "two", "three", "four"],
//     values: [
//       [1,2,3,4],
//       [5,6,7,8],
//       [9,0,1,2],
//       [3,4,5,6]
//     ]
//   }
// }
// ```
//
// On the webui side, these become
// ```
// one0 => 1, two0 => 2, three0 => 3, four0 => 4
// one1 => 5, two1 => 6, three1 => 7, four1 => 8
// ...etc...
// ```
// Where each row in values is the specific index, and the key string is taken from the schema list
// Obviously, number of elements is always expected to match

#include <ArduinoJson.h>

#include "settings.h"

namespace web {
namespace ws {

// TODO: use `const char*', but somehow force the arduinojson layer to *always* use flash funcs for reading them?
// TODO: try to minimize the ROM by implementing things in .cpp
// TODO: generic templated funcs instead of pointers? also, ROM...

struct EnumerableConfig {
    alignas(4)
    static const char SchemaKey[];

    using Check = bool(*)(size_t);
    using Callback = void(*)(JsonArray&, size_t);

    struct Pair {
        const __FlashStringHelper* key;
        Callback callback;
    };

    using Pairs = std::initializer_list<Pair>;

    EnumerableConfig(JsonObject& root, const __FlashStringHelper* name);
    void operator()(const __FlashStringHelper* name, ::settings::Iota, Check, Pairs&&);

    void operator()(const __FlashStringHelper* name, ::settings::Iota iota, Pairs&& pairs) {
        (*this)(name, iota, nullptr, std::move(pairs));
    }

    void operator()(const __FlashStringHelper* name, size_t end, Pairs&& pairs) {
        (*this)(name, ::settings::Iota{end}, nullptr, std::move(pairs));
    }

    JsonObject& root() {
        return _root;
    }

private:
    JsonObject& _root;
};

} // namespace ws
} // namespace web
