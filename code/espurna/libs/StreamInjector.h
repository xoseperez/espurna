/*

StreamInjector

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#pragma once

#include <Stream.h>

class StreamInjector : public Stream {

    public:

        typedef std::function<void(uint8_t ch)> writeCallback;

        StreamInjector(size_t buflen = 128) : _buffer_size(buflen) {
            _buffer = new char[buflen];
        }

        ~StreamInjector() {
            delete[] _buffer;
        }

        // ---------------------------------------------------------------------

        virtual uint8_t inject(char ch) {
            _buffer[_buffer_write] = ch;
            _buffer_write = (_buffer_write + 1) % _buffer_size;
            return 1;
        }

        virtual uint8_t inject(char *data, size_t len) {
            for (uint8_t i=0; i<len; i++) {
                inject(data[i]);
            }
            return len;
        }

        virtual void callback(writeCallback c) {
            _callback = c;
        }

        // ---------------------------------------------------------------------

        virtual size_t write(uint8_t ch) {
            if (_callback) _callback(ch);
            return 1;
        }

        virtual int read() {
            int ch = -1;
            if (_buffer_read != _buffer_write) {
                ch = _buffer[_buffer_read];
                _buffer_read = (_buffer_read + 1) % _buffer_size;
            }
            return ch;
        }

        virtual int available() {
            unsigned int bytes = 0;
            if (_buffer_read > _buffer_write) {
                bytes += (_buffer_write - _buffer_read + _buffer_size);
            } else if (_buffer_read < _buffer_write) {
                bytes += (_buffer_write - _buffer_read);
            }
            return bytes;
        }

        virtual int peek() {
            int ch = -1;
            if (_buffer_read != _buffer_write) {
                ch = _buffer[_buffer_read];
            }
            return ch;
        }

        virtual void flush() {
            _buffer_read = _buffer_write;
        }

    private:

        char * _buffer;
        unsigned char _buffer_size;
        unsigned char _buffer_write = 0;
        unsigned char _buffer_read = 0;
        writeCallback _callback = NULL;

};
