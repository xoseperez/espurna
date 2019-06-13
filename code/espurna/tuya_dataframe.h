#pragma once

#include "tuya_states.h"

namespace TuyaDimmer {

    struct payload_t {
        Command command;
        std::vector<uint8_t> data;
    };

    void _printRaw(Print& stream, uint8_t data) {
        stream.write(data);
    }

    void _printRawArray(Print& stream, uint8_t* data, size_t size) {
        stream.write(data, size);
    }

    void _printHex(Print& stream, uint8_t data) {
        stream.print(data, HEX);
    }

    void _printHexArray(Print& stream, uint8_t* data, size_t size) {
        for (size_t n=0; n<size; ++n) {
            stream.print(data[n], HEX);
        }
    }

    class DataFrame {

    private:

        DataFrame(DataFrame& rhs) { }
        DataFrame(DataFrame&& rhs) { }

    public:

        ~DataFrame() { }

        DataFrame(uint8_t command) :
            command(command),
            length(0)
        {}

        DataFrame(Command command) :
            DataFrame(static_cast<uint8_t>(command))
        {}

        DataFrame(uint8_t command, const uint8_t* data, uint16_t length) :
            command(command),
            data(data),
            length(length)
        {}

        DataFrame(Command command, const uint8_t* data, uint16_t length) :
            DataFrame(static_cast<uint8_t>(command), data, length)
        {}

        DataFrame(const SerialBuffer& buffer) :
            DataFrame(buffer.command(), buffer.dataPtr(), buffer.length())
        {}

        DataFrame(const payload_t& payload) :
            command(static_cast<uint8_t>(payload.command)),
            length(payload.data.size()),
            data(payload.data.data())
        {}

        bool operator &(Command command) const {
            return (static_cast<uint8_t>(command) == this->command);
        }

        uint8_t checksum() {
            uint8_t result = (0xff + version);
            result += command;
            result += length;

            if (!this->data) return result;
            if (!this->length) return result;

            size_t index = this->length;
            while (--index) {
                result += this->data[index];
            }

            return result;
        }

        void printTo(Print& stream, bool pretty=false) const {

            auto write_byte_func = _printRaw;
            auto write_array_func = _printRawArray;
            if (pretty) {
                write_byte_func = _printHex;
                write_array_func = _printHexArray;
            }

            uint8_t buffer[6] = {
                0x55, 0xaa,
                version, command,
                uint8_t(length >> 8), uint8_t(length & 0xff)
            };

            uint8_t checksum = (0xff + version);
            checksum += command;
            checksum += length;

            write_array_func(stream, buffer, 6);

            if (this->data && this->length) {

                size_t index = 0;
                while (index < this->length) {
                    checksum += this->data[index];
                    write_byte_func(stream, this->data[index]);
                    ++index;
                }

            }

            write_byte_func(stream, checksum);

        }

        const uint8_t* data = nullptr;
        uint8_t command = 0;
        uint8_t version = 0;
        uint16_t length = 0;

    };

}
