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

PathParts::PathParts(espurna::StringView path) :
    _path(path)
{
    if (!_path.length()) {
        _ok = false;
        return;
    }

    PathPart::Type type { PathPart::Type::Unknown };
    size_t length { 0ul };
    size_t offset { 0ul };

    const char* p { _path.begin() };
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

espurna::StringView PathParts::wildcard(const PathParts& pattern, const PathParts& value, int index) {
    if (index < 0) {
        index = std::abs(index + 1);
    }

    espurna::StringView out;

    if (std::abs(index) < pattern.parts().size()) {
        const auto& pattern_parts = pattern.parts();
        int counter { 0 };

        for (size_t part = 0; part < pattern.size(); ++part) {
            const auto& lhs = pattern_parts[part];
            const auto& rhs = value.parts()[part];

            const auto path = value.path();

            switch (lhs.type) {
            case PathPart::Type::Value:
            case PathPart::Type::Unknown:
                break;

            case PathPart::Type::SingleWildcard:
                if (counter == index) {
                    out = espurna::StringView(
                        path.begin() + rhs.offset, path.begin() + rhs.offset + rhs.length);
                    return out;
                }
                ++counter;
                break;

            case PathPart::Type::MultiWildcard:
                if (counter == index) {
                    out = espurna::StringView(
                        path.begin() + rhs.offset, path.end());
                }
                return out;
            }
        }
    }

    return out;
}

size_t PathParts::wildcards(const PathParts& pattern) {
    size_t out { 0 };

    for (const auto& part : pattern) {
        switch (part.type) {
        case PathPart::Type::Unknown:
        case PathPart::Type::Value:
        case PathPart::Type::MultiWildcard:
            break;
        case PathPart::Type::SingleWildcard:
            ++out;
            break;
        }
    }

    return out;
}

#if WEB_SUPPORT

String ApiRequest::wildcard(int index) const {
    return PathParts::wildcard(_pattern, _parts, index).toString();
}

size_t ApiRequest::wildcards() const {
    return PathParts::wildcards(_pattern);
}

#endif

// -----------------------------------------------------------------------------

#if API_SUPPORT

namespace espurna {
namespace api {
namespace content_type {
namespace {

STRING_VIEW_INLINE(Anything, "*/*");
STRING_VIEW_INLINE(Text, "text/plain");
STRING_VIEW_INLINE(Json, "application/json");
STRING_VIEW_INLINE(Form, "application/x-www-form-urlencoded");

} // namespace
} // namespace content_type

StringView Request::param(const String& name) {
    const auto* result = _request.getParam(name, HTTP_PUT == _request.method());

    espurna::StringView out;
    if (result) {
        out = result->value();
    }

    return out;
}

void Request::send(const String& payload) {
    if (_done) {
        return;
    }

    _done = true;
    if (payload.length()) {
        _request.send(200,
            content_type::Text.toString(),
            payload);
    } else {
        _request.send(204);
    }
}

namespace {

bool accepts(AsyncWebServerRequest* request, StringView pattern) {
    STRING_VIEW_INLINE(Accept, "Accept");
    auto* header = request->getHeader(Accept.toString());
    if (!header) {
        return true;
    }

    return (header->value().indexOf(
                StringView(content_type::Anything).toString()) >= 0)
        || (header->value().indexOf(pattern.toString()) >= 0);
}

bool accepts_text(AsyncWebServerRequest* request) {
    return accepts(request, content_type::Text);
}

bool accepts_json(AsyncWebServerRequest* request) {
    return accepts(request, content_type::Json);
}

bool is_content_type(AsyncWebServerRequest* request, StringView value) {
    return value == request->contentType();
}

bool is_form_data(AsyncWebServerRequest* request) {
    return is_content_type(request, content_type::Form);
}

bool is_json(AsyncWebServerRequest* request) {
    return is_content_type(request, content_type::Json);
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

void attach_helper(AsyncWebServerRequest& request, RequestHelper&& helper) {
    request._tempObject = new RequestHelper(std::move(helper));
    request.onDisconnect(
        [&]() {
            auto* ptr = reinterpret_cast<RequestHelper*>(request._tempObject);
            delete ptr;
            request._tempObject = nullptr;
        });
    request.addInterestingHeader(
        STRING_VIEW("Api-Key").toString());
    request.addInterestingHeader(
        STRING_VIEW("Accept").toString());
}

class BaseWebHandler : public AsyncWebHandler {
public:
    BaseWebHandler() = delete;

    BaseWebHandler(const BaseWebHandler&) = delete;
    BaseWebHandler& operator=(const BaseWebHandler&) = delete;

    BaseWebHandler(BaseWebHandler&&) = delete;
    BaseWebHandler& operator=(BaseWebHandler&&) = delete;

    // In case this needs to be copied or moved, ensure PathParts copy references the new object's string
    template <typename T,
              typename = typename std::enable_if<
                  std::is_constructible<String, T>::value>::type>
    explicit BaseWebHandler(T&& pattern) :
        _pattern(std::forward<T>(pattern)),
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

class JsonWebHandler final : public BaseWebHandler {
public:
    static constexpr size_t BufferSize { API_JSON_BUFFER_SIZE };

    JsonWebHandler() = delete;

    JsonWebHandler(const JsonWebHandler&) = delete;
    JsonWebHandler& operator=(const JsonWebHandler&) = delete;

    JsonWebHandler(JsonWebHandler&&) = delete;
    JsonWebHandler& operator=(JsonWebHandler&&) = delete;

    template <typename Path, typename Get, typename Put>
    JsonWebHandler(Path&& path, Get&& get, Put&& put) :
        BaseWebHandler(std::forward<Path>(path)),
        _get(std::forward<Get>(get)),
        _put(std::forward<Put>(put))
    {}

    bool isRequestHandlerTrivial() override {
        return true;
    }

    bool canHandle(AsyncWebServerRequest* request) override {
        if (!apiEnabled()) {
            return false;
        }

        auto helper = RequestHelper(*request, parts());
        if (helper.match() && apiAuthenticate(request)) {
            switch (request->method()) {
            case HTTP_HEAD:
                return true;
            case HTTP_PUT:
                if (!is_json(request)) {
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
            attach_helper(*request, std::move(helper));
            return true;
        }

        return false;
    }

    void _handleGet(AsyncWebServerRequest* request, Request& apireq) {
        DynamicJsonBuffer jsonBuffer(BufferSize);
        JsonObject& root = jsonBuffer.createObject();
        if (!_get(apireq, root)) {
            request->send(500);
            return;
        }

        if (!apireq.done()) {
            auto* response = request->beginResponseStream(
                content_type::Json.toString(), root.measureLength() + 1);
            root.printTo(*response);
            request->send(response);
            return;
        }

        request->send(500);
    }

    void _handlePut(AsyncWebServerRequest* request, uint8_t* data, size_t size) {
        // XXX: arduinojson v5 de-serializer will happily read garbage from raw ptr, since there's no length limit
        //      this is fixed in v6 though. for now, use a wrapper, but be aware that this actually uses more mem for the jsonbuffer
        auto* ptr = reinterpret_cast<const char*>(data);
        auto reader = StringView(ptr, ptr + size);

        DynamicJsonBuffer jsonBuffer(BufferSize);
        JsonObject& root = jsonBuffer.parseObject(reader);
        if (!root.success()) {
            request->send(500);
            return;
        }

        auto& helper = *reinterpret_cast<RequestHelper*>(request->_tempObject);

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
        if (!accepts_json(request)) {
            request->send(406,
                content_type::Text.toString(),
                content_type::Json.toString());
            return;
        }

        auto& helper = *reinterpret_cast<RequestHelper*>(request->_tempObject);

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

    using BaseWebHandler::pattern;
    using BaseWebHandler::parts;

private:
    JsonHandler _get;
    JsonHandler _put;
};

// ESPurna legacy API configuration
// - ?apikey=... to authorize in GET or PUT
// - ?anything=... for input data (common key is "value")
// MUST correctly override isRequestHandlerTrivial() to allow auth with PUT
// (i.e. so that ESPAsyncWebServer parses the body and adds form-data to request params list)

class BasicWebHandler final : public BaseWebHandler {
public:
    template <typename Path, typename Get, typename Put>
    BasicWebHandler(Path&& path, Get&& get, Put&& put) :
        BaseWebHandler(std::forward<Path>(path)),
        _get(std::forward<Get>(get)),
        _put(std::forward<Put>(put))
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
            if (!is_form_data(request)) {
                return false;
            }
            break;
        default:
            return false;
        }

        auto helper = RequestHelper(*request, parts());
        if (helper.match()) {
            attach_helper(*request, std::move(helper));
            return true;
        }

        return false;
    }

    void handleRequest(AsyncWebServerRequest* request) override {
        if (!apiAuthenticate(request)) {
            request->send(403);
            return;
        }

        if (!accepts_text(request)) {
            request->send(406,
                content_type::Text.toString(),
                content_type::Text.toString());
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
            auto& helper = *reinterpret_cast<RequestHelper*>(request->_tempObject);

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

    const BasicHandler& get() const {
        return _get;
    }

    const BasicHandler& put() const {
        return _put;
    }

    using BaseWebHandler::pattern;
    using BaseWebHandler::parts;

private:
    BasicHandler _get;
    BasicHandler _put;
};

namespace internal {

std::forward_list<BaseWebHandler*> list;

} // namespace internal

namespace simple {

bool ok(Request& request) {
    STRING_VIEW_INLINE(Ok, "OK");
    request.send(Ok.toString());
    return true;
}

bool error(ApiRequest& request) {
    STRING_VIEW_INLINE(Error, "ERROR");
    request.send(Error.toString());
    return true;
}

} // namespace simple

STRING_VIEW_INLINE(BasePath, API_BASE_PATH);

void add(BaseWebHandler* ptr) {
    webServer().addHandler(ptr);
    internal::list.emplace_front(ptr);
}

template <typename Handler, typename Get, typename Put>
void add(String path, Get&& get, Put&& put) {
    add(new Handler(
        BasePath + path,
        std::forward<Get>(get),
        std::forward<Put>(put)));
}

template <typename Handler, typename Get, typename Put>
void add(StringView path, Get&& get, Put&& put) {
    add<Handler, Get, Put>(
        path.toString(),
        std::forward<Get>(get), 
        std::forward<Put>(put));
}

void setup() {
    add<BasicWebHandler, BasicHandler>(
        STRING_VIEW("list"),
        [](Request& request) {
            String paths;
            for (auto& api : internal::list) {
                paths += api->pattern();
                paths += '\r';
                paths += '\n';
            }
            request.send(paths);
            return true;
        },
        nullptr
    );

    add<BasicWebHandler, BasicHandler>(
        STRING_VIEW("rpc"),
        nullptr,
        [](Request& request) {
            STRING_VIEW_INLINE(Action, "action");
            if (rpcHandleAction(request.param(Action.toString()))) {
                return simple::ok(request);
            }
            return simple::error(request);
        }
    );
}

} // namespace
} // namespace api 
} // namespace espurna

// -----------------------------------------------------------------------------

void apiRegister(String path, espurna::api::BasicHandler&& get, espurna::api::BasicHandler&& put) {
    using namespace espurna::api;
    add<BasicWebHandler>(std::move(path), std::move(get), std::move(put));
}

void apiRegister(String path, espurna::api::JsonHandler&& get, espurna::api::JsonHandler&& put) {
    using namespace espurna::api;
    add<JsonWebHandler>(std::move(path), std::move(get), std::move(put));
}

void apiSetup() {
    espurna::api::setup();
}

bool apiOk(ApiRequest& request) {
    return espurna::api::simple::ok(request);
}

bool apiError(ApiRequest& request) {
    return espurna::api::simple::error(request);
}

#endif // API_SUPPORT
