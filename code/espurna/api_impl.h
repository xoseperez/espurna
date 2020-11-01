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

    String toString() const {
        return path.substring(offset, offset + length);
    }

    const String& path;
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

    ApiLevel& emplace_back(ApiLevel::Type type, size_t offset, size_t length) {
        ApiLevel level { _path, type, offset, length };
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
        return _levels[index].toString();
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
    ApiRequest(ApiRequest&&) = delete;

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
    void param_foreach(const String& name, T&& handler) {
        const size_t params { _request.params() };
        for (size_t current = 0; current < params; ++current) {
            auto* param = _request.getParam(current);
            if (param->name() == name) {
                handler(param->value());
            }
        }
    }

    const String& param(const String& name) {
        auto* result = _request.getParam(name, HTTP_PUT == _request.method());
        if (result) {
            return result->value();
        }

        return empty();
    }

    void send(const String& payload) {
        _done = true;
        _request.send(200, "text/plain", payload);
    }

    bool done() const {
        return _done;
    }

    const ApiLevels& levels() const {
        return _levels;
    }

    const ApiLevels& wildcards() const {
        return _wildcards;
    }

    private:

    const String& empty() {
        static String string;
        return string;
    }

    bool _done { false };

    AsyncWebServerRequest& _request;
    const ApiLevels& _levels;
    const ApiLevels& _wildcards;
};

