/*

Part of WEBSERVER MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#include <ESPAsyncWebServer.h>

#include <functional>
#include <list>
#include <vector>

namespace espurna {
namespace web {
namespace print {

struct Config {
    struct Backlog {
        size_t count;
        size_t size;
        duration::Seconds timeout;
    };

    const char* const mimeType;
    Backlog backlog;
};

class RequestPrint : public Print {
public:
    enum class State {
        None,
        Sending,
        Done,
        Error
    };

    using BufferType = std::vector<uint8_t>;
    using TimeSource = espurna::time::CoreClock;

    // To be able to safely output data right from the request callback,
    // we schedule a 'printer' task that will print into the request response buffer via AsyncChunkedResponse
    // Note: implementation must be included in the header
    template <typename CallbackType>
    static void scheduleFromRequest(Config config, AsyncWebServerRequest*, CallbackType);

    template <typename CallbackType>
    static void scheduleFromRequest(AsyncWebServerRequest*, CallbackType);

    void flush() final override;

    size_t write(uint8_t) final override;
    size_t write(const uint8_t *buffer, size_t size) final override;

    State state() const {
        return _state;
    }

    void state(State state) {
        _state = state;
    }

    AsyncWebServerRequest* request() const {
        return _request;
    }

private:
    Config _config;

    std::list<BufferType> _buffers;
    AsyncWebServerRequest* const _request;
    State _state;

    RequestPrint(Config config, AsyncWebServerRequest* request) :
        _config(config),
        _request(request),
        _state(State::None)
    {}

    bool _addBuffer();
    bool _exhaustBuffers();

    void _prepareRequest();
    size_t _handleRequest(uint8_t* data, size_t maxLen);

    void _onDisconnect();

    template <typename CallbackType>
    void _callback(CallbackType&&);
};

template <typename T>
void scheduleFromRequest(AsyncWebServerRequest* request, T&& callback) {
    RequestPrint::scheduleFromRequest(request, std::forward<T>(callback));
}

} // namespace print
} // namespace web
} // namespace espurna
