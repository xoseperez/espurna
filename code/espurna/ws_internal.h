/*

WEBSOCKET MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#include <IPAddress.h>

#include <cstdint>
#include <memory>
#include <vector>

constexpr const size_t WS_DEBUG_MSG_BUFFER = 8;

// -----------------------------------------------------------------------------
// WS authentication
// -----------------------------------------------------------------------------

struct ws_ticket_t {
    IPAddress ip;
    unsigned long timestamp = 0;
};

// -----------------------------------------------------------------------------
// WS callbacks
// -----------------------------------------------------------------------------

struct ws_counter_t {

    ws_counter_t() : current(0), start(0), stop(0) {}

    ws_counter_t(uint32_t start, uint32_t stop) :
        current(start), start(start), stop(stop) {}

    void reset() {
        current = start;
    }

    void next() {
        if (current < stop) {
            ++current;
        }
    }

    bool done() {
        return (current >= stop);
    }

    uint32_t current;
    uint32_t start;
    uint32_t stop;
};

struct ws_data_t {

    enum mode_t {
        SEQUENCE,
        ALL
    };

    ws_data_t(const ws_on_send_callback_f& cb) :
        storage(new ws_on_send_callback_list_t {cb}),
        client_id(0),
        mode(ALL),
        callbacks(*storage.get()),
        counter(0, 1)
    {}

    ws_data_t(uint32_t client_id, const ws_on_send_callback_f& cb) :
        storage(new ws_on_send_callback_list_t {cb}),
        client_id(client_id),
        mode(ALL),
        callbacks(*storage.get()),
        counter(0, 1)
    {}

    ws_data_t(const uint32_t client_id, ws_on_send_callback_list_t&& callbacks, mode_t mode = SEQUENCE) :
        storage(new ws_on_send_callback_list_t(std::move(callbacks))),
        client_id(client_id),
        mode(mode),
        callbacks(*storage.get()),
        counter(0, (storage.get())->size())
    {}

    ws_data_t(const uint32_t client_id, const ws_on_send_callback_list_t& callbacks, mode_t mode = SEQUENCE) :
        client_id(client_id),
        mode(mode),
        callbacks(callbacks),
        counter(0, callbacks.size())
    {}

    bool done() {
        return counter.done();
    }

    void sendAll(JsonObject& root) {
        while (!counter.done()) counter.next();
        for (auto& callback : callbacks) {
            callback(root);
        }
    }

    void sendCurrent(JsonObject& root) {
        callbacks[counter.current](root);
        counter.next();
    }

    void send(JsonObject& root) {
        switch (mode) {
            case SEQUENCE: sendCurrent(root); break;
            case ALL: sendAll(root); break;
        }
    }

    std::unique_ptr<ws_on_send_callback_list_t> storage;

    const uint32_t client_id;
    const mode_t mode;
    const ws_on_send_callback_list_t& callbacks;
    ws_counter_t counter;
};

// -----------------------------------------------------------------------------
// Debug
// -----------------------------------------------------------------------------

using ws_debug_msg_t = std::pair<String, String>;

struct ws_debug_t {

    ws_debug_t(size_t capacity) :
        flush(false),
        current(0),
        capacity(capacity)
    {
        messages.reserve(capacity);
    }

    void clear() {
        messages.clear();
        current = 0;
        flush = false;
    }

    void add(const char* prefix, const char* message) {
        if (current >= capacity) {
            flush = true;
            send(wsConnected());
        }

        messages.emplace(messages.begin() + current, prefix, message);
        flush = true;
        ++current;
    }

    void send(const bool connected);

    bool flush;
    size_t current;
    const size_t capacity;
    std::vector<ws_debug_msg_t> messages;

};
