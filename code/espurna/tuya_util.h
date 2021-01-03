/*

TUYA MODULE

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <cstdint>
#include <algorithm>
#include <vector>

namespace tuya {

struct Dp {
    Type type;
    uint8_t id;
};

struct DpId {
    uint8_t id;
    uint8_t dp;
};

// Specifically for relay (or channel) <=> DP id association
// Caller is expected to check for uniqueness manually, when `add(...)`ing

struct DpMap {
    using map_type = std::vector<DpId>;
    DpMap() = default;

    bool add(uint8_t id, uint8_t dp) {
        for (auto& map : _map) {
            if ((map.id == id) || (map.dp == dp)) {
                return false;
            }
        }
        _map.push_back(DpId{id, dp});
        return true;
    }

    const map_type& map() {
        return _map;
    }

    const DpId* dp(unsigned char id) const {
        for (const auto& map : _map) {
            if (map.id == id) {
                return &map;
            }
        }

        return nullptr;
    }

    const DpId* id(unsigned char dp) const {
        for (const auto& map : _map) {
            if (map.dp == dp) {
                return &map;
            }
        }

        return nullptr;
    }

    size_t size() const {
        return _map.size();
    }

private:
    map_type _map;
};

using Dps = std::vector<Dp>;

class Discovery {
public:
    Discovery() = delete;
    Discovery(uint32_t start, uint32_t timeout) :
        _start(start),
        _timeout(timeout)
    {}

    explicit Discovery(uint32_t timeout) :
        Discovery(millis(), timeout)
    {}

    explicit operator bool() {
        return (millis() - _start > _timeout);
    }

    void feed() {
        _start = millis();
    }

    void add(Type type, uint8_t dp) {
        feed();
        _dps.push_back(Dp{type, dp});
    }

    Dps& get() {
        return _dps;
    }

private:
    std::vector<Dp> _dps;

    uint32_t _start;
    const uint32_t _timeout;
};

} // namespace tuya
