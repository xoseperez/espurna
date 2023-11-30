/*

Part of the API MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "api_async_server.h"
#include "api_path.h"

namespace espurna {
namespace api {

// temporary object, which we can only create while doing the API dispatch

struct Request {
    Request() = delete;

    Request(const Request&) = default;
    Request& operator=(const Request&) = delete;

    Request(Request&&) noexcept = default;
    Request& operator=(Request&&) = delete;

    Request(AsyncWebServerRequest& request, const PathParts& pattern, const PathParts& parts) :
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

    bool done() const {
        return _done;
    }

    const PathParts& parts() const {
        return _parts;
    }

    String part(size_t index) const {
        return _parts[index].toString();
    }

    // Only works when pattern cointains '+', retrieving the part at the same index from the real path
    // e.g. for the pair of `some/+/path` and `some/data/path`, calling `wildcard(0)` will return `data`
    String wildcard(int index) const;
    size_t wildcards() const;

    // Extract form data parameter value from request by name
    StringView param(const String&);

    // Send out the payload and finish the request
    // By default, payload is sent with status 200
    // For zero-length payloads, status is set to 204
    void send(const String& payload);

private:
    bool _done { false };

    AsyncWebServerRequest& _request;
    const PathParts& _pattern;
    const PathParts& _parts;
};

struct RequestHelper {
    RequestHelper() = delete;

    RequestHelper(const RequestHelper&) = delete;
    RequestHelper& operator=(const RequestHelper&) = delete;

    RequestHelper(RequestHelper&&) noexcept = default;
    RequestHelper& operator=(RequestHelper&&) = delete;

    // &path is expected to be request->url(), which is valid throughout the request's lifetime
    RequestHelper(AsyncWebServerRequest& request, const PathParts& pattern) :
        _request(request),
        _pattern(pattern),
        _path(request.url()),
        _match(_pattern.match(_path))
    {}

    Request request() const {
        return Request(_request, _pattern, _path);
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

} // namespace api
} // namespace espurna
