/*

Part of MQTT and API modules

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"
#include "rpc.h"

#include <cstring>

#include "system_orch.h"
#include "utils.h"

namespace espurna {
namespace rpc {
namespace {

STRING_VIEW_INLINE(Heartbeat, "heartbeat");
STRING_VIEW_INLINE(Reboot, "reboot");
STRING_VIEW_INLINE(Reload, "reload");

STRING_VIEW_INLINE(On, "on");
STRING_VIEW_INLINE(Off, "off");
STRING_VIEW_INLINE(Toggle, "toggle");

void prepareReset() {
    ::prepareReset(CustomResetReason::Rpc);
}

void prepareResetOnce() {
    espurnaRegisterOnce(espurna::rpc::prepareReset);
}

using Callback = void (*)();

struct Action {
    StringView name;
    Callback callback;
};

constexpr const Action Actions[] PROGMEM {
    {Heartbeat, systemScheduleHeartbeat},
    {Reboot, prepareResetOnce},
    {Reload, espurnaReload},
};

bool handle_action(StringView other) {
    for (const auto& action : Actions) {
        if (action.name.equals(other)) {
            action.callback();
            return true;
        }
    }

    return false;
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

