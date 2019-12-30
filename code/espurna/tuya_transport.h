/*

TUYA MODULE

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Print.h>
#include <StreamString.h>

#include <iterator>
#include <vector>

namespace Tuya {

    class PrintRaw {
        public:
            static void write(Print& printer, uint8_t data) {
                printer.write(data);
            }

            static void write(Print& printer, const uint8_t* data, size_t size) {
                printer.write(data, size);
            }
    };

    class PrintHex {
        public:
            static void write(Print& printer, uint8_t data) {
                char buffer[3] = {0};
                snprintf(buffer, sizeof(buffer), "%02x", data);
                printer.write(buffer, 2);
            }

            static void write(Print& printer, const uint8_t* data, size_t size) {
                for (size_t n=0; n<size; ++n) {
                    char buffer[3] = {0};
                    snprintf(buffer, sizeof(buffer), "%02x", data[n]);
                    printer.write(buffer, 2);
                }
            }
    };

    class StreamWrapper {

    protected:

        Stream& _stream;

    public:

        StreamWrapper(Stream& stream) :
            _stream(stream)
        {}

        int available() {
            return _stream.available();
        }

        void rewind() {
            while(_stream.read() != -1);
        }

    };

    class Output : public virtual StreamWrapper {

    public:


        Output(Stream& stream) :
            StreamWrapper(stream) 
        {}

        Output(StreamString& stream, size_t length) : 
            Output(stream)
        {
            stream.reserve((length * 2) + 1);
        }

        template <typename T, typename PrintType>
        void _write(const T& data) {

            const uint8_t header[2] = {0x55, 0xaa};
            uint8_t checksum = 0xff;

            PrintType::write(_stream, header, 2);

            for (auto it = data.cbegin(); it != data.cend(); ++it) {
                checksum += *it;
                PrintType::write(_stream, *it);
            }

            PrintType::write(_stream, checksum);

        }

        template <typename T>
        void write(const T& data) {
            _write<T, PrintRaw>(data);
        }

        template <typename T>
        void writeHex(const T& data) {
            _write<T, PrintHex>(data);
        }


    };

    class Input : public virtual StreamWrapper {

        // Buffer depth based on the SDK recommendations
        constexpr static size_t LIMIT = 256;

        // 9600 baud ~= 1.04 bytes per second
        // 256 * 1.04 = 266.24
        constexpr static size_t TIME_LIMIT = 267;

        using const_iterator = std::vector<uint8_t>::const_iterator;

    public:

        Input(Stream& stream) :
            StreamWrapper(stream)
        {
            _buffer.reserve(LIMIT);
        }

        bool full() { return (_index >= LIMIT); }
        bool done() { return _done; }
        size_t size() { return _index; }

        uint8_t operator[](size_t i) {
            if (i > LIMIT) return 0;
            return _buffer[i];
        }

        const_iterator cbegin() const {
            return _buffer.cbegin();
        }

        const_iterator cend() const {
            return _buffer.cend();
        }

        void read() {

            if (_done) return;
            if (full()) return;

            if (_last && (millis() - _last > TIME_LIMIT)) reset();
            _last = millis();

            int byte = _stream.read();
            if (byte < 0) return;

            //DEBUG_MSG("i=%u byte=%u chk=%u:%u\n",
            //  _index, byte, _checksum, uint8_t(_checksum + byte));

            // check that header value is 0x55aa
            if (0 == _index) {
                if (0x55 != byte) return;
            } else if (1 == _index) {
                if (0xaa != byte) return;
            }

            // set read boundary from received length
            if (4 == _index) {
                _read_until = byte << 8;
            }

            if (5 == _index) {
                _read_until += byte + _index + 1;
                //DEBUG_MSG("read_until=%u\n", _read_until);
            }

            // verify that the checksum byte is the same that we got so far
            if ((_index > 5) && (_index >= _read_until)) {
                if (_checksum != byte) {
                    //DEBUG_MSG("chk err, recv=%u calc=%u\n", byte, _checksum);
                    reset();
                    return;
                }

                //DEBUG_MSG("chk ok\n");
                _done = true;
                return;
            }

            _buffer[_index] = byte;
            _checksum += byte;

            ++_index;

        }

        void reset() {
            std::fill(_buffer.begin(), _buffer.end(), 0);
            _read_until = LIMIT;
            _checksum = 0;
            _index = 0;
            _done = false;
            _last = 0;
        }

    private:

        bool _done = false;
        size_t _index = 0;
        size_t _read_until = LIMIT;
        uint8_t _checksum = 0;
        std::vector<uint8_t> _buffer;
        unsigned long _last = 0;

    };

    class Transport : public Input, public Output, public virtual StreamWrapper {
    public:
        Transport(Stream& stream) :
            StreamWrapper(stream), Input(stream), Output(stream)
        {}
    };

}
