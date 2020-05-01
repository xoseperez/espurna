/*

Part of MQTT and API modules

*/

#pragma once

#include "espurna.h"

#include <vector>
#include <utility>

enum class PayloadStatus {
    Off = 0,
    On = 1,
    Toggle = 2,
    Unknown = 0xFF
};

using rpc_payload_check_t = PayloadStatus(*)(const char*);

bool rpcHandleAction(const String& action);
PayloadStatus rpcParsePayload(const char* payload, const rpc_payload_check_t ext_check = nullptr);
