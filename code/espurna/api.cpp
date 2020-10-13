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
#include <cstring>
#include <forward_list>
#include <vector>

// -----------------------------------------------------------------------------

struct Api {
    String path;
    ApiHandler handler;
};

struct ApiParsedPath {

    // given the path 'topic/one/two/three' and pattern 'topic/+/two/+', provide
    // wildcards [0] -> one and [1] -> three
    struct Match {
        explicit Match(const ApiParsedPath& other) :
            path(other.path()),
            levels(other.levels()),
            wildcards(other.path())
        {}

        explicit operator bool() const {
            return ok;
        }

        const String& path;
        const ApiLevels& levels;
        ApiLevels wildcards;
        bool ok { false };
    };

    template <typename T>
    explicit ApiParsedPath(T&& path) :
        _path(std::forward<T>(path)),
        _levels(_path)
    {
        init(); 
    }

    void init() {
        ApiLevel::Type type { ApiLevel::Type::Unknown };
        uint32_t length { 0ul };
        uint32_t offset { 0ul };

        const char* p { _path.c_str() };
        if (*p == '\0') {
           goto error;
        } 

        _levels.reserve(std::count(_path.begin(), _path.end(), '/') + 1);

    start:
        type = ApiLevel::Type::Unknown;
        length = 0;
        offset = p - _path.c_str();

        switch (*p) {
        case '/':
            goto push_result;
        case '+':
            goto parse_single_wildcard;
        case '#':
            goto parse_multi_wildcard;
        default:
            goto parse_value;
        }

    parse_value:
        type = ApiLevel::Type::Value;

        switch (*p) {
        case '+':
        case '#':
            goto error;
        case '/':
        case '\0':
            goto push_result;
        }

        ++p;
        ++length;

        goto parse_value;

    parse_single_wildcard:
        type = ApiLevel::Type::SingleWildcard;

        ++p;
        switch (*p) {
        case '/':
            ++p;
        case '\0':
            goto push_result;
        }

        goto error;

    parse_multi_wildcard:
        type = ApiLevel::Type::MultiWildcard;

        ++p;
        if (*p == '\0') {
            goto push_result;
        }
        goto error;

    push_result:
        _levels.emplace_back(type, offset, length);
        if (*p == '/') {
            ++p;
            goto start;
        } else if (*p != '\0') {
            goto start;
        }
        goto success;

    error:
        _levels.clear();
        _ok = false;

    success:
        _ok = true;
    }

    explicit operator bool() {
        return _ok;
    }

    Match match(const String& other) const {
        return match(ApiParsedPath(other));
    }

    Match match(const ApiParsedPath& other) const {
        Match result(other);

        auto lhs = _levels.begin();
        auto rhs = other._levels.begin();

        if (!_ok) {
            goto error;
        }

    loop:
        switch ((*lhs).type) {
        case ApiLevel::Type::Value:
            if (
                (rhs != other._levels.end())
                && ((*rhs).type == ApiLevel::Type::Value)
                && ((*rhs).offset == (*lhs).offset)
                && ((*rhs).length == (*lhs).length)
            ) {
                if (0 == std::memcmp(
                    _path.c_str() + (*lhs).offset,
                    other._path.c_str() + (*rhs).offset,
                    (*rhs).length))
                {
                    ++lhs;
                    ++rhs;
                    goto loop;
                }
            }
            goto error;

        case ApiLevel::Type::SingleWildcard:
            if (
                (rhs != other._levels.end())
                && ((*rhs).type == ApiLevel::Type::Value)
            ) {
                result.wildcards.emplace_back((*rhs).type, (*rhs).offset, (*rhs).length);
                DEBUG_MSG_P(PSTR("[API] + %u byte(s) at pos %u\n"), (*rhs).length, (*rhs).offset);
                DEBUG_MSG_P(PSTR("[API] =toString() %s\n"), result.wildcards.levels().back().toString().c_str());
                DEBUG_MSG_P(PSTR("[API] =c_str() %s\n"), result.wildcards.path().c_str());
                DEBUG_MSG_P(PSTR("[API] =c_str()+ %s\n"), result.wildcards.path().c_str() + (*rhs).offset);
                std::advance(lhs, 1);
                std::advance(rhs, 1);
                goto loop;
            }
            goto error;

        case ApiLevel::Type::MultiWildcard:
            if (std::next(lhs) == _levels.end()) {
                while (rhs != other._levels.end()) {
                    if ((*rhs).type != ApiLevel::Type::Value) {
                        goto error;
                    }
                    result.wildcards.emplace_back((*rhs).type, (*rhs).offset, (*rhs).length);
                    std::advance(rhs, 1);
                }
                lhs = _levels.end();
                break;
            }
            goto error;

        case ApiLevel::Type::Unknown:
            goto error;
        };

        if (lhs == _levels.end() && rhs == other._levels.end()) {
            result.ok = true;
            goto return_result;
        }

    error:
        result.ok = false;
        result.wildcards.clear();

    return_result:
        return result;
    }

    const String& path() const {
        return _path;
    }

    const ApiLevels& levels() const {
        return _levels;
    }

private:

    String _path;
    ApiLevels _levels;
    bool _ok { false };

};

std::forward_list<Api> _apis;

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
    AsyncResponseStream *response = request->beginResponseStream("text/plain");
    for (auto& api : _apis) {
        response->print(api.path);
        response->write("\r\n");
    }
    request->send(response);
}

constexpr size_t ApiJsonBufferSize = 1024;

void _onAPIsJson(AsyncWebServerRequest *request) {

    DynamicJsonBuffer jsonBuffer(ApiJsonBufferSize);
    JsonArray& root = jsonBuffer.createArray();
    for (auto& api : _apis) {
        root.add(api.path);
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

enum class ApiType {
    Unknown,
    Basic,
    Json
};

struct ApiMatch {
    ApiMatch(const Api& api, ApiType type, const ApiParsedPath::Match& match) :
        _type(type),
        _handler(api.handler),
        _path(match.path),
        _levels(match.levels),
        _wildcards(match.wildcards)
    {}

    ApiType type() const {
        return _type;
    }

    const ApiHandler& handler() const {
        return _handler;
    }

    const String& path() const {
        return _path;
    }

    const ApiLevels& levels() const {
        return _levels;
    }

    const ApiLevels& wildcards() const {
        return _wildcards;
    }

private:
    ApiType _type { ApiType::Unknown };
    const ApiHandler& _handler;

    const String& _path;
    ApiLevels _levels;
    ApiLevels _wildcards;
};

std::unique_ptr<ApiMatch> _apiMatch(AsyncWebServerRequest* request, const ApiParsedPath& path) {
    auto type = _asJson(request)
        ? ApiType::Json
        : ApiType::Basic;

    auto can_handle = [](ApiType type, const ApiHandler& handler) {
        switch (type) {
        case ApiType::Basic:
            if (handler.put || handler.get) {
                return true;
            }
            break;
        case ApiType::Json:
            if (handler.json) {
                return true;
            }
            break;
        case ApiType::Unknown:
        default:
            break;
        }

        return false;
    };

    std::unique_ptr<ApiMatch> result;

    for (auto& api : _apis) {
        if (!can_handle(type, api.handler)) {
            continue;
        }

        auto pattern = ApiParsedPath(api.path);
        auto match = pattern.match(path);
        if (match) {
            result = std::make_unique<ApiMatch>(api, type, match);
            break;
        }
    }

    return result;
}

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

    auto parsed_path = ApiParsedPath(path);
    auto match = _apiMatch(request, parsed_path);
    if (!match) {
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

    ApiRequest api_req(*request, match->levels(), match->wildcards());

    auto& handler = match->handler();

    switch (match->type()) {
    case ApiType::Basic: {
        if (!handler.get) {
            break;
        }

        ApiBuffer buffer;
        if (is_put) {
            if (!handler.put) {
                break;
            }
            auto value = request->getParam("value", HTTP_PUT == method)->value();
            if (!buffer.copy(value.c_str(), value.length())) {
                break;
            }
            if (!handler.put(api_req, buffer)) {
                break;
            }
            buffer.clear();
        }

        if (!handler.get(api_req, buffer)) {
            break;
        }
        if (!api_req.sent()) {
            request->send(200, "text/plain", buffer.data);
        }

        return;
    }

    // TODO: pass request body as well, allow PUT
    // TODO: can we get the body contents *without* body callback and when request is c-t:application/json?
    case ApiType::Json: {
        if (!handler.json || is_put) {
            break;
        }

        DynamicJsonBuffer jsonBuffer(API_BUFFER_SIZE);
        JsonObject& root = jsonBuffer.createObject();
        if (!handler.json(api_req, root)) {
            break;
        }

        if (!api_req.sent()) {
            AsyncResponseStream *response = request->beginResponseStream("application/json", root.measureLength() + 1);
            root.printTo(*response);
            request->send(response);
        }

        return;
    }

    case ApiType::Unknown:
        break;
    }

    if (!api_req.sent()) {
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

void apiRegister(const String& path, ApiHandler handler) {
    Api api { path, std::move(handler) };
    _apis.push_front(std::move(api));
}

void apiSetup() {
    webRequestRegister(_apiRequestCallback);
}

bool apiOk(ApiRequest&, ApiBuffer& buffer) {
    buffer.data[0] = 'O';
    buffer.data[1] = 'K';
    buffer.data[2] = '\0';
    return true;
}

bool apiError(ApiRequest&, ApiBuffer& buffer) {
    buffer.data[0] = '-';
    buffer.data[1] = 'E';
    buffer.data[2] = 'R';
    buffer.data[3] = 'R';
    buffer.data[4] = 'O';
    buffer.data[5] = 'R';
    buffer.data[6] = '\0';
    return true;
}

#endif // API_SUPPORT

