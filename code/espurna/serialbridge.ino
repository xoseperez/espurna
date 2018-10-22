/*

SERIAL_BRIDGE MODULE

Copyright (C) 2018 by Throsten von Eicken

Module key prefix: sbr

*/

#if SERIAL_BRIDGE_SUPPORT

#define INFO DEBUG_MSG_P
#define DBG(...)
#define TRC(...)

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

static bool _sbr_overrun = false; // only warn once per overrun event

// helper functions

// _sbrRxBufToUart writes cahrs from an rx buffer to the uart.
// The volatile is there to foil the ino-to-cpp converter (sigh).
static volatile void _sbrRxBufToUart(SbrClient *sbr_cli, int writable) {
    int w = sbr_cli->rxBufSize - sbr_cli->rxBufNext;
    if (w > writable) w = writable;
    DBG(PSTR("[SERIAL_BRIDGE] writing %d buf->uart\n"), w);
    TRC(PSTR("wr<%d>"), w);
    int n = SERIAL_BRIDGE_PORT.write(sbr_cli->rxBuf+sbr_cli->rxBufNext, w);
    sbr_cli->rxBufNext += n;
    if (sbr_cli->rxBufNext == sbr_cli->rxBufSize) {
        DBG(PSTR("[SERIAL_BRIDGE] free 0x%x, cli %x\n"), sbr_cli->rxBuf, sbr_cli);
        free(sbr_cli->rxBuf);
        sbr_cli->rxBuf = 0;
        if (sbr_cli->client) // null if connection is already closed
            sbr_cli->client->ack(sbr_cli->rxBufSize);
    }
}

// client socket event handlers

static void _sbrHandleError(void* arg, AsyncClient* client, int8_t error) {
    INFO(PSTR("[SERIAL_BRIDGE] conn err client %s: %s\n"),
        client->remoteIP().toString().c_str(),
        client->errorToString(error));
}

static void _sbrHandleData(void* arg, AsyncClient* client, void *data, size_t len) {
        SbrClient *sbr_cli = (SbrClient*)arg;
        DBG(PSTR("[SERIAL_BRIDGE] rcv client %s: %d bytes\n"),
                client->remoteIP().toString().c_str(), len);
        size_t writable = SERIAL_BRIDGE_PORT.availableForWrite();
        TRC(PSTR("rx<%d/%d/%d>"), len, sbr_cli->rxBuf ? sbr_cli->rxBufSize - sbr_cli->rxBufNext : 0, writable);
        // if we have buffered chars take this opportunity to stuff some into the uart
        if (writable > 0 && sbr_cli->rxBuf != 0) {
            _sbrRxBufToUart(sbr_cli, writable);
            writable = SERIAL_BRIDGE_PORT.availableForWrite();
        }
        // if we can write all to uart then we're done
        if (!sbr_cli->rxBuf && writable > len) {
            TRC(PSTR("wr{%d}"), len);
            DBG(PSTR("[SERIAL_BRIDGE] writing all %d to uart\n"), len);
            SERIAL_BRIDGE_PORT.write((uint8_t*)data, len);
            return;
        }
        // write what we can
        if (!sbr_cli->rxBuf && writable > 0) {
            client->ackLater();
            TRC(PSTR("wr[%d]"), writable);
            DBG(PSTR("[SERIAL_BRIDGE] writing %d to uart\n"), writable);
            SERIAL_BRIDGE_PORT.write((uint8_t *)data, writable);
            client->ack(writable);
        } else {
            writable = 0;
            client->ackLater();
        }
        // buffer what we couldn't write
        if (!sbr_cli->rxBuf) {
            DBG(PSTR("[SERIAL_BRIDGE] new buffer %d\n"), len-writable);
            sbr_cli->rxBufSize = len-writable;
            sbr_cli->rxBuf = (uint8_t *)calloc(1, sbr_cli->rxBufSize);
            if (sbr_cli->rxBuf != 0) {
                DBG(PSTR("[SERIAL_BRIDGE] calloc 0x%x, cli %x\n"), sbr_cli->rxBuf, sbr_cli);
                sbr_cli->rxBufNext = 0;
                memcpy(sbr_cli->rxBuf, (uint8_t*)data+writable, sbr_cli->rxBufSize);
            } else {
                INFO(PSTR("[SERIAL_BRIDGE] calloc failed\n"));
            }
        } else {
            DBG(PSTR("[SERIAL_BRIDGE] append buffer %d\n"), len-writable);
            int sz = sbr_cli->rxBufSize + len-writable;
            sbr_cli->rxBuf = (uint8_t *)realloc(sbr_cli->rxBuf, sz);
            if (sbr_cli->rxBuf != 0) {
                DBG(PSTR("[SERIAL_BRIDGE] realloc 0x%x, cli %x\n"), sbr_cli->rxBuf, sbr_cli);
                memcpy(sbr_cli->rxBuf+sbr_cli->rxBufSize, (uint8_t*)data+writable, len-writable);
                sbr_cli->rxBufSize = sz;
            } else {
                INFO(PSTR("[SERIAL_BRIDGE] realloc failed\n"));
            }
        }
}

static void _sbrHandleDisconnect(void* arg, AsyncClient* client) {
        SbrClient *sbr_cli = (SbrClient*)arg;
        INFO(PSTR("[SERIAL_BRIDGE] client %s disconnect\n"),
            client->remoteIP().toString().c_str());
        sbr_cli->client = 0;
}

static void _sbrHandleTimeOut(void* arg, AsyncClient* client, uint32_t time) {
        INFO(PSTR("[SERIAL_BRIDGE] client %s TCP timeout\n"),
            client->remoteIP().toString().c_str());
}


// server socket event handlers

static void _sbrHandleNewClient(void* arg, AsyncClient* client) {
        INFO(PSTR("[SERIAL_BRIDGE] connect from %s\n"),
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
    if (SERIAL_BRIDGE_PORT.peek() < 0) {
        SERIAL_BRIDGE_PORT.hasOverrun(); // clear flag in uart driver
        _sbr_overrun = false;
        return;
    }
    if (_sbr_clients.empty()) {
        // no client connected, drop incoming chars on the floor
        while (SERIAL_BRIDGE_PORT.read() != -1) ;
        SERIAL_BRIDGE_PORT.hasOverrun(); // clear flag in uart driver
        return;
    }
    // warn about input overrun
    if (!_sbr_overrun && SERIAL_BRIDGE_PORT.hasOverrun()) {
        _sbr_overrun = true;
        INFO(PSTR("[SERIAL_BRIDGE] uart input overrun\n"));
    }
    // we always send the same to all clients: determine minimum we can send to all
    size_t min_sendable = SERIAL_BRIDGE_PORT.available();
    size_t avail = min_sendable;
    for (SbrClient* cli : _sbr_clients) {
        if (!cli->client) continue; // already closed
        if (!cli->client->canSend()) {
            min_sendable = 0;
        } else if (cli->client->space() < min_sendable) {
            min_sendable = cli->client->space();
        }
    }
    if (min_sendable <= 0) { TRC(PSTR("tx{0/%d}"), avail); return; }
    // read from serial into buffer
    char buf[min_sendable];
    for (size_t i=0; i<min_sendable; i++) {
        buf[i] = SERIAL_BRIDGE_PORT.read();
    }
    // send buffer to each client
    for (SbrClient* cli : _sbr_clients) {
        if (!cli->client) continue; // already closed
        size_t n = cli->client->add(buf, min_sendable, 0);
        if (n != min_sendable) { // should never occur..
            INFO(PSTR("[SERIAL_BRIDGE] err client %s: will=%d sendable=%d\n"),
                cli->client->remoteIP().toString().c_str(), n, min_sendable);
        } else {
            TRC(PSTR("tx<%d/%d>"), n, avail);
            DBG(PSTR("[SERIAL_BRIDGE] sent %d bytes to cli %x\n"), n, cli);
        }
        if (!cli->client->send()) INFO(PSTR("[SERIAL_BRIDGE] send failed\n"));
    }
}

// _sbrRecvTCPCheck handles data that got received but couldn't be stuffed into the uart.
void _sbrRecvTCPCheck() {
    for (SbrClient* cli : _sbr_clients) {
        if (cli->rxBuf == 0) continue;
        int writable = SERIAL_BRIDGE_PORT.availableForWrite();
        if (writable <= 0) continue;
        // looks like we have something that we can write to the UART, so do it...
        _sbrRxBufToUart(cli, writable);
    }
}

// _sbrGC garbage collects client descriptors that have no connection and no buffer
void _sbrGC() {
    for (auto cli = _sbr_clients.begin(); cli != _sbr_clients.end(); ) {
        if ((*cli)->client || (*cli)->rxBuf) {
            cli++; // client connected or buffer still has data
        } else {
            cli = _sbr_clients.erase(cli); // no connection and no buffer: garbage collect
        }
    }
}

void _sbrLoop() {
    if (!_sbr_clients.empty()) TRC("{");
    _sbrRecvUartCheck();
    _sbrRecvTCPCheck();
    _sbrGC();
    if (!_sbr_clients.empty()) TRC("}");
}

void serialBridgeSetup() {

    // Init port
    SERIAL_BRIDGE_PORT.setRxBufferSize(2000); // 173ms of buffering at 115200 baud...
    SERIAL_BRIDGE_PORT.begin(SERIAL_BRIDGE_BAUDRATE);

    _sbr_clients.clear();
    AsyncServer* server = new AsyncServer(2323);
    server->onClient(&_sbrHandleNewClient, server);
    server->begin();

    // Register key check
    settingsRegisterKeyCheck(_sbrKeyCheck);

    // Register loop
    espurnaRegisterLoop(_sbrLoop);

}

#endif // SERIAL_BRIDGE_SUPPORT
