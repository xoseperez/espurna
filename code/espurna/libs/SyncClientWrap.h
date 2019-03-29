/*

SyncClientWrap

Temporary wrap to fix https://github.com/me-no-dev/ESPAsyncTCP/issues/109
*/

#pragma once

#include <SyncClient.h>

class SyncClientWrap: public SyncClient {

    public:

        int connect(const char *host, uint16_t port);
        int connect(CONST IPAddress& ip, uint16_t port) { return connect(ip, port); }
        bool flush(unsigned int maxWaitMs = 0) { flush(); return true; }
        bool stop(unsigned int maxWaitMs = 0) { stop(); return true; }

};
