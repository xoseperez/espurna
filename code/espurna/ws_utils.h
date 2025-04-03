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

#include "system_time.h"
#include "settings.h"

namespace espurna {
namespace web {
namespace ws {

// generic way to set up iterable payload with a schema
struct EnumerablePayload {
    using Check = bool(*)(size_t);
    using Generator = void(*)(JsonArray&, size_t);

    struct Pair {
        StringView name;
        Generator generate;
    };

    using Pairs = std::initializer_list<Pair>;

    EnumerablePayload(JsonObject& root, StringView name);

    void operator()(StringView name, settings::Iota iota, Check, Pairs&&);
    void operator()(StringView name, size_t iota_end, Pairs&& pairs) {
        (*this)(name, settings::Iota { iota_end }, nullptr, std::move(pairs));
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
    using Setting = const settings::query::IndexedSetting;

    using SourceFunc = Setting::ValueFunc;
    using TargetFunc = void (*)(JsonArray&, size_t);

    EnumerableConfig(JsonObject& root, StringView name);

    void operator()(StringView name, settings::Iota iota, Check check, Setting* begin, Setting* end);
    void operator()(StringView name, settings::Iota iota, Setting* begin, Setting* end) {
        (*this)(name, iota, nullptr, begin, end);
    }

    template <typename T>
    void operator()(StringView name, settings::Iota iota, T&& settings) {
        (*this)(name, iota, std::begin(settings), std::end(settings));
    }

    template <typename T>
    void operator()(StringView name, size_t iota_end, T&& settings) {
        (*this)(name, settings::Iota{iota_end}, std::forward<T>(settings));
    }

    template <typename T>
    void operator()(StringView name, size_t iota_end, Check check, T&& settings) {
        (*this)(name, settings::Iota{iota_end}, check, std::begin(settings), std::end(settings));
    }

    JsonObject& root() {
        return _root;
    }

    void replacement(SourceFunc, TargetFunc);

private:
    struct Replacement {
        SourceFunc source;
        TargetFunc target;
    };


    std::vector<Replacement> _replacements;
    JsonObject& _root;
};

struct EnumerableTypes {
    template <typename T>
    using Enumeration = settings::options::Enumeration<T>;

    EnumerableTypes(JsonObject& root, StringView name);

    template <typename T>
    void operator()(const Enumeration<T>* begin, const Enumeration<T>* end) {
        for (auto it = begin; it != end; ++it) {
            (*this)((*it).numeric(), (*it).string());
        }
    }

    template <typename T>
    void operator()(const T& other) {
        (*this)(std::begin(other), std::end(other));
    }

    void operator()(int, StringView);

private:
    JsonArray& _root;
};

struct PostponedDebug;

struct PostponedPayload {
    static constexpr size_t BufferHint = size_t{ JSON_OBJECT_SIZE(1) + JSON_ARRAY_SIZE(1) };
    static constexpr size_t CountMax { 8 };

    struct Flag {
        explicit Flag(PostponedPayload&);
        ~Flag();

        Flag(const Flag&) = delete;
        Flag& operator=(const Flag&) = delete;

        Flag(Flag&&) = delete;
        Flag& operator=(Flag&&) = delete;

        const char* data() const {
            return _ref._data.c_str();
        }

        bool pending() const {
            return _ref.pending();
        }

    private:
        PostponedPayload& _ref;
    };

    PostponedPayload();
    explicit PostponedPayload(uint32_t id) :
        _id(id)
    {}

    bool pending() const {
        return _pending;
    }

    bool post(bool connected);
    bool post();

    void buffer(const char*, size_t);
    bool connected() const;

    std::shared_ptr<Flag> make_flag();

private:
    friend Flag;
    friend PostponedDebug;

    void buffer_impl(StringView);
    void buffer_impl(const char*, size_t);

    size_t _count{};

    String _data;
    bool _pending { false };

    uint32_t _id{};
};

struct PostponedDebug : public PostponedPayload {
    void buffer(const DebugPrefix&, const char*, size_t);
};

struct InplaceLog;

struct InplacePayload {
    static constexpr size_t BufferHint = size_t{ JSON_OBJECT_SIZE(1) + JSON_ARRAY_SIZE(1) };

    using Clock = espurna::time::CoreClock;
    using Send = std::function<void(JsonObject&, String&)>;

    static constexpr size_t CountMax { 8 };

    static constexpr auto DefaultTimeout = duration::Seconds{ 2 };
    static constexpr auto DefaultWait = duration::Milliseconds{ 100 };

    InplacePayload() = delete;
    InplacePayload(JsonObject& root, uint32_t id);

    void reset();

    bool connected() const;
    bool can_send() const;

    void write(const char*, size_t);
    bool poll_send();
    bool send();

    void timeout(Clock::duration duration) {
        _timeout = duration;
    }

    void wait_time(Clock::duration duration) {
        _wait = duration;
    }

private:
    void write_impl(const char*, size_t);
    void send_impl();

    friend InplaceLog;

    Clock::duration _timeout { DefaultTimeout };
    Clock::duration _wait { DefaultWait };

    String _data;
    size_t _count{};

    JsonObject& _root;
    uint32_t _id;
};

struct InplaceLog : public InplacePayload {
    InplaceLog(JsonObject&, uint32_t);

    void write(const char* , size_t);
    bool send();
};

} // namespace ws
} // namespace web
} // namespace espurna
