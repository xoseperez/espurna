/*

RF BRIDGE MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "rfbridge.h"

#if RF_SUPPORT

#include <queue>

#include "api.h"
#include "relay.h"
#include "terminal.h"
#include "mqtt.h"
#include "ws.h"

// -----------------------------------------------------------------------------
// DEFINITIONS
// -----------------------------------------------------------------------------

// EFM8 Protocol

#define RF_MESSAGE_SIZE         9
#define RF_MAX_MESSAGE_SIZE     (112+4)
#define RF_CODE_START           0xAA
#define RF_CODE_ACK             0xA0
#define RF_CODE_LEARN           0xA1
#define RF_CODE_LEARN_KO        0xA2
#define RF_CODE_LEARN_OK        0xA3
#define RF_CODE_RFIN            0xA4
#define RF_CODE_RFOUT           0xA5
#define RF_CODE_SNIFFING_ON     0xA6
#define RF_CODE_SNIFFING_OFF    0xA7
#define RF_CODE_RFOUT_NEW       0xA8
#define RF_CODE_LEARN_NEW       0xA9
#define RF_CODE_LEARN_KO_NEW    0xAA
#define RF_CODE_LEARN_OK_NEW    0xAB
#define RF_CODE_RFOUT_BUCKET    0xB0
#define RF_CODE_STOP            0x55

// Settings

#define RF_MAX_KEY_LENGTH       (9)

// -----------------------------------------------------------------------------
// GLOBALS TO THE MODULE
// -----------------------------------------------------------------------------

unsigned char _uartbuf[RF_MESSAGE_SIZE+3] = {0};
unsigned char _uartpos = 0;
unsigned char _learnId = 0;

bool _learnStatus = true;
bool _rfbin = false;

struct rfb_message_t {
    uint8_t code[RF_MESSAGE_SIZE];
    uint8_t times;
};
static std::queue<rfb_message_t> _rfb_message_queue;

#if RFB_DIRECT
    RCSwitch * _rfModem;
    bool _learning = false;
#endif

bool _rfb_receive = false;
bool _rfb_transmit = false;
unsigned char _rfb_repeat = RF_SEND_TIMES;

// -----------------------------------------------------------------------------
// PRIVATES
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

void _rfbWebSocketSendCodeArray(JsonObject& root, unsigned char start, unsigned char size) {
    JsonObject& rfb = root.createNestedObject("rfb");
    rfb["size"] = size;
    rfb["start"] = start;

    JsonArray& on = rfb.createNestedArray("on");
    JsonArray& off = rfb.createNestedArray("off");

    for (uint8_t id=start; id<start+size; id++) {
        on.add(rfbRetrieve(id, true));
        off.add(rfbRetrieve(id, false));
    }
}

void _rfbWebSocketOnVisible(JsonObject& root) {
    root["rfbVisible"] = 1;
}

void _rfbWebSocketOnConnected(JsonObject& root) {
    root["rfbRepeat"] = getSetting("rfbRepeat", RF_SEND_TIMES);
    root["rfbCount"] = relayCount();
    #if RFB_DIRECT
        root["rfbdirectVisible"] = 1;
        root["rfbRX"] = getSetting("rfbRX", RFB_RX_PIN);
        root["rfbTX"] = getSetting("rfbTX", RFB_TX_PIN);
    #endif
}

void _rfbWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "rfblearn") == 0) rfbLearn(data["id"], data["status"]);
    if (strcmp(action, "rfbforget") == 0) rfbForget(data["id"], data["status"]);
    if (strcmp(action, "rfbsend") == 0) rfbStore(data["id"], data["status"], data["data"].as<const char*>());
}

bool _rfbWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "rfb", 3) == 0);
}

void _rfbWebSocketOnData(JsonObject& root) {
    _rfbWebSocketSendCodeArray(root, 0, relayCount());    
}

#endif // WEB_SUPPORT

// From a byte array to an hexa char array ("A220EE...", double the size)
static size_t _rfbHexFromBytearray(uint8_t * in, size_t in_size, char * out, size_t out_size) {
    if ((2 * in_size + 1) > (out_size)) return 0;

    static const char base16[] = "0123456789ABCDEF";
    unsigned char index = 0;

    while (index < in_size) {
        out[(index*2)]   = base16[(in[index] & 0xf0) >> 4];
        out[(index*2)+1] = base16[(in[index] & 0xf)];
        ++index;
    }

    out[2*index] = '\0';

    return index ? (1 + (2 * index)) : 0;
}


// From an hexa char array ("A220EE...") to a byte array (half the size)
static size_t _rfbBytearrayFromHex(const char* in, size_t in_size, uint8_t* out, uint8_t out_size) {
    if (out_size < (in_size / 2)) return 0;

    unsigned char index = 0;
    unsigned char out_index = 0;

    auto char2byte = [](char ch) -> uint8_t {
        if ((ch >= '0') && (ch <= '9')) {
            return (ch - '0');
        } else if ((ch >= 'a') && (ch <= 'f')) {
            return 10 + (ch - 'a');
        } else if ((ch >= 'A') && (ch <= 'F')) {
            return 10 + (ch - 'A');
        } else {
            return 0;
        }
    };

    while (index < in_size) {
        out[out_index] = char2byte(in[index]) << 4;
        out[out_index] += char2byte(in[index + 1]);

        index += 2;
        out_index += 1;
    }

    return out_index ? (1 + out_index) : 0;
}

void _rfbAckImpl();
void _rfbLearnImpl();
void _rfbSendImpl(uint8_t * message);
void _rfbReceiveImpl();

bool _rfbMatch(char* code, unsigned char& relayID, unsigned char& value, char* buffer = NULL) {

    if (strlen(code) != 18) return false;

    bool found = false;
    String compareto = String(&code[12]);
    compareto.toUpperCase();
    DEBUG_MSG_P(PSTR("[RF] Trying to match code %s\n"), compareto.c_str());

    for (unsigned char i=0; i<relayCount(); i++) {

        String code_on = rfbRetrieve(i, true);
        if (code_on.length() && code_on.endsWith(compareto)) {
            DEBUG_MSG_P(PSTR("[RF] Match ON code for relay %d\n"), i);
            value = 1;
            found = true;
            if (buffer) strcpy(buffer, code_on.c_str());
        }

        String code_off = rfbRetrieve(i, false);
        if (code_off.length() && code_off.endsWith(compareto)) {
            DEBUG_MSG_P(PSTR("[RF] Match OFF code for relay %d\n"), i);
            if (found) value = 2;
            found = true;
            if (buffer) strcpy(buffer, code_off.c_str());
        }

        if (found) {
            relayID = i;
            return true;
        }

    }

    return false;

}

void _rfbDecode() {

    static unsigned long last = 0;
    if (millis() - last < RF_RECEIVE_DELAY) return;
    last = millis();

    uint8_t action = _uartbuf[0];
    char buffer[RF_MESSAGE_SIZE * 2 + 1] = {0};
    DEBUG_MSG_P(PSTR("[RF] Action 0x%02X\n"), action);

    if (action == RF_CODE_LEARN_KO) {
        _rfbAckImpl();
        DEBUG_MSG_P(PSTR("[RF] Learn timeout\n"));
    }

    if (action == RF_CODE_LEARN_OK || action == RF_CODE_RFIN) {

        _rfbAckImpl();
        if (_rfbHexFromBytearray(&_uartbuf[1], RF_MESSAGE_SIZE, buffer, sizeof(buffer))) {
            DEBUG_MSG_P(PSTR("[RF] Received message '%s'\n"), buffer);
        }

    }

    if (action == RF_CODE_LEARN_OK) {

        DEBUG_MSG_P(PSTR("[RF] Learn success\n"));
        rfbStore(_learnId, _learnStatus, buffer);

        // Websocket update
        #if WEB_SUPPORT
            wsPost([](JsonObject& root) {
                _rfbWebSocketSendCodeArray(root, _learnId, 1);
            });
        #endif

    }

    if (action == RF_CODE_RFIN) {

        /* Look for the code, possibly replacing the code with the exact learned one on match
         * we want to do this on learn too to be sure that the learned code is the same if it
         * is equivalent
         */
        unsigned char id;
        unsigned char status;
        bool matched = _rfbMatch(buffer, id, status, buffer);
        
        if (matched) {
            DEBUG_MSG_P(PSTR("[RF] Matched message '%s'\n"), buffer);
            _rfbin = true;
            if (status == 2) {
                relayToggle(id);
            } else {
                relayStatus(id, status == 1);
            }
        }

        #if MQTT_SUPPORT
            mqttSend(MQTT_TOPIC_RFIN, buffer, false, false);
        #endif

    }

}


//
// RF handler implementations
//

#if !RFB_DIRECT // Default for ITEAD SONOFF RFBRIDGE

void _rfbSendRaw(const uint8_t *message, unsigned char size) {
    Serial.write(message, size);
}

void _rfbAckImpl() {
    DEBUG_MSG_P(PSTR("[RF] Sending ACK\n"));
    Serial.println();
    Serial.write(RF_CODE_START);
    Serial.write(RF_CODE_ACK);
    Serial.write(RF_CODE_STOP);
    Serial.flush();
    Serial.println();
}

void _rfbLearnImpl() {
    DEBUG_MSG_P(PSTR("[RF] Sending LEARN\n"));
    Serial.println();
    Serial.write(RF_CODE_START);
    Serial.write(RF_CODE_LEARN);
    Serial.write(RF_CODE_STOP);
    Serial.flush();
    Serial.println();
}

void _rfbSendImpl(uint8_t * message) {
    Serial.println();
    Serial.write(RF_CODE_START);
    Serial.write(RF_CODE_RFOUT);
    _rfbSendRaw(message, RF_MESSAGE_SIZE);
    Serial.write(RF_CODE_STOP);
    Serial.flush();
    Serial.println();
}

void _rfbReceiveImpl() {

    static bool receiving = false;

    while (Serial.available()) {

        yield();
        uint8_t c = Serial.read();
        //DEBUG_MSG_P(PSTR("[RF] Received 0x%02X\n"), c);

        if (receiving) {
            if (c == RF_CODE_STOP && (_uartpos == 1 || _uartpos == RF_MESSAGE_SIZE + 1)) {
                _rfbDecode();
                receiving = false;
            } else if (_uartpos <= RF_MESSAGE_SIZE) {
                _uartbuf[_uartpos++] = c;
            } else {
                // wrong message, should have received a RF_CODE_STOP
                receiving = false;
            }
        } else if (c == RF_CODE_START) {
            _uartpos = 0;
            receiving = true;
        }

    }

}

void _rfbParseRaw(char * raw) {
    int rawlen = strlen(raw);
    if (rawlen > (RF_MAX_MESSAGE_SIZE * 2)) return;
    if ((rawlen < 2) || (rawlen & 1)) return;

    DEBUG_MSG_P(PSTR("[RF] Sending RAW MESSAGE '%s'\n"), raw);

    uint8_t message[RF_MAX_MESSAGE_SIZE];
    size_t bytes = _rfbBytearrayFromHex(raw, (size_t)rawlen, message, sizeof(message));
    _rfbSendRaw(message, bytes);
}

#else // RFB_DIRECT

void _rfbAckImpl() {}

void _rfbLearnImpl() {
    DEBUG_MSG_P(PSTR("[RF] Entering LEARN mode\n"));
    _learning = true;
}

void _rfbSendImpl(uint8_t * message) {

    if (!_rfb_transmit) return;

    unsigned int protocol = message[1];
    unsigned int timing =
        (message[2] <<  8) |
        (message[3] <<  0) ;
    unsigned int bitlength = message[4];
    unsigned long rf_code =
        (message[5] << 24) |
        (message[6] << 16) |
        (message[7] <<  8) |
        (message[8] <<  0) ;
    _rfModem->setProtocol(protocol);
    if (timing > 0) {
        _rfModem->setPulseLength(timing);
    }
    _rfModem->send(rf_code, bitlength);
    _rfModem->resetAvailable();

}

void _rfbReceiveImpl() {

    if (!_rfb_receive) return;

    static long learn_start = 0;
    if (!_learning && learn_start) {
        learn_start = 0;
    }
    if (_learning) {
        if (!learn_start) {
            DEBUG_MSG_P(PSTR("[RF] Arming learn timeout\n"));
            learn_start = millis();
        }
        if (learn_start > 0 && millis() - learn_start > RF_LEARN_TIMEOUT) {
            DEBUG_MSG_P(PSTR("[RF] Learn timeout triggered\n"));
            memset(_uartbuf, 0, sizeof(_uartbuf));
            _uartbuf[0] = RF_CODE_LEARN_KO;
            _rfbDecode();
            _learning = false;
        }
    }

    if (_rfModem->available()) {
        static unsigned long last = 0;
        if (millis() - last > RF_DEBOUNCE) {
            last = millis();
            unsigned long rf_code = _rfModem->getReceivedValue();
            if ( rf_code > 0) {
                DEBUG_MSG_P(PSTR("[RF] Received code: %08X\n"), rf_code);
                unsigned int timing = _rfModem->getReceivedDelay();
                memset(_uartbuf, 0, sizeof(_uartbuf));
                unsigned char *msgbuf = _uartbuf + 1;
                _uartbuf[0] = _learning ? RF_CODE_LEARN_OK: RF_CODE_RFIN;
                msgbuf[0] = 0xC0;
                msgbuf[1] = _rfModem->getReceivedProtocol();
                msgbuf[2] = timing  >>  8;
                msgbuf[3] = timing  >>  0;
                msgbuf[4] = _rfModem->getReceivedBitlength();
                msgbuf[5] = rf_code >> 24;
                msgbuf[6] = rf_code >> 16;
                msgbuf[7] = rf_code >>  8;
                msgbuf[8] = rf_code >>  0;
                _rfbDecode();
                _learning = false;
            }
        }
        _rfModem->resetAvailable();
    }

    yield();

}

#endif // RFB_DIRECT

void _rfbEnqueue(uint8_t * code, unsigned char times) {

    if (!_rfb_transmit) return;

    // rc-switch will repeat on its own
    #if RFB_DIRECT
        times = 1;
    #endif

    char buffer[RF_MESSAGE_SIZE];
    _rfbHexFromBytearray(code, RF_MESSAGE_SIZE, buffer, sizeof(buffer));
    DEBUG_MSG_P(PSTR("[RF] Enqueuing MESSAGE '%s' %d time(s)\n"), buffer, times);

    rfb_message_t message;
    memcpy(message.code, code, RF_MESSAGE_SIZE);
    message.times = times;
    _rfb_message_queue.push(message);

}

void _rfbSendQueued() {

    if (!_rfb_transmit) return;

    // Check if there is something in the queue
    if (_rfb_message_queue.empty()) return;

    static unsigned long last = 0;
    if (millis() - last < RF_SEND_DELAY) return;
    last = millis();

    // Pop the first message and send it
    rfb_message_t message = _rfb_message_queue.front();
    _rfb_message_queue.pop();
    _rfbSendImpl(message.code);

    // Push it to the stack again if we need to send it more than once
    if (message.times > 1) {
        message.times = message.times - 1;
        _rfb_message_queue.push(message);
    }

    yield();

}

bool _rfbCompare(const char * code1, const char * code2) {
    return strcmp(&code1[12], &code2[12]) == 0;
}

bool _rfbSameOnOff(unsigned char id) {
    return _rfbCompare(rfbRetrieve(id, true).c_str(), rfbRetrieve(id, false).c_str());
}

void _rfbParseCode(char * code) {

    // The payload may be a code in HEX format ([0-9A-Z]{18}) or
    // the code comma the number of times to transmit it.
    char * tok = strtok(code, ",");

    // Check if a switch is linked to that message
    unsigned char id;
    unsigned char status = 0;
    if (_rfbMatch(tok, id, status)) {
        if (status == 2) {
            relayToggle(id);
        } else {
            relayStatus(id, status == 1);
        }
        return;
    }

    uint8_t message[RF_MESSAGE_SIZE];
    if (_rfbBytearrayFromHex(tok, strlen(tok), message, sizeof(message))) {
        tok = strtok(nullptr, ",");
        uint8_t times = (tok != nullptr) ? atoi(tok) : 1;
        _rfbEnqueue(message, times);
    }

}

#if MQTT_SUPPORT

void _rfbMqttCallback(unsigned int type, const char * topic, char * payload) {

    if (type == MQTT_CONNECT_EVENT) {

        char buffer[strlen(MQTT_TOPIC_RFLEARN) + 3];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s/+"), MQTT_TOPIC_RFLEARN);
        mqttSubscribe(buffer);

        if (_rfb_transmit) {
            mqttSubscribe(MQTT_TOPIC_RFOUT);
        }

        #if !RFB_DIRECT
            mqttSubscribe(MQTT_TOPIC_RFRAW);
        #endif

    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = mqttMagnitude((char *) topic);

        // Check if should go into learn mode
        if (t.startsWith(MQTT_TOPIC_RFLEARN)) {

            _learnId = t.substring(strlen(MQTT_TOPIC_RFLEARN)+1).toInt();
            if (_learnId >= relayCount()) {
                DEBUG_MSG_P(PSTR("[RF] Wrong learnID (%d)\n"), _learnId);
                return;
            }
            _learnStatus = (char)payload[0] != '0';
            _rfbLearnImpl();
            return;

        }

        if (t.equals(MQTT_TOPIC_RFOUT)) {
            _rfbParseCode(payload);
        }

        #if !RFB_DIRECT
            if (t.equals(MQTT_TOPIC_RFRAW)) {
                _rfbParseRaw(payload);
            }
        #endif

    }

}

#endif // MQTT_SUPPORT

#if API_SUPPORT

void _rfbAPISetup() {

    apiRegister(MQTT_TOPIC_RFOUT,
        [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("OK"));
        },
        [](const char * payload) {
            _rfbParseCode((char *) payload);
        }
    );

    apiRegister(MQTT_TOPIC_RFLEARN,
        [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("OK"));
        },
        [](const char * payload) {
            // The payload must be the relayID plus the mode (0 or 1)
            char * tok = strtok((char *) payload, ",");
            if (NULL == tok) return;
            if (!isNumber(tok)) return;
            _learnId = atoi(tok);
            if (_learnId >= relayCount()) {
                DEBUG_MSG_P(PSTR("[RF] Wrong learnID (%d)\n"), _learnId);
                return;
            }
            tok = strtok(NULL, ",");
            if (NULL == tok) return;
            _learnStatus = (char) tok[0] != '0';
            _rfbLearnImpl();
        }
    );

    #if !RFB_DIRECT
        apiRegister(MQTT_TOPIC_RFRAW,
            [](char * buffer, size_t len) {
                snprintf_P(buffer, len, PSTR("OK"));
            },
            [](const char * payload) {
                _rfbParseRaw((char *)payload);
            }
        );
    #endif

}

#endif // API_SUPPORT

#if TERMINAL_SUPPORT

void _rfbInitCommands() {

    terminalRegisterCommand(F("LEARN"), [](Embedis* e) {

        if (e->argc < 3) {
            terminalError(F("Wrong arguments"));
            return;
        }
        
        int id = String(e->argv[1]).toInt();
        if (id >= relayCount()) {
            DEBUG_MSG_P(PSTR("-ERROR: Wrong relayID (%d)\n"), id);
            return;
        }

        int status = String(e->argv[2]).toInt();

        rfbLearn(id, status == 1);

        terminalOK();

    });

    terminalRegisterCommand(F("FORGET"), [](Embedis* e) {

        if (e->argc < 3) {
            terminalError(F("Wrong arguments"));
            return;
        }
        
        int id = String(e->argv[1]).toInt();
        if (id >= relayCount()) {
            DEBUG_MSG_P(PSTR("-ERROR: Wrong relayID (%d)\n"), id);
            return;
        }

        int status = String(e->argv[2]).toInt();

        rfbForget(id, status == 1);

        terminalOK();

    });

    #if !RFB_DIRECT
        terminalRegisterCommand(F("RFB.WRITE"), [](Embedis* e) {
            if (e->argc != 2) return;
            String arg(e->argv[1]);
            uint8_t data[RF_MAX_MESSAGE_SIZE];
            size_t bytes = _rfbBytearrayFromHex(arg.c_str(), arg.length(), data, sizeof(data));
            if (bytes) {
                _rfbSendRaw(data, bytes);
            }
        });
    #endif

}

#endif // TERMINAL_SUPPORT

// -----------------------------------------------------------------------------
// PUBLIC
// -----------------------------------------------------------------------------

void rfbStore(unsigned char id, bool status, const char * code) {
    DEBUG_MSG_P(PSTR("[RF] Storing %d-%s => '%s'\n"), id, status ? "ON" : "OFF", code);
    if (status) {
        setSetting({"rfbON", id}, code);
    } else {
        setSetting({"rfbOFF", id}, code);
    }
}

String rfbRetrieve(unsigned char id, bool status) {
    if (status) {
        return getSetting({"rfbON", id});
    } else {
        return getSetting({"rfbOFF", id});
    }
}

void rfbStatus(unsigned char id, bool status) {

    String value = rfbRetrieve(id, status);
    if (value.length() && !(value.length() & 1)) {

        uint8_t message[RF_MAX_MESSAGE_SIZE];
        size_t bytes = _rfbBytearrayFromHex(value.c_str(), value.length(), message, sizeof(message));
        if (bytes && !_rfbin) {
            if (value.length() == (RF_MESSAGE_SIZE * 2)) {
                _rfbEnqueue(message, _rfbSameOnOff(id) ? 1 : _rfb_repeat);
            } else {
                #if !RFB_DIRECT
                    _rfbSendRaw(message, bytes);
                #endif
            }
        }

    }

    _rfbin = false;

}

void rfbLearn(unsigned char id, bool status) {
    _learnId = id;
    _learnStatus = status;
    _rfbLearnImpl();
}

void rfbForget(unsigned char id, bool status) {

    char key[RF_MAX_KEY_LENGTH] = {0};
    snprintf_P(key, sizeof(key), PSTR("rfb%s%d"), status ? "ON" : "OFF", id);
    delSetting(key);

    // Websocket update
    #if WEB_SUPPORT
        wsPost([id](JsonObject& root) {
            _rfbWebSocketSendCodeArray(root, id, 1);
        });
    #endif

}

// -----------------------------------------------------------------------------
// SETUP & LOOP
// -----------------------------------------------------------------------------

void rfbSetup() {

    #if MQTT_SUPPORT
        mqttRegister(_rfbMqttCallback);
    #endif

    #if API_SUPPORT
        _rfbAPISetup();
    #endif

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_rfbWebSocketOnVisible)
            .onConnected(_rfbWebSocketOnConnected)
            .onData(_rfbWebSocketOnData)
            .onAction(_rfbWebSocketOnAction)
            .onKeyCheck(_rfbWebSocketOnKeyCheck);
    #endif

    #if TERMINAL_SUPPORT
        _rfbInitCommands();
    #endif

    _rfb_repeat = getSetting("rfbRepeat", RF_SEND_TIMES);

    #if RFB_DIRECT
        const auto rx = getSetting("rfbRX", RFB_RX_PIN);
        const auto tx = getSetting("rfbTX", RFB_TX_PIN);

        _rfb_receive = gpioValid(rx);
        _rfb_transmit = gpioValid(tx);
        if (!_rfb_transmit && !_rfb_receive) {
            DEBUG_MSG_P(PSTR("[RF] Neither RX or TX are set\n"));
            return;
        }

        _rfModem = new RCSwitch();
        if (_rfb_receive) {
            _rfModem->enableReceive(rx);
            DEBUG_MSG_P(PSTR("[RF] RF receiver on GPIO %u\n"), rx);
        }
        if (_rfb_transmit) {
            _rfModem->enableTransmit(tx);
            _rfModem->setRepeatTransmit(_rfb_repeat);
            DEBUG_MSG_P(PSTR("[RF] RF transmitter on GPIO %u\n"), tx);
        }
    #else
        _rfb_receive = true;
        _rfb_transmit = true;
    #endif

    // Register loop only when properly configured
    espurnaRegisterLoop([]() -> void {
        _rfbReceiveImpl();
        _rfbSendQueued();
    });

}

#endif
