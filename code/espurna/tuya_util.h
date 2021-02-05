/*

TUYA MODULE

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "tuya_types.h"

namespace tuya {

inline bool operator==(uint8_t lhs, Command rhs) {
    return lhs == static_cast<uint8_t>(rhs);
}

inline bool operator==(Command lhs, uint8_t rhs) {
    return static_cast<uint8_t>(lhs) == rhs;
}

inline bool operator!=(uint8_t lhs, Command rhs) {
    return !(lhs == rhs);
}

inline bool operator!=(Command lhs, uint8_t rhs) {
    return !(lhs == rhs);
}

struct StateId {
    StateId() = default;

    void filter(bool value) {
        _filter = value;
    }

    bool filter() {
        return _filter;
    }

    uint8_t id() {
        return _id;
    }

    StateId& operator=(uint8_t value) {
        _id = value;
        return *this;
    }

    explicit operator bool() {
        return _id != 0u;
    }

private:
    uint8_t _id { 0 };
    bool _filter { false };
};

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
