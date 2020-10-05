/*

Part of the API MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>
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

struct ApiHandle {
    using PathParam = std::pair<const String&, const String&>;
    using PathParams = std::forward_list<PathParam>;

    ApiHandle() = delete;
    ApiHandle(const ApiHandle&) = delete;
    ApiHandle(ApiHandle&&) = delete;

    explicit ApiHandle(AsyncWebServerRequest& request, PathParams&& params) :
        _request(request),
        _params(std::move(params))
    {}

    const String& findParam(const String& name) const {
        static String empty;
        for (auto& ref : _params) {
            if (name == ref.first) {
                return ref.second;
            }
        }

        return empty;
    }

    template <typename T>
    void send(T&& param) {
        _sent = true;
        _request.send(std::forward<T>(param));
    }

    AsyncWebServerRequest& request() {
        return _request;
    }

    bool sent() const {
        return _sent;
    }

    void clear() {
        _params.clear();
    }

    private:

    bool _sent { false };
    AsyncWebServerRequest& _request;
    PathParams _params;
};


