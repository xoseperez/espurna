/*

API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "api.h"

// -----------------------------------------------------------------------------

#if API_SUPPORT

#include <vector>

#include "system.h"
#include "web.h"
#include "rpc.h"

#include <ESPAsyncTCP.h>
#include <ArduinoJson.h>

constexpr size_t ApiPathSizeMax { 64ul };
std::vector<Api> _apis;

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

bool _asJson(AsyncWebServerRequest *request) {
    bool asJson = false;
    if (request->hasHeader("Accept")) {
        AsyncWebHeader* h = request->getHeader("Accept");
        asJson = h->value().equals("application/json");
    }
    return asJson;
}

void _onAPIsText(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/plain");
    char buffer[ApiPathSizeMax] = {0};
    for (auto& api : _apis) {
        sprintf_P(buffer, PSTR("/api/%s\n"), api.path.c_str());
        response->write(buffer);
    }
    request->send(response);
}

constexpr size_t ApiJsonBufferSize = 1024;

void _onAPIsJson(AsyncWebServerRequest *request) {

    DynamicJsonBuffer jsonBuffer(ApiJsonBufferSize);
    JsonArray& root = jsonBuffer.createArray();

    char buffer[ApiPathSizeMax] = {0};
    for (auto& api : _apis) {
        sprintf(buffer, "/api/%s", api.path.c_str());
        root.add(buffer);
    }

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    root.printTo(*response);
    request->send(response);

}

void _onAPIs(AsyncWebServerRequest *request) {

    webLog(request);
    if (!apiAuthenticate(request)) return;

    bool asJson = _asJson(request);

    String output;
    if (asJson) {
        _onAPIsJson(request);
    } else {
        _onAPIsText(request);
    }

}

void _onRPC(AsyncWebServerRequest *request) {

    webLog(request);
    if (!apiAuthenticate(request)) return;

    //bool asJson = _asJson(request);
    int response = 404;

    if (request->hasParam("action")) {

        AsyncWebParameter* p = request->getParam("action");

        const auto action = p->value();
        DEBUG_MSG_P(PSTR("[RPC] Action: %s\n"), action.c_str());

        if (rpcHandleAction(action)) {
            response = 204;
        }

    }

    request->send(response);

}

struct ApiMatch {
    Api* api { nullptr };
    Api::Type type { Api::Type::Basic };
};

ApiMatch _apiMatch(const String& url, AsyncWebServerRequest* request) {

    ApiMatch result;
    char buffer[ApiPathSizeMax] = {0};

    for (auto& api : _apis) {
        sprintf_P(buffer, PSTR("/api/%s"), api.path.c_str());
        if (url != buffer) {
            continue;
        }

        auto type = _asJson(request)
            ? Api::Type::Json
            : Api::Type::Basic;

        result.api = &api;
        result.type = type;
        break;
    }

    return result;
}

bool _apiDispatchRequest(const String& url, AsyncWebServerRequest* request) {

    auto match = _apiMatch(url, request);
    if (!match.api) {
        return false;
    }

    if (match.type != match.api->type) {
        DEBUG_MSG_P(PSTR("[API] Cannot handle the request type\n"));
        request->send(404);
        return true;
    }

    const bool is_put = (
        (!apiRestFul() || (request->method() == HTTP_PUT))
        && request->hasParam("value", request->method() == HTTP_PUT)
    );

    ApiBuffer buffer;

    switch (match.api->type) {

    case Api::Type::Basic: {
        if (!match.api->get.basic) {
            break;
        }

        if (is_put) {
            if (!match.api->put.basic) {
                break;
            }
            auto value = request->getParam("value", request->method() == HTTP_PUT)->value();
            if (buffer.size < (value.length() + 1ul)) {
                break;
            }
            std::copy(value.c_str(), value.c_str() + value.length() + 1, buffer.data);
            match.api->put.basic(*match.api, buffer);
            buffer.erase();
        }

        match.api->get.basic(*match.api, buffer);
        request->send(200, "text/plain", buffer.data);

        return true;
    }

    // TODO: pass the body instead of `value` param
    // TODO: handle HTTP_PUT
    case Api::Type::Json: {
        if (!match.api->get.json || is_put) {
            break;
        }

        DynamicJsonBuffer jsonBuffer(API_BUFFER_SIZE);
        JsonObject& root = jsonBuffer.createObject();

        match.api->get.json(*match.api, root);

        AsyncResponseStream *response = request->beginResponseStream("application/json", root.measureLength() + 1);
        root.printTo(*response);
        request->send(response);

        return true;
    }

    }

    DEBUG_MSG_P(PSTR("[API] Method not supported\n"));
    request->send(405);

    return true;

}

bool _apiRequestCallback(AsyncWebServerRequest* request) {

    String url = request->url();

    if (url.equals("/rpc")) {
        _onRPC(request);
        return true;
    }

    if (url.equals("/api") || url.equals("/apis")) {
        _onAPIs(request);
        return true;
    }

    if (!url.startsWith("/api/")) return false;

// [alexa] don't call the http api -> response for alexa is done by fauxmoesp lib
#if ALEXA_SUPPORT
    if (url.indexOf("/lights") > 14 ) return false;
#endif

    if (!apiAuthenticate(request)) return false;

    return _apiDispatchRequest(url, request);

}

// -----------------------------------------------------------------------------

void apiReserve(size_t size) {
    _apis.reserve(_apis.size() + size);
}

void apiRegister(const Api& api) {
    if (api.path.length() >= (ApiPathSizeMax - strlen("/api/") - 1ul)) {
        return;
    }
    _apis.push_back(api);
}

void apiSetup() {
    webRequestRegister(_apiRequestCallback);
}

void apiOk(const Api&, ApiBuffer& buffer) {
    buffer.data[0] = 'O';
    buffer.data[1] = 'K';
    buffer.data[2] = '\0';
}

void apiError(const Api&, ApiBuffer& buffer) {
    buffer.data[0] = '-';
    buffer.data[1] = 'E';
    buffer.data[2] = 'R';
    buffer.data[3] = 'R';
    buffer.data[4] = 'O';
    buffer.data[5] = 'R';
    buffer.data[6] = '\0';
}

#endif // API_SUPPORT

