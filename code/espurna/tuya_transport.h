#pragma once

#include "tuya_dataframe.h"

namespace TuyaDimmer {

    class BufferedTransport {

        // TODO: half of this?
        constexpr static size_t LIMIT = 256;

        // 9600 baud ~= 1.04 bytes per second
        // 256 * 1.04 = 266.24
        constexpr static size_t TIME_LIMIT = 267;

    public:

        BufferedTransport(Stream& stream) :
            _stream(stream)
        {
            _buffer.reserve(LIMIT);
        }

        bool full() { return (_index >= BufferedTransport::LIMIT); }
        bool done() { return _done; }
        size_t size() { return _index; }

        uint8_t operator[](size_t i) {
            if (i > LIMIT) return 0;
            return _buffer[i];
        }

        uint8_t version() const {
            return _buffer[2];
        }

        uint8_t command() const {
            return _buffer[3];
        }

        uint16_t length() const {
            return (_buffer[4] << 8) + _buffer[5];
        }

        uint8_t checksum() const {
            return _buffer[_index];
        }

        const uint8_t* dataPtr() const {
            if (!length()) return nullptr;
            return _buffer.data() + 6;
        }

        int available() {
            return _stream.available();
        }

        void rewind() {
            while(_stream.read() != -1);
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

        template <typename T = PrintRaw>
        void write(const DataFrame& frame) const {

            uint8_t buffer[6] = {
                0x55, 0xaa,
                frame.version, frame.command,
                uint8_t(frame.length >> 8),
                uint8_t(frame.length & 0xff)
            };

            uint8_t checksum = (0xff + frame.version);
            checksum += frame.command;
            checksum += frame.length;

            T::write(_stream, buffer, 6);

            if (frame.data && frame.length) {

                size_t index = 0;
                while (index < frame.length) {
                    checksum += frame.data[index];
                    T::write(_stream, frame.data[index]);
                    ++index;
                }

            }

            T::write(_stream, checksum);

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
        Stream& _stream;
        unsigned long _last = 0;

    };

}
