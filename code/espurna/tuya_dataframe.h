#pragma once

#include "tuya_types.h"

namespace TuyaDimmer {

    class DataFrame {

    private:

        DataFrame(DataFrame& rhs) { }

    public:

        ~DataFrame() { }

        DataFrame(DataFrame&& rhs) :
            data(rhs.data),
            command(rhs.command),
            version(rhs.version),
            length(rhs.length)
        {}

        DataFrame(uint8_t command) :
            data(nullptr),
            command(command),
            length(0)
        {}

        DataFrame(Command command) :
            DataFrame(static_cast<uint8_t>(command))
        {}

        DataFrame(uint8_t command, const uint8_t* data, uint16_t length, uint8_t version = 0) :
            data(data),
            command(command),
            version(version),
            length(length)
        {}

        DataFrame(Command command, const uint8_t* data, uint16_t length, uint8_t version = 0) :
            data(data),
            command(static_cast<uint8_t>(command)),
            length(length)
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

        const uint8_t* data = nullptr;
        uint8_t command = 0;
        uint8_t version = 0;
        uint16_t length = 0;

    };

}
