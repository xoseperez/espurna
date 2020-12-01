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
    auto rhs = path.begin();

    auto lhs_end = end();
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
            && ((*rhs).offset == (*lhs).offset)
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

// -----------------------------------------------------------------------------

bool _apiAccepts(AsyncWebServerRequest* request, const __FlashStringHelper* str) {
    auto* header = request->getHeader(F("Accept"));
    if (header) {
        return
            (header->value().indexOf(F("*/*")) >= 0)
         || (header->value().indexOf(str) >= 0);
    }

    return false;
}

bool _apiAcceptsText(AsyncWebServerRequest* request) {
    return _apiAccepts(request, F("text/plain"));
}

bool _apiAcceptsJson(AsyncWebServerRequest* request) {
    return _apiAccepts(request, F("application/json"));
}

bool _apiMatchHeader(AsyncWebServerRequest* request, const __FlashStringHelper* key, const __FlashStringHelper* value) {
    auto* header = request->getHeader(key);
    if (header) {
        return header->value().equals(value);
    }

    return false;
}

bool _apiIsJsonContent(AsyncWebServerRequest* request) {
    return _apiMatchHeader(request, F("Content-Type"), F("application/json"));
}

bool _apiIsFormDataContent(AsyncWebServerRequest* request) {
    return _apiMatchHeader(request, F("Content-Type"), F("application/x-www-form-urlencoded"));
}

// 'Modernized' API configuration:
// - `Api-Key` header for both GET and PUT
// - Parse request body as JSON object

struct ApiRequestHelper {
    template <typename T>
    struct ReadOnlyStream : public Stream {
        ReadOnlyStream() = delete;
        explicit ReadOnlyStream(const T& buffer) :
            _buffer(buffer),
            _size(buffer.size())
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
                std::copy(_buffer.data() + _index, _buffer.data() + _index + size, ptr);
                _index += size;
                return size;
            }

            return 0;
        }

        void flush() override {
        }

        size_t write(const uint8_t*, size_t) override {
            return 0;
        }

        size_t write(uint8_t) override {
            return 0;
        }

        const T& _buffer;
        const size_t _size;
        size_t _index { 0 };
    };

    using Buffer = std::vector<uint8_t>;
    using BodyStream = ReadOnlyStream<Buffer>;

    ApiRequestHelper(const ApiRequestHelper&) = delete;
    ApiRequestHelper(ApiRequestHelper&&) noexcept = default;

    // &path is expected to be request->url(), which is valid throughout the request's lifetime

    explicit ApiRequestHelper(AsyncWebServerRequest& request, const PathParts& pattern) :
        _request(request),
        _pattern(pattern),
        _path(request.url()),
        _match(_pattern.match(_path))
    {}

    void body(uint8_t* ptr, size_t size) {
        _buffer.insert(_buffer.end(), ptr, ptr + size);
    }

    BodyStream body() const {
        return BodyStream(_buffer);
    }

    ApiRequest request() const {
        return ApiRequest(_request, _pattern, _path);
    }

    const PathParts& parts() const {
        return _path;
    }

    bool match() const {
        return _match;
    }

private:
    AsyncWebServerRequest& _request;
    const PathParts& _pattern;
    PathParts _path;
    Buffer _buffer;
    bool _match;
};

// Because the webserver request is split between multiple separate function invocations, we need to preserve some state.
// TODO: in case we are dealing with multicore, perhaps a custom stack-like allocator would be better here?
//
// Some quirks to deal with:
// - espasyncwebserver will `free(_tempObject)` when request is disconnected, but only after this callbackhandler is done.
//   make sure to set nullptr before returning
// - ALL headers are parsed (and we could access those during this callback), but we need to explicitly
//   request them to stay in memory for the actual handler

void _apiAttachHelper(AsyncWebServerRequest& request, ApiRequestHelper&& helper) {
    request._tempObject = new ApiRequestHelper(std::move(helper));
    request.onDisconnect([&]() {
        auto* ptr = reinterpret_cast<ApiRequestHelper*>(request._tempObject);
        delete ptr;
        request._tempObject = nullptr;  
    });
    request.addInterestingHeader(F("Api-Key"));
}

class ApiBaseWebHandler : public AsyncWebHandler {
public:
    ApiBaseWebHandler() = delete;
    ApiBaseWebHandler(const ApiBaseWebHandler&) = delete;
    ApiBaseWebHandler(ApiBaseWebHandler&&) = delete;

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

class ApiJsonWebHandler : public ApiBaseWebHandler {
public:
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
        return false;
    }

    bool canHandle(AsyncWebServerRequest* request) override {
        if (!apiEnabled()) {
            return false;
        }

        if (!_apiAcceptsJson(request)) {
            return false;
        }

        auto helper = ApiRequestHelper(*request, parts());
        if (helper.match() && apiAuthenticate(request)) {
            switch (request->method()) {
            case HTTP_HEAD:
                return true;
            case HTTP_GET:
            case HTTP_PUT:
                break;
            default:
                return false;
            }
            _apiAttachHelper(*request, std::move(helper));
            return true;
        }

        return false;
    }

    void handleBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t total) override {
        reinterpret_cast<ApiRequestHelper*>(request->_tempObject)->body(data, len);
    }

    void handleRequest(AsyncWebServerRequest* request) override {
        auto& helper = *reinterpret_cast<ApiRequestHelper*>(request->_tempObject);

        auto apireq = helper.request();
        switch (request->method()) {
        case HTTP_HEAD:
            request->send(204);
            return;

        case HTTP_GET: {
            DynamicJsonBuffer jsonBuffer(API_BUFFER_SIZE);
            JsonObject& root = jsonBuffer.createObject();
            if (!_get(apireq, root)) {
                break;
            }
            if (!apireq.done()) {
                AsyncResponseStream *response = request->beginResponseStream("application/json", root.measureLength() + 1);
                root.printTo(*response);
                request->send(response);
                return;
            }
            break;
        }

        case HTTP_PUT: {
            auto body = helper.body(); // XXX: ArduinoJson template deductor does not like temporaries
            DynamicJsonBuffer jsonBuffer(2 * API_BUFFER_SIZE);
            JsonObject& root = jsonBuffer.parseObject(body);
            if (!_put(apireq, root)) {
                break;
            }
            if (!apireq.done()) {
                request->send(204);
                return;
            }
        }

        default:
            request->send(405);
            return;
        }


        request->send(500);
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
// - ?value=... for PUT values
// MUST be trivial (see isRequestHandlerTrivial) to allow auth with PUT
// (or, we reimplement body parsing like JSON variant, but for form-data)

class ApiBasicWebHandler : public ApiBaseWebHandler {
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

        if (!_apiAcceptsText(request)) {
            return false;
        }

        switch (request->method()) {
        case HTTP_HEAD:
        case HTTP_GET:
        case HTTP_PUT:
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
        }
        default:
            request->send(405);
            return;
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
constexpr size_t ApiPathSizeMax { 128ul };

// -----------------------------------------------------------------------------

const String& _apiBase() {
    static const String base(F("/api/"));
    return base;
}

template <typename Handler, typename Callback>
void _apiRegister(const String& path, Callback&& get, Callback&& put) {
    auto* ptr = new Handler(_apiBase() + path, std::forward<Callback>(get), std::forward<Callback>(put));
    webServer().addHandler(reinterpret_cast<AsyncWebHandler*>(ptr));
    _apis.emplace_front(ptr);
}

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

