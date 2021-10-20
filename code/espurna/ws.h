/*

WEBSOCKET MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <ArduinoJson.h>

#include <functional>
#include <vector>

#include "web.h"
#include "utils.h"

#include "ws_utils.h"

// Generalized WS lifetime callbacks.
// Each callback is kept as std::function, thus we can use complex objects, and not just basic function pointers.
//
// Connection start:
// - on_visible will be the very first message sent, callback data will be grouped together
// - on_connected is sent next, but each callback's data will be sent separately
// - on_data is the final one, each callback is executed separately
//
// While connected:
// - on_action will be ran whenever we receive special JSON 'action' payload
// - on_keycheck will be used to determine if we can handle specific settings keys

using ws_on_send_callback_f = std::function<void(JsonObject& root)>;
using ws_on_action_callback_f = std::function<void(uint32_t client_id, const char * action, JsonObject& data)>;
using ws_on_keycheck_callback_f = std::function<bool(const char * key, JsonVariant& value)>;

// TODO: use iterators as inputs for Post(), avoid depending on vector / any specific container
using ws_on_send_callback_list_t = std::vector<ws_on_send_callback_f>;
using ws_on_action_callback_list_t = std::vector<ws_on_action_callback_f>;
using ws_on_keycheck_callback_list_t = std::vector<ws_on_keycheck_callback_f>;

struct ws_callbacks_t {
    using on_send_f = void(*)(JsonObject&);
    ws_callbacks_t& onVisible(on_send_f);
    ws_callbacks_t& onConnected(on_send_f);
    ws_callbacks_t& onData(on_send_f);

    using on_action_f = void(*)(uint32_t, const char*, JsonObject&);
    ws_callbacks_t& onAction(on_action_f);

    using on_keycheck_f = bool(*)(const char*, JsonVariant&);
    ws_callbacks_t& onKeyCheck(on_keycheck_f);

    ws_on_send_callback_list_t on_visible;
    ws_on_send_callback_list_t on_connected;
    ws_on_send_callback_list_t on_data;

    ws_on_action_callback_list_t on_action;
    ws_on_keycheck_callback_list_t on_keycheck;
};

// Postponed debug messages. best-effort, will not be re-scheduled when ws queue is full

bool wsDebugSend(const char* prefix, const char* message);

// Postponed json messages. schedules callback(s) to be called when resources to do so are available.
// Queued item is removed on client disconnection *or* when internal timeout occurs

// There are two policies set on how to send the data:
// - All will use the same JsonObject for each callback
// - Sequence will use a different JsonObject for each callback
// Default is All
//
// WARNING: callback lists are taken by reference! make sure that list is ether:
// - std::move(...)'ed to give control of the callback list to us
// - persistent and will be available after the current block ends (global, heap-allocated, etc.)
//   de-allocation is not expected e.g. referenced struct from `wsRegister()` is never destroyed

void wsPost(uint32_t client_id, ws_on_send_callback_f&& cb);
void wsPost(ws_on_send_callback_f&& cb);
void wsPost(uint32_t client_id, const ws_on_send_callback_f& cb);
void wsPost(const ws_on_send_callback_f& cb);

void wsPostAll(uint32_t client_id, ws_on_send_callback_list_t&& cbs);
void wsPostAll(ws_on_send_callback_list_t&& cbs);
void wsPostAll(uint32_t client_id, const ws_on_send_callback_list_t& cbs);
void wsPostAll(const ws_on_send_callback_list_t& cbs);

void wsPostSequence(uint32_t client_id, ws_on_send_callback_list_t&& cbs);
void wsPostSequence(ws_on_send_callback_list_t&& cbs);
void wsPostSequence(uint32_t client_id, const ws_on_send_callback_list_t& cbs);
void wsPostSequence(const ws_on_send_callback_list_t& cbs);

// Immmediatly try to serialize and send JsonObject&
// May silently fail when network is busy sending previous requests, or there's not enough RAM

void wsSend(JsonObject& root);
void wsSend(uint32_t client_id, JsonObject& root);

void wsSend(JsonObject& root);
void wsSend(ws_on_send_callback_f callback);
void wsSend(const char* data);

// Check if any or specific client_id is connected
// Server will try to set unique ID for each client

bool wsConnected();
bool wsConnected(uint32_t client_id);

// Append module's name that webui can make it's widgets visible
// (for the payload in `on_send` callback(s))

void wsPayloadModule(JsonObject& root, const char* const name);

// Access to our module-specific lifetime callbacks.
// Expected usage is through the on() methods

ws_callbacks_t& wsRegister();
void wsSetup();
