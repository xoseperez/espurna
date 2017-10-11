/*

RELAY MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <EEPROM.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>

typedef struct {
    unsigned char pin;
    unsigned char type;
    unsigned char reset_pin;
    unsigned char led;
    unsigned long delay_on;
    unsigned long delay_off;
    unsigned int floodWindowStart;
    unsigned char floodWindowChanges;
    bool scheduled;
    unsigned int scheduledStatusTime;
    bool scheduledStatus;
    bool scheduledReport;
    Ticker pulseTicker;
} relay_t;
std::vector<relay_t> _relays;
bool recursive = false;
Ticker _relaySaveTicker;

#if RELAY_PROVIDER == RELAY_PROVIDER_DUAL
unsigned char _dual_status = 0;
#endif

// -----------------------------------------------------------------------------
// RELAY PROVIDERS
// -----------------------------------------------------------------------------

void relayProviderStatus(unsigned char id, bool status) {

    if (id >= _relays.size()) return;

    #if RELAY_PROVIDER == RELAY_PROVIDER_RFBRIDGE
        rfbStatus(id, status);
    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_DUAL
        _dual_status ^= (1 << id);
        Serial.flush();
        Serial.write(0xA0);
        Serial.write(0x04);
        Serial.write(_dual_status);
        Serial.write(0xA1);
        Serial.flush();
    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_LIGHT
        lightState(status);
        lightUpdate(true, true);
    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_RELAY
        if (_relays[id].type == RELAY_TYPE_NORMAL) {
            digitalWrite(_relays[id].pin, status);
        } else if (_relays[id].type == RELAY_TYPE_INVERSE) {
            digitalWrite(_relays[id].pin, !status);
        } else if (_relays[id].type == RELAY_TYPE_LATCHED) {
            digitalWrite(_relays[id].pin, LOW);
            digitalWrite(_relays[id].reset_pin, LOW);
            if (status) {
                digitalWrite(_relays[id].pin, HIGH);
            } else {
                digitalWrite(_relays[id].reset_pin, HIGH);
            }
            delay(RELAY_LATCHING_PULSE);
            digitalWrite(_relays[id].pin, LOW);
            digitalWrite(_relays[id].reset_pin, LOW);
        }
    #endif

}

bool relayProviderStatus(unsigned char id) {

    if (id >= _relays.size()) return false;

    #if RELAY_PROVIDER == RELAY_PROVIDER_RFBRIDGE
        return _relays[id].scheduledStatus;
    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_DUAL
        return ((_dual_status & (1 << id)) > 0);
    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_LIGHT
        return lightState();
    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_RELAY
        if (_relays[id].type == RELAY_TYPE_NORMAL) {
            return (digitalRead(_relays[id].pin) == HIGH);
        } else if (_relays[id].type == RELAY_TYPE_INVERSE) {
            return (digitalRead(_relays[id].pin) == LOW);
        } else if (_relays[id].type == RELAY_TYPE_LATCHED) {
            return _relays[id].scheduledStatus;
        }
    #endif

}

// -----------------------------------------------------------------------------
// RELAY
// -----------------------------------------------------------------------------

void relayPulse(unsigned char id) {

    byte relayPulseMode = getSetting("relayPulseMode", RELAY_PULSE_MODE).toInt();
    if (relayPulseMode == RELAY_PULSE_NONE) return;
    long relayPulseTime = 1000.0 * getSetting("relayPulseTime", RELAY_PULSE_TIME).toFloat();
    if (relayPulseTime == 0) return;

    bool status = relayStatus(id);
    bool pulseStatus = (relayPulseMode == RELAY_PULSE_ON);
    if (pulseStatus == status) {
        _relays[id].pulseTicker.detach();
        return;
    }

   _relays[id].pulseTicker.once_ms(relayPulseTime, relayToggle, id);

}

unsigned int relayPulseMode() {
    unsigned int value = getSetting("relayPulseMode", RELAY_PULSE_MODE).toInt();
    return value;
}

void relayPulseMode(unsigned int value, bool report) {

    setSetting("relayPulseMode", value);

    /*
    if (report) {
        char topic[strlen(MQTT_TOPIC_RELAY) + 10];
        snprintf_P(topic, sizeof(topic), PSTR("%s/pulse"), MQTT_TOPIC_RELAY);
        char value[2];
        snprintf_P(value, sizeof(value), PSTR("%d"), value);
        mqttSend(topic, value);
    }
    */

    #if WEB_SUPPORT
        char message[20];
        snprintf_P(message, sizeof(message), PSTR("{\"relayPulseMode\": %d}"), value);
        wsSend(message);
    #endif

}

void relayPulseMode(unsigned int value) {
    relayPulseMode(value, true);
}

void relayPulseToggle() {
    unsigned int value = relayPulseMode();
    value = (value == RELAY_PULSE_NONE) ? RELAY_PULSE_OFF : RELAY_PULSE_NONE;
    relayPulseMode(value);
}

bool relayStatus(unsigned char id, bool status, bool report) {

    if (id >= _relays.size()) return false;

    bool changed = false;

    #if TRACK_RELAY_STATUS
    if (relayStatus(id) == status) {
        if (_relays[id].scheduled) {
            DEBUG_MSG_P(PSTR("[RELAY] #%d scheduled change cancelled\n"), id);
            _relays[id].scheduled = false;
            _relays[id].scheduledStatus = status;
            _relays[id].scheduledReport = false;
            changed = true;
        }
    } else {
    #endif

        unsigned int currentTime = millis();
        unsigned int floodWindowEnd = _relays[id].floodWindowStart + 1000 * RELAY_FLOOD_WINDOW;
        unsigned long delay = status ? _relays[id].delay_on : _relays[id].delay_off;

        _relays[id].floodWindowChanges++;
        _relays[id].scheduledStatusTime = currentTime + delay;

        // If currentTime is off-limits the floodWindow...
        if (currentTime < _relays[id].floodWindowStart || floodWindowEnd <= currentTime) {

            // We reset the floodWindow
            _relays[id].floodWindowStart = currentTime;
            _relays[id].floodWindowChanges = 1;

        // If currentTime is in the floodWindow and there have been too many requests...
        } else if (_relays[id].floodWindowChanges >= RELAY_FLOOD_CHANGES) {

            // We schedule the changes to the end of the floodWindow
            // unless it's already delayed beyond that point
            if (floodWindowEnd - delay > currentTime) {
                _relays[id].scheduledStatusTime = floodWindowEnd;
            }

        }

        _relays[id].scheduled = true;
        _relays[id].scheduledStatus = status;
        if (report) _relays[id].scheduledReport = true;

        DEBUG_MSG_P(PSTR("[RELAY] #%d scheduled %s in %u ms\n"),
                id, status ? "ON" : "OFF",
                (_relays[id].scheduledStatusTime - currentTime));

        changed = true;

    #if TRACK_RELAY_STATUS
    }
    #endif

    return changed;
}

bool relayStatus(unsigned char id, bool status) {
    return relayStatus(id, status, true);
}

bool relayStatus(unsigned char id) {
    return relayProviderStatus(id);
}

void relaySync(unsigned char id) {

    if (_relays.size() > 1) {

        recursive = true;

        byte relaySync = getSetting("relaySync", RELAY_SYNC).toInt();
        bool status = relayStatus(id);

        // If RELAY_SYNC_SAME all relays should have the same state
        if (relaySync == RELAY_SYNC_SAME) {
            for (unsigned short i=0; i<_relays.size(); i++) {
                if (i != id) relayStatus(i, status);
            }

        // If NONE_OR_ONE or ONE and setting ON we should set OFF all the others
        } else if (status) {
            if (relaySync != RELAY_SYNC_ANY) {
                for (unsigned short i=0; i<_relays.size(); i++) {
                    if (i != id) relayStatus(i, false);
                }
            }

        // If ONLY_ONE and setting OFF we should set ON the other one
        } else {
            if (relaySync == RELAY_SYNC_ONE) {
                unsigned char i = (id + 1) % _relays.size();
                relayStatus(i, true);
            }
        }

        recursive = false;

    }

}

void relaySave() {
    unsigned char bit = 1;
    unsigned char mask = 0;
    for (unsigned int i=0; i < _relays.size(); i++) {
        if (relayStatus(i)) mask += bit;
        bit += bit;
    }
    EEPROM.write(EEPROM_RELAY_STATUS, mask);
    DEBUG_MSG_P(PSTR("[RELAY] Saving mask: %d\n"), mask);
    EEPROM.commit();
}

void relayRetrieve(bool invert) {
    recursive = true;
    unsigned char bit = 1;
    unsigned char mask = invert ? ~EEPROM.read(EEPROM_RELAY_STATUS) : EEPROM.read(EEPROM_RELAY_STATUS);
    DEBUG_MSG_P(PSTR("[RELAY] Retrieving mask: %d\n"), mask);
    for (unsigned int id=0; id < _relays.size(); id++) {
        _relays[id].scheduled = true;
        _relays[id].scheduledStatus = ((mask & bit) == bit);
        _relays[id].scheduledReport = true;
        bit += bit;
    }
    if (invert) {
        EEPROM.write(EEPROM_RELAY_STATUS, mask);
        EEPROM.commit();
    }
    recursive = false;
}

void relayToggle(unsigned char id) {
    if (id >= _relays.size()) return;
    relayStatus(id, !relayStatus(id));
}

unsigned char relayCount() {
    return _relays.size();
}

unsigned char relayParsePayload(const char * payload) {

    // Payload could be "OFF", "ON", "TOGGLE"
    // or its number equivalents: 0, 1 or 2

    // trim payload
    char * p = ltrim((char *)payload);

    // to lower
    for (unsigned char i=0; i<strlen(p); i++) {
        p[i] = tolower(p[i]);
    }

    unsigned int value;

    if (strcmp(p, "off") == 0) {
        value = 0;
    } else if (strcmp(p, "on") == 0) {
        value = 1;
    } else if (strcmp(p, "toggle") == 0) {
        value = 2;
    } else if (strcmp(p, "query") == 0) {
        value = 3;
    } else {
        value = p[0] - '0';
    }

    if (0 <= value && value <=3) return value;
    return 0xFF;

}
//------------------------------------------------------------------------------
// REST API
//------------------------------------------------------------------------------

#if WEB_SUPPORT

void relaySetupAPI() {

    // API entry points (protected with apikey)
    for (unsigned int relayID=0; relayID<relayCount(); relayID++) {

        char url[15];
        snprintf_P(url, sizeof(url), PSTR("%s/%d"), MQTT_TOPIC_RELAY, relayID);

        char key[10];
        snprintf_P(key, sizeof(key), PSTR("%s%d"), MQTT_TOPIC_RELAY, relayID);

        apiRegister(url, key,
            [relayID](char * buffer, size_t len) {
				snprintf_P(buffer, len, PSTR("%d"), relayStatus(relayID) ? 1 : 0);
            },
            [relayID](const char * payload) {

                unsigned char value = relayParsePayload(payload);

                if (value == 0xFF) {
                    DEBUG_MSG_P(PSTR("[RELAY] Wrong payload (%s)\n"), payload);
                    return;
                }

                if (value == 0) {
                    relayStatus(relayID, false);
                } else if (value == 1) {
                    relayStatus(relayID, true);
                } else if (value == 2) {
                    relayToggle(relayID);
                }

            }
        );

    }

}

#endif // WEB_SUPPORT

//------------------------------------------------------------------------------
// WebSockets
//------------------------------------------------------------------------------

#if WEB_SUPPORT

void relayWS() {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonArray& relay = root.createNestedArray("relayStatus");
    for (unsigned char i=0; i<relayCount(); i++) {
        relay.add(relayStatus(i));
    }
    String output;
    root.printTo(output);
    wsSend(output.c_str());
}

#endif

//------------------------------------------------------------------------------
// MQTT
//------------------------------------------------------------------------------

void relayMQTT(unsigned char id) {
    if (id >= _relays.size()) return;
    mqttSend(MQTT_TOPIC_RELAY, id, relayStatus(id) ? "1" : "0");
}

void relayMQTT() {
    for (unsigned int i=0; i < _relays.size(); i++) {
        relayMQTT(i);
    }
}

void relayMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {

        #if not HEARTBEAT_REPORT_RELAY
            relayMQTT();
        #endif

        char buffer[strlen(MQTT_TOPIC_RELAY) + 3];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s/+"), MQTT_TOPIC_RELAY);
        mqttSubscribe(buffer);

    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = mqttSubtopic((char *) topic);
        if (!t.startsWith(MQTT_TOPIC_RELAY)) return;

        // Get value
        unsigned char value = relayParsePayload(payload);
        if (value == 0xFF) {
            DEBUG_MSG_P(PSTR("[RELAY] Wrong payload (%s)\n"), payload);
            return;
        }

        // Pulse topic
        if (t.endsWith("pulse")) {
            relayPulseMode(value, mqttForward());
            return;
        }

        // Get relay ID
        unsigned int relayID = t.substring(strlen(MQTT_TOPIC_RELAY)+1).toInt();
        if (relayID >= relayCount()) {
            DEBUG_MSG_P(PSTR("[RELAY] Wrong relayID (%d)\n"), relayID);
            return;
        }

        // Action to perform
        if (value == 0) {
            relayStatus(relayID, false, mqttForward());
        } else if (value == 1) {
            relayStatus(relayID, true, mqttForward());
        } else if (value == 2) {
            relayToggle(relayID);
        }

    }

}

void relaySetupMQTT() {
    mqttRegister(relayMQTTCallback);
}

//------------------------------------------------------------------------------
// InfluxDB
//------------------------------------------------------------------------------

#if INFLUXDB_SUPPORT
void relayInfluxDB(unsigned char id) {
    if (id >= _relays.size()) return;
    char buffer[10];
    snprintf_P(buffer, sizeof(buffer), PSTR("%s,id=%d"), MQTT_TOPIC_RELAY, id);
    influxDBSend(buffer, relayStatus(id) ? "1" : "0");
}
#endif

//------------------------------------------------------------------------------
// Setup
//------------------------------------------------------------------------------

void relaySetup() {

    // Dummy relays for AI Light, Magic Home LED Controller, H801,
    // Sonoff Dual and Sonoff RF Bridge
    #ifdef DUMMY_RELAY_COUNT

        for (unsigned char i=0; i < DUMMY_RELAY_COUNT; i++) {
            _relays.push_back((relay_t) {0, RELAY_TYPE_NORMAL});
            _relays[i].scheduled = false;
        }

    #else

        #ifdef RELAY1_PIN
            _relays.push_back((relay_t) { RELAY1_PIN, RELAY1_TYPE, RELAY1_RESET_PIN, RELAY1_LED, RELAY1_DELAY_ON, RELAY1_DELAY_OFF });
        #endif
        #ifdef RELAY2_PIN
            _relays.push_back((relay_t) { RELAY2_PIN, RELAY2_TYPE, RELAY2_RESET_PIN, RELAY2_LED, RELAY2_DELAY_ON, RELAY2_DELAY_OFF });
        #endif
        #ifdef RELAY3_PIN
            _relays.push_back((relay_t) { RELAY3_PIN, RELAY3_TYPE, RELAY3_RESET_PIN, RELAY3_LED, RELAY3_DELAY_ON, RELAY3_DELAY_OFF });
        #endif
        #ifdef RELAY4_PIN
            _relays.push_back((relay_t) { RELAY4_PIN, RELAY4_TYPE, RELAY4_RESET_PIN, RELAY4_LED, RELAY4_DELAY_ON, RELAY4_DELAY_OFF });
        #endif

    #endif

    byte relayMode = getSetting("relayMode", RELAY_MODE).toInt();
    for (unsigned int i=0; i < _relays.size(); i++) {
        pinMode(_relays[i].pin, OUTPUT);
        if (relayMode == RELAY_MODE_OFF) relayStatus(i, false);
        if (relayMode == RELAY_MODE_ON) relayStatus(i, true);
    }
    if (relayMode == RELAY_MODE_SAME) relayRetrieve(false);
    if (relayMode == RELAY_MODE_TOOGLE) relayRetrieve(true);
    relayLoop();

    #if WEB_SUPPORT
        relaySetupAPI();
    #endif
    relaySetupMQTT();

    DEBUG_MSG_P(PSTR("[RELAY] Number of relays: %d\n"), _relays.size());

}

void relayLoop(void) {

    unsigned char id;

    for (id = 0; id < _relays.size(); id++) {

        unsigned int currentTime = millis();
        bool status = _relays[id].scheduledStatus;

        if (_relays[id].scheduled && currentTime >= _relays[id].scheduledStatusTime) {

            DEBUG_MSG_P(PSTR("[RELAY] #%d set to %s\n"), id, status ? "ON" : "OFF");

            // Call the provider to perform the action
            relayProviderStatus(id, status);

            // Change the binded LED if any
            if (_relays[id].led > 0) {
                ledStatus(_relays[id].led - 1, status);
            }

            // Send MQTT report if requested
            if (_relays[id].scheduledReport) {
                relayMQTT(id);
            }

            if (!recursive) {
                relayPulse(id);
                relaySync(id);
                _relaySaveTicker.once_ms(RELAY_SAVE_DELAY, relaySave);
                #if WEB_SUPPORT
                    relayWS();
                #endif
            }

            #if DOMOTICZ_SUPPORT
                domoticzSendRelay(id);
            #endif

            #if INFLUXDB_SUPPORT
                relayInfluxDB(id);
            #endif

            _relays[id].scheduled = false;
            _relays[id].scheduledReport = false;

        }

    }

}
