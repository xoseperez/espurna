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

struct ApiMatch;

struct ApiParsedPath {
    explicit ApiParsedPath(const String& path) :
        _path(path),
        _levels(_path)
    {
        init();
    }

    explicit ApiParsedPath(String&& path) :
        _path(std::move(path)),
        _levels(_path)
    {
        init();
    }

    ApiParsedPath(const ApiParsedPath& other) :
        _path(other._path),
        _levels(_path, other._levels),
        _ok(other._ok)
    {}

    ApiParsedPath(ApiParsedPath&& other) :
        _path(std::move(other._path)),
        _levels(_path, std::move(other._levels)),
        _ok(other._ok)
    {}

    void init() {
        ApiLevel::Type type { ApiLevel::Type::Unknown };
        size_t length { 0ul };
        size_t offset { 0ul };

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
        case '+':
            goto parse_single_wildcard;
        case '#':
            goto parse_multi_wildcard;
        case '/':
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
        return;

    success:
        _ok = true;
        return;
    }

    explicit operator bool() const {
        return _ok;
    }

    const String& path() const {
        return _path;
    }

    const ApiLevels& levels() const {
        return _levels;
    }

    ApiMatch match(const ApiParsedPath& other) const;
    ApiMatch match(const String& other) const;

private:
    String _path;
    ApiLevels _levels;
    bool _ok { false };
};

// given the path 'topic/one/two/three' and pattern 'topic/+/two/+', provide
// wildcards [0] -> one and [1] -> three

struct ApiMatch {
    explicit ApiMatch(ApiMatch&& other) :
        _path(std::move(other._path)),
        _wildcards(_path.path(), std::move(other._wildcards))
    {}

    explicit ApiMatch(const ApiParsedPath& pattern, const ApiParsedPath& other) :
        _path(other),
        _wildcards(_path.path())
    {
        _init(pattern, other);
    }

    explicit operator bool() const {
        return _ok;
    }

    const String& path() {
        return _path.path();
    }

    const ApiLevels& levels() const {
        return _path.levels();
    }

    const ApiLevels& wildcards() const {
        return _wildcards;
    }

    void reserve(size_t size) {
        _wildcards.reserve(size);
    }

    void clear() {
        _wildcards.clear();
    }

    bool ok() const {
        return _ok;
    }

    private:

    void _init(const ApiParsedPath& pattern, const ApiParsedPath& other);

    ApiParsedPath _path;
    ApiLevels _wildcards;
    bool _ok { false };
};

void ApiMatch::_init(const ApiParsedPath& pattern, const ApiParsedPath& other) {
    if (!pattern || !other) {
        return;
    }

    auto lhs = pattern.levels().begin();
    auto rhs = other.levels().begin();

    auto lhs_end = pattern.levels().end();
    auto rhs_end = other.levels().end();

loop:
    if (lhs == lhs_end) {
        goto check_end;
    }

    switch ((*lhs).type) {
    case ApiLevel::Type::Value:
        if (
            (rhs != rhs_end)
            && ((*rhs).type == ApiLevel::Type::Value)
            && ((*rhs).offset == (*lhs).offset)
            && ((*rhs).length == (*lhs).length)
        ) {
            if (0 == std::memcmp(
                pattern.path().c_str() + (*lhs).offset,
                other.path().c_str() + (*rhs).offset,
                (*rhs).length))
            {
                std::advance(lhs, 1);
                std::advance(rhs, 1);
                goto loop;
            }
        }
        goto error;

    case ApiLevel::Type::SingleWildcard:
        if (
            (rhs != rhs_end)
            && ((*rhs).type == ApiLevel::Type::Value)
        ) {
            _wildcards.emplace_back((*rhs).type, (*rhs).offset, (*rhs).length);
            std::advance(lhs, 1);
            std::advance(rhs, 1);
            goto loop;
        }
        goto error;

    case ApiLevel::Type::MultiWildcard:
        if (std::next(lhs) == lhs_end) {
            while (rhs != rhs_end) {
                if ((*rhs).type != ApiLevel::Type::Value) {
                    goto error;
                }
                _wildcards.emplace_back((*rhs).type, (*rhs).offset, (*rhs).length);
                std::advance(rhs, 1);
            }
            lhs = lhs_end;
            break;
        }
        goto error;

    case ApiLevel::Type::Unknown:
        goto error;
    };

check_end:
    if (lhs == lhs_end && rhs == rhs_end) {
        _ok = true;
        goto return_result;
    }

error:
    _ok = false;
    clear();

return_result:
    return;
}

ApiMatch ApiParsedPath::match(const String& other) const {
    return match(ApiParsedPath(other));
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

    ApiRequestHelper(ApiRequestHelper&& other) :
        _request(other._request),
        _pattern(std::move(other._pattern)),
        _path(std::move(other._path)),
        _match(_pattern, _path)
    {}

    template <typename Pattern, typename Path>
    explicit ApiRequestHelper(AsyncWebServerRequest& request, Pattern&& pattern, Path&& path) :
        _request(request),
        _pattern(std::forward<Pattern>(pattern)),
        _path(std::forward<Path>(path)),
        _match(_pattern, _path)
    {}

    const ApiMatch& match() const {
        return _match;
    }

    void body(uint8_t* ptr, size_t size) {
        _buffer.insert(_buffer.end(), ptr, ptr + size);
    }

    BodyStream body() {
        return BodyStream(_buffer);
    }

    ApiRequest request() {
        return ApiRequest(_request, _match.levels(), _match.wildcards());
    }

private:
    AsyncWebServerRequest& _request;
    const ApiParsedPath& _pattern;
    ApiParsedPath _path;
    ApiMatch _match;
    Buffer _buffer;
};

// Because the webserver request is split between multiple separate function invocations, we need to preserve some state.
// TODO: in case we are dealing with multicore, perhaps a custom stack-like allocator would be better here
// (e.g. static-allocated buffer within handler class, giving out fixed-size N free objects)

void _apiAttachHelper(AsyncWebServerRequest& request, ApiRequestHelper&& helper) {
    request._tempObject = new ApiRequestHelper(std::move(helper));
    request.onDisconnect([&]() {
        auto* ptr = reinterpret_cast<ApiRequestHelper*>(request._tempObject);
        delete ptr;
        request._tempObject = nullptr;  // XXX: espasyncwebserver will free the pointer when request is disconnected, but only after this callback
    });
    // XXX: ALL headers are parsed, but 'uninteresting' ones are deleted from internal list after this callback is done
    // (i.e. canHandle *or* filter of the handler)
    request.addInterestingHeader(F("Api-Key"));
}

class ApiJsonWebHandler : public AsyncWebHandler {
private:

public:
    template <typename Path, typename Callback>
    ApiJsonWebHandler(Path&& path, Callback&& get, Callback&& put) :
        _path(std::forward<Path>(path)),
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

        auto helper = ApiRequestHelper(*request, _path, request->url());
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
        auto& helper = *reinterpret_cast<ApiRequestHelper*>(request->_tempObject);
        helper.body(data, len);
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

    const String& path() const {
        return _path.path();
    }

private:
    ApiParsedPath _path;
    ApiJsonHandler _get;
    ApiJsonHandler _put;
};

// ESPurna legacy API configuration
// - ?apikey=... to authorize in GET or PUT
// - ?value=... for PUT values

class ApiBasicWebHandler : public AsyncWebHandler {
public:
    template <typename Path, typename Callback>
    ApiBasicWebHandler(Path&& path, Callback&& get, Callback&& put) :
        _path(std::forward<Path>(path)),
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

        auto helper = ApiRequestHelper(*request, _path, request->url());
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

    const String& path() const {
        return _path.path();
    }

    const ApiBasicHandler& get() const {
        return _get;
    }

    const ApiBasicHandler& put() const {
        return _put;
    }

private:
    ApiParsedPath _path;
    ApiBasicHandler _get;
    ApiBasicHandler _put;
};

std::forward_list<std::reference_wrapper<const String>> _apis;
constexpr size_t ApiPathSizeMax { 128ul };

// -----------------------------------------------------------------------------

const String& _apiBase() {
    static const String base(F("/api/"));
    return base;
}

template <typename Handler, typename Callback>
void _apiRegister(const String& path, Callback&& get, Callback&& put) {
    auto* ptr = new Handler(_apiBase() + path, std::forward<Callback>(get), std::forward<Callback>(put));
    webServer().addHandler(ptr);
    _apis.emplace_front(std::ref(ptr->path()));
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
                paths += api.get() + "\r\n";
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

