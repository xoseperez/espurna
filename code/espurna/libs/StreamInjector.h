// -----------------------------------------------------------------------------
// Stream Injector
// -----------------------------------------------------------------------------

#pragma once

#define STREAM_INJECTOR_BUFFER_SIZE     32

class StreamInjector : public Stream {

    public:

        typedef std::function<void(uint8_t ch)> writeCallback;

        StreamInjector(Stream& serial) : _stream(serial) {}

        virtual void callback(writeCallback c) {
            _callback = c;
        }

        virtual size_t write(uint8_t ch) {
            if (_callback) _callback(ch);
            return _stream.write(ch);
        }

        virtual int read() {
            int ch = _stream.read();
            if (ch == -1) {
                if (_buffer_read != _buffer_write) {
                    ch = _buffer[_buffer_read];
                    _buffer_read = (_buffer_read + 1) % STREAM_INJECTOR_BUFFER_SIZE;
                }
            }
            return ch;
        }

        virtual int available() {
            unsigned int bytes = _stream.available();
            if (_buffer_read > _buffer_write) {
                bytes += (_buffer_write - _buffer_read + STREAM_INJECTOR_BUFFER_SIZE);
            } else if (_buffer_read < _buffer_write) {
                bytes += (_buffer_write - _buffer_read);
            }
            return bytes;
        }

        virtual int peek() {
            int ch = _stream.peek();
            if (ch == -1) {
                if (_buffer_read != _buffer_write) {
                    ch = _buffer[_buffer_read];
                }
            }
            return ch;
        }

        virtual void flush() {
            _stream.flush();
            _buffer_read = _buffer_write;
        }

        virtual void inject(char *data, size_t len) {
            for (int i=0; i<len; i++) {
                _buffer[_buffer_write] = data[i];
                _buffer_write = (_buffer_write + 1) % STREAM_INJECTOR_BUFFER_SIZE;
            }
        }

    private:

        Stream& _stream;
        unsigned char _buffer[STREAM_INJECTOR_BUFFER_SIZE];
        unsigned char _buffer_write = 0;
        unsigned char _buffer_read = 0;
        writeCallback _callback = NULL;


};
