/*

RF MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if defined(ITEAD_SONOFF_RFBRIDGE) || RF_SUPPORT

#include <queue>
#include <Ticker.h>

#if RFB_DIRECT || RF_SUPPORT
#include <RCSwitch.h>
#endif

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

#if not RF_SUPPORT
typedef struct {
    byte code[RF_MESSAGE_SIZE];
    byte times;
} rfb_message_t;
static std::queue<rfb_message_t> _rfb_message_queue;
Ticker _rfb_ticker;
bool _rfb_ticker_active = false;
#endif

#if RFB_DIRECT || RF_SUPPORT
    RCSwitch * _rfModem;
    bool _learning = false;
#endif

#if WEB_SUPPORT
    Ticker _rfb_sendcodes;
#endif

// -----------------------------------------------------------------------------
// PRIVATES
// -----------------------------------------------------------------------------

/*
 From a byte array to an hexa char array ("A220EE...", double the size)
 */
static bool _rfbToChar(byte * in, char * out, int n = RF_MESSAGE_SIZE) {
    for (unsigned char p = 0; p<n; p++) {
        sprintf_P(&out[p*2], PSTR("%02X"), in[p]);
    }
    return true;
}

#if WEB_SUPPORT

void _rfbWebSocketSendCodeArray(unsigned char start, unsigned char size) {
    
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    JsonObject& rfb = root.createNestedObject("rfb");
    rfb["size"] = size;
    rfb["start"] = start;

    JsonArray& on = rfb.createNestedArray("on");
    JsonArray& off = rfb.createNestedArray("off");

    for (byte id=start; id<start+size; id++) {
        on.add(rfbRetrieve(id, true));
        off.add(rfbRetrieve(id, false));
    }

    wsSend(root);
    
}

void _rfbWebSocketSendCode(unsigned char id) {
    _rfbWebSocketSendCodeArray(id, 1);
}

void _rfbWebSocketSendCodes() {
    _rfbWebSocketSendCodeArray(0, relayCount());
}

void _rfbWebSocketOnSend(JsonObject& root) {
    root["rfbVisible"] = 1;
    root["rfbCount"] = relayCount();
    #if RF_RAW_SUPPORT
        root["rfbrawVisible"] = 1;
    #endif
    _rfb_sendcodes.once_ms(1000, _rfbWebSocketSendCodes);
}

void _rfbWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "rfblearn") == 0) rfbLearn(data["id"], data["status"]);
    if (strcmp(action, "rfbforget") == 0) rfbForget(data["id"], data["status"]);
    if (strcmp(action, "rfbsend") == 0) rfbStore(data["id"], data["status"], data["data"].as<const char*>());
}

#endif // WEB_SUPPORT

void _rfbAck() {
    #if (not RFB_DIRECT) && (not RF_SUPPORT)
        DEBUG_MSG_P(PSTR("[RF] Sending ACK\n"));
        Serial.println();
        Serial.write(RF_CODE_START);
        Serial.write(RF_CODE_ACK);
        Serial.write(RF_CODE_STOP);
        Serial.flush();
        Serial.println();
    #endif
}

void _rfbLearn() {
    #if RFB_DIRECT || RF_SUPPORT
        DEBUG_MSG_P(PSTR("[RF] Entering LEARN mode\n"));
        _learning = true;
    #else
        DEBUG_MSG_P(PSTR("[RF] Sending LEARN\n"));
        Serial.println();
        Serial.write(RF_CODE_START);
        Serial.write(RF_CODE_LEARN);
        Serial.write(RF_CODE_STOP);
        Serial.flush();
        Serial.println();
    #endif

    #if WEB_SUPPORT
        char buffer[100];
        snprintf_P(buffer, sizeof(buffer), PSTR("{\"action\": \"rfbLearn\", \"data\":{\"id\": %d, \"status\": %d}}"), _learnId, _learnStatus ? 1 : 0);
        wsSend(buffer);
    #endif

}

/*
 From an hexa char array ("A220EE...") to a byte array (half the size)
 */
static int _rfbToArray(const char * in, byte * out, int length = RF_MESSAGE_SIZE * 2) {
    int n = strlen(in);
    if (n > RF_MAX_MESSAGE_SIZE*2 || (length > 0 && n != length)) return 0;
    char tmp[3] = {0,0,0};
    n /= 2;
    for (unsigned char p = 0; p<n; p++) {
        memcpy(tmp, &in[p*2], 2);
        out[p] = strtol(tmp, NULL, 16);
    }
    return n;
}

#if not RF_SUPPORT

void _rfbSendRaw(const byte *message, const unsigned char n = RF_MESSAGE_SIZE) {
    for (unsigned char j=0; j<n; j++) {
        Serial.write(message[j]);
    }
}

void _rfbSend(byte * message) {
    #if RFB_DIRECT
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
    #else
        Serial.println();
        Serial.write(RF_CODE_START);
        Serial.write(RF_CODE_RFOUT);
        _rfbSendRaw(message);
        Serial.write(RF_CODE_STOP);
        Serial.flush();
        Serial.println();
    #endif
}

void _rfbSend() {

    // Check if there is something in the queue
    if (_rfb_message_queue.empty()) return;

    // Pop the first element
    rfb_message_t message = _rfb_message_queue.front();
    _rfb_message_queue.pop();

    // Send the message
    _rfbSend(message.code);

    // If it should be further sent, push it to the stack again
    if (message.times > 1) {
        message.times = message.times - 1;
        _rfb_message_queue.push(message);
    }

    // if there are still messages in the queue...
    if (_rfb_message_queue.empty()) {
        _rfb_ticker.detach();
        _rfb_ticker_active = false;
    }

}

void _rfbSend(byte * code, unsigned char times) {
    #if RFB_DIRECT
        times = 1;
    #endif

    char buffer[RF_MESSAGE_SIZE];
    _rfbToChar(code, buffer);
    DEBUG_MSG_P(PSTR("[RF] Enqueuing MESSAGE '%s' %d time(s)\n"), buffer, times);

    rfb_message_t message;
    memcpy(message.code, code, RF_MESSAGE_SIZE);
    message.times = times;
    _rfb_message_queue.push(message);

    // Enable the ticker if not running
    if (!_rfb_ticker_active) {
        _rfb_ticker_active = true;
        _rfb_ticker.attach_ms(RF_SEND_DELAY, _rfbSend);
    }

}

#endif // not RF_SUPPORT

#if RF_RAW_SUPPORT

void _rfbSendRawOnce(byte *code, unsigned char length) {
    char buffer[length*2];
    _rfbToChar(code, buffer, length);
    DEBUG_MSG_P(PSTR("[RF] Sending RAW MESSAGE '%s'\n"), buffer);
    _rfbSendRaw(code, length);
}

#endif // RF_RAW_SUPPORT

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

    byte action = _uartbuf[0];
    char buffer[RF_MESSAGE_SIZE * 2 + 1] = {0};
    DEBUG_MSG_P(PSTR("[RF] Action 0x%02X\n"), action);

    if (action == RF_CODE_LEARN_KO) {
        _rfbAck();
        DEBUG_MSG_P(PSTR("[RF] Learn timeout\n"));
        #if WEB_SUPPORT
            wsSend_P(PSTR("{\"action\": \"rfbTimeout\"}"));
        #endif
    }

    if (action == RF_CODE_LEARN_OK || action == RF_CODE_RFIN) {

        _rfbAck();
        _rfbToChar(&_uartbuf[1], buffer);

        DEBUG_MSG_P(PSTR("[RF] Received message '%s'\n"), buffer);

    }

    if (action == RF_CODE_LEARN_OK) {

        DEBUG_MSG_P(PSTR("[RF] Learn success\n"));
        rfbStore(_learnId, _learnStatus, buffer);

        // Websocket update
        #if WEB_SUPPORT
            _rfbWebSocketSendCode(_learnId);
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

void _rfbReceive() {
    #if RFB_DIRECT || RF_SUPPORT
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
    #else
        static bool receiving = false;

        while (Serial.available()) {

            yield();
            byte c = Serial.read();
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
    #endif
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

    #if RF_RAW_SUPPORT

        byte message[RF_MAX_MESSAGE_SIZE];
        int len = _rfbToArray(tok, message, 0);
        if ((len > 0) && (isRFRaw || len != RF_MESSAGE_SIZE)) {
            _rfbSendRawOnce(message, len);
        } else {
            tok = strtok(NULL, ",");
            byte times = (tok != NULL) ? atoi(tok) : 1;
            _rfbSend(message, times);
        }

    #else // RF_RAW_SUPPORT

        byte message[RF_MESSAGE_SIZE];
        if (_rfbToArray(tok, message)) {
            tok = strtok(NULL, ",");
            byte times = (tok != NULL) ? atoi(tok) : 1;
            _rfbSend(message, times);
        }

    #endif // RF_RAW_SUPPORT    
}

#if MQTT_SUPPORT

void _rfbMqttCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        char buffer[strlen(MQTT_TOPIC_RFLEARN) + 3];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s/+"), MQTT_TOPIC_RFLEARN);
        mqttSubscribe(buffer);
        #if not RF_SUPPORT
        mqttSubscribe(MQTT_TOPIC_RFOUT);
        #endif
        #if RF_RAW_SUPPORT
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
            _rfbLearn();
            return;

        }

        #if not RF_SUPPORT
            bool isRFOut = t.equals(MQTT_TOPIC_RFOUT);
        #endif
        
        #if RF_RAW_SUPPORT
            bool isRFRaw = !isRFOut && t.equals(MQTT_TOPIC_RFRAW);
        #elif not RF_SUPPORT
            bool isRFRaw = false;
        #endif

        #if not RF_SUPPORT
            if (isRFOut || isRFRaw) {
                _rfbParseCode((char *) payload);
            }
        #endif // not RF_SUPPORT

    }

}

#endif // MQTT_SUPPORT

#if API_SUPPORT

void _rfbAPISetup() {

    #if not RF_SUPPORT
    apiRegister(MQTT_TOPIC_RFOUT,
        [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("OK"));
        },
        [](const char * payload) {
            _rfbParseCode((char *) payload);
        }
    );
    #endif // RF_SUPPORT

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
            _rfbLearn();
        }
    );

    #if RF_RAW_SUPPORT
        apiRegister(MQTT_TOPIC_RFRAW,
            [](char * buffer, size_t len) {
                snprintf_P(buffer, len, PSTR("OK"));
            },
            [](const char * payload) {
                _rfbParseCode(payload);
            }
        );
    #endif // RF_RAW_SUPPORT

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

}

#endif // TERMINAL_SUPPORT

// -----------------------------------------------------------------------------
// PUBLIC
// -----------------------------------------------------------------------------

void rfbStore(unsigned char id, bool status, const char * code) {
    DEBUG_MSG_P(PSTR("[RF] Storing %d-%s => '%s'\n"), id, status ? "ON" : "OFF", code);
    char key[RF_MAX_KEY_LENGTH] = {0};
    snprintf_P(key, sizeof(key), PSTR("rfb%s%d"), status ? "ON" : "OFF", id);
    setSetting(key, code);
}

String rfbRetrieve(unsigned char id, bool status) {
    char key[RF_MAX_KEY_LENGTH] = {0};
    snprintf_P(key, sizeof(key), PSTR("rfb%s%d"), status ? "ON" : "OFF", id);
    return getSetting(key);
}

#if not RF_SUPPORT

void rfbStatus(unsigned char id, bool status) {

    String value = rfbRetrieve(id, status);
    if (value.length() > 0) {

        bool same = _rfbSameOnOff(id);

        #if RF_RAW_SUPPORT

            byte message[RF_MAX_MESSAGE_SIZE];
            int len = _rfbToArray(value.c_str(), message, 0);

            if (len == RF_MESSAGE_SIZE &&               // probably a standard msg
                (message[0] != RF_CODE_START        ||  // raw would start with 0xAA
                 message[1] != RF_CODE_RFOUT_BUCKET ||  // followed by 0xB0,
                 message[2] + 4 != len              ||  // needs a valid length,
                 message[len-1] != RF_CODE_STOP)) {     // and finish with 0x55

                 if (!_rfbin) {
                     unsigned char times = same ? 1 : RF_SEND_TIMES;
                     _rfbSend(message, times);
                 }

             } else {
                 _rfbSendRawOnce(message, len);          // send a raw message
             }

        #else // RF_RAW_SUPPORT

            if (!_rfbin) {
                byte message[RF_MESSAGE_SIZE];
                _rfbToArray(value.c_str(), message);
                unsigned char times = same ? 1 : RF_SEND_TIMES;
                _rfbSend(message, times);
            }

        #endif // RF_RAW_SUPPORT

    }

    _rfbin = false;

}
#endif // not RF_SUPPORT

void rfbLearn(unsigned char id, bool status) {
    _learnId = id;
    _learnStatus = status;
    _rfbLearn();
}

void rfbForget(unsigned char id, bool status) {

    char key[RF_MAX_KEY_LENGTH] = {0};
    snprintf_P(key, sizeof(key), PSTR("rfb%s%d"), status ? "ON" : "OFF", id);
    delSetting(key);

    // Websocket update
    #if WEB_SUPPORT
        char wsb[100];
        snprintf_P(wsb, sizeof(wsb), PSTR("{\"rfb\":[{\"id\": %d, \"status\": %d, \"data\": \"\"}]}"), id, status ? 1 : 0);
        wsSend(wsb);
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
        wsOnSendRegister(_rfbWebSocketOnSend);
        wsOnActionRegister(_rfbWebSocketOnAction);
    #endif

    #if TERMINAL_SUPPORT
        _rfbInitCommands();
    #endif

    #if RFB_DIRECT || RF_SUPPORT
        _rfModem = new RCSwitch();
        #if RF_SUPPORT
        _rfModem->enableReceive(RF_PIN);
        DEBUG_MSG_P(PSTR("[RF] RF receiver on GPIO %u\n"), RF_PIN);
        #else
        _rfModem->enableReceive(RFB_RX_PIN);
        _rfModem->enableTransmit(RFB_TX_PIN);
        _rfModem->setRepeatTransmit(6);
        DEBUG_MSG_P(PSTR("[RF] RF receiver on GPIO %u\n"), RFB_RX_PIN);
        DEBUG_MSG_P(PSTR("[RF] RF transmitter on GPIO %u\n"), RFB_TX_PIN);
        #endif
    #endif

    // Register loop
    espurnaRegisterLoop(rfbLoop);

}

void rfbLoop() {
    _rfbReceive();
}

#endif
