#pragma once

#include "tuya_types.h"
#include "tuya_transport.h"

namespace TuyaDimmer {

    class DataFrame {

    private:

        DataFrame(DataFrame& rhs) { }

    public:

        ~DataFrame() { }

        DataFrame(DataFrame&& rhs) :
            command(rhs.command),
            length(rhs.length),
            _begin(rhs.cbegin()),
            _end(rhs.cend())
        {}

        DataFrame(uint8_t command) :
            command(command),
            length(0),
            _begin(nullptr),
            _end(nullptr)
        {}

        DataFrame(Command command) :
            DataFrame(static_cast<uint8_t>(command))
        {}

        DataFrame(Command command, uint16_t length,
                const std::vector<uint8_t>::const_iterator begin,
                const std::vector<uint8_t>::const_iterator end) :
            command(static_cast<uint8_t>(command)),
            length(length),
            _begin(begin),
            _end(end)
        {}

        DataFrame(uint8_t version, uint8_t command, uint16_t length,
                const std::vector<uint8_t>::const_iterator begin,
                const std::vector<uint8_t>::const_iterator end) :
            version(version),
            command(command),
            length(length),
            _begin(begin),
            _end(end)
        {}

        bool commandEquals(Command command) const {
            return (static_cast<uint8_t>(command) == this->command);
        }

        std::vector<uint8_t>::const_iterator cbegin() const {
            return _begin;
        };

        std::vector<uint8_t>::const_iterator cend() const {
            return _end;
        };

        uint8_t operator[](size_t i) const {
            if (!length) return 0;
            return _begin[i];
        }

        const std::vector<uint8_t> serialize() const {
            std::vector<uint8_t> result = {
                version, command,
                uint8_t(length >> 8),
                uint8_t(length & 0xff)
            };

            result.reserve(result.size() + length);
            if (length && (_begin != _end)) {
                result.insert(result.end(), _begin, _end);
            }

            return result;
        }

        uint8_t version = 0;
        uint8_t command = 0;
        uint16_t length = 0;

    protected:

        std::vector<uint8_t>::const_iterator _begin;
        std::vector<uint8_t>::const_iterator _end;

    };

    class StaticDataFrame : public DataFrame {
        public:

        // Note: Class constructor is called before member regardless of member list order

        StaticDataFrame(Command command, std::initializer_list<uint8_t> data = {}) :
            DataFrame(command),
            _data(data)
        {
            _begin = _data.cbegin();
            _end = _data.cend();
            length = std::distance(_begin, _end);
        }

        StaticDataFrame(Command command, std::vector<uint8_t>&& data) :
            DataFrame(command),
            _data(data)
        {
            _begin = _data.cbegin();
            _end = _data.cend();
            length = std::distance(_begin, _end);
        }

        std::vector<uint8_t>::const_iterator dumb_begin() const {
            return _data.cbegin();
        };

        std::vector<uint8_t>::const_iterator dumb_end() const {
            return _data.cend();
        };

        protected:
            std::vector<uint8_t> _data;
    };

    inline DataFrame fromTransport(const Transport& input) {
        return DataFrame(input.version(), input.command(), input.length(), input.dataStart(), input.dataEnd());
    }

}
