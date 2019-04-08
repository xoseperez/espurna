/*

SyncClientWrap

Temporary wrap to fix https://github.com/me-no-dev/ESPAsyncTCP/issues/109
*/

#pragma once

#include <SyncClient.h>

// ref Core 2.5.0: cores/esp8266/IPAddress.h
#ifndef CONST
#include <lwip/init.h>

#if LWIP_VERSION_MAJOR == 1
#define CONST
#else
#define CONST const
#endif

#endif

class SyncClientWrap: public SyncClient {

    public:
        SyncClientWrap() {}
        ~SyncClientWrap() {}

        // int connect(const char*, uint16_t);
        using SyncClient::connect;

        int connect(CONST IPAddress& ip, uint16_t port) { IPAddress _ip(ip); return SyncClient::connect(_ip, port); }
        bool flush(unsigned int maxWaitMs = 0) { SyncClient::flush(); return true; }
        bool stop(unsigned int maxWaitMs = 0) { SyncClient::stop(); return true; }

};
