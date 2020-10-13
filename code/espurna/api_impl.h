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

struct ApiBuffer {
    void clear() {
        std::fill(data, data + size(), '\0');
    }

    bool copy(const char* ptr, size_t size) {
        if (this->size() < (size + 1ul)) {
            return false;
        }

        std::copy(ptr, ptr + size + 1, data);
        return true;
    }

    constexpr size_t size() const {
        return sizeof(data);
    }

    char data[API_BUFFER_SIZE];
};

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

    using difference_type = Levels::difference_type;
    using value_type = Levels::value_type;
    using pointer = Levels::pointer;
    using reference = Levels::reference;

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

    // TODO: return response, manage terminal & prometheus through this thing

    template <typename ...Args>
    void send(Args&&... args) {
        _sent = true;
        _request.send(std::forward<Args>(args)...);
    }

    bool sent() const {
        return _sent;
    }

    const ApiLevels& levels() const {
        return _levels;
    }

    const ApiLevels& wildcards() const {
        return _wildcards;
    }

    private:

    bool _sent { false };

    AsyncWebServerRequest& _request;
    const ApiLevels& _levels;
    const ApiLevels& _wildcards;
};

