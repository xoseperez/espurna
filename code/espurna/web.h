/*

WebSocketIncommingBuffer

Code by Hermann Kraus (https://bitbucket.org/hermr2d2/)
and slightly modified.

https://bitbucket.org/xoseperez/espurna/pull-requests/30/safer-buffer-handling-for-websocket-data

*/

#pragma once

#define MAX_WS_MSG_SIZE 4000
typedef std::function<void(AsyncWebSocketClient *client, uint8_t *data, size_t len)> AwsMessageHandler;

class WebSocketIncommingBuffer {

    public:
        WebSocketIncommingBuffer(AwsMessageHandler cb, bool terminate_string = true, bool cb_on_fragments = false) :
            _cb(cb),
            _cb_on_fragments(cb_on_fragments),
            _terminate_string(terminate_string),
            _buffer(0)
            {}

        ~WebSocketIncommingBuffer() {
            if (_buffer) delete _buffer;
        }

        void data_event(AsyncWebSocketClient *client, AwsFrameInfo *info, uint8_t *data, size_t len) {

            if((info->final || _cb_on_fragments) &&
               !_terminate_string && info->index == 0 && info->len == len) {
                /* The whole message is in a single frame and we got all of it's
                   data therefore we can parse it without copying the data first.*/
                _cb(client, data, len);
            } else {
                if (info->len > MAX_WS_MSG_SIZE) return;
                /* Check if previous fragment was discarded because it was too long. */
                if (!_cb_on_fragments && info->num > 0 && !_buffer) return;

                if (!_buffer) {
                    _buffer = new std::vector<uint8_t>();
                }
                if (info->index == 0) {
                    //New frame => preallocate memory
                    if (_cb_on_fragments) {
                        _buffer->reserve(info->len + 1);
                    } else {
                        /* The current fragment would lead to a message which is
                           too long. So discard everything received so far. */
                        if (info->len + _buffer->size() > MAX_WS_MSG_SIZE) {
                            delete _buffer;
                            _buffer = 0;
                            return;
                        } else {
                            _buffer->reserve(info->len + _buffer->size() + 1);
                        }
                    }
                }
                //assert(_buffer->size() == info->index);
                _buffer->insert(_buffer->end(), data, data+len);
                if (info->index + len == info->len &&
                    (info->final || _cb_on_fragments)) {
                    // Frame/message complete
                    if (_terminate_string) {
                        _buffer->push_back(0);
                    }
                    _cb(client, _buffer->data(), _buffer->size());
                    _buffer->clear();
                }
            }
        }

    private:

        AwsMessageHandler _cb;
        bool _cb_on_fragments;
        bool _terminate_string;
        std::vector<uint8_t> *_buffer;

};
