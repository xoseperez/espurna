// -----------------------------------------------------------------------------
// NtpClient overrides to avoid triggering network sync
// -----------------------------------------------------------------------------

#pragma once

#include <WiFiUdp.h>
#include <NtpClientLib.h>

class NTPClientWrap : public NTPClient {

public:

    NTPClientWrap() : NTPClient() {
        udp = new WiFiUDP();
        _lastSyncd = 0;
    }

    bool setInterval(int shortInterval, int longInterval) {
        _shortInterval = shortInterval;
        _longInterval = longInterval;
        return true;
    }

};

// NOTE: original NTP should be discarded by the linker
// TODO: allow NTP client object to be destroyed
NTPClientWrap NTPw;
