/*

WEBSOCKET MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#include <ArduinoJson.h>

#include <queue>
#include <functional>
#include <vector>

#include "web.h"
#include "utils.h"

using ws_on_send_callback_f = std::function<void(JsonObject& root)>;
using ws_on_action_callback_f = std::function<void(uint32_t client_id, const char * action, JsonObject& data)>;
using ws_on_keycheck_callback_f = std::function<bool(const char * key, JsonVariant& value)>;

using ws_on_send_callback_list_t = std::vector<ws_on_send_callback_f>;
using ws_on_action_callback_list_t = std::vector<ws_on_action_callback_f>;
using ws_on_keycheck_callback_list_t = std::vector<ws_on_keycheck_callback_f>;

struct ws_callbacks_t {
    ws_on_send_callback_list_t on_visible;
    ws_on_send_callback_list_t on_connected;
    ws_on_send_callback_list_t on_data;

    ws_on_action_callback_list_t on_action;
    ws_on_keycheck_callback_list_t on_keycheck;

    ws_callbacks_t& onVisible(ws_on_send_callback_f);
    ws_callbacks_t& onConnected(ws_on_send_callback_f);
    ws_callbacks_t& onData(ws_on_send_callback_f);
    ws_callbacks_t& onAction(ws_on_action_callback_f);
    ws_callbacks_t& onKeyCheck(ws_on_keycheck_callback_f);
};

ws_callbacks_t& wsRegister();

void wsSetup();
void wsSend(uint32_t client_id, const char* data);
void wsSend(uint32_t client_id, JsonObject& root);
void wsSend(JsonObject& root);
void wsSend(ws_on_send_callback_f callback);
void wsSend(const char* data);

void wsSend_P(PGM_P data);
void wsSend_P(uint32_t client_id, PGM_P data);

void wsPost(const ws_on_send_callback_f& callback);
void wsPost(uint32_t client_id, const ws_on_send_callback_f& callback);
void wsPost(const ws_on_send_callback_list_t& callbacks);
void wsPost(uint32_t client_id, const ws_on_send_callback_list_t& callbacks);

void wsPostAll(uint32_t client_id, const ws_on_send_callback_list_t& callbacks);
void wsPostAll(const ws_on_send_callback_list_t& callbacks);

void wsPostSequence(uint32_t client_id, const ws_on_send_callback_list_t& callbacks);
void wsPostSequence(uint32_t client_id, ws_on_send_callback_list_t&& callbacks);
void wsPostSequence(const ws_on_send_callback_list_t& callbacks);

bool wsConnected();
bool wsConnected(uint32_t client_id);
bool wsDebugSend(const char* prefix, const char* message);
