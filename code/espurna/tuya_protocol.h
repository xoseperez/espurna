/*

TUYA MODULE

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <cstdint>

#include "tuya_dataframe.h"
#include "tuya_types.h"
#include "tuya_transport.h"

namespace tuya {

    // 2 known Data Protocols:
    //
    // id   type      len                 value
    // 0x?? 0x01 0x00 0x01                0x00  - bool (0 for false, 1 for true)
    // 0x?? 0x02 0x00 0x04 0x00 0x00 0x00 0x00  - int
    //
    // Note: 'id' varies between devices. instead, matching DP in sequence with local controllers
    // Note: 'int' type is mostly used for dimmer and while it is 4 byte value,
    //       only the first byte is used (i.e. value is between 0 and 255)

    template <typename T>
    uint8_t dataProtocol(const T& frame) {
        if (!frame.length()) {
            return 0;
        }

        return frame[0];
    }

    template <typename T>
    Type dataType(const T& frame) {
        if (!frame.length()) {
            return Type::UNKNOWN;
        }

        const Type type = static_cast<Type>(frame[1]);
        switch (type) {
            case Type::BOOL:
                if (frame.length() != 5) break;
                if (frame[3] != 0x01) break;
                return type;
            case Type::INT:
                if (frame.length() != 8) break;
                if (frame[3] != 0x04) break;
                return type;
            default:
                return Type::UNKNOWN;
        }

        return Type::UNKNOWN;

    }

    // Since we know of the type only at runtime, specialize the protocol container

    template <typename Value>
    class DataProtocol {
    public:
        explicit DataProtocol(const container& data);
        DataProtocol(uint8_t id, Value value) :
            _id(id),
            _value(value)
        {}

        uint8_t id() const {
            return _id;
        }

        Value value() const {
            return _value;
        }

        container serialize();

    private:
        uint8_t _id;
        Value _value;
    };

#if 0
    template <typename T, typename Frame>
    DataProtocol<T>::DataProtocol(const Frame& frame) {
        static_assert(sizeof(T) != sizeof(T), "No constructor yet for this type!");
    }
#endif

    template <>
    DataProtocol<bool>::DataProtocol(const container& data) {
        _id = data[0];
        _value = data[4];
    }

    template <>
    DataProtocol<uint32_t>::DataProtocol(const container& data) {
        _id = data[0];
        _value = static_cast<uint32_t>(data[4] << 24)
               | static_cast<uint32_t>(data[5] << 16)
               | static_cast<uint32_t>(data[6] << 8)
               | static_cast<uint32_t>(data[7]);
    }

    template <>
    container DataProtocol<bool>::serialize() {
        return std::vector<uint8_t> {
            _id, static_cast<uint8_t>(Type::BOOL), 0x00, 0x01,
            static_cast<uint8_t>(_value)
        };
    }

    template <>
    container DataProtocol<uint32_t>::serialize() {
        return std::vector<uint8_t> {
            _id, static_cast<uint8_t>(Type::INT), 0x00, 0x04,
            static_cast<uint8_t>((_value >> 24) & 0xff),
            static_cast<uint8_t>((_value >> 16) & 0xff),
            static_cast<uint8_t>((_value >> 8) & 0xff),
            static_cast<uint8_t>(_value & 0xff)
        };
    }

}
