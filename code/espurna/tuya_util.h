/*

TUYA MODULE

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <cstdint>
#include <algorithm>
#include <vector>

namespace tuya {

namespace util {

template <typename T>
bool command_equals(const T& frame, Command command) {
    return (frame.command() == static_cast<uint8_t>(command));
}

}

struct Dp {
    Type type;
    uint8_t id;
};

struct DpRelation {
    uint8_t local_id;
    uint8_t dp_id;
};

bool operator==(const DpRelation& lhs, const DpRelation& rhs) {
    return (lhs.local_id == rhs.local_id) || (lhs.dp_id == rhs.dp_id);
}

// Specifically for relay (or channel) <=> DP id association
// Caller is expected to check for uniqueness manually, when `add(...)`ing

struct DpMap {
    using map_type = std::vector<DpRelation>;
    DpMap() = default;

    bool exists(const DpRelation& other) {
        for (const auto& entry : _map) {
            if (entry == other) {
                return true;
            }
        }

        return false;
    }

    bool add(const DpRelation& entry) {
        if (!exists(entry)) {
            _map.push_back(entry);
            return true;
        }

        return false;
    }

    bool add(uint8_t local_id, uint8_t dp_id) {
        return add(DpRelation{local_id, dp_id});
    }

    const map_type& map() {
        return _map;
    }

    const DpRelation* find_local(unsigned char local_id) const {
        for (const auto& entry : _map) {
            if (entry.local_id == local_id) {
                return &entry;
            }
        }

        return nullptr;
    }

    const DpRelation* find_dp(unsigned char dp_id) const {
        for (const auto& entry : _map) {
            if (entry.dp_id == dp_id) {
                return &entry;
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
    Dps _dps;

    uint32_t _start;
    const uint32_t _timeout;
};

} // namespace tuya
