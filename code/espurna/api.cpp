/*

API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "api.h"

// -----------------------------------------------------------------------------

#if API_SUPPORT

#include "system.h"
#include "web.h"
#include "rpc.h"

#include <ESPAsyncTCP.h>
#include <ArduinoJson.h>

#include <algorithm>
#include <vector>
#include <forward_list>

// -----------------------------------------------------------------------------

struct Api {
    ApiPathListGenerator list;
    ApiPathGenerator generator;
};

std::forward_list<Api> _apis;

struct ApiPathElement {
    enum class Type {
        Unknown,
        Value,
        Placeholder
    };

    bool operator==(const ApiPathElement& other) const {
        if (other.type == Type::Placeholder) {
            return false;
        }

        if (type == Type::Placeholder) {
            return true;
        }

        return value == other.value;
    }

    bool operator!=(const ApiPathElement& other) const {
        return !(*this == other);
    }

    Type type;
    String value;
};

using ApiPath = std::vector<ApiPathElement>;

ApiPath _apiConstructPath(const String& pattern) {
    ApiPath result;
    result.reserve(std::count(pattern.begin(), pattern.end(), '/') + 1);

    String value;
    value.reserve(pattern.length());

    ApiPathElement::Type type { ApiPathElement::Type::Unknown };
    const char* p { pattern.c_str() };

parse_value:
    type = ApiPathElement::Type::Value;

    switch (*p) {
    case '/':
    case '\0':
        goto push_result;
    case '{':
        ++p;
        goto parse_placeholder;
    case '}':
        goto error;
    }

    value += *(p++);
    goto parse_value;

parse_placeholder:
    type = ApiPathElement::Type::Placeholder;

    switch (*p) {
    case '{':
    case '\0':
    case '/':
        goto error;
    case '}':
        switch (*(p + 1)) {
        case '/':
            p += 2;
            break;
        case '\0':
            p += 1;
            break;
        default:
            goto error;
        }

        goto push_result;
    }

    value += *(p++);
    goto parse_placeholder;

push_result:
    result.push_back({type, std::move(value)});
    type = ApiPathElement::Type::Unknown;
    if (*p == '/') {
        ++p;
        goto parse_value;
    } else if (*p != '\0') {
        goto parse_value;
    }
    goto return_result;

error:
    result.clear();

return_result:
    return result;
}

String repr(const ApiPath& path) {
    String result;
    result.reserve(128u);

    for (auto& elem : path) {
        result += '\'';
        result += elem.value;
        result += '\'';
        result += ' ';
    }

    return result;
}


struct ApiPathMatcher {

    bool match(const ApiPath& other) const {
        String lhs(repr(_path));
        String rhs(repr(other));
        DEBUG_MSG_P(PSTR("[API] %s (API) vs. %s (other)\n"),
            lhs.c_str(), rhs.c_str());

        if (!_path.size() || !other.size() || (_path.size() != other.size())) {
            DEBUG_MSG_P(PSTR("[API] sizes %u vs. %u mismatch\n"),
                _path.size(), other.size());
            return false;
        }

        for (unsigned index = 0; index < _path.size(); ++index) {
            if (_path[index] != other[index]) {
                DEBUG_MSG_P(PSTR("[API] path index %u mismatch (%s vs. %s)\n"),
                    index, _path[index].value.c_str(), other[index].value.c_str());
                return false;
            }
        }

        return true;
    }

    ApiHandle::PathParams params(const ApiPath& other) const {
        ApiHandle::PathParams result;

        if (!_path.size() || (_path.size() != other.size())) {
            return result;
        }

        auto lhs = _path.rbegin();
        auto rhs = other.rbegin();
        while (lhs != _path.rend() && rhs != other.rend()) {
            if ((*lhs).type == ApiPathElement::Type::Placeholder) {
                result.emplace_front((*lhs).value, (*rhs).value);
            }
            ++lhs;
            ++rhs;
        }

        return result;
    }

    private:

    ApiPath _path;
};

constexpr size_t ApiPathSizeMax { 128ul };

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
    ApiPathList list;
    for (auto& api : _apis) {
        api.list(list);
    }

    AsyncResponseStream *response = request->beginResponseStream("text/plain");
    for (auto& path : list) {
        DEBUG_MSG_P(PSTR("[api] sending list -> %s"), path.c_str());
        response->print(path);
        response->write("\r\n");
    }
    request->send(response);
}

constexpr size_t ApiJsonBufferSize = 1024;

void _onAPIsJson(AsyncWebServerRequest *request) {

    ApiPathList list;
    for (auto& api : _apis) {
        api.list(list);
    }

    DynamicJsonBuffer jsonBuffer(ApiJsonBufferSize);
    JsonArray& root = jsonBuffer.createArray();
    for (auto& path : list) {
        root.add(path);
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
    ApiPtr api;
    ApiType type;
};

ApiMatch _apiMatch(AsyncWebServerRequest* request, const String& path) {
    ApiMatch result;
    for (auto& api : _apis) {
        result.api = api.generator(path);
        if (result.api) {
            break;
        }
    }

    if (!result.api) {
        return result;
    }

    result.type = _asJson(request)
        ? ApiType::Json
        : ApiType::Basic;

    if ((ApiType::Json == result.type) && (!result.api->json)) {
        result.api = nullptr;
    }

    return result;
}

// Notice that we don't register handlers for specific paths, but functions that generate such handlers on demand
// This allows us to use some common code between HTTP and MQTT endpoints and delay RAM allocation until the API is actually used
//
// TODO: provide dynamic paths such as relay/{id:uint} and do implicit maching in API dispatch, storing given path variables as a map-like structure for the callback.
// however, unlike many other url matching libraries, we *will* allow exact same patterns for multiple routes, thus requiring the registree to return 'status' whether the request can actually be handled by it

void _apiDispatchRequest(AsyncWebServerRequest* request, const String& path) {
    if (!path.length()) {
        request->send(500);
        return;
    }

    auto method = request->method();
    if ((HTTP_PUT != method) && (HTTP_GET != method) && (HTTP_HEAD != method)) {
        DEBUG_MSG_P(PSTR("[API] Method not implemented\n"));
        request->send(501);
        return;
    }

    auto match = _apiMatch(request, path);
    if (!match.api) {
        DEBUG_MSG_P(PSTR("[API] No matching API found!\n"));
        request->send(404);
        return;
    }

    if (HTTP_HEAD == method) {
        request->send(204);
        return;
    }

    const bool is_put = (
        (!apiRestFul() || (HTTP_PUT == method))
        && request->hasParam("value", HTTP_PUT == method)
    );

    ApiHandle::PathParams params;
    ApiHandle handle(*request, std::move(params));

    switch (match.type) {

    case ApiType::Basic: {
        if (!match.api->get) {
            break;
        }

        ApiBuffer buffer;
        if (is_put) {
            if (!match.api->put) {
                break;
            }
            auto value = request->getParam("value", HTTP_PUT == method)->value();
            if (!buffer.copy(value.c_str(), value.length())) {
                break;
            }
            if (!match.api->put(handle, buffer)) {
                break;
            }
            buffer.clear();
        }

        if (!match.api->get(handle, buffer)) {
            break;
        }
        if (!handle.sent()) {
            request->send(200, "text/plain", buffer.data);
        }

        return;
    }

    // TODO: pass request body as well, allow PUT
    // TODO: can we get the body contents *without* body callback and when request is c-t:application/json?
    case ApiType::Json: {
        if (!match.api->json || is_put) {
            break;
        }

        DynamicJsonBuffer jsonBuffer(API_BUFFER_SIZE);
        JsonObject& root = jsonBuffer.createObject();
        if (!match.api->json(handle, root)) {
            break;
        }

        if (!handle.sent()) {
            AsyncResponseStream *response = request->beginResponseStream("application/json", root.measureLength() + 1);
            root.printTo(*response);
            request->send(response);
        }

        return;
    }

    }

    if (!handle.sent()) {
        DEBUG_MSG_P(PSTR("[API] Request was not handled\n"));
        request->send(500);
    }

    return;

}

bool _apiRequestCallback(AsyncWebServerRequest* request) {

    auto path = request->url();
    if (path.equals("/rpc")) {
        _onRPC(request);
        return true;
    }

    if (path.equals("/api") || path.equals("/apis")) {
        _onAPIs(request);
        return true;
    }

    if (!path.startsWith("/api/")) return false;
    if (!apiAuthenticate(request)) return false;

    _apiDispatchRequest(request, path.substring(strlen("/api/")));
    return true;

}

// -----------------------------------------------------------------------------

void apiRegister(ApiPathListGenerator list, ApiPathGenerator path) {
    Api api { std::move(list), std::move(path) };
    _apis.push_front(std::move(api));
}

void apiSetup() {
    webRequestRegister(_apiRequestCallback);
}

void apiOk(ApiHandle&, ApiBuffer& buffer) {
    buffer.data[0] = 'O';
    buffer.data[1] = 'K';
    buffer.data[2] = '\0';
}

void apiError(ApiHandle&, ApiBuffer& buffer) {
    buffer.data[0] = '-';
    buffer.data[1] = 'E';
    buffer.data[2] = 'R';
    buffer.data[3] = 'R';
    buffer.data[4] = 'O';
    buffer.data[5] = 'R';
    buffer.data[6] = '\0';
}

#endif // API_SUPPORT

