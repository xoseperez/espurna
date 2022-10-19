/*

Part of MQTT and API modules

*/

#pragma once

#include <Arduino.h>

#include "types.h"

// --------------------------------------------------------------------------

enum class PayloadStatus {
    Off = 0,
    On = 1,
    Toggle = 2,
    Unknown = 0xFF
};

using RpcPayloadCheck = PayloadStatus(*)(espurna::StringView);

bool rpcHandleAction(espurna::StringView);
PayloadStatus rpcParsePayload(espurna::StringView, RpcPayloadCheck);
PayloadStatus rpcParsePayload(espurna::StringView);
