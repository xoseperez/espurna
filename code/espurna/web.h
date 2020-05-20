/*

WEBSERVER MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#if WEB_SUPPORT

#include <functional>
#include <list>
#include <vector>

#include <Print.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <FS.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

struct AsyncWebPrintBacklogConfig {
    const size_t countMax;
    const size_t sizeMax;
    const decltype(millis()) timeout;
};

constexpr AsyncWebPrintBacklogConfig AsyncWebPrintBacklogDefaults {
    /*countMax=*/ 2,
    /*sizeMax= */ TCP_MSS,
    /*timeout= */ 5000
};

struct AsyncWebPrint : public Print {

    enum class State {
        None,
        Sending,
        Done,
        Error
    };

    using BufferType = std::vector<uint8_t>;

    // to be able to safely output data right from the request callback
    template<typename CallbackType>
    static void scheduleFromRequest(const AsyncWebPrintBacklogConfig& config, AsyncWebServerRequest*, CallbackType);

    // to be able to safely output data right from the request callback
    template<typename CallbackType>
    static void scheduleFromRequest(AsyncWebServerRequest*, CallbackType);

    State getState();
    void setState(State);

    // note: existing implementation only expects this to be available via AsyncWebPrint
#if defined(ARDUINO_ESP8266_RELEASE_2_3_0)
    void flush();
#else
    void flush() final override;
#endif

    size_t write(uint8_t) final override;
    size_t write(const uint8_t *buffer, size_t size) final override;

    const size_t backlogCountMax;
    const size_t backlogSizeMax;
    const decltype(millis()) backlogTimeout;

    protected:

    std::list<BufferType> _buffers;
    AsyncWebServerRequest* const _request;
    State _state;
    bool _ready;
    bool _done;

    AsyncWebPrint(const AsyncWebPrintBacklogConfig&, AsyncWebServerRequest* req);

    bool _addBuffer();
    bool _exhaustBuffers();
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
