#pragma once

namespace TuyaDimmer {

    class SerialBuffer {

        constexpr static size_t LIMIT = 255;

    public:

        SerialBuffer(const Stream& stream) :
            _stream(stream)
        {
            _data.reserve(LIMIT);
        }

        bool full() { return (_index >= SerialBuffer::LIMIT); }
        bool done() { return _done; }
        size_t size() { return _index; }

        uint8_t operator[](size_t i) {
            if (i > LIMIT) return 0;
            return _data[i];
        }

        uint8_t version() const {
            return _data[2];
        }

        uint8_t command() const {
            return _data[3];
        }

        uint16_t length() const {
            return (_data[4] << 8) + _data[5];
        }

        uint8_t checksum() const {
            return _data[_index];
        }

        const uint8_t* dataPtr() const {
            if (!length()) return nullptr;
            return _data.data();
        }

        bool available() {
            return _stream.available();
        }

        void rewind() {
            while(_stream.read() != -1);
        }

        void read() {

            if (_done) return;
            if (full()) return;

            int byte = _stream.read();

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
                _read_until += byte;
            }

            // verify that the checksum byte is the same that we got so far
            if (_index >= _read_until) {
                if (_checksum != byte) {
                    reset();
                    return;
                }

                _done = true;
            }

            _data[_index++] = byte;
            if (_index > 1) _checksum += byte;

        }

        void reset() {
            std::fill(_data.begin(), _data.end(), 0);
            _checksum = 0;
            _index = 0;
            _done = false;
        }

    private:

        bool _done = false;
        size_t _index = 0;
        size_t _read_until = LIMIT;
        uint8_t _checksum = 0xff;
        std::vector<uint8_t> _data;
        const Stream _stream&;

    };

}
