/*

Part of MQTT and API modules

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "rpc.h"

#include <Schedule.h>
#include <cstring>

#include "system.h"
#include "utils.h"

bool rpcHandleAction(const String& action) {
    bool result = false;
    if (action.equals("reboot")) {
        result = true;
        schedule_function([]() {
            prepareReset(CustomResetReason::Rpc);
        });
    } else if (action.equals("heartbeat")) {
        result = true;
        systemScheduleHeartbeat();
    }
    return result;
}

PayloadStatus rpcParsePayload(const char* payload, const rpc_payload_check_t ext_check) {

    // Don't parse empty strings
    const auto len = strlen(payload);
    if (!len) return PayloadStatus::Unknown;

    // Check most commonly used payloads
    if (len == 1) {
        if (payload[0] == '0') return PayloadStatus::Off;
        if (payload[0] == '1') return PayloadStatus::On;
        if (payload[0] == '2') return PayloadStatus::Toggle;
        return PayloadStatus::Unknown;
    }

    // If possible, use externally provided payload checker
    if (ext_check) {
        const PayloadStatus result = ext_check(payload);
        if (result != PayloadStatus::Unknown) {
            return result;
        }
    }

    // Finally, check for "OFF", "ON", "TOGGLE" (both lower and upper cases)
    String temp(payload);
    temp.trim();

    if (temp.equalsIgnoreCase("off")) {
        return PayloadStatus::Off;
    } else if (temp.equalsIgnoreCase("on")) {
        return PayloadStatus::On;
    } else if (temp.equalsIgnoreCase("toggle")) {
        return PayloadStatus::Toggle;
    }

    return PayloadStatus::Unknown;

}
