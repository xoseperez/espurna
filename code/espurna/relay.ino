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

    // Configuration variables

    unsigned char pin;          // GPIO pin for the relay
    unsigned char type;
    unsigned char reset_pin;
    unsigned char led;
    unsigned long delay_on;
    unsigned long delay_off;

    // Status variables

    bool current_status;
    bool target_status;
    unsigned int fw_start;
    unsigned char fw_count;
    unsigned int change_time;
    bool report;
    bool group_report;

    // Helping objects

    Ticker pulseTicker;

} relay_t;
std::vector<relay_t> _relays;
bool _relayRecursive = false;
Ticker _relaySaveTicker;

// -----------------------------------------------------------------------------
// RELAY PROVIDERS
// -----------------------------------------------------------------------------

#if RELAY_PROVIDER == RELAY_PROVIDER_DUAL

#endif

void _relayProviderStatus(unsigned char id, bool status) {

    // Check relay ID
    if (id >= _relays.size()) return;

    // Store new current status
    _relays[id].current_status = status;

    #if RELAY_PROVIDER == RELAY_PROVIDER_RFBRIDGE
        rfbStatus(id, status);
    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_DUAL

        // Calculate mask
        unsigned char mask=0;
        for (unsigned char i=0; i<_relays.size(); i++) {
            if (_relays[i].current_status) mask = mask + (1 << i);
        }

        // Send it to F330
        Serial.flush();
        Serial.write(0xA0);
        Serial.write(0x04);
        Serial.write(mask);
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

void relayPulseMode(unsigned int value) {

    setSetting("relayPulseMode", value);

    #if WEB_SUPPORT
        char message[20];
        snprintf_P(message, sizeof(message), PSTR("{\"relayPulseMode\": %d}"), value);
        wsSend(message);
    #endif

}

void relayPulseToggle() {
    unsigned int value = relayPulseMode();
    value = (value == RELAY_PULSE_NONE) ? RELAY_PULSE_OFF : RELAY_PULSE_NONE;
    relayPulseMode(value);
}

bool relayStatus(unsigned char id, bool status, bool report, bool group_report) {

    if (id >= _relays.size()) return false;

    bool changed = false;

    if (_relays[id].current_status == status) {

        if (_relays[id].target_status != status) {
            DEBUG_MSG_P(PSTR("[RELAY] #%d scheduled change cancelled\n"), id);
            _relays[id].target_status = status;
            _relays[id].report = false;
            _relays[id].group_report = false;
            changed = true;
        }

    } else {

        unsigned int current_time = millis();
        unsigned int fw_end = _relays[id].fw_start + 1000 * RELAY_FLOOD_WINDOW;
        unsigned long delay = status ? _relays[id].delay_on : _relays[id].delay_off;

        _relays[id].fw_count++;
        _relays[id].change_time = current_time + delay;

        // If current_time is off-limits the floodWindow...
        if (current_time < _relays[id].fw_start || fw_end <= current_time) {

            // We reset the floodWindow
            _relays[id].fw_start = current_time;
            _relays[id].fw_count = 1;

        // If current_time is in the floodWindow and there have been too many requests...
        } else if (_relays[id].fw_count >= RELAY_FLOOD_CHANGES) {

            // We schedule the changes to the end of the floodWindow
            // unless it's already delayed beyond that point
            if (fw_end - delay > current_time) {
                _relays[id].change_time = fw_end;
            }

        }

        _relays[id].target_status = status;
        if (report) _relays[id].report = true;
        if (group_report) _relays[id].group_report = true;

        relaySync(id);

        DEBUG_MSG_P(PSTR("[RELAY] #%d scheduled %s in %u ms\n"),
                id, status ? "ON" : "OFF",
                (_relays[id].change_time - current_time));

        changed = true;

    }

    return changed;

}

bool relayStatus(unsigned char id, bool status) {
    return relayStatus(id, status, true, true);
}

bool relayStatus(unsigned char id) {

    // Check relay ID
    if (id >= _relays.size()) return false;

    // GEt status from storage
    return _relays[id].current_status;

}

void relaySync(unsigned char id) {

    // No sync if none or only one relay
    if (_relays.size() < 2) return;

    // Do not go on if we are comming from a previous sync
    if (_relayRecursive) return;

    // Flag sync mode
    _relayRecursive = true;

    byte relaySync = getSetting("relaySync", RELAY_SYNC).toInt();
    bool status = _relays[id].target_status;

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

    // Unflag sync mode
    _relayRecursive = false;

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
    _relayRecursive = true;
    unsigned char bit = 1;
    unsigned char mask = invert ? ~EEPROM.read(EEPROM_RELAY_STATUS) : EEPROM.read(EEPROM_RELAY_STATUS);
    DEBUG_MSG_P(PSTR("[RELAY] Retrieving mask: %d\n"), mask);
    for (unsigned int id=0; id < _relays.size(); id++) {
        _relays[id].target_status = ((mask & bit) == bit);
        _relays[id].report = true;
        _relays[id].group_report = false; // Don't do group report on start
        bit += bit;
    }
    if (invert) {
        EEPROM.write(EEPROM_RELAY_STATUS, mask);
        EEPROM.commit();
    }
    _relayRecursive = false;
}

void relayToggle(unsigned char id, bool report, bool group_report) {
    if (id >= _relays.size()) return;
    relayStatus(id, !relayStatus(id), report, group_report);
}

void relayToggle(unsigned char id) {
    relayToggle(id, true, true);
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
// WEBSOCKETS
//------------------------------------------------------------------------------

#if WEB_SUPPORT

void _relayWebSocketUpdate() {

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    // Statuses
    JsonArray& relay = root.createNestedArray("relayStatus");
    for (unsigned char i=0; i<relayCount(); i++) {
        relay.add(_relays[i].target_status);
    }

    String output;
    root.printTo(output);
    wsSend((char *) output.c_str());

}

void _relayWebSocketOnSend(JsonObject& root) {

    if (relayCount() == 0) return;

    root["relayVisible"] = 1;
    
    // Statuses
    JsonArray& relay = root.createNestedArray("relayStatus");
    for (unsigned char relayID=0; relayID<relayCount(); relayID++) {
        relay.add(relayStatus(relayID));
    }

    // Configuration
    root["relayMode"] = getSetting("relayMode", RELAY_MODE);
    root["relayPulseMode"] = getSetting("relayPulseMode", RELAY_PULSE_MODE);
    root["relayPulseTime"] = getSetting("relayPulseTime", RELAY_PULSE_TIME).toFloat();
    if (relayCount() > 1) {
        root["multirelayVisible"] = 1;
        root["relaySync"] = getSetting("relaySync", RELAY_SYNC);
    }

    // Group topics
    #if MQTT_SUPPORT
        JsonArray& groups = root.createNestedArray("relayGroups");
        for (unsigned char i=0; i<relayCount(); i++) {
            JsonObject& group = groups.createNestedObject();
            group["mqttGroup"] = getSetting("mqttGroup", i, "");
            group["mqttGroupInv"] = getSetting("mqttGroupInv", i, 0).toInt() == 1;
        }
    #endif

}

void _relayWebSocketOnAction(const char * action, JsonObject& data) {

    if (strcmp(action, "relay") != 0) return;

    if (data.containsKey("status")) {

        unsigned char value = relayParsePayload(data["status"]);

        if (value == 3) {

            _relayWebSocketUpdate();

        } else if (value < 3) {

            unsigned int relayID = 0;
            if (data.containsKey("id")) {
                String value = data["id"];
                relayID = value.toInt();
            }

            // Action to perform
            if (value == 0) {
                relayStatus(relayID, false);
            } else if (value == 1) {
                relayStatus(relayID, true);
            } else if (value == 2) {
                relayToggle(relayID);
            }

        }

    }

}

void relaySetupWS() {
    wsOnSendRegister(_relayWebSocketOnSend);
    wsOnActionRegister(_relayWebSocketOnAction);
}

#endif // WEB_SUPPORT

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
// MQTT
//------------------------------------------------------------------------------

#if MQTT_SUPPORT

void relayMQTT(unsigned char id) {

    if (id >= _relays.size()) return;

    // Send state topic
    if (_relays[id].report) {
        _relays[id].report = false;
        mqttSend(MQTT_TOPIC_RELAY, id, _relays[id].current_status ? "1" : "0");
    }

    // Check group topic
    if (_relays[id].group_report) {
        _relays[id].group_report = false;
        String t = getSetting("mqttGroup", id, "");
        if (t.length() > 0) {
            bool status = relayStatus(id);
            if (getSetting("mqttGroupInv", id, 0).toInt() == 1) status = !status;
            mqttSendRaw(t.c_str(), status ? "1" : "0");
        }
    }

}

void relayMQTT() {
    for (unsigned int id=0; id < _relays.size(); id++) {
        mqttSend(MQTT_TOPIC_RELAY, id, _relays[id].current_status ? "1" : "0");
    }
}

void relayMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {

        // Send status on connect
        #if not HEARTBEAT_REPORT_RELAY
            relayMQTT();
        #endif

        // Subscribe to own /set topic
        char buffer[strlen(MQTT_TOPIC_RELAY) + 3];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s/+"), MQTT_TOPIC_RELAY);
        mqttSubscribe(buffer);

        // Subscribe to group topics
        for (unsigned int i=0; i < _relays.size(); i++) {
            String t = getSetting("mqttGroup", i, "");
            if (t.length() > 0) mqttSubscribeRaw(t.c_str());
        }

    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Get relay
        unsigned int relayID;
        bool is_group_topic = false;

        // Get value
        unsigned char value = relayParsePayload(payload);
        if (value == 0xFF) {
            DEBUG_MSG_P(PSTR("[RELAY] Wrong payload (%s)\n"), payload);
            return;
        }

        // Check group topics
        for (unsigned int i=0; i < _relays.size(); i++) {
            String t = getSetting("mqttGroup", i, "");
            if (t.equals(topic)) {
                is_group_topic = true;
                relayID = i;
                if (getSetting("mqttGroupInv", relayID, 0).toInt() == 1) {
                    if (value < 2) value = 1 - value;
                }
                DEBUG_MSG_P(PSTR("[RELAY] Matched group topic for relayID %d\n"), relayID);
                break;
            }
        }

        // Not group topic, look for own topic
        if (!is_group_topic) {

            // Match topic
            String t = mqttSubtopic((char *) topic);
            if (!t.startsWith(MQTT_TOPIC_RELAY)) return;

            // Pulse topic
            if (t.endsWith("pulse")) {
                relayPulseMode(value);
                return;
            }

            // Get relay ID
            relayID = t.substring(strlen(MQTT_TOPIC_RELAY)+1).toInt();
            if (relayID >= relayCount()) {
                DEBUG_MSG_P(PSTR("[RELAY] Wrong relayID (%d)\n"), relayID);
                return;
            }

        }

        // Action to perform
        if (value == 0) {
            relayStatus(relayID, false, mqttForward(), !is_group_topic);
        } else if (value == 1) {
            relayStatus(relayID, true, mqttForward(), !is_group_topic);
        } else if (value == 2) {
            relayToggle(relayID, true, true);
        }

    }

}

void relaySetupMQTT() {
    mqttRegister(relayMQTTCallback);
}

#endif

//------------------------------------------------------------------------------
// InfluxDB
//------------------------------------------------------------------------------

#if INFLUXDB_SUPPORT
void relayInfluxDB(unsigned char id) {
    if (id >= _relays.size()) return;
    char buffer[10];
    snprintf_P(buffer, sizeof(buffer), PSTR("%s,id=%d"), MQTT_TOPIC_RELAY, id);
    idbSend(buffer, relayStatus(id) ? "1" : "0");
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
        relaySetupWS();
    #endif

    #if MQTT_SUPPORT
        relaySetupMQTT();
    #endif

    DEBUG_MSG_P(PSTR("[RELAY] Number of relays: %d\n"), _relays.size());

}

void relayLoop(void) {

    unsigned char id;

    for (id = 0; id < _relays.size(); id++) {

        unsigned int current_time = millis();
        bool status = _relays[id].target_status;

        if ((_relays[id].current_status != status)
            && (current_time >= _relays[id].change_time)) {

            DEBUG_MSG_P(PSTR("[RELAY] #%d set to %s\n"), id, status ? "ON" : "OFF");

            // Call the provider to perform the action
            _relayProviderStatus(id, status);

            // Change the binded LED if any
            if (_relays[id].led > 0) {
                ledStatus(_relays[id].led - 1, status);
            }

            // Send MQTT
            #if MQTT_SUPPORT
                relayMQTT(id);
            #endif

            if (!_relayRecursive) {
                relayPulse(id);
                _relaySaveTicker.once_ms(RELAY_SAVE_DELAY, relaySave);
                #if WEB_SUPPORT
                    _relayWebSocketUpdate();
                #endif
            }

            #if DOMOTICZ_SUPPORT
                domoticzSendRelay(id);
            #endif

            #if INFLUXDB_SUPPORT
                relayInfluxDB(id);
            #endif

            _relays[id].report = false;
            _relays[id].group_report = false;

        }

    }

}
