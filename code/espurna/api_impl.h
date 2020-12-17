/*

Part of the API MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <ESPAsyncWebServer.h>

#include <algorithm>
#include <memory>
#include <vector>

// -----------------------------------------------------------------------------

struct PathPart {
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

struct PathParts {
    using Parts = std::vector<PathPart>;

    PathParts() = delete;

    PathParts(const PathParts&) = default;
    PathParts(PathParts&&) noexcept = default;

    explicit PathParts(const String& path);

    explicit operator bool() const {
        return _ok;
    }

    void clear() {
        _parts.clear();
    }

    void reserve(size_t size) {
        _parts.reserve(size);
    }

    String operator[](size_t index) const {
        auto& part = _parts[index];
        return _path.substring(part.offset, part.offset + part.length);
    }

    const String& path() const {
        return _path;
    }

    const Parts& parts() const {
        return _parts;
    }

    size_t size() const {
        return _parts.size();
    }

    Parts::const_iterator begin() const {
        return _parts.begin();
    }

    Parts::const_iterator end() const {
        return _parts.end();
    }

    bool match(const PathParts& path) const;
    bool match(const String& path) const {
        return match(PathParts(path));
    }

private:
    PathPart& emplace_back(PathPart::Type type, size_t offset, size_t length) {
        PathPart part { type, offset, length };
        _parts.push_back(std::move(part));
        return _parts.back();
    }

    const String& _path;
    Parts _parts;
    bool _ok { false };
};

// this is a purely temporary object, which we can only create while doing the API dispatch

struct ApiRequest {
    ApiRequest() = delete;

    ApiRequest(const ApiRequest&) = default;
    ApiRequest(ApiRequest&&) noexcept = default;

    explicit ApiRequest(AsyncWebServerRequest& request, const PathParts& pattern, const PathParts& parts) :
        _request(request),
        _pattern(pattern),
        _parts(parts)
    {}

    template <typename T>
    void handle(T&& handler) {
        if (_done) return;
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
        if (_done) return;
        _done = true;

        if (payload.length()) {
            _request.send(200, "text/plain", payload);
        } else {
            _request.send(204);
        }
    }

    bool done() const {
        return _done;
    }

    const PathParts& parts() const {
        return _parts;
    }

    String part(size_t index) const {
        return _parts[index];
    }

    // Only works when pattern cointains '+', retrieving the part at the same index from the real path
    // e.g. for the pair of `some/+/path` and `some/data/path`, calling `wildcard(0)` will return `data`
    String wildcard(int index) const;
    size_t wildcards() const;

private:
    const String& _empty_string() const {
        static const String string;
        return string;
    }

    bool _done { false };

    AsyncWebServerRequest& _request;
    const PathParts& _pattern;
    const PathParts& _parts;
};

struct ApiRequestHelper {
    ApiRequestHelper(const ApiRequestHelper&) = delete;
    ApiRequestHelper(ApiRequestHelper&&) noexcept = default;

    // &path is expected to be request->url(), which is valid throughout the request's lifetime
    explicit ApiRequestHelper(AsyncWebServerRequest& request, const PathParts& pattern) :
        _request(request),
        _pattern(pattern),
        _path(request.url()),
        _match(_pattern.match(_path))
    {}

    ApiRequest request() const {
        return ApiRequest(_request, _pattern, _path);
    }

    const PathParts& parts() const {
        return _path;
    }

    bool match() const {
        return _match;
    }

private:
    AsyncWebServerRequest& _request;
    const PathParts& _pattern;
    PathParts _path;
    bool _match;
};
