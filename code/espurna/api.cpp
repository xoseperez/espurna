/*

API & WEB API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "api.h"

// -----------------------------------------------------------------------------

#include "system.h"
#include "rpc.h"

#if WEB_SUPPORT
#include "web.h"
#include <ESPAsyncTCP.h>
#include <ArduinoJson.h>
#endif

#include <algorithm>
#include <memory>
#include <cstring>
#include <forward_list>
#include <vector>

// -----------------------------------------------------------------------------

PathParts::PathParts(const String& path) :
    _path(path)
{
    if (!_path.length()) {
        _ok = false;
        return;
    }

    PathPart::Type type { PathPart::Type::Unknown };
    size_t length { 0ul };
    size_t offset { 0ul };

    const char* p { _path.c_str() };
    if (*p == '\0') {
       goto error;
    }

    _parts.reserve(std::count(_path.begin(), _path.end(), '/') + 1);

start:
    type = PathPart::Type::Unknown;
    length = 0;
    offset = p - _path.c_str();

    switch (*p) {
    case '+':
        goto parse_single_wildcard;
    case '#':
        goto parse_multi_wildcard;
    case '/':
    default:
        goto parse_value;
    }

parse_value:
    type = PathPart::Type::Value;

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
    type = PathPart::Type::SingleWildcard;

    ++p;
    switch (*p) {
    case '/':
        ++p;
    case '\0':
        goto push_result;
    }

    goto error;

parse_multi_wildcard:
    type = PathPart::Type::MultiWildcard;

    ++p;
    if (*p == '\0') {
        goto push_result;
    }
    goto error;

push_result:
    emplace_back(type, offset, length);
    if (*p == '/') {
        ++p;
        goto start;
    } else if (*p != '\0') {
        goto start;
    }
    goto success;

error:
    _ok = false;
    _parts.clear();
    return;

success:
    _ok = true;
}

// match when, for example, given the path 'topic/one/two/three' and pattern 'topic/+/two/+'

bool PathParts::match(const PathParts& path) const {
    if (!_ok || !path) {
        return false;
    }

    auto lhs = begin();
    auto lhs_end = end();

    auto rhs = path.begin();
    auto rhs_end = path.end();
loop:
    if (lhs == lhs_end) {
        goto check_end;
    }

    switch ((*lhs).type) {
    case PathPart::Type::Value:
        if (
            (rhs != rhs_end)
            && ((*rhs).type == PathPart::Type::Value)
            && ((*rhs).length == (*lhs).length)
        ) {
            if (0 == std::memcmp(
                _path.c_str() + (*lhs).offset,
                path.path().c_str() + (*rhs).offset,
                (*rhs).length))
            {
                std::advance(lhs, 1);
                std::advance(rhs, 1);
                goto loop;
            }
        }
        goto error;

    case PathPart::Type::SingleWildcard:
        if (
            (rhs != rhs_end)
            && ((*rhs).type == PathPart::Type::Value)
        ) {
            std::advance(lhs, 1);
            std::advance(rhs, 1);
            goto loop;
        }
        goto error;

    case PathPart::Type::MultiWildcard:
        if (std::next(lhs) == lhs_end) {
            while (rhs != rhs_end) {
                if ((*rhs).type != PathPart::Type::Value) {
                    goto error;
                }
                std::advance(rhs, 1);
            }
            lhs = lhs_end;
            break;
        }
        goto error;

    case PathPart::Type::Unknown:
        goto error;
    };

check_end:
    if ((lhs == lhs_end) && (rhs == rhs_end)) {
        return true;
    }

error:
    return false;
}

#if WEB_SUPPORT

String ApiRequest::wildcard(int index) const {
    if (index < 0) {
        index = std::abs(index + 1);
    }

    if (std::abs(index) >= _pattern.parts().size()) {
        return _empty_string();
    }

    int counter { 0 };
    auto& pattern = _pattern.parts();

    for (unsigned int part = 0; part < pattern.size(); ++part) {
        auto& lhs = pattern[part];
        if (PathPart::Type::SingleWildcard == lhs.type) {
            if (counter == index) {
                auto& rhs = _parts.parts()[part];
                return _parts.path().substring(rhs.offset, rhs.offset + rhs.length);
            }
            ++counter;
        }
    }

    return _empty_string();
}

size_t ApiRequest::wildcards() const {
    size_t result { 0ul };
    for (auto& part : _pattern) {
        if (PathPart::Type::SingleWildcard == part.type) {
            ++result;
        }
    }

    return result;
}

#endif

// -----------------------------------------------------------------------------

#if API_SUPPORT

namespace {

bool _apiAccepts(AsyncWebServerRequest* request, const __FlashStringHelper* str) {
    auto* header = request->getHeader(F("Accept"));
    if (!header) {
        return true;
    }

    return (header->value().indexOf(F("*/*")) >= 0)
        || (header->value().indexOf(str) >= 0);
}

bool _apiAcceptsText(AsyncWebServerRequest* request) {
    return _apiAccepts(request, F("text/plain"));
}

bool _apiAcceptsJson(AsyncWebServerRequest* request) {
    return _apiAccepts(request, F("application/json"));
}

bool _apiIsContentType(AsyncWebServerRequest* request, const char* value) {
    const auto& type = request->contentType();
    return strncmp_P(type.c_str(), value, type.length()) == 0;
}

bool _apiIsFormDataContent(AsyncWebServerRequest* request) {
    return _apiIsContentType(request, PSTR("application/x-www-form-urlencoded"));
}

bool _apiIsJsonContent(AsyncWebServerRequest* request) {
    return _apiIsContentType(request, PSTR("application/json"));
}

// Because the webserver request is split between multiple separate function invocations, we need to preserve some state.
// TODO: in case we are dealing with multicore, perhaps enforcing static-size data structs instead of the vector would we better,
//       to avoid calling generic malloc when paths are parsed?
//
// Some quirks to deal with:
// - handleBody is called before handleRequest, and there's no way to signal completion / success of both callbacks to the server
// - Server never checks for request closing in filter or canHandle, so if we don't want to handle large content-length, it
//   will still flow through the lwip backend.
// - `request->_tempObject` is used to keep API request state, but it's just a plain void pointer
// - `request->send(..., payload)` creates a heap-allocated `reponse` object that will copy the payload and tracks it by a basic pointer.
//   In case we call `request->send` a 2nd time (regardless of the type of the send()), it creates a 2nd object without de-allocating the 1st one.
// - espasyncwebserver will `free(_tempObject)` when request is disconnected, but only after this callbackhandler is done.
//   make sure it's set to nullptr via `AsyncWebServerRequest::onDisconnect`
// - ALL headers are parsed (and we could access those during filter and canHandle callbacks), but we need to explicitly
//   request them to stay in memory so that the actual handler can work with them

void _apiAttachHelper(AsyncWebServerRequest& request, ApiRequestHelper&& helper) {
    request._tempObject = new ApiRequestHelper(std::move(helper));
    request.onDisconnect([&]() {
        auto* ptr = reinterpret_cast<ApiRequestHelper*>(request._tempObject);
        delete ptr;
        request._tempObject = nullptr;
    });
    request.addInterestingHeader(F("Api-Key"));
    request.addInterestingHeader(F("Accept"));
}

class ApiBaseWebHandler : public AsyncWebHandler {
public:
    ApiBaseWebHandler() = delete;
    ApiBaseWebHandler(const ApiBaseWebHandler&) = delete;
    ApiBaseWebHandler(ApiBaseWebHandler&&) = delete;

    // In case this needs to be copied or moved, ensure PathParts copy references the new object's string

    template <typename Pattern>
    explicit ApiBaseWebHandler(Pattern&& pattern) :
        _pattern(std::forward<Pattern>(pattern)),
        _parts(_pattern)
    {}

    const String& pattern() const {
        return _pattern;
    }

    const PathParts& parts() const {
        return _parts;
    }

private:
    String _pattern;
    PathParts _parts;
};

// 'Modernized' API configuration:
// - `Api-Key` header for both GET and PUT
// - Parse request body as JSON object. Limited to LWIP internal buffer size, and will also break when client
//   does weird stuff and PUTs data in multiple packets b/c only the initial packet is parsed.
// - Same as the text/plain, when ApiRequest::handle was not called it will then call GET
//
// TODO: bump to arduinojson v6 to handle partial / broken data payloads
// TODO: somehow detect partial data and buffer (optionally)
// TODO: POST instead of PUT?

class ApiJsonWebHandler final : public ApiBaseWebHandler {
public:
    static constexpr size_t BufferSize { API_JSON_BUFFER_SIZE };

    struct ReadOnlyStream : public Stream {
        ReadOnlyStream() = delete;
        explicit ReadOnlyStream(const uint8_t* buffer, size_t size) :
            _buffer(buffer),
            _size(size)
        {}

        int available() override {
            return _size - _index;
        }

        int peek() override {
            if (_index < _size) {
                return static_cast<int>(_buffer[_index]);
            }

            return -1;
        }

        int read() override {
            auto peeked = peek();
            if (peeked >= 0) {
                ++_index;
            }

            return peeked;
        }

        // since we are fixed in size, no need for any timeouts and the only available option is to return full chunk of data
        size_t readBytes(uint8_t* ptr, size_t size) override {
            if ((_index < _size) && ((_size - _index) >= size)) {
                std::copy(_buffer + _index, _buffer + _index + size, ptr);
                _index += size;
                return size;
            }

            return 0;
        }

        size_t readBytes(char* ptr, size_t size) override {
			return readBytes(reinterpret_cast<uint8_t*>(ptr), size);
		}

        void flush() override {
        }

        size_t write(const uint8_t*, size_t) override {
            return 0;
        }

        size_t write(uint8_t) override {
            return 0;
        }

        const uint8_t* _buffer;
        const size_t _size;
        size_t _index { 0 };
    };

    ApiJsonWebHandler() = delete;
    ApiJsonWebHandler(const ApiJsonWebHandler&) = delete;
    ApiJsonWebHandler(ApiJsonWebHandler&&) = delete;

    template <typename Path, typename Callback>
    ApiJsonWebHandler(Path&& path, Callback&& get, Callback&& put) :
        ApiBaseWebHandler(std::forward<Path>(path)),
        _get(std::forward<Callback>(get)),
        _put(std::forward<Callback>(put))
    {}

    bool isRequestHandlerTrivial() override {
        return true;
    }

    bool canHandle(AsyncWebServerRequest* request) override {
        if (!apiEnabled()) {
            return false;
        }

        auto helper = ApiRequestHelper(*request, parts());
        if (helper.match() && apiAuthenticate(request)) {
            switch (request->method()) {
            case HTTP_HEAD:
                return true;
            case HTTP_PUT:
                if (!_apiIsJsonContent(request)) {
                    return false;
                }
                if (!_put) {
                    return false;
                }
                // fallthrough!
            case HTTP_GET:
                if (!_get) {
                    return false;
                }
                break;
            default:
                return false;
            }
            _apiAttachHelper(*request, std::move(helper));
            return true;
        }

        return false;
    }

    void _handleGet(AsyncWebServerRequest* request, ApiRequest& apireq) {
        DynamicJsonBuffer jsonBuffer(API_JSON_BUFFER_SIZE);
        JsonObject& root = jsonBuffer.createObject();
        if (!_get(apireq, root)) {
            request->send(500);
            return;
        }

        if (!apireq.done()) {
            AsyncResponseStream *response = request->beginResponseStream("application/json", root.measureLength() + 1);
            root.printTo(*response);
            request->send(response);
            return;
        }

        request->send(500);
    }

    void _handlePut(AsyncWebServerRequest* request, uint8_t* data, size_t size) {
        // XXX: arduinojson v5 de-serializer will happily read garbage from raw ptr, since there's no length limit
        //      this is fixed in v6 though. for now, use a wrapper, but be aware that this actually uses more mem for the jsonbuffer
        DynamicJsonBuffer jsonBuffer(API_JSON_BUFFER_SIZE);
        ReadOnlyStream stream(data, size);

        JsonObject& root = jsonBuffer.parseObject(stream);
        if (!root.success()) {
            request->send(500);
            return;
        }

        auto& helper = *reinterpret_cast<ApiRequestHelper*>(request->_tempObject);

        auto apireq = helper.request();
        if (!_put(apireq, root)) {
            request->send(500);
            return;
        }

        if (!apireq.done()) {
            _handleGet(request, apireq);
        }

        return;
    }

    void handleBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t total) override {
        if (total && (len == total)) {
            _handlePut(request, data, total);
        }
    }

    void handleRequest(AsyncWebServerRequest* request) override {
        if (!_apiAcceptsJson(request)) {
            request->send(406, F("text/plain"), F("application/json"));
            return;
        }

        auto& helper = *reinterpret_cast<ApiRequestHelper*>(request->_tempObject);

        switch (request->method()) {
        case HTTP_HEAD:
            request->send(204);
            return;

        case HTTP_GET: {
            auto apireq = helper.request();
            _handleGet(request, apireq);
            return;
        }

        // see handleBody()
        case HTTP_PUT:
            break;

        default:
            request->send(405);
            break;
        }
    }

    const String& pattern() const {
        return ApiBaseWebHandler::pattern();
    }

    const PathParts& parts() const {
        return ApiBaseWebHandler::parts();
    }

private:
    ApiJsonHandler _get;
    ApiJsonHandler _put;
};

// ESPurna legacy API configuration
// - ?apikey=... to authorize in GET or PUT
// - ?anything=... for input data (common key is "value")
// MUST correctly override isRequestHandlerTrivial() to allow auth with PUT
// (i.e. so that ESPAsyncWebServer parses the body and adds form-data to request params list)

class ApiBasicWebHandler final : public ApiBaseWebHandler {
public:
    template <typename Path, typename Callback>
    ApiBasicWebHandler(Path&& path, Callback&& get, Callback&& put) :
        ApiBaseWebHandler(std::forward<Path>(path)),
        _get(std::forward<Callback>(get)),
        _put(std::forward<Callback>(put))
    {}

    bool isRequestHandlerTrivial() override {
        return false;
    }

    bool canHandle(AsyncWebServerRequest* request) override {
        if (!apiEnabled()) {
            return false;
        }

        switch (request->method()) {
        case HTTP_HEAD:
        case HTTP_GET:
            break;
        case HTTP_PUT:
            if (!_apiIsFormDataContent(request)) {
                return false;
            }
            break;
        default:
            return false;
        }

        auto helper = ApiRequestHelper(*request, parts());
        if (helper.match()) {
            _apiAttachHelper(*request, std::move(helper));
            return true;
        }

        return false;
    }

    void handleRequest(AsyncWebServerRequest* request) override {
        if (!apiAuthenticate(request)) {
            request->send(403);
            return;
        }

        if (!_apiAcceptsText(request)) {
            request->send(406, F("text/plain"), F("text/plain"));
            return;
        }

        auto method = request->method();
        const bool is_put = (
            (!apiRestFul()|| (HTTP_PUT == method))
            && request->hasParam("value", HTTP_PUT == method)
        );

        switch (method) {
        case HTTP_HEAD:
            request->send(204);
            return;

        case HTTP_GET:
        case HTTP_PUT: {
            auto& helper = *reinterpret_cast<ApiRequestHelper*>(request->_tempObject);

            auto apireq = helper.request();
            if (is_put) {
                if (!_put(apireq)) {
                    request->send(500);
                    return;
                }

                if (apireq.done()) {
                    return;
                }
            }

            if (!_get(apireq)) {
                request->send(500);
                return;
            }

            if (!apireq.done()) {
                request->send(204);
                return;
            }

            break;
        }

        default:
            request->send(405);
            break;
        }
    }

    const ApiBasicHandler& get() const {
        return _get;
    }

    const ApiBasicHandler& put() const {
        return _put;
    }

    const String& pattern() const {
        return ApiBaseWebHandler::pattern();
    }

    const PathParts& parts() const {
        return ApiBaseWebHandler::parts();
    }

private:
    ApiBasicHandler _get;
    ApiBasicHandler _put;
};

std::forward_list<ApiBaseWebHandler*> _apis;

template <typename Handler, typename Callback>
void _apiRegister(const String& path, Callback&& get, Callback&& put) {
    // `String` is a given, since we *do* need to construct this dynamically in sensors
    auto* ptr = new Handler(String(F(API_BASE_PATH)) + path, std::forward<Callback>(get), std::forward<Callback>(put));
    webServer().addHandler(reinterpret_cast<AsyncWebHandler*>(ptr));
    _apis.emplace_front(ptr);
}

} // namespace

// -----------------------------------------------------------------------------

void apiRegister(const String& path, ApiBasicHandler&& get, ApiBasicHandler&& put) {
    _apiRegister<ApiBasicWebHandler>(path, std::move(get), std::move(put));
}

void apiRegister(const String& path, ApiJsonHandler&& get, ApiJsonHandler&& put) {
    _apiRegister<ApiJsonWebHandler>(path, std::move(get), std::move(put));
}

void apiSetup() {
    apiRegister(F("list"),
        [](ApiRequest& request) {
            String paths;
            for (auto& api : _apis) {
                paths += api->pattern() + "\r\n";
            }
            request.send(paths);
            return true;
        },
        nullptr
    );

    apiRegister(F("rpc"),
        nullptr,
        [](ApiRequest& request) {
            if (rpcHandleAction(request.param(F("action")))) {
                return apiOk(request);
            }
            return apiError(request);
        }
    );

}

bool apiOk(ApiRequest& request) {
    request.send(F("OK"));
    return true;
}

bool apiError(ApiRequest& request) {
    request.send(F("ERROR"));
    return true;
}

#endif // API_SUPPORT
