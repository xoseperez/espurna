/*

SERIAL_BRIDGE MODULE

Copyright (C) 2018 by Throsten von Eicken

Module key prefix: sbr

*/

#if SERIAL_BRIDGE_SUPPORT

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

typedef struct {
    AsyncClient *client;    // handle to ESPAsyncTCP client
    uint8_t     *rxBuf;     // malloc'ed buffer with received characters
    uint16_t    rxBufSize;  // size of buffer in bytes
    uint16_t    rxBufNext;  // next char in buffer to send to UART
} SbrClient;

static std::vector<SbrClient*> _sbr_clients; // a list to hold all clients

// helper functions

// _sbrRxBufToUart writes cahrs from an rx buffer to the uart.
// The volatile is there to foil the ino-to-cpp converter (sigh).
static volatile void _sbrRxBufToUart(SbrClient *sbr_cli, int writable) {
    int w = sbr_cli->rxBufSize - sbr_cli->rxBufNext;
    if (w > writable) w = writable;
    int n = SERIAL_BRIDGE_PORT.write(sbr_cli->rxBuf+sbr_cli->rxBufNext, w);
    if (n == w) {
        free(sbr_cli->rxBuf);
        sbr_cli->rxBuf = 0;
    } else {
        sbr_cli->rxBufNext += n;
    }
}

// client socket event handlers

static void _sbrHandleError(void* arg, AsyncClient* client, int8_t error) {
    DEBUG_MSG_P(PSTR("[SERIAL_BRIDGE] conn err client %s: %s\n"),
        client->remoteIP().toString().c_str(),
        client->errorToString(error));
}

static void _sbrHandleData(void* arg, AsyncClient* client, void *data, size_t len) {
        SbrClient *sbr_cli = (SbrClient*)arg;
        DEBUG_MSG_P(PSTR("[SERIAL_BRIDGE] rcv client %s: %d bytes\n"),
            client->remoteIP().toString().c_str(), len);
        size_t writable = SERIAL_BRIDGE_PORT.availableForWrite();
        // if we have buffered chars take this opportunity to stuff some into the uart
        if (writable > 0 && sbr_cli->rxBuf != 0) {
            _sbrRxBufToUart(sbr_cli, writable);
            writable = SERIAL_BRIDGE_PORT.availableForWrite();
        }
        // if we can write all to uart then we're done
        if (writable > len) {
            SERIAL_BRIDGE_PORT.write((uint8_t*)data, len);
            return;
        }
        // write what we can, buffer the rest
        if (writable > 0) {
            client->ackLater();
            SERIAL_BRIDGE_PORT.write((uint8_t *)data, writable);
            client->ack(writable);
        } else {
            client->ackLater();
        }
        if (sbr_cli->rxBuf == 0) {
            sbr_cli->rxBufSize = len-writable;
            sbr_cli->rxBuf = (uint8_t *)calloc(1, sbr_cli->rxBufSize);
            sbr_cli->rxBufNext = 0;
            memcpy(sbr_cli->rxBuf, (uint8_t*)data+writable, sbr_cli->rxBufSize);
        } else {
            int sz = sbr_cli->rxBufSize + len-writable;
            sbr_cli->rxBuf = (uint8_t *)realloc(sbr_cli->rxBuf, sz);
            memcpy(sbr_cli->rxBuf+sbr_cli->rxBufSize, (uint8_t*)data+writable, len-writable);
            sbr_cli->rxBufSize = sz;
        }
}

static void _sbrHandleDisconnect(void* arg, AsyncClient* client) {
        DEBUG_MSG_P(PSTR("[SERIAL_BRIDGE] client %s disconnect\n"),
            client->remoteIP().toString().c_str());
}

static void _sbrHandleTimeOut(void* arg, AsyncClient* client, uint32_t time) {
        DEBUG_MSG_P(PSTR("[SERIAL_BRIDGE] client %s TCP timeout\n"),
            client->remoteIP().toString().c_str());
}


// server socket event handlers

static void _sbrHandleNewClient(void* arg, AsyncClient* client) {
        DEBUG_MSG_P(PSTR("[SERIAL_BRIDGE] connect from %s\n"),
            client->remoteIP().toString().c_str());

        // add to list
        SbrClient *sbr_cli = (SbrClient*)calloc(1, sizeof(SbrClient));
        sbr_cli->client = client;
        _sbr_clients.push_back(sbr_cli);

        // register events
        client->onData(&_sbrHandleData, sbr_cli);
        client->onError(&_sbrHandleError, sbr_cli);
        client->onDisconnect(&_sbrHandleDisconnect, sbr_cli);
        client->onTimeout(&_sbrHandleTimeOut, sbr_cli);
}

bool _sbrKeyCheck(const char * key) {
    return (strncmp(key, "sbr", 3) == 0);
}


// -----------------------------------------------------------------------------
// SETUP & LOOP
// -----------------------------------------------------------------------------

// _sbrRecvUartCheck checks whether something arrived on the uart and tries to send it out on
// connected clients. It only pulls out of the uart receive buffer what it can send to all clients.
// The assumption here is that the interrupt handler's buffer is sufficient.
void _sbrRecvUartCheck() {
    // check that we have connected clients and there's something to send
    if (SERIAL_BRIDGE_PORT.peek() < 0) return;
    if (_sbr_clients.empty()) {
        // no client connected, drop incoming chars on the floor
        while (SERIAL_BRIDGE_PORT.read() != -1) ;
        return;
    }
    // we always send the same to all clients: determine minimum we can send to all
    size_t min_sendable = SERIAL_BRIDGE_PORT.available();
    for (SbrClient* cli : _sbr_clients) {
        if (cli->client->space() < min_sendable) {
            min_sendable = cli->client->space();
        }
    }
    if (min_sendable <= 0) return;
    // read from serial into buffer
    char buf[min_sendable];
    for (size_t i=0; i<min_sendable; i++) {
        buf[i] = SERIAL_BRIDGE_PORT.read();
    }
    // send buffer to each client
    for (SbrClient* cli : _sbr_clients) {
        size_t n = cli->client->add(buf, min_sendable, 0);
        if (n != min_sendable) {
            DEBUG_MSG_P(PSTR("[SERIAL_BRIDGE] err client %s: will=%d sendable=%d\n"),
                cli->client->remoteIP().toString().c_str(), n, min_sendable);
        }
        cli->client->send();
    }
}

// _sbrRecvTCPCheck handles data that got received but couldn't be stuffed into the uart.
void _sbrRecvTCPCheck() {
    for (SbrClient* cli : _sbr_clients) {
        if (cli->rxBuf == 0) continue;
        int writable = SERIAL_BRIDGE_PORT.availableForWrite();
        if (writable <= 0) continue;
        // looks like we have something that we can write to the UART, so do it...
        int w = cli->rxBufSize - cli->rxBufNext;
        if (w > writable) w = writable;
        int n = SERIAL_BRIDGE_PORT.write(cli->rxBuf+cli->rxBufNext, w);
        if (n == w) {
            free(cli->rxBuf);
            cli->rxBuf = 0;
        } else {
            cli->rxBufNext += n;
        }
    }
}

void _sbrLoop() {
    _sbrRecvUartCheck();
    _sbrRecvTCPCheck();
}

void serialBridgeSetup() {

    // Init port
    SERIAL_BRIDGE_PORT.setRxBufferSize(2000); // 173ms of buffering at 115200 baud...
    SERIAL_BRIDGE_PORT.begin(SERIAL_BRIDGE_BAUDRATE);

    AsyncServer* server = new AsyncServer(2323);
    server->onClient(&_sbrHandleNewClient, server);
    server->begin();

    // Register key check
    settingsRegisterKeyCheck(_sbrKeyCheck);

    // Register loop
    espurnaRegisterLoop(_sbrLoop);

}

#endif // SERIAL_BRIDGE_SUPPORT
