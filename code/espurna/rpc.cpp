/*

Part of MQTT and API modules

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"
#include "rpc.h"

#include <cstring>

#include "system.h"
#include "utils.h"

namespace espurna {
namespace rpc {
namespace {

PROGMEM_STRING(Reboot, "reboot");
PROGMEM_STRING(Heartbeat, "heartbeat");

PROGMEM_STRING(On, "on");
PROGMEM_STRING(Off, "off");
PROGMEM_STRING(Toggle, "toggle");

void rpcPrepareReset() {
    prepareReset(CustomResetReason::Rpc);
}

bool handle_action(StringView action) {
    bool result = false;
    if (action.equals(Reboot)) {
        result = true;
        espurnaRegisterOnce(rpcPrepareReset);
    } else if (action.equals(Heartbeat)) {
        result = true;
        systemScheduleHeartbeat();
    }
    return result;
}

PayloadStatus parse(StringView payload, RpcPayloadCheck check) {
    if (!payload.length()) {
        return PayloadStatus::Unknown;
    }

    // Check most commonly used payloads
    if (payload.length() == 1) {
        switch (*payload.begin()) {
        case '0':
            return PayloadStatus::Off;
        case '1':
            return PayloadStatus::On;
        case '2':
            return PayloadStatus::Toggle;
        }
        return PayloadStatus::Unknown;
    }

    // If possible, use externally provided payload checker
    if (check) {
        const auto result = check(payload);
        if (result != PayloadStatus::Unknown) {
            return result;
        }
    }

    // Finally, check for "OFF", "ON", "TOGGLE" (both lower and upper cases)
    if (payload.equalsIgnoreCase(Off)) {
        return PayloadStatus::Off;
    } else if (payload.equalsIgnoreCase(On)) {
        return PayloadStatus::On;
    } else if (payload.equalsIgnoreCase(Toggle)) {
        return PayloadStatus::Toggle;
    }

    return PayloadStatus::Unknown;
}

} // namespace
} // namespace rpc
} // namespace espurna

bool rpcHandleAction(espurna::StringView action) {
    return espurna::rpc::handle_action(action);
}

PayloadStatus rpcParsePayload(espurna::StringView payload, RpcPayloadCheck check) {
    return espurna::rpc::parse(payload, check);
}

PayloadStatus rpcParsePayload(espurna::StringView payload) {
    return espurna::rpc::parse(payload, nullptr);
}
