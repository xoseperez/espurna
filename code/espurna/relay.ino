/*

RELAY MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <EEPROM_Rotate.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>

typedef struct {

    // Configuration variables

    unsigned char pin;          // GPIO pin for the relay
    unsigned char type;         // RELAY_TYPE_NORMAL, RELAY_TYPE_INVERSE, RELAY_TYPE_LATCHED or RELAY_TYPE_LATCHED_INVERSE
    unsigned char reset_pin;    // GPIO to reset the relay if RELAY_TYPE_LATCHED
    unsigned long delay_on;     // Delay to turn relay ON
    unsigned long delay_off;    // Delay to turn relay OFF
    unsigned char pulse;        // RELAY_PULSE_NONE, RELAY_PULSE_OFF or RELAY_PULSE_ON
    unsigned long pulse_ms;     // Pulse length in millis

    // Status variables

    bool current_status;        // Holds the current (physical) status of the relay
    bool target_status;         // Holds the target status
    unsigned char lock;         // Holds the value of target status, that cannot be changed afterwards. (0 for false, 1 for true, 2 to disable)
    unsigned long fw_start;     // Flood window start time
    unsigned char fw_count;     // Number of changes within the current flood window
    unsigned long change_start;      // Time when relay was scheduled to change
    unsigned long change_delay;      // Delay until the next change
    bool report;                // Whether to report to own topic
    bool group_report;          // Whether to report to group topic

    // Helping objects

    Ticker pulseTicker;         // Holds the pulse back timer

} relay_t;
std::vector<relay_t> _relays;
bool _relayRecursive = false;

unsigned long _relay_flood_window = (1000 * RELAY_FLOOD_WINDOW);
unsigned long _relay_flood_changes = RELAY_FLOOD_CHANGES;

unsigned long _relay_delay_interlock;
unsigned char _relay_sync_mode = RELAY_SYNC_ANY;
bool _relay_sync_locked = false;

Ticker _relay_save_timer;
Ticker _relay_sync_timer;

#if WEB_SUPPORT

bool _relay_report_ws = false;

#endif // WEB_SUPPORT

#if MQTT_SUPPORT

String _relay_mqtt_payload_on;
String _relay_mqtt_payload_off;
String _relay_mqtt_payload_toggle;

#endif // MQTT_SUPPORT

// -----------------------------------------------------------------------------
// UTILITY
// -----------------------------------------------------------------------------

bool _relayHandlePayload(unsigned char relayID, const char* payload) {
    auto value = relayParsePayload(payload);
    if (value == RelayStatus::UNKNOWN) return false;

    if (value == RelayStatus::OFF) {
        relayStatus(relayID, false);
    } else if (value == RelayStatus::ON) {
        relayStatus(relayID, true);
    } else if (value == RelayStatus::TOGGLE) {
        relayToggle(relayID);
    }

    return true;
}

RelayStatus _relayStatusInvert(RelayStatus status) {
    return (status == RelayStatus::ON) ? RelayStatus::OFF : status;
}

RelayStatus _relayStatusTyped(unsigned char id) {
    if (id >= _relays.size()) return RelayStatus::OFF;

    const bool status = _relays[id].current_status;
    return (status) ? RelayStatus::ON : RelayStatus::OFF;
}

void _relayLockAll() {
    for (auto& relay : _relays) {
        relay.lock = relay.target_status;
    }
    _relay_sync_locked = true;
}

void _relayUnlockAll() {
    for (auto& relay : _relays) {
        relay.lock = RELAY_LOCK_DISABLED;
    }
    _relay_sync_locked = false;
}

bool _relayStatusLock(unsigned char id, bool status) {
    if (_relays[id].lock != RELAY_LOCK_DISABLED) {
        bool lock = _relays[id].lock == RELAY_LOCK_ON;
        if ((lock != status) || (lock != _relays[id].target_status)) {
            _relays[id].target_status = lock;
            _relays[id].change_delay = 0;
            return false;
        }
    }

    return true;
}

// https://github.com/xoseperez/espurna/issues/1510#issuecomment-461894516
// completely reset timing on the other relay to sync with this one
// to ensure that they change state sequentially
void _relaySyncRelaysDelay(unsigned char first, unsigned char second) {
    _relays[second].fw_start = _relays[first].change_start;
    _relays[second].fw_count = 1;
    _relays[second].change_delay = std::max({
        _relay_delay_interlock,
        _relays[first].change_delay,
        _relays[second].change_delay
    });
}

void _relaySyncUnlock() {
    bool unlock = true;
    bool all_off = true;
    for (const auto& relay : _relays) {
        unlock = unlock && (relay.current_status == relay.target_status);
        if (!unlock) break;
        all_off = all_off && !relay.current_status;
    }

    if (!unlock) return;

    auto action = []() {
        _relayUnlockAll();
        #if WEB_SUPPORT
            _relay_report_ws = true;
        #endif
    };

    if (all_off) {
        _relay_sync_timer.once_ms(_relay_delay_interlock, action);
    } else {
        action();
    }
}

// -----------------------------------------------------------------------------
// RELAY PROVIDERS
// -----------------------------------------------------------------------------

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

        DEBUG_MSG_P(PSTR("[RELAY] [DUAL] Sending relay mask: %d\n"), mask);

        // Send it to F330
        Serial.flush();
        Serial.write(0xA0);
        Serial.write(0x04);
        Serial.write(mask);
        Serial.write(0xA1);
        Serial.flush();

    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_STM
        Serial.flush();
        Serial.write(0xA0);
        Serial.write(id + 1);
        Serial.write(status);
        Serial.write(0xA1 + status + id);

        // The serial init are not full recognized by relais board.
        // References: https://github.com/xoseperez/espurna/issues/1519 , https://github.com/xoseperez/espurna/issues/1130
        delay(100);

        Serial.flush();
    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_LIGHT

        // Real relays
        uint8_t physical = _relays.size() - DUMMY_RELAY_COUNT;

        // Support for a mixed of dummy and real relays
        // Reference: https://github.com/xoseperez/espurna/issues/1305
        if (id >= physical) {

            // If the number of dummy relays matches the number of light channels
            // assume each relay controls one channel.
            // If the number of dummy relays is the number of channels plus 1
            // assume the first one controls all the channels and
            // the rest one channel each.
            // Otherwise every dummy relay controls all channels.
            if (DUMMY_RELAY_COUNT == lightChannels()) {
                lightState(id-physical, status);
                lightState(true);
            } else if (DUMMY_RELAY_COUNT == (lightChannels() + 1u)) {
                if (id == physical) {
                    lightState(status);
                } else {
                    lightState(id-1-physical, status);
                }
            } else {
                lightState(status);
            }

            lightUpdate(true, true);
            return;
        
        }

    #endif

    #if (RELAY_PROVIDER == RELAY_PROVIDER_RELAY) || (RELAY_PROVIDER == RELAY_PROVIDER_LIGHT)

        // If this is a light, all dummy relays have already been processed above
        // we reach here if the user has toggled a physical relay

        if (_relays[id].type == RELAY_TYPE_NORMAL) {
            digitalWrite(_relays[id].pin, status);
        } else if (_relays[id].type == RELAY_TYPE_INVERSE) {
            digitalWrite(_relays[id].pin, !status);
        } else if (_relays[id].type == RELAY_TYPE_LATCHED || _relays[id].type == RELAY_TYPE_LATCHED_INVERSE) {
            bool pulse = RELAY_TYPE_LATCHED ? HIGH : LOW;
            digitalWrite(_relays[id].pin, !pulse);
            if (GPIO_NONE != _relays[id].reset_pin) digitalWrite(_relays[id].reset_pin, !pulse);
            if (status || (GPIO_NONE == _relays[id].reset_pin)) {
                digitalWrite(_relays[id].pin, pulse);
            } else {
                digitalWrite(_relays[id].reset_pin, pulse);
            }
            nice_delay(RELAY_LATCHING_PULSE);
            digitalWrite(_relays[id].pin, !pulse);
            if (GPIO_NONE != _relays[id].reset_pin) digitalWrite(_relays[id].reset_pin, !pulse);
        }

    #endif

}

/**
 * Walks the relay vector processing only those relays
 * that have to change to the requested mode
 * @bool mode Requested mode
 */
void _relayProcess(bool mode) {

    bool changed = false;

    for (unsigned char id = 0; id < _relays.size(); id++) {

        bool target = _relays[id].target_status;

        // Only process the relays we have to change
        if (target == _relays[id].current_status) continue;

        // Only process the relays we have to change to the requested mode
        if (target != mode) continue;

        // Only process if the change delay has expired
        if (millis() - _relays[id].change_start < _relays[id].change_delay) continue;

        // Purge existing delay in case of cancelation
        _relays[id].change_delay = 0;
        changed = true;

        DEBUG_MSG_P(PSTR("[RELAY] #%d set to %s\n"), id, target ? "ON" : "OFF");

        // Call the provider to perform the action
        _relayProviderStatus(id, target);

        // Send to Broker
        #if BROKER_SUPPORT
            brokerPublish(BROKER_MSG_TYPE_STATUS, MQTT_TOPIC_RELAY, id, target ? "1" : "0");
        #endif

        // Send MQTT
        #if MQTT_SUPPORT
            relayMQTT(id);
        #endif

        #if WEB_SUPPORT
            _relay_report_ws = true;
        #endif

        if (!_relayRecursive) {

            relayPulse(id);

            // We will trigger a eeprom save only if
            // we care about current relay status on boot
            unsigned char boot_mode = getSetting("relayBoot", id, RELAY_BOOT_MODE).toInt();
            bool save_eeprom = ((RELAY_BOOT_SAME == boot_mode) || (RELAY_BOOT_TOGGLE == boot_mode));
            _relay_save_timer.once_ms(RELAY_SAVE_DELAY, relaySave, save_eeprom);

        }

        _relays[id].report = false;
        _relays[id].group_report = false;

    }

    // Whenever we are using sync modes and any relay had changed the state, check if we can unlock
    const bool needs_unlock = ((_relay_sync_mode == RELAY_SYNC_NONE_OR_ONE) || (_relay_sync_mode == RELAY_SYNC_ONE));
    if (_relay_sync_locked && needs_unlock && changed) {
        _relaySyncUnlock();
    }

}

#if defined(ITEAD_SONOFF_IFAN02)

unsigned char _relay_ifan02_speeds[] = {0, 1, 3, 5};

unsigned char getSpeed() {
    unsigned char speed =
        (_relays[1].target_status ? 1 : 0) +
        (_relays[2].target_status ? 2 : 0) +
        (_relays[3].target_status ? 4 : 0);
    for (unsigned char i=0; i<4; i++) {
        if (_relay_ifan02_speeds[i] == speed) return i;
    }
    return 0;
}

void setSpeed(unsigned char speed) {
    if ((0 <= speed) & (speed <= 3)) {
        if (getSpeed() == speed) return;
        unsigned char states = _relay_ifan02_speeds[speed];
        for (unsigned char i=0; i<3; i++) {
            relayStatus(i+1, states & 1 == 1);
            states >>= 1;
        }
    }
}

#endif

// -----------------------------------------------------------------------------
// RELAY
// -----------------------------------------------------------------------------

void _relayMaskRtcmem(uint32_t mask) {
    Rtcmem->relay = mask;
}

uint32_t _relayMaskRtcmem() {
    return Rtcmem->relay;
}

void relayPulse(unsigned char id) {

    _relays[id].pulseTicker.detach();

    byte mode = _relays[id].pulse;
    if (mode == RELAY_PULSE_NONE) return;
    unsigned long ms = _relays[id].pulse_ms;
    if (ms == 0) return;

    bool status = relayStatus(id);
    bool pulseStatus = (mode == RELAY_PULSE_ON);

    if (pulseStatus != status) {
        DEBUG_MSG_P(PSTR("[RELAY] Scheduling relay #%d back in %lums (pulse)\n"), id, ms);
        _relays[id].pulseTicker.once_ms(ms, relayToggle, id);
        // Reconfigure after dynamic pulse
        _relays[id].pulse = getSetting("relayPulse", id, RELAY_PULSE_MODE).toInt();
        _relays[id].pulse_ms = 1000 * getSetting("relayTime", id, RELAY_PULSE_MODE).toFloat();
    }

}

bool relayStatus(unsigned char id, bool status, bool report, bool group_report) {

    if (id >= _relays.size()) return false;

    if (!_relayStatusLock(id, status)) {
        DEBUG_MSG_P(PSTR("[RELAY] #%d is locked to %s\n"), id, _relays[id].current_status ? "ON" : "OFF");
        _relays[id].report = true;
        _relays[id].group_report = true;
        return false;
    }

    bool changed = false;

    if (_relays[id].current_status == status) {

        if (_relays[id].target_status != status) {
            DEBUG_MSG_P(PSTR("[RELAY] #%d scheduled change cancelled\n"), id);
            _relays[id].target_status = status;
            _relays[id].report = false;
            _relays[id].group_report = false;
            _relays[id].change_delay = 0;
            changed = true;
        }

        // For RFBridge, keep sending the message even if the status is already the required
        #if RELAY_PROVIDER == RELAY_PROVIDER_RFBRIDGE
            rfbStatus(id, status);
        #endif

        // Update the pulse counter if the relay is already in the non-normal state (#454)
        relayPulse(id);

    } else {

        unsigned long current_time = millis();
        unsigned long change_delay = status ? _relays[id].delay_on : _relays[id].delay_off;

        _relays[id].fw_count++;
        _relays[id].change_start = current_time;
        _relays[id].change_delay = std::max(_relays[id].change_delay, change_delay);

        // If current_time is off-limits the floodWindow...
        const auto fw_diff = current_time - _relays[id].fw_start;
        if (fw_diff > _relay_flood_window) {

            // We reset the floodWindow
            _relays[id].fw_start = current_time;
            _relays[id].fw_count = 1;

        // If current_time is in the floodWindow and there have been too many requests...
        } else if (_relays[id].fw_count >= _relay_flood_changes) {

            // We schedule the changes to the end of the floodWindow
            // unless it's already delayed beyond that point
            _relays[id].change_delay = std::max(change_delay, _relay_flood_window - fw_diff);

            // Another option is to always move it forward, starting from current time
            //_relays[id].fw_start = current_time;

        }

        _relays[id].target_status = status;
        if (report) _relays[id].report = true;
        if (group_report) _relays[id].group_report = true;

        relaySync(id);

        DEBUG_MSG_P(PSTR("[RELAY] #%d scheduled %s in %u ms\n"),
            id, status ? "ON" : "OFF", _relays[id].change_delay
        );

        changed = true;

    }

    return changed;

}

bool relayStatus(unsigned char id, bool status) {
    return relayStatus(id, status, mqttForward(), true);
}

bool relayStatus(unsigned char id) {

    // Check that relay ID is valid
    if (id >= _relays.size()) return false;

    // Get status directly from storage
    return _relays[id].current_status;

}

void relaySync(unsigned char id) {

    // No sync if none or only one relay
    if (_relays.size() < 2) return;

    // Do not go on if we are comming from a previous sync
    if (_relayRecursive) return;

    // Flag sync mode
    _relayRecursive = true;

    bool status = _relays[id].target_status;

    // If RELAY_SYNC_SAME all relays should have the same state
    if (_relay_sync_mode == RELAY_SYNC_SAME) {
        for (unsigned short i=0; i<_relays.size(); i++) {
            if (i != id) relayStatus(i, status);
        }

    // If RELAY_SYNC_FIRST all relays should have the same state as first if first changes
    } else if (_relay_sync_mode == RELAY_SYNC_FIRST) {
        if (id == 0) {
            for (unsigned short i=1; i<_relays.size(); i++) {
                relayStatus(i, status);
            }
        }

    } else if ((_relay_sync_mode == RELAY_SYNC_NONE_OR_ONE) || (_relay_sync_mode == RELAY_SYNC_ONE)) {
        // If NONE_OR_ONE or ONE and setting ON we should set OFF all the others
        if (status) {
            if (_relay_sync_mode != RELAY_SYNC_ANY) {
                for (unsigned short other_id=0; other_id<_relays.size(); other_id++) {
                    if (other_id != id) {
                        relayStatus(other_id, false);
                        if (relayStatus(other_id)) {
                            _relaySyncRelaysDelay(other_id, id);
                        }
                    }
                }
            }
        // If ONLY_ONE and setting OFF we should set ON the other one
        } else {
            if (_relay_sync_mode == RELAY_SYNC_ONE) {
                unsigned char other_id = (id + 1) % _relays.size();
                _relaySyncRelaysDelay(id, other_id);
                relayStatus(other_id, true);
            }
        }
        _relayLockAll();
    }

    // Unflag sync mode
    _relayRecursive = false;

}

void relaySave(bool eeprom) {

    auto mask = std::bitset<RELAY_SAVE_MASK_MAX>(0);

    unsigned char count = relayCount();
    if (count > RELAY_SAVE_MASK_MAX) count = RELAY_SAVE_MASK_MAX;

    for (unsigned int i=0; i < count; ++i) {
        mask.set(i, relayStatus(i));
    }

    const uint32_t mask_value = mask.to_ulong();

    DEBUG_MSG_P(PSTR("[RELAY] Setting relay mask: %u\n"), mask_value);

    // Persist only to rtcmem, unless requested to save to the eeprom
    _relayMaskRtcmem(mask_value);

    // The 'eeprom' flag controls wether we are commiting this change or not.
    // It is useful to set it to 'false' if the relay change triggering the
    // save involves a relay whose boot mode is independent from current mode,
    // thus storing the last relay value is not absolutely necessary.
    // Nevertheless, we store the value in the EEPROM buffer so it will be written
    // on the next commit.
    if (eeprom) {
        EEPROMr.write(EEPROM_RELAY_STATUS, mask_value);
        // We are actually enqueuing the commit so it will be
        // executed on the main loop, in case this is called from a system context callback
        eepromCommit();
    }

}

void relaySave() {
    relaySave(false);
}

void relayToggle(unsigned char id, bool report, bool group_report) {
    if (id >= _relays.size()) return;
    relayStatus(id, !relayStatus(id), report, group_report);
}

void relayToggle(unsigned char id) {
    relayToggle(id, mqttForward(), true);
}

unsigned char relayCount() {
    return _relays.size();
}

RelayStatus relayParsePayload(const char * payload) {

    // Don't parse empty strings
    const auto len = strlen(payload);
    if (!len) return RelayStatus::UNKNOWN;

    // Check most commonly used payloads
    if (len == 1) {
        if (payload[0] == '0') return RelayStatus::OFF;
        if (payload[0] == '1') return RelayStatus::ON;
        if (payload[0] == '2') return RelayStatus::TOGGLE;
        return RelayStatus::UNKNOWN;
    }

    // If possible, compare to locally configured payload strings
    #if MQTT_SUPPORT
        if (_relay_mqtt_payload_off.equals(payload)) return RelayStatus::OFF;
        if (_relay_mqtt_payload_on.equals(payload)) return RelayStatus::ON;
        if (_relay_mqtt_payload_toggle.equals(payload)) return RelayStatus::TOGGLE;
    #endif // MQTT_SUPPORT

    // Finally, check for "OFF", "ON", "TOGGLE" (both lower and upper cases)
    String temp(payload);
    temp.trim();

    if (temp.equalsIgnoreCase("off")) {
        return RelayStatus::OFF;
    } else if (temp.equalsIgnoreCase("on")) {
        return RelayStatus::ON;
    } else if (temp.equalsIgnoreCase("toggle")) {
        return RelayStatus::TOGGLE;
    }

    return RelayStatus::UNKNOWN;

}

// BACKWARDS COMPATIBILITY
void _relayBackwards() {

    for (unsigned int i=0; i<_relays.size(); i++) {
        if (!hasSetting("mqttGroupInv", i)) continue;
        setSetting("mqttGroupSync", i, getSetting("mqttGroupInv", i));
        delSetting("mqttGroupInv", i);
    }

}

void _relayBoot() {

    _relayRecursive = true;
    bool trigger_save = false;
    uint32_t stored_mask = 0;

    if (rtcmemStatus()) {
        stored_mask = _relayMaskRtcmem();
    } else {
        stored_mask = EEPROMr.read(EEPROM_RELAY_STATUS);
    }

    DEBUG_MSG_P(PSTR("[RELAY] Retrieving mask: %u\n"), stored_mask);

    auto mask = std::bitset<RELAY_SAVE_MASK_MAX>(stored_mask);

    // Walk the relays
    unsigned char lock;
    bool status;
    for (unsigned char i=0; i<relayCount(); ++i) {

        unsigned char boot_mode = getSetting("relayBoot", i, RELAY_BOOT_MODE).toInt();
        DEBUG_MSG_P(PSTR("[RELAY] Relay #%u boot mode %u\n"), i, boot_mode);

        status = false;
        lock = RELAY_LOCK_DISABLED;
        switch (boot_mode) {
            case RELAY_BOOT_SAME:
                if (i < 8) {
                    status = mask.test(i);
                }
                break;
            case RELAY_BOOT_TOGGLE:
                if (i < 8) {
                    status = !mask[i];
                    mask.flip(i);
                    trigger_save = true;
                }
                break;
            case RELAY_BOOT_LOCKED_ON:
                status = true;
                lock = RELAY_LOCK_ON;
                break;
            case RELAY_BOOT_LOCKED_OFF:
                lock = RELAY_LOCK_OFF;
                break;
            case RELAY_BOOT_ON:
                status = true;
                break;
            case RELAY_BOOT_OFF:
            default:
                break;
        }

        _relays[i].current_status = !status;
        _relays[i].target_status = status;
        _relays[i].change_start = millis();

        #if RELAY_PROVIDER == RELAY_PROVIDER_STM
            // XXX hack for correctly restoring relay state on boot
            // because of broken stm relay firmware
            _relays[i].change_delay = 3000 + 1000 * i;
        #endif

        _relays[i].lock = lock;

     }

    // Save if there is any relay in the RELAY_BOOT_TOGGLE mode
    if (trigger_save) {
        _relayMaskRtcmem(mask.to_ulong());

        EEPROMr.write(EEPROM_RELAY_STATUS, mask.to_ulong());
        eepromCommit();
    }

    _relayRecursive = false;

}

constexpr const unsigned long _relayDelayOn(unsigned char index) {
    return (
        (index == 0) ? RELAY1_DELAY_ON :
        (index == 1) ? RELAY2_DELAY_ON :
        (index == 2) ? RELAY3_DELAY_ON :
        (index == 3) ? RELAY4_DELAY_ON :
        (index == 4) ? RELAY5_DELAY_ON :
        (index == 5) ? RELAY6_DELAY_ON :
        (index == 6) ? RELAY7_DELAY_ON :
        (index == 7) ? RELAY8_DELAY_ON : 0
    );
}

constexpr const unsigned long _relayDelayOff(unsigned char index) {
    return (
        (index == 0) ? RELAY1_DELAY_OFF :
        (index == 1) ? RELAY2_DELAY_OFF :
        (index == 2) ? RELAY3_DELAY_OFF :
        (index == 3) ? RELAY4_DELAY_OFF :
        (index == 4) ? RELAY5_DELAY_OFF :
        (index == 5) ? RELAY6_DELAY_OFF :
        (index == 6) ? RELAY7_DELAY_OFF :
        (index == 7) ? RELAY8_DELAY_OFF : 0
    );
}

void _relayConfigure() {
    for (unsigned int i=0; i<_relays.size(); i++) {
        _relays[i].pulse = getSetting("relayPulse", i, RELAY_PULSE_MODE).toInt();
        _relays[i].pulse_ms = 1000 * getSetting("relayTime", i, RELAY_PULSE_MODE).toFloat();

        _relays[i].delay_on = getSetting("relayDelayOn", i, _relayDelayOn(i)).toInt();
        _relays[i].delay_off = getSetting("relayDelayOff", i, _relayDelayOff(i)).toInt();

        if (GPIO_NONE == _relays[i].pin) continue;

        pinMode(_relays[i].pin, OUTPUT);
        if (GPIO_NONE != _relays[i].reset_pin) {
            pinMode(_relays[i].reset_pin, OUTPUT);
        }
        if (_relays[i].type == RELAY_TYPE_INVERSE) {
            //set to high to block short opening of relay
            digitalWrite(_relays[i].pin, HIGH);
        }
    }

    _relay_flood_window = (1000 * getSetting("relayFloodTime", RELAY_FLOOD_WINDOW).toInt());
    _relay_flood_changes = getSetting("relayFloodChanges", RELAY_FLOOD_CHANGES).toInt();

    _relay_delay_interlock = getSetting("relayDelayInterlock", RELAY_DELAY_INTERLOCK).toInt();
    _relay_sync_mode = getSetting("relaySync", RELAY_SYNC).toInt();

    #if MQTT_SUPPORT
        settingsProcessConfig({
            {_relay_mqtt_payload_on,     "relayPayloadOn",     RELAY_MQTT_ON},
            {_relay_mqtt_payload_off,    "relayPayloadOff",    RELAY_MQTT_OFF},
            {_relay_mqtt_payload_toggle, "relayPayloadToggle", RELAY_MQTT_TOGGLE},
        });
    #endif // MQTT_SUPPORT
}

//------------------------------------------------------------------------------
// WEBSOCKETS
//------------------------------------------------------------------------------

#if WEB_SUPPORT

bool _relayWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "relay", 5) == 0);
}

void _relayWebSocketUpdate(JsonObject& root) {
    JsonObject& state = root.createNestedObject("relayState");
    state["size"] = relayCount();

    JsonArray& status = state.createNestedArray("status");
    JsonArray& lock = state.createNestedArray("lock");

    for (unsigned char i=0; i<relayCount(); i++) {
        status.add<uint8_t>(_relays[i].target_status);
        lock.add(_relays[i].lock);
    }
}

String _relayFriendlyName(unsigned char i) {
    String res = String("GPIO") + String(_relays[i].pin);

    if (GPIO_NONE == _relays[i].pin) {
        #if (RELAY_PROVIDER == RELAY_PROVIDER_LIGHT)
            uint8_t physical = _relays.size() - DUMMY_RELAY_COUNT;
            if (i >= physical) {
                if (DUMMY_RELAY_COUNT == lightChannels()) {
                    res = String("CH") + String(i-physical);
                } else if (DUMMY_RELAY_COUNT == (lightChannels() + 1u)) {
                    if (physical == i) {
                        res = String("Light");
                    } else {
                        res = String("CH") + String(i-1-physical);
                    }
                } else {
                    res = String("Light");
                }
            } else {
                res = String("?");
            }
        #else
            res = String("SW") + String(i);
        #endif
    }

    return res;
}

void _relayWebSocketSendRelays(JsonObject& root) {
    JsonObject& relays = root.createNestedObject("relayConfig");

    relays["size"] = relayCount();
    relays["start"] = 0;

    JsonArray& gpio = relays.createNestedArray("gpio");
    JsonArray& type = relays.createNestedArray("type");
    JsonArray& reset = relays.createNestedArray("reset");
    JsonArray& boot = relays.createNestedArray("boot");
    JsonArray& pulse = relays.createNestedArray("pulse");
    JsonArray& pulse_time = relays.createNestedArray("pulse_time");

    #if SCHEDULER_SUPPORT
        JsonArray& sch_last = relays.createNestedArray("sch_last");
    #endif

    #if MQTT_SUPPORT
        JsonArray& group = relays.createNestedArray("group");
        JsonArray& group_sync = relays.createNestedArray("group_sync");
        JsonArray& on_disconnect = relays.createNestedArray("on_disc");
    #endif

    for (unsigned char i=0; i<relayCount(); i++) {
        gpio.add(_relayFriendlyName(i));

        type.add(_relays[i].type);
        reset.add(_relays[i].reset_pin);
        boot.add(getSetting("relayBoot", i, RELAY_BOOT_MODE).toInt());

        pulse.add(_relays[i].pulse);
        pulse_time.add(_relays[i].pulse_ms / 1000.0);

        #if SCHEDULER_SUPPORT
            sch_last.add(getSetting("relayLastSch", i, SCHEDULER_RESTORE_LAST_SCHEDULE).toInt());
        #endif

        #if MQTT_SUPPORT
            group.add(getSetting("mqttGroup", i, ""));
            group_sync.add(getSetting("mqttGroupSync", i, 0).toInt());
            on_disconnect.add(getSetting("relayOnDisc", i, 0).toInt());
        #endif
    }
}

void _relayWebSocketOnVisible(JsonObject& root) {
    if (relayCount() == 0) return;

    if (relayCount() > 1) {
        root["multirelayVisible"] = 1;
        root["relaySync"] = getSetting("relaySync", RELAY_SYNC);
    }

    root["relayVisible"] = 1;
}

void _relayWebSocketOnConnected(JsonObject& root) {

    if (relayCount() == 0) return;

    // Per-relay configuration
    _relayWebSocketSendRelays(root);

}

void _relayWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {

    if (strcmp(action, "relay") != 0) return;

    if (data.containsKey("status")) {

        unsigned int relayID = 0;
        if (data.containsKey("id") && data.is<int>("id")) {
            relayID = data["id"];
        }

        _relayHandlePayload(relayID, data["status"]);

    }

}

void relaySetupWS() {
    wsRegister()
        .onVisible(_relayWebSocketOnVisible)
        .onConnected(_relayWebSocketOnConnected)
        .onData(_relayWebSocketUpdate)
        .onAction(_relayWebSocketOnAction)
        .onKeyCheck(_relayWebSocketOnKeyCheck);
}

#endif // WEB_SUPPORT

//------------------------------------------------------------------------------
// REST API
//------------------------------------------------------------------------------

#if API_SUPPORT

void relaySetupAPI() {

    char key[20];

    // API entry points (protected with apikey)
    for (unsigned int relayID=0; relayID<relayCount(); relayID++) {

        snprintf_P(key, sizeof(key), PSTR("%s/%d"), MQTT_TOPIC_RELAY, relayID);
        apiRegister(key,
            [relayID](char * buffer, size_t len) {
				snprintf_P(buffer, len, PSTR("%d"), _relays[relayID].target_status ? 1 : 0);
            },
            [relayID](const char * payload) {

                if (_relayHandlePayload(relayID, payload)) {
                    DEBUG_MSG_P(PSTR("[RELAY] Wrong payload (%s)\n"), payload);
                    return;
                }

            }
        );

        snprintf_P(key, sizeof(key), PSTR("%s/%d"), MQTT_TOPIC_PULSE, relayID);
        apiRegister(key,
            [relayID](char * buffer, size_t len) {
                dtostrf((double) _relays[relayID].pulse_ms / 1000, 1, 3, buffer);
            },
            [relayID](const char * payload) {

                unsigned long pulse = 1000 * atof(payload);
                if (0 == pulse) return;

                if (RELAY_PULSE_NONE != _relays[relayID].pulse) {
                    DEBUG_MSG_P(PSTR("[RELAY] Overriding relay #%d pulse settings\n"), relayID);
                }

                _relays[relayID].pulse_ms = pulse;
                _relays[relayID].pulse = relayStatus(relayID) ? RELAY_PULSE_ON : RELAY_PULSE_OFF;
                relayToggle(relayID, true, false);

            }
        );

        #if defined(ITEAD_SONOFF_IFAN02)

            apiRegister(MQTT_TOPIC_SPEED,
                [relayID](char * buffer, size_t len) {
                    snprintf(buffer, len, "%u", getSpeed());
                },
                [relayID](const char * payload) {
                    setSpeed(atoi(payload));
                }
            );

        #endif

    }

}

#endif // API_SUPPORT

//------------------------------------------------------------------------------
// MQTT
//------------------------------------------------------------------------------

#if MQTT_SUPPORT

const String& relayPayloadOn() {
    return _relay_mqtt_payload_on;
}

const String& relayPayloadOff() {
    return _relay_mqtt_payload_off;
}

const String& relayPayloadToggle() {
    return _relay_mqtt_payload_toggle;
}

const char* relayPayload(RelayStatus status) {

    if (status == RelayStatus::OFF) {
        return _relay_mqtt_payload_off.c_str();
    } else if (status == RelayStatus::ON) {
        return _relay_mqtt_payload_on.c_str();
    } else if (status == RelayStatus::TOGGLE) {
        return _relay_mqtt_payload_toggle.c_str();
    }

    return "";
}

void _relayMQTTGroup(unsigned char id) {
    String topic = getSetting("mqttGroup", id, "");
    if (!topic.length()) return;

    unsigned char mode = getSetting("mqttGroupSync", id, RELAY_GROUP_SYNC_NORMAL).toInt();
    if (mode == RELAY_GROUP_SYNC_RECEIVEONLY) return;

    auto status = _relayStatusTyped(id);
    if (mode == RELAY_GROUP_SYNC_INVERSE) status = _relayStatusInvert(status);
    mqttSendRaw(topic.c_str(), relayPayload(status));
}

void relayMQTT(unsigned char id) {

    if (id >= _relays.size()) return;

    // Send state topic
    if (_relays[id].report) {
        _relays[id].report = false;
        mqttSend(MQTT_TOPIC_RELAY, id, relayPayload(_relayStatusTyped(id)));
    }

    // Check group topic
    if (_relays[id].group_report) {
        _relays[id].group_report = false;
        _relayMQTTGroup(id);
    }

    // Send speed for IFAN02
    #if defined (ITEAD_SONOFF_IFAN02)
        char buffer[5];
        snprintf(buffer, sizeof(buffer), "%u", getSpeed());
        mqttSend(MQTT_TOPIC_SPEED, buffer);
    #endif

}

void relayMQTT() {
    for (unsigned int id=0; id < _relays.size(); id++) {
        mqttSend(MQTT_TOPIC_RELAY, id, relayPayload(_relayStatusTyped(id)));
    }
}

void relayStatusWrap(unsigned char id, RelayStatus value, bool is_group_topic) {
    switch (value) {
        case RelayStatus::OFF:
            relayStatus(id, false, mqttForward(), !is_group_topic);
            break;
        case RelayStatus::ON:
            relayStatus(id, true, mqttForward(), !is_group_topic);
            break;
        case RelayStatus::TOGGLE:
            relayToggle(id, true, true);
            break;
        default:
            _relays[id].report = true;
            relayMQTT(id);
            break;
    }
}

void relayMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {

        // Send status on connect
        #if (HEARTBEAT_MODE == HEARTBEAT_NONE) or (not HEARTBEAT_REPORT_RELAY)
            relayMQTT();
        #endif

        // Subscribe to own /set topic
        char relay_topic[strlen(MQTT_TOPIC_RELAY) + 3];
        snprintf_P(relay_topic, sizeof(relay_topic), PSTR("%s/+"), MQTT_TOPIC_RELAY);
        mqttSubscribe(relay_topic);

        // Subscribe to pulse topic
        char pulse_topic[strlen(MQTT_TOPIC_PULSE) + 3];
        snprintf_P(pulse_topic, sizeof(pulse_topic), PSTR("%s/+"), MQTT_TOPIC_PULSE);
        mqttSubscribe(pulse_topic);

        #if defined(ITEAD_SONOFF_IFAN02)
            mqttSubscribe(MQTT_TOPIC_SPEED);
        #endif

        // Subscribe to group topics
        for (unsigned int i=0; i < _relays.size(); i++) {
            String t = getSetting("mqttGroup", i, "");
            if (t.length() > 0) mqttSubscribeRaw(t.c_str());
        }

    }

    if (type == MQTT_MESSAGE_EVENT) {

        String t = mqttMagnitude((char *) topic);

        // magnitude is relay/#/pulse
        if (t.startsWith(MQTT_TOPIC_PULSE)) {

            unsigned int id = t.substring(strlen(MQTT_TOPIC_PULSE)+1).toInt();

            if (id >= relayCount()) {
                DEBUG_MSG_P(PSTR("[RELAY] Wrong relayID (%d)\n"), id);
                return;
            }

            unsigned long pulse = 1000 * atof(payload);
            if (0 == pulse) return;

            if (RELAY_PULSE_NONE != _relays[id].pulse) {
                DEBUG_MSG_P(PSTR("[RELAY] Overriding relay #%d pulse settings\n"), id);
            }

            _relays[id].pulse_ms = pulse;
            _relays[id].pulse = relayStatus(id) ? RELAY_PULSE_ON : RELAY_PULSE_OFF;
            relayToggle(id, true, false);

            return;

        }

        // magnitude is relay/#
        if (t.startsWith(MQTT_TOPIC_RELAY)) {

            // Get relay ID
            unsigned int id = t.substring(strlen(MQTT_TOPIC_RELAY)+1).toInt();
            if (id >= relayCount()) {
                DEBUG_MSG_P(PSTR("[RELAY] Wrong relayID (%d)\n"), id);
                return;
            }

            // Get value
            auto value = relayParsePayload(payload);
            if (value == RelayStatus::UNKNOWN) return;

            relayStatusWrap(id, value, false);

            return;
        }


        // Check group topics
        for (unsigned int i=0; i < _relays.size(); i++) {

            String t = getSetting("mqttGroup", i, "");

            if ((t.length() > 0) && t.equals(topic)) {

                auto value = relayParsePayload(payload);
                if (value == RelayStatus::UNKNOWN) return;

                if ((value == RelayStatus::ON) || (value == RelayStatus::OFF)) {
                    if (getSetting("mqttGroupSync", i, RELAY_GROUP_SYNC_NORMAL).toInt() == RELAY_GROUP_SYNC_INVERSE) {
                        value = _relayStatusInvert(value);
                    }
                }

                DEBUG_MSG_P(PSTR("[RELAY] Matched group topic for relayID %d\n"), i);
                relayStatusWrap(i, value, true);

            }
        }

        // Itead Sonoff IFAN02
        #if defined (ITEAD_SONOFF_IFAN02)
            if (t.startsWith(MQTT_TOPIC_SPEED)) {
                setSpeed(atoi(payload));
            }
        #endif

    }

    if (type == MQTT_DISCONNECT_EVENT) {
        for (unsigned int i=0; i < _relays.size(); i++){
            int reaction = getSetting("relayOnDisc", i, 0).toInt();
            if (1 == reaction) {     // switch relay OFF
                DEBUG_MSG_P(PSTR("[RELAY] Reset relay (%d) due to MQTT disconnection\n"), i);
                relayStatusWrap(i, RelayStatus::OFF, false);
            } else if(2 == reaction) { // switch relay ON
                DEBUG_MSG_P(PSTR("[RELAY] Set relay (%d) due to MQTT disconnection\n"), i);
                relayStatusWrap(i, RelayStatus::ON, false);
            }
        }

    }

}

void relaySetupMQTT() {
    mqttRegister(relayMQTTCallback);
}

#endif

//------------------------------------------------------------------------------
// Settings
//------------------------------------------------------------------------------

#if TERMINAL_SUPPORT

void _relayInitCommands() {

    terminalRegisterCommand(F("RELAY"), [](Embedis* e) {
        if (e->argc < 2) {
            terminalError(F("Wrong arguments"));
            return;
        }
        int id = String(e->argv[1]).toInt();
        if (id >= relayCount()) {
            DEBUG_MSG_P(PSTR("-ERROR: Wrong relayID (%d)\n"), id);
            return;
        }

        if (e->argc > 2) {
            int value = String(e->argv[2]).toInt();
            if (value == 2) {
                relayToggle(id);
            } else {
                relayStatus(id, value == 1);
            }
        }
        DEBUG_MSG_P(PSTR("Status: %s\n"), _relays[id].target_status ? "true" : "false");
        if (_relays[id].pulse != RELAY_PULSE_NONE) {
            DEBUG_MSG_P(PSTR("Pulse: %s\n"), (_relays[id].pulse == RELAY_PULSE_ON) ? "ON" : "OFF");
            DEBUG_MSG_P(PSTR("Pulse time: %d\n"), _relays[id].pulse_ms);

        }
        terminalOK();
    });

    #if 0
    terminalRegisterCommand(F("RELAY.INFO"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("    cur tgt pin type reset lock  delay_on   delay_off  pulse  pulse_ms\n"));
        DEBUG_MSG_P(PSTR("    --- --- --- ---- ----- ---- ---------- ----------- ----- ----------\n"));
        for (unsigned char index = 0; index < _relays.size(); ++index) {
            const auto& relay = _relays.at(index);
            DEBUG_MSG_P(PSTR("%3u %3s %3s %3u %4u %5u %4u %10u %11u %5u %10u\n"),
                index,
                relay.current_status ? "ON" : "OFF",
                relay.target_status ? "ON" : "OFF",
                relay.pin, relay.type, relay.reset_pin,
                relay.lock,
                relay.delay_on, relay.delay_off,
                relay.pulse, relay.pulse_ms
            );
        }
    });
    #endif

}

#endif // TERMINAL_SUPPORT

//------------------------------------------------------------------------------
// Setup
//------------------------------------------------------------------------------

void _relayLoop() {
    _relayProcess(false);
    _relayProcess(true);
    #if WEB_SUPPORT
        if (_relay_report_ws) {
            wsPost(_relayWebSocketUpdate);
            _relay_report_ws = false;
        }
    #endif
}

void relaySetup() {

    // Ad-hoc relays
    #if RELAY1_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY1_PIN, RELAY1_TYPE, RELAY1_RESET_PIN });
    #endif
    #if RELAY2_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY2_PIN, RELAY2_TYPE, RELAY2_RESET_PIN });
    #endif
    #if RELAY3_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY3_PIN, RELAY3_TYPE, RELAY3_RESET_PIN });
    #endif
    #if RELAY4_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY4_PIN, RELAY4_TYPE, RELAY4_RESET_PIN });
    #endif
    #if RELAY5_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY5_PIN, RELAY5_TYPE, RELAY5_RESET_PIN });
    #endif
    #if RELAY6_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY6_PIN, RELAY6_TYPE, RELAY6_RESET_PIN });
    #endif
    #if RELAY7_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY7_PIN, RELAY7_TYPE, RELAY7_RESET_PIN });
    #endif
    #if RELAY8_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY8_PIN, RELAY8_TYPE, RELAY8_RESET_PIN });
    #endif

    // Dummy relays for AI Light, Magic Home LED Controller, H801, Sonoff Dual and Sonoff RF Bridge
    // No delay_on or off for these devices to easily allow having more than
    // 8 channels. This behaviour will be recovered with v2.
    for (unsigned char i=0; i < DUMMY_RELAY_COUNT; i++) {
        _relays.push_back((relay_t) { GPIO_NONE, RELAY_TYPE_NORMAL, GPIO_NONE });
    }

    _relayBackwards();
    _relayConfigure();
    _relayBoot();
    _relayLoop();

    #if WEB_SUPPORT
        relaySetupWS();
    #endif
    #if API_SUPPORT
        relaySetupAPI();
    #endif
    #if MQTT_SUPPORT
        relaySetupMQTT();
    #endif
    #if TERMINAL_SUPPORT
        _relayInitCommands();
    #endif

    // Main callbacks
    espurnaRegisterLoop(_relayLoop);
    espurnaRegisterReload(_relayConfigure);

    DEBUG_MSG_P(PSTR("[RELAY] Number of relays: %d\n"), _relays.size());

}
