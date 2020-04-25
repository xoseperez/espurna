/*

TUYA MODULE

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <cstdint>
#include <vector>

#include "tuya_types.h"
#include "tuya_transport.h"

namespace Tuya {

    class DataFrame {

    public:

        using container = std::vector<uint8_t>;
        using const_iterator = container::const_iterator;

        DataFrame(DataFrame& rhs) = delete;

        ~DataFrame() = default;
        DataFrame(DataFrame&& rhs) = default;

        DataFrame(uint8_t command) :
            command(command),
            length(0)
        {}

        DataFrame(Command command) :
            DataFrame(static_cast<uint8_t>(command))
        {}

        DataFrame(Command command, uint16_t length,
                const const_iterator begin,
                const const_iterator end) :
            command(static_cast<uint8_t>(command)),
            length(length),
            _begin(begin),
            _end(end)
        {}

        DataFrame(uint8_t version, uint8_t command, uint16_t length,
                const const_iterator begin,
                const const_iterator end) :
            version(version),
            command(command),
            length(length),
            _begin(begin),
            _end(end)
        {}

        DataFrame(Command command, std::initializer_list<uint8_t> data) :
            command(static_cast<uint8_t>(command)),
            length(data.size()),
            _data(data),
            _begin(_data.cbegin()),
            _end(_data.cend())
        {}

        DataFrame(Command command, std::vector<uint8_t>&& data) :
            command(static_cast<uint8_t>(command)),
            length(data.size()),
            _data(std::move(data)),
            _begin(_data.cbegin()),
            _end(_data.cend())
        {}

        DataFrame(const_iterator iter) :
            version(iter[2]),
            command(iter[3]),
            length((iter[4] << 8) + iter[5]),
            _begin(iter + 6),
            _end(iter + 6 + length)
        {}

        DataFrame(const Transport& input) :
            DataFrame(input.cbegin())
        {}

        bool commandEquals(Command command) const {
            return (static_cast<uint8_t>(command) == this->command);
        }

        const_iterator cbegin() const {
            return _begin;
        };

        const_iterator cend() const {
            return _end;
        };

        uint8_t operator[](size_t i) const {
            if (!length) return 0;
            return _begin[i];
        }

        container serialize() const {
            container result;

            result.reserve(6 + length);
            result.assign({
                version, command,
                uint8_t(length >> 8),
                uint8_t(length & 0xff)
            });

            if (length && (_begin != _end)) {
                result.insert(result.end(), _begin, _end);
            }

            return result;
        }

        uint8_t version = 0;
        uint8_t command = 0;
        uint16_t length = 0;

    protected:

        container _data;
        const_iterator _begin;
        const_iterator _end;

    };


}
