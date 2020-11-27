/*

Part of the API MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

#include <algorithm>
#include <forward_list>
#include <memory>

// -----------------------------------------------------------------------------

struct ApiLevel {
    enum class Type {
        Unknown,
        Value,
        SingleWildcard,
        MultiWildcard
    };

    Type type;
    size_t offset;
    size_t length;
};

static_assert(std::is_nothrow_move_constructible<ApiLevel>::value, "");
static_assert(std::is_copy_constructible<ApiLevel>::value, "");

struct ApiLevels {
    ApiLevels() = delete;
    ApiLevels(const ApiLevels&) = default;
    ApiLevels(ApiLevels&&) noexcept = default;

    using Levels = std::vector<ApiLevel>;

    explicit ApiLevels(const String& path) :
        _path(path)
    {}

    explicit ApiLevels(const String& path, const ApiLevels& other) :
        _path(path),
        _levels(other._levels)
    {}

    explicit ApiLevels(const String& path, ApiLevels&& other) :
        _path(path),
        _levels(std::move(other._levels))
    {}

    ApiLevel& emplace_back(ApiLevel::Type type, size_t offset, size_t length) {
        ApiLevel level { type, offset, length };
        _levels.push_back(std::move(level));
        return _levels.back();
    }

    void clear() {
        _levels.clear();
    }

    void reserve(size_t size) {
        _levels.reserve(size);
    }

    String operator[](size_t index) const {
        auto& level = _levels[index];
        return _path.substring(level.offset, level.offset + level.length);
    }

    const String& path() const {
        return _path;
    }

    const Levels& levels() const {
        return _levels;
    }

    size_t size() const {
        return _levels.size();
    }

    Levels::const_iterator begin() const {
        return _levels.begin();
    }

    Levels::const_iterator end() const {
        return _levels.end();
    }

private:

    const String& _path;
    Levels _levels;
};

struct ApiRequest {

    // this is a purely temporary object, which we can only create while doing the API dispatch

    ApiRequest() = delete;
    ApiRequest(const ApiRequest&) = delete;

    ApiRequest(ApiRequest&&) = default;
    explicit ApiRequest(AsyncWebServerRequest& request, const ApiLevels& levels, const ApiLevels& wildcards) :
        _request(request),
        _levels(levels),
        _wildcards(wildcards)
    {}

    template <typename T>
    void handle(T&& handler) {
        _done = true;
        handler(&_request);
    }

    template <typename T>
    void param_foreach(T&& handler) {
        const size_t params { _request.params() };
        for (size_t current = 0; current < params; ++current) {
            auto* param = _request.getParam(current);
            handler(param->name(), param->value());
        }
    }

    template <typename T>
    void param_foreach(const String& name, T&& handler) {
        param_foreach([&](const String& param_name, const String& param_value) {
            if (param_name == name) {
                handler(param_value);
            }
        });
    }

    const String& param(const String& name) {
        auto* result = _request.getParam(name, HTTP_PUT == _request.method());
        if (result) {
            return result->value();
        }

        return _empty_string();
    }

    void send(const String& payload) {
        if (payload.length()) {
            _request.send(200, "text/plain", payload);
        } else {
            _request.send(204);
        }
        _done = true;
    }

    bool done() const {
        return _done;
    }

    const ApiLevels& levels() const {
        return _levels;
    }

    String levels(size_t index) const {
        return _levels[index];
    }

    const ApiLevels& wildcards() const {
        return _wildcards;
    }

    String wildcards(size_t index) const {
        return _wildcards[index];
    }

    private:

    const String& _empty_string() {
        static String string;
        return string;
    }

    bool _done { false };

    AsyncWebServerRequest& _request;
    const ApiLevels& _levels;
    const ApiLevels& _wildcards;
};

