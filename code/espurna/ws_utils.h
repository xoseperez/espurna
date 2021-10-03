/*

Part of the WEBSOCKET MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

// Generic payload for indexed aka enumerable entries
// For the specific root container, add 'name'ed key and the function that will generate entry from index / id
// Each key is appended to the 'schema' list that will be used by the webui to know the actual settings key

#include <ArduinoJson.h>

namespace web {
namespace ws {

// TODO: use `const char*', but somehow force the arduinojson layer to *always* use flash funcs for reading them?

struct EnumerableConfig {
    alignas(4)
    static const char SchemaKey[];

    using Callback = void(*)(JsonArray&, size_t);

    struct Pair {
        const __FlashStringHelper* key;
        Callback callback;
    };

    using Pairs = std::initializer_list<Pair>;

    EnumerableConfig(JsonObject& root, const __FlashStringHelper* name);
    void operator()(const __FlashStringHelper* name, size_t count, Pairs&&);

private:
    JsonObject& _root;
};

} // namespace ws
} // namespace web

struct WsJsonEnumerables {
    using Callback = void(*)(JsonArray&, size_t);

    struct Pair {
        const __FlashStringHelper* key;
        Callback callback;
    };

    using Pairs = std::initializer_list<Pair>;

    WsJsonEnumerables(JsonObject& root, const __FlashStringHelper* name);
    void operator()(const __FlashStringHelper* name, size_t count, Pairs&& pairs);

private:
    JsonObject& _root;
    static const char SchemaKey[];
};
