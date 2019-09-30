
#pragma once

#include <utility>
#include <functional>
#include <vector>
#include <memory>
#include <limits>
#include <cstring>

#include <pgmspace.h>
#include <ESPAsyncTCP.h>

#ifndef ASYNC_HTTP_DEBUG
#define ASYNC_HTTP_DEBUG(...)  //DEBUG_PORT.printf(__VA_ARGS__)
#endif

namespace Headers {
    PROGMEM const char HOST[] = "Host";
    PROGMEM const char USER_AGENT[] = "User-Agent";
    PROGMEM const char CONNECTION[] = "Connection";
    PROGMEM const char CONTENT_TYPE[] = "Content-Type";
    PROGMEM const char CONTENT_LENGTH[] = "Content-Length";
};

class AsyncHttpHeader {

    public:

        using key_value_t = std::pair<const String&, const String&>;

    private:

        const String _key;
        const String _value;
        key_value_t _kv;

    public:

        AsyncHttpHeader(const char* key, const char* value) :
            _key(FPSTR(key)),
            _value(FPSTR(value)),
            _kv(_key, _value)
        {}

        AsyncHttpHeader(const AsyncHttpHeader& other) :
            _key(other._key),
            _value(other._value),
            _kv(_key, _value)
        {}

        AsyncHttpHeader(const AsyncHttpHeader&& other) :
            _key(std::move(other._key)),
            _value(std::move(other._value)),
            _kv(_key, _value)
        {}

        const key_value_t& get() const {
            return _kv;
        }

        const char* key() const {
            return _key.c_str();
        }

        const char* value() const {
            return _value.c_str();
        }

        size_t keyLength() const {
            return _key.length();
        }

        size_t valueLength() const {
            return _value.length();
        }

        bool operator ==(const AsyncHttpHeader& header) {
            return (
                (header._key == _key) && (header._value == _value)
            );
        }

};

class AsyncHttpHeaders {

    using header_t = AsyncHttpHeader;
    using headers_t = std::vector<header_t>;

    private:

    headers_t _headers;
    size_t _index;
    size_t _last;
    String _value;

    public:

    AsyncHttpHeaders() :
        _index(0),
        _last(std::numeric_limits<size_t>::max())
    {}

    AsyncHttpHeaders(headers_t& headers) :
        _headers(headers),
        _index(0),
        _last(std::numeric_limits<size_t>::max())
    {}

    void add(const char* key, const char* value) {
        _headers.emplace_back(key, value);
    }

    size_t size() {
        return _headers.size();
    }

    void reserve(size_t size) {
        _headers.reserve(size);
    }

    bool has(const char* key) {
        for (const auto& header : _headers) {
            if (strncmp(key, header.key(), header.keyLength()) == 0) return true;
        }
        return false;
    }

    String& current() {
        if (_last == _index) return _value;
        if (_headers.size() && (_index < _headers.size())) {
            const auto& current = _headers.at(_index);
            _value.reserve(
                current.keyLength()
                + current.valueLength()
                + strlen(": \r\n")
            );

            _value = current.key();
            _value += ": ";
            _value += current.value();
            _value += "\r\n";
        } else {
            _value = "";
        }

        _last = _index;

        return _value;
    }

    String& next() {
        ++_index;
        return current();
    }

    bool done() {
        return (_index >= _headers.size());
    }

    void clear() {
        _index = 0;
        _last = std::numeric_limits<size_t>::max();
        _headers.clear();
    }

    headers_t::const_iterator begin() {
        return _headers.begin();
    }

    headers_t::const_iterator end() {
        return _headers.end();
    }

};

class AsyncHttpError {

    public:

    enum error_t {
        EMPTY,
        CLIENT_ERROR,
        REQUEST_TIMEOUT,
        NETWORK_TIMEOUT,
    };

    const error_t error;
    const String data;

    AsyncHttpError() :
        error(EMPTY), data(0)
    {}

    AsyncHttpError(AsyncHttpError&) = default;

    AsyncHttpError(const error_t& err, const String& data) :
        error(err), data(data)
    {}

    bool operator==(const error_t& err) const {
        return err == error;
    }

    bool operator==(const AsyncHttpError& obj) const {
        return obj.error == error;
    }

};

class AsyncHttp {

    constexpr static const size_t DEFAULT_TIMEOUT = 5000;
    constexpr static const size_t DEFAULT_PATH_BUFSIZE = 256;

    public:

        AsyncClient client;

        enum cfg_t {
            HTTP_SEND = 1 << 0,
            HTTP_RECV = 1 << 1
        };

        enum class state_t : uint8_t {
            NONE,
            HEADERS,
            BODY
        };

        using on_connected_f = std::function<void(AsyncHttp*)>;
        using on_status_f = std::function<bool(AsyncHttp*, uint16_t status)>;
        using on_disconnected_f = std::function<void(AsyncHttp*)>;
        using on_error_f = std::function<void(AsyncHttp*, const AsyncHttpError&)>;

        using on_body_recv_f = std::function<void(AsyncHttp*, uint8_t* data, size_t len)>;
        using on_body_send_f = std::function<size_t(AsyncHttp*, AsyncClient* client)>;

        int cfg = HTTP_RECV;
        state_t state = state_t::NONE;
        AsyncHttpError::error_t last_error;

        on_connected_f on_connected;
        on_disconnected_f on_disconnected;

        on_status_f on_status;
        on_error_f on_error;

        on_body_recv_f on_body_recv;
        on_body_send_f on_body_send_prepare;
        on_body_send_f on_body_send_data;

        String method;
        String path;

        // WebRequest.cpp
        //LinkedList<AsyncWebHeader*> headers;
        //std::vector<AsyncHttpHeader> headers;
        AsyncHttpHeaders headers;

        String host;
        uint16_t port;

        uint32_t ts;
        uint32_t timeout = DEFAULT_TIMEOUT;

        bool connected = false;
        bool connecting = false;

        // TODO ref: https://github.com/xoseperez/espurna/pull/1909#issuecomment-533319480
        //       since LWIP_NETIF_TX_SINGLE_PBUF is enabled, no need to buffer anything and we can directly use client->add with non-persistent data
        //       buuut... this exposes asyncclient to the modules, maybe this needs a simple cbuf periodically flushing the data and this method simply filling it
        void trySend() {
            if (!client.canSend()) return;
            if (!on_body_send_data) {
                client.close(true);
                return;
            }
            on_body_send_data(this, &client);
        }

        bool trySendHeaders() {
            if (headers.done()) return true;

            const auto& string = headers.current();
            const auto len = string.length();

            if (!len) {
                return true;
            }

            if (client.space() >= (len + 2)) {
                if (client.add(string.c_str(), len)) {
                    if (!headers.next().length()) {
                        client.add("\r\n", 2);
                    }
                }
                client.send();
            }

            return false;
        }


    protected:

        static AsyncHttpError _timeoutError(AsyncHttpError::error_t error, const __FlashStringHelper* message, uint32_t ts) {
            String data;
            data.reserve(32);
            data += message;
            data += ' ';
            data += String(ts);
            return {error, data};
        }

        static void _onDisconnect(void* http_ptr, AsyncClient*) {
            AsyncHttp* http = static_cast<AsyncHttp*>(http_ptr);
            if (http->on_disconnected) http->on_disconnected(http);
            http->ts = 0;
            http->connected = false;
            http->connecting = false;
            http->state = AsyncHttp::state_t::NONE;
        }

        static void _onTimeout(void* http_ptr, AsyncClient* client, uint32_t time) {
            client->close(true);

            AsyncHttp* http = static_cast<AsyncHttp*>(http_ptr);
            http->last_error = AsyncHttpError::NETWORK_TIMEOUT;
            if (http->on_error) http->on_error(http, _timeoutError(AsyncHttpError::NETWORK_TIMEOUT, F("Network timeout after"), time));
        }

        static void _onPoll(void* http_ptr, AsyncClient* client) {
            AsyncHttp* http = static_cast<AsyncHttp*>(http_ptr);
            const auto diff = millis() - http->ts;
            if (diff > http->timeout) {
                if (http->on_error) http->on_error(http, _timeoutError(AsyncHttpError::REQUEST_TIMEOUT, F("No response after"), diff));
                client->close(true);
            }
        }

        static void _onData(void* http_ptr, AsyncClient* client, void* response, size_t len) {

            AsyncHttp* http = static_cast<AsyncHttp*>(http_ptr);
            http->ts = millis();

            char * p = nullptr;

            do {

                p = nullptr;

                switch (http->state) {
                    case AsyncHttp::state_t::NONE:
                        {
                            if (len < strlen("HTTP/1.1 ... OK") + 1) {
                                ASYNC_HTTP_DEBUG("err | not enough len\n");
                                client->close(true);
                                return;
                            }
                            p = strnstr(reinterpret_cast<const char *>(response), "HTTP/1.", len);
                            if (!p) {
                                ASYNC_HTTP_DEBUG("err | not http\n");
                                client->close(true);
                                return;
                            }

                            p += strlen("HTTP/1.");
                            if ((p[0] != '1') && (p[0] != '0')) {
                                ASYNC_HTTP_DEBUG("err | not http/1.1 or http/1.0 c=%c\n", p[0]);
                                client->close(true);
                                return;
                            }

                            p += 2; // ' '
                            char buf[4] = {
                                p[0], p[1], p[2], '\0'
                            };
                            ASYNC_HTTP_DEBUG("log | buf code=%s\n", buf);

                            unsigned int code = atoi(buf);
                            if (http->on_status && !http->on_status(http, code)) {
                                ASYNC_HTTP_DEBUG("cb err | http code=%u\n", code);
                                return;
                            }

                            http->state = AsyncHttp::state_t::HEADERS;
                            continue;
                        }
                    case AsyncHttp::state_t::HEADERS:
                        {
                            // TODO: for now, simply skip all headers and go directly to the body
                            p = strnstr(reinterpret_cast<const char *>(response), "\r\n\r\n", len);
                            if (!p) {
                                ASYNC_HTTP_DEBUG("wait | headers not in first %u...\n", len);
                                return;
                            }
                            ASYNC_HTTP_DEBUG("ok | p=%p response=%p diff=%u len=%u\n",
                                p, response, (p - ((char*)response)), len
                            );
                            size_t end = p - ((char*)response) + 4;
                            if (len - end > len) {
                                client->close(true);
                                return;
                            }
                            response = ((char*)response) + end;
                            len -= end;
                            http->state = AsyncHttp::state_t::BODY;
                        }
                    case AsyncHttp::state_t::BODY:
                        {
                            if (!len) {
                                ASYNC_HTTP_DEBUG("wait | len is 0\n");
                                return;
                            }
                            ASYNC_HTTP_DEBUG("ok | body len %u!\n", len);

                            if (http->on_body_recv) http->on_body_recv(http, (uint8_t*) response, len);
                            return;
                        }
                }

            } while (http->state != AsyncHttp::state_t::NONE);

        }

        static void _onConnect(void* http_ptr, AsyncClient * client) {
            AsyncHttp* http = static_cast<AsyncHttp*>(http_ptr);

            http->ts = millis();
            http->connected = true;
            http->connecting = false;

            if (http->on_connected) http->on_connected(http);

            {
                size_t data_len = 0;
                if (http->cfg & HTTP_SEND) {
                    if (!http->on_body_send_prepare || !http->on_body_send_data) {
                        ASYNC_HTTP_DEBUG("err | no body_send_data/_prepare callbacks set\n");
                        client->close(true);
                        return;
                    }
                    data_len = http->on_body_send_prepare(http, client);
                    if (!data_len) {
                        ASYNC_HTTP_DEBUG("xxx | chunked encoding not implemented!\n");
                        client->close(true);
                    }
                    char data_buf[16];
                    snprintf(data_buf, sizeof(data_buf), "%u", data_len);
                    http->headers.add(Headers::CONTENT_LENGTH, data_buf);
                }
            }

            {
                // XXX: current path limit is 256 - 16 = 240 chars (including leading slash)
                char buf[DEFAULT_PATH_BUFSIZE] = {0};
                int res = snprintf_P(
                    buf, sizeof(buf), PSTR("%s %s HTTP/1.1\r\n"),
                    http->method.c_str(), http->path.c_str()
                );

                if ((res < 0) || (static_cast<size_t>(res) > sizeof(buf))) {
                    ASYNC_HTTP_DEBUG("err | could not print initial line\n");
                    client->close(true);
                    return;
                }

                client->add(buf, res);
            }

            if (http->trySendHeaders()) {
                if (http->cfg & HTTP_SEND) http->trySend();
            }
        }

        static void _onError(void* http_ptr, AsyncClient* client, err_t err) {
            AsyncHttp* http = static_cast<AsyncHttp*>(http_ptr);
            if (http->on_error) http->on_error(http, {AsyncHttpError::CLIENT_ERROR, client->errorToString(err)});
        }

        static void _onAck(void* http_ptr, AsyncClient* client, size_t, uint32_t) {
            AsyncHttp* http = static_cast<AsyncHttp*>(http_ptr);
            http->ts = millis();
            if (http->trySendHeaders()) {
                if (http->cfg & HTTP_SEND) http->trySend();
            }
        }


    public:
        AsyncHttp()
        {
            client.onDisconnect(_onDisconnect, this);
            client.onTimeout(_onTimeout, this);
            client.onPoll(_onPoll, this);
            client.onData(_onData, this);
            client.onConnect(_onConnect, this);
            client.onError(_onError, this);
            client.onAck(_onAck, this);
        }
        ~AsyncHttp() = default;

        bool busy() {
            return connecting || connected;
        }

        bool connect(const char* method, const char* host, uint16_t port, const char* path, bool use_ssl = false) {

            this->method = method;
            this->host = host;
            this->port = port;

            // XXX: current path limit is 256 - 16 = 240 chars (including leading slash)
            this->path = path;
            if (!this->path.length() || (this->path[0] != '/')) {
                ASYNC_HTTP_DEBUG("err | empty path / no leading slash\n");
                return false;
            }

            if (this->path.length() > (DEFAULT_PATH_BUFSIZE - 1)) {
                ASYNC_HTTP_DEBUG("err | cannot handle path larger than %u\n", DEFAULT_PATH_BUFSIZE - 1);
                return false;
            }

            this->ts = millis();

            // Treat every method as GET (receive-only), exception for POST / PUT to send data out
            size_t headers_size = 3;
            this->cfg = HTTP_RECV;
            if (this->method.equals("POST") || this->method.equals("PUT")) {
                if (!this->on_body_send_prepare) {
                    ASYNC_HTTP_DEBUG("err | on_body_send_prepare is required for POST / PUT requests\n");
                    return false;
                }
                if (!this->on_body_send_data) {
                    ASYNC_HTTP_DEBUG("err | on_body_send_data is required for POST / PUT requests\n");
                    return false;
                }
                this->cfg = HTTP_SEND | HTTP_RECV;
                headers_size += 2;
            }

            headers.reserve(headers_size);
            headers.clear();

            headers.add(Headers::HOST, this->host.c_str());
            headers.add(Headers::USER_AGENT, PSTR("ESPurna"));
            headers.add(Headers::CONNECTION, PSTR("close"));

            bool status = false;
            #if ASYNC_TCP_SSL_ENABLED
                status = client.connect(this->host.c_str(), this->port, use_ssl);
            #else
                status = client.connect(this->host.c_str(), this->port);
            #endif

            this->connecting = status;

            if (!status) {
                client.close(true);
            }

            return status;
        }

};
