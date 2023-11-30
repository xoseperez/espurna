/*

TUYA MODULE

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <cstdint>
#include <vector>

#include "tuya_types.h"
#include "tuya_transport.h"

namespace tuya {

using container = std::vector<uint8_t>;
using const_iterator = container::const_iterator;

namespace util {

template <typename T>
container serialize_frame(const T& frame) {
    container result;

    result.reserve(6 + frame.length());

    result.push_back(frame.version());
    result.push_back(frame.command());
    result.push_back(static_cast<uint8_t>(frame.length() >> 8) & 0xff);
    result.push_back(static_cast<uint8_t>(frame.length() & 0xff));

    if (frame.length()) {
        result.insert(result.end(), frame.cbegin(), frame.cend());
    }

    return result;
}

} // namespace util

class DataFrameView {

public:
    explicit DataFrameView(const Transport& input) :
        _version(input[2]),
        _command(input[3]),
        _length((input[4] << 8) + input[5]),
        _begin(input.cbegin() + 6),
        _end(_begin + _length)
    {}

    explicit DataFrameView(const_iterator it) :
        _version(it[0]),
        _command(it[1]),
        _length((it[2] << 8) + it[3]),
        _begin(it + 4),
        _end(_begin + _length)
    {}

    explicit DataFrameView(const container& data) :
        DataFrameView(data.cbegin())
    {}

    container data() const {
        return container(_begin, _end);
    }

    container serialize() const {
        return util::serialize_frame(*this);
    }

    const_iterator cbegin() const {
        return _begin;
    };

    const_iterator cend() const {
        return _end;
    };

    uint8_t operator[](size_t i) const {
        return *(_begin + i);
    }
     
    uint8_t version() const {
        return _version;
    }

    uint8_t command() const {
        return _command;
    }

    uint16_t length() const {
        return _length;
    }

private:
    uint8_t _version { 0u };
    uint8_t _command { 0u };
    uint16_t _length { 0u };

    const_iterator _begin;
    const_iterator _end;
};

class DataFrame {
public:
    template <typename T>
    DataFrame(Command command, uint8_t version, T&& data) :
        _data(std::forward<T>(data)),
        _command(static_cast<uint8_t>(command)),
        _version(version)
    {}

    template <typename T>
    DataFrame(Command command, T&& data) :
        _data(std::forward<T>(data)),
        _command(static_cast<uint8_t>(command))
    {}

    explicit DataFrame(uint8_t command) :
        _command(command)
    {}

    explicit DataFrame(Command command) :
        DataFrame(static_cast<uint8_t>(command))
    {}

    explicit DataFrame(const Transport& input) :
        _command(input[3]),
        _version(input[2])
    {
        auto length = (input[4] << 8) + input[5];
        _data.reserve(length);

        auto data = input.cbegin() + 6;
        _data.insert(_data.begin(), data, data + length);
    }

    explicit DataFrame(const DataFrameView& view) :
        _data(view.cbegin(), view.cend()),
        _command(view.command()),
        _version(view.version())
    {}

    const container& data() const {
        return _data;
    }

    const_iterator cbegin() const {
        return _data.cbegin();
    };

    const_iterator cend() const {
        return _data.cend();
    };

    uint8_t operator[](size_t i) const {
        return _data[i];
    }

    container serialize() const {
        return util::serialize_frame(*this);
    }

    uint8_t version() const {
        return _version;
    }

    uint8_t command() const {
        return _command;
    }

    uint16_t length() const {
        return _data.size();
    }

private:
    container _data;

    uint8_t _command { 0u };
    uint8_t _version { 0u };
};

} // namespace
