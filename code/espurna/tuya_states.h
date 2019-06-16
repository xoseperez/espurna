#pragma once

namespace TuyaDimmer {

    enum class Command : uint8_t {
        Heartbeat = 0x00,
        QueryProduct = 0x01,
        QueryMode = 0x02,
        WiFiStatus = 0x03,
        WiFiResetCfg = 0x04,
        WiFiResetSelect = 0x05,
        SetDP = 0x06,
        ReportDP = 0x07,
        QueryDP = 0x08,
        OTAInit = 0x0a,
        OTATransmit = 0x0b,
        LocalTime = 0x1c,
        WiFiTest = 0x0e
    };

    enum class State {
        INIT,
        HEARTBEAT,
        QUERY_PRODUCT,
        QUERY_MODE,
        QUERY_DP,
        IDLE
    };

    enum class Heartbeat {
        NONE,
        SLOW,
        FAST
    };

}
