/*

TELNET MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#include <Arduino.h>
#include <Schedule.h>

#include <memory>
#include <list>

#if TELNET_SERVER == TELNET_SERVER_ASYNC

#include <ESPAsyncTCP.h>

struct AsyncBufferedClient {
    public:
        constexpr static const size_t BUFFERS_MAX = 5;
        using buffer_t = std::vector<uint8_t>;

        AsyncBufferedClient(AsyncClient* client);

        size_t write(char c);
        size_t write(const char* data, size_t size=0);

        void flush();
        size_t available();

        bool connect(const char *host, uint16_t port);
        void close(bool now = false);
        bool connected();

    private:
        void _addBuffer();

        static void _trySend(AsyncBufferedClient* client);
        static void _s_onAck(void* client_ptr, AsyncClient*, size_t, uint32_t);
        static void _s_onPoll(void* client_ptr, AsyncClient* client);

        std::unique_ptr<AsyncClient> _client;
        std::list<buffer_t> _buffers;
};

using TTelnetServer = AsyncServer;

#if TELNET_SERVER_ASYNC_BUFFERED
    using TTelnetClient = AsyncBufferedClient;
#else
    using TTelnetClient = AsyncClient;
#endif // TELNET_SERVER_ASYNC_BUFFERED

#elif TELNET_SERVER == TELNET_SERVER_WIFISERVER

using TTelnetServer = WiFiServer;
using TTelnetClient = WiFiClient;

#else
#error "TELNET_SERVER value was not properly set"
#endif

constexpr unsigned char TELNET_IAC = 0xFF;
constexpr unsigned char TELNET_XEOF = 0xEC;

bool telnetConnected();
unsigned char telnetWrite(unsigned char ch);
bool telnetDebugSend(const char* prefix, const char* data);
void telnetSetup();

