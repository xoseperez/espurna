/*

Part of the LED MODULE

*/

#pragma once

#include <cstddef>
#include "system_time.h"

namespace espurna {
namespace led {

using Duration = espurna::time::CpuClock::duration;

struct Delay {
    Duration on;
    Duration off;
    size_t repeats;
};

bool operator==(const Delay&, const Delay&);

} // namespace led
} // namespace espurna

enum class LedMode {
    Manual,
    WiFi,
    Relay,
    RelayInverse,
    FindMe,
    FindMeWiFi,
    On,
    Off,
    Relays,
    RelaysWiFi,
};
