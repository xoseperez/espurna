#pragma once

#include "tuya_types.h"
#include "tuya_transport.h"

namespace Tuya {

    class DataFrame {

    private:

    public:

        ~DataFrame() = default;

        DataFrame(DataFrame& rhs) = default;
        DataFrame(DataFrame&& rhs) = default;

        DataFrame(uint8_t command) :
            command(command),
            length(0)
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

        std::vector<uint8_t> serialize() const {
            std::vector<uint8_t> result = {
                version, command,
                uint8_t(length >> 8),
                uint8_t(length & 0xff)
            };

            if (length && (_begin != _end)) {
                result.reserve(result.size() + length);
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
        private:
            void data_update() {
                _begin = _data.cbegin();
                _end = _data.cend();
                length = _data.size();
                if (!length) _begin = _end;
            }
        public:

        // Note: Class constructor is called before member regardless of member list order
        StaticDataFrame(Command command, std::initializer_list<uint8_t> data = {}) :
            DataFrame(command),
            _data(data)
        {
            data_update();
        }

        StaticDataFrame(Command command, std::vector<uint8_t>&& data) :
            DataFrame(command),
            _data(std::move(data))
        {
            data_update();
        }

        StaticDataFrame(StaticDataFrame& frame) :
            DataFrame(frame),
            _data(frame._data)
        {
            data_update();
        }

        StaticDataFrame(StaticDataFrame&& frame) :
            DataFrame(std::move(frame))
        {
            _data = std::move(frame._data);
            data_update();
        }


        protected:
            std::vector<uint8_t> _data;
    };

    inline DataFrame fromTransport(const Transport& input) {
        return DataFrame(input.version(), input.command(), input.length(), input.dataBegin(), input.dataEnd());
    }

}
