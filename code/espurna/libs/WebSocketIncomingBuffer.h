/*

WebSocketIncommingBuffer

Code by Hermann Kraus (https://bitbucket.org/hermr2d2/)
and slightly modified.

*/

#pragma once

#include <ESPAsyncWebServer.h>

struct WebSocketIncomingBuffer {
    static constexpr size_t MessageSizeMax { 4096 };
    using Callback = void(*)(AsyncWebSocketClient* client, uint8_t* data, size_t len);

    WebSocketIncomingBuffer() = delete;
    WebSocketIncomingBuffer(Callback cb, bool terminate_string, bool cb_on_fragments) :
        _cb(cb),
        _cb_on_fragments(cb_on_fragments),
        _terminate_string(terminate_string)
    {}

    explicit WebSocketIncomingBuffer(Callback cb) :
        WebSocketIncomingBuffer(cb, true, false)
    {}

    void data_event(AsyncWebSocketClient *client, AwsFrameInfo *info, uint8_t *data, size_t len) {
        if ((info->final || _cb_on_fragments)
            && !_terminate_string
            && info->index == 0
            && info->len == len)
        {

            /* The whole message is in a single frame and we got all of it's
               data therefore we can parse it without copying the data first.*/
            _cb(client, data, len);
            return;
        }

        if (info->len > MessageSizeMax) {
            return;
        }

        /* Check if previous fragment was discarded because it was too long. */
        //if (!_cb_on_fragments && info->num > 0 && !_buffer) return;

        if (info->index == 0) {
            if (_cb_on_fragments) {
                _buffer.reserve(info->len + 1);
            } else {
                /* The current fragment would lead to a message which is
                   too long. So discard everything received so far. */
                const size_t reserve = info->len + _buffer.size() + 1;
                if (reserve > MessageSizeMax) {
                    _buffer = Buffer();
                    return;
                }

                _buffer.reserve(reserve);
            }
        }

        //assert(_buffer->size() == info->index);
        _buffer.insert(_buffer.end(), data, data + len);
        if (info->index + len == info->len
            && (info->final || _cb_on_fragments))
        {
            // Frame/message complete
            const size_t len = _buffer.size();
            if (_terminate_string) {
                _buffer.push_back(0);
            }
            _cb(client, _buffer.data(), len);
            _buffer.clear();
        }
    }

private:
    using Buffer = std::vector<uint8_t>;
    Buffer _buffer;

    Callback _cb;
    bool _cb_on_fragments;
    bool _terminate_string;
};
