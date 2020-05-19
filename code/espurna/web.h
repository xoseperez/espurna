/*

WEBSERVER MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#if WEB_SUPPORT

#include <functional>
#include <list>
#include <memory>
#include <vector>

#include <Print.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <FS.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

struct AsyncWebPrint : public Print, public std::enable_shared_from_this<AsyncWebPrint> {

    enum class State {
        None,
        Sending,
        Done,
        Error
    };

    constexpr static size_t BacklogMax = 2;
    using BufferType = std::vector<uint8_t>;

    AsyncWebPrint(AsyncWebServerRequest* req);

    State getState();
    void setState(State);

    void flush() final override;
    size_t write(uint8_t) final override;
    size_t write(const uint8_t *buffer, size_t size) final override;

    protected:

    std::list<BufferType> _buffers;
    AsyncWebServerRequest* const _request;
    State _state;
    bool _ready;
    bool _done;

    void _exhaustBuffers();
    void _addBuffer();
    void _prepareRequest();

};

using web_body_callback_f = std::function<bool(AsyncWebServerRequest*, uint8_t* data, size_t len, size_t index, size_t total)>;
using web_request_callback_f = std::function<bool(AsyncWebServerRequest*)>;

AsyncWebServer* webServer();

bool webAuthenticate(AsyncWebServerRequest *request);
void webLog(AsyncWebServerRequest *request);

void webBodyRegister(web_body_callback_f);
void webRequestRegister(web_request_callback_f);

uint16_t webPort();
void webSetup();

#endif // WEB_SUPPORT == 1
