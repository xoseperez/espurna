/*

Part of the WEBSOCKET MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <IPAddress.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "system.h"
#include "ws.h"

// -----------------------------------------------------------------------------
// WS authentication
// -----------------------------------------------------------------------------

struct WsTicket {
    using TimeSource = espurna::time::CoreClock;
    IPAddress ip;
    TimeSource::time_point timestamp{};
};

// -----------------------------------------------------------------------------
// WS callbacks
// -----------------------------------------------------------------------------

// The idea here is to bind either:
// - constant 'callbacks' list as reference, which was registered via wsRegister()
// - in-place callback / callbacks that will be moved inside this container

class WsPostponedCallbacks {
public:
    using TimeSource = espurna::time::CpuClock;

    enum class Mode {
        Sequence,
        All
    };

    WsPostponedCallbacks(uint32_t client_id, ws_on_send_callback_f&& cb) :
        _client_id(client_id),
        _timestamp(TimeSource::now()),
        _mode(Mode::All),
        _storage(new ws_on_send_callback_list_t {std::move(cb)}),
        _callbacks(*_storage.get()),
        _current(_callbacks.begin())
    {}

    WsPostponedCallbacks(uint32_t client_id, const ws_on_send_callback_f& cb) :
        _client_id(client_id),
        _timestamp(TimeSource::now()),
        _mode(Mode::All),
        _storage(new ws_on_send_callback_list_t {cb}),
        _callbacks(*_storage.get()),
        _current(_callbacks.begin())
    {}

    template <typename T>
    explicit WsPostponedCallbacks(T&& cb) :
        WsPostponedCallbacks(0, std::forward<T>(cb))
    {}

    WsPostponedCallbacks(uint32_t client_id, const ws_on_send_callback_list_t& cbs, Mode mode = Mode::Sequence) :
        _client_id(client_id),
        _timestamp(TimeSource::now()),
        _mode(mode),
        _callbacks(cbs),
        _current(_callbacks.begin())
    {}

    WsPostponedCallbacks(uint32_t client_id, ws_on_send_callback_list_t&& cbs, Mode mode = Mode::All) :
        _client_id(client_id),
        _timestamp(TimeSource::now()),
        _mode(mode),
        _storage(new ws_on_send_callback_list_t(std::move(cbs))),
        _callbacks(*_storage.get()),
        _current(_callbacks.begin())
    {}

    bool done() {
        return _current == _callbacks.end();
    }

    void sendAll(JsonObject& root) {
        _current = _callbacks.end();
        for (auto& callback : _callbacks) {
            callback(root);
        }
    }

    void sendCurrent(JsonObject& root) {
        if (_current == _callbacks.end()) return;
        (*_current)(root);
        ++_current;
    }

    void send(JsonObject& root) {
        switch (_mode) {
        case Mode::Sequence:
            sendCurrent(root);
            break;
        case Mode::All:
            sendAll(root);
            break;
        }
    }

    uint32_t id() const {
        return _client_id;
    }

    TimeSource::time_point timestamp() const {
        return _timestamp;
    }

private:
    uint32_t _client_id;
    TimeSource::time_point _timestamp;
    Mode _mode;

    std::unique_ptr<ws_on_send_callback_list_t> _storage;

    const ws_on_send_callback_list_t& _callbacks;
    ws_on_send_callback_list_t::const_iterator _current;
};

// -----------------------------------------------------------------------------
// Debug
// -----------------------------------------------------------------------------

class WsDebug {
public:
    using Message = std::pair<String, String>;
    using MsgList = std::vector<Message>;

    WsDebug(size_t capacity) :
        _flush(false),
        _current(0),
        _capacity(capacity)
    {
        _messages.reserve(_capacity);
    }

    void clear() {
        _messages.clear();
        _current = 0;
        _flush = false;
    }

    template <typename T = Message>
    void add(T&& message) {
        if (_current >= _capacity) {
            _flush = true;
            send(wsConnected());
        }

        _messages.emplace(_messages.begin() + _current, std::forward<T>(message));
        _flush = true;
        ++_current;
    }

    void add(const char* prefix, const char* message) {
        add(std::make_pair(prefix, message));
    }

    void send(bool connected);

private:
    bool _flush;
    size_t _current;
    const size_t _capacity;
    MsgList _messages;
};
