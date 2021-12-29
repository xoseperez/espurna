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

// generic way to set up iterable payload with a schema
struct EnumerablePayload {
    using Check = bool(*)(size_t);
    using Generator = void(*)(JsonArray&, size_t);
    using Name = ::settings::StringView;

    struct Pair {
        Name name;
        Generator generate;
    };

    using Pairs = std::initializer_list<Pair>;

    EnumerablePayload(JsonObject& root, Name name);

    void operator()(Name name, ::settings::Iota iota, Check, Pairs&&);
    void operator()(Name name, size_t iota_end, Pairs&& pairs) {
        (*this)(name, ::settings::Iota { iota_end }, nullptr, std::move(pairs));
    }

    JsonObject& root() {
        return _root;
    }

private:
    JsonObject& _root;
};

// payload generator for IndexedSettings
struct EnumerableConfig {
    using Check = bool(*)(size_t);

    using Name = ::settings::StringView;
    using Setting = const ::settings::query::IndexedSetting;

    EnumerableConfig(JsonObject& root, Name name);

    void operator()(Name name, ::settings::Iota iota, Check check, Setting* begin, Setting* end);
    void operator()(Name name, ::settings::Iota iota, Setting* begin, Setting* end) {
        (*this)(name, iota, nullptr, begin, end);
    }

    template <typename T>
    void operator()(Name name, ::settings::Iota iota, T&& settings) {
        (*this)(name, iota, std::begin(settings), std::end(settings));
    }

    template <typename T>
    void operator()(Name name, size_t iota_end, T&& settings) {
        (*this)(name, ::settings::Iota{iota_end}, std::forward<T>(settings));
    }

    template <typename T>
    void operator()(Name name, size_t iota_end, Check check, T&& settings) {
        (*this)(name, ::settings::Iota{iota_end}, check, std::begin(settings), std::end(settings));
    }

    JsonObject& root() {
        return _root;
    }

private:
    JsonObject& _root;
};

} // namespace ws
} // namespace web
