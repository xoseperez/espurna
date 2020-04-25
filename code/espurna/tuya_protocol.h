/*

TUYA MODULE

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <cstdint>

#include "tuya_dataframe.h"
#include "tuya_types.h"
#include "tuya_transport.h"

namespace Tuya {

    // 2 known Data Protocols:
    //
    // id   type      len                 value
    // 0x?? 0x01 0x00 0x01                0x00  - bool (0 for false, 1 for true)
    // 0x?? 0x02 0x00 0x04 0x00 0x00 0x00 0x00  - int
    //
    // Note: 'id' varies between devices. instead, matching DP in sequence with local controllers
    // Note: 'int' type is mostly used for dimmer and while it is 4 byte value,
    //       only the first byte is used (i.e. value is between 0 and 255)

    Type dataType(const DataFrame& frame) {

        if (!frame.length) return Type::UNKNOWN;

        const Type type = static_cast<Type>(frame[1]);

        switch (type) {
            case Type::BOOL:
                if (frame.length != 5) break;
                if (frame[3] != 0x01) break;
                return type;
            case Type::INT:
                if (frame.length != 8) break;
                if (frame[3] != 0x04) break;
                return type;
            default:
                return Type::UNKNOWN;
        }

        return Type::UNKNOWN;

    }

    // Since we know of the type only at runtime, specialize the protocol container

    template <typename T>
    class DataProtocol {

        public:

            DataProtocol(const uint8_t id, const T value) :
                _id(id), _value(value)
            {}

            DataProtocol(const DataFrame& frame);

            uint8_t id() const { return _id; }
            T value() const { return _value; }

            std::vector<uint8_t> serialize();

        private:

            uint8_t _id;
            T _value;

    };

    template <typename T>
    DataProtocol<T>::DataProtocol(const DataFrame& frame) {
        static_assert(sizeof(T) != sizeof(T), "No constructor yet for this type!");
    }

    template <>
    DataProtocol<bool>::DataProtocol(const DataFrame& frame) {
        auto data = frame.cbegin();
        _id = data[0],
        _value = data[4];
    }

    template <>
    DataProtocol<uint32_t>::DataProtocol(const DataFrame& frame) {
        auto data = frame.cbegin();
        _id = data[0];
        _value = static_cast<uint32_t>(data[4] << 24)
               | static_cast<uint32_t>(data[5] << 16)
               | static_cast<uint32_t>(data[6] << 8)
               | static_cast<uint32_t>(data[7]);
    }

    template <>
    std::vector<uint8_t> DataProtocol<bool>::serialize() {
        return std::vector<uint8_t> {
            _id, static_cast<uint8_t>(Type::BOOL), 0x00, 0x01, 
            static_cast<uint8_t>(_value)
        };
    }

    template <>
    std::vector<uint8_t> DataProtocol<uint32_t>::serialize() {
        return std::vector<uint8_t> {
            _id, static_cast<uint8_t>(Type::INT), 0x00, 0x04,
            static_cast<uint8_t>((_value >> 24) & 0xff),
            static_cast<uint8_t>((_value >> 16) & 0xff),
            static_cast<uint8_t>((_value >> 8) & 0xff),
            static_cast<uint8_t>(_value & 0xff)
        };
    }

}
