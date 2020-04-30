/*

RELAY MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "relay.h"

#if RELAY_SUPPORT

#include <Ticker.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>
#include <bitset>

#include "api.h"
#include "broker.h"
#include "light.h"
#include "mqtt.h"
#include "rfbridge.h"
#include "rpc.h"
#include "rtcmem.h"
#include "settings.h"
#include "storage_eeprom.h"
#include "tuya.h"
#include "utils.h"
#include "ws.h"

#include "relay_config.h"

struct relay_t {

    // Default to dummy (virtual) relay configuration

    relay_t(unsigned char pin, unsigned char type, unsigned char reset_pin) :
        pin(pin),
        type(type),
        reset_pin(reset_pin),
        delay_on(0),
        delay_off(0),
        pulse(RELAY_PULSE_NONE),
        pulse_ms(0),
        current_status(false),
        target_status(false),
        lock(RELAY_LOCK_DISABLED),
        fw_start(0),
        fw_count(0),
        change_start(0),
        change_delay(0),
        report(false),
        group_report(false)
    {}

    relay_t() :
        relay_t(GPIO_NONE, RELAY_TYPE_NORMAL, GPIO_NONE)
    {}

    // ... unless there are pre-configured values

    relay_t(unsigned char id) :
        relay_t(_relayPin(id), _relayType(id), _relayResetPin(id))
    {}

    // Configuration variables

    unsigned char pin;           // GPIO pin for the relay
    unsigned char type;          // RELAY_TYPE_NORMAL, RELAY_TYPE_INVERSE, RELAY_TYPE_LATCHED or RELAY_TYPE_LATCHED_INVERSE
    unsigned char reset_pin;     // GPIO to reset the relay if RELAY_TYPE_LATCHED
    unsigned long delay_on;      // Delay to turn relay ON
    unsigned long delay_off;     // Delay to turn relay OFF
    unsigned char pulse;         // RELAY_PULSE_NONE, RELAY_PULSE_OFF or RELAY_PULSE_ON
    unsigned long pulse_ms;      // Pulse length in millis

    // Status variables

    bool current_status;         // Holds the current (physical) status of the relay
    bool target_status;          // Holds the target status
    unsigned char lock;          // Holds the value of target status, that cannot be changed afterwards. (0 for false, 1 for true, 2 to disable)
    unsigned long fw_start;      // Flood window start time
    unsigned char fw_count;      // Number of changes within the current flood window
    unsigned long change_start;  // Time when relay was scheduled to change
    unsigned long change_delay;  // Delay until the next change
    bool report;                 // Whether to report to own topic
    bool group_report;           // Whether to report to group topic

    // Helping objects

    Ticker pulseTicker;          // Holds the pulse back timer

};

std::vector<relay_t> _relays;
bool _relayRecursive = false;
size_t _relayDummy = 0;

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

#if MQTT_SUPPORT || API_SUPPORT

String _relay_rpc_payload_on;
String _relay_rpc_payload_off;
String _relay_rpc_payload_toggle;

#endif // MQTT_SUPPORT || API_SUPPORT

// -----------------------------------------------------------------------------
// UTILITY
// -----------------------------------------------------------------------------

bool _relayHandlePayload(unsigned char relayID, const char* payload) {
    auto value = relayParsePayload(payload);
    if (value == PayloadStatus::Unknown) return false;

    if (value == PayloadStatus::Off) {
        relayStatus(relayID, false);
    } else if (value == PayloadStatus::On) {
        relayStatus(relayID, true);
    } else if (value == PayloadStatus::Toggle) {
        relayToggle(relayID);
    }

    return true;
}

PayloadStatus _relayStatusInvert(PayloadStatus status) {
    return (status == PayloadStatus::On) ? PayloadStatus::Off : status;
}

PayloadStatus _relayStatusTyped(unsigned char id) {
    if (id >= _relays.size()) return PayloadStatus::Off;

    const bool status = _relays[id].current_status;
    return (status) ? PayloadStatus::On : PayloadStatus::Off;
}

void _relayLockAll() {
    for (auto& relay : _relays) {
        relay.lock = relay.target_status ? RELAY_LOCK_ON : RELAY_LOCK_OFF;
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
        size_t physical = _relays.size() - _relayDummy;

        // Support for a mixed of dummy and real relays
        // Reference: https://github.com/xoseperez/espurna/issues/1305
        if (id >= physical) {

            // If the number of dummy relays matches the number of light channels
            // assume each relay controls one channel.
            // If the number of dummy relays is the number of channels plus 1
            // assume the first one controls all the channels and
            // the rest one channel each.
            // Otherwise every dummy relay controls all channels.
            if (_relayDummy == lightChannels()) {
                lightState(id-physical, status);
                lightState(true);
            } else if (_relayDummy == (lightChannels() + 1u)) {
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
            bool pulse = (_relays[id].type == RELAY_TYPE_LATCHED) ? HIGH : LOW;
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
        if (_relays[id].change_delay && (millis() - _relays[id].change_start < _relays[id].change_delay)) continue;

        // Purge existing delay in case of cancelation
        _relays[id].change_delay = 0;
        changed = true;

        DEBUG_MSG_P(PSTR("[RELAY] #%d set to %s\n"), id, target ? "ON" : "OFF");

        // Call the provider to perform the action
        _relayProviderStatus(id, target);

        // Send to Broker
        #if BROKER_SUPPORT
            StatusBroker::Publish(MQTT_TOPIC_RELAY, id, target);
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
            const auto boot_mode = getSetting({"relayBoot", id}, RELAY_BOOT_MODE);
            const bool save_eeprom = ((RELAY_BOOT_SAME == boot_mode) || (RELAY_BOOT_TOGGLE == boot_mode));
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

// State persistance persistance
namespace {

String u32toString(uint32_t value, int base) {
    String result;
    result.reserve(32 + 2);

    if (base == 2) {
        result += "0b";
    } else if (base == 8) {
        result += "0o";
    } else if (base == 16) {
        result += "0x";
    }

    char buffer[33] = {0};
    ultoa(value, buffer, base);
    result += buffer;

    return result;
}

struct RelayMask {
    const String as_string;
    uint32_t as_u32;
};

RelayMask INLINE _relayMask(uint32_t mask) {
    return {std::move(u32toString(mask, 2)), mask};
}

RelayMask INLINE _relayMaskRtcmem() {
    return _relayMask(Rtcmem->relay);
}

void INLINE _relayMaskRtcmem(uint32_t mask) {
    Rtcmem->relay = mask;
}

void INLINE _relayMaskRtcmem(const RelayMask& mask) {
    _relayMaskRtcmem(mask.as_u32);
}

void INLINE _relayMaskRtcmem(const std::bitset<RELAYS_MAX>& bitset) {
    _relayMaskRtcmem(bitset.to_ulong());
}

RelayMask INLINE _relayMaskSettings() {
    constexpr unsigned long defaultMask { 0ul };
    auto value = getSetting("relayBootMask", defaultMask);
    return _relayMask(value);
}

void INLINE _relayMaskSettings(uint32_t mask) {
    setSetting("relayBootMask", u32toString(mask, 2));
}

void INLINE _relayMaskSettings(const RelayMask& mask) {
    setSetting("relayBootMask", mask.as_string);
}

void INLINE _relayMaskSettings(const std::bitset<RELAYS_MAX>& bitset) {
    _relayMaskSettings(bitset.to_ulong());
}

} // ns anonymous

// Pulse timers (timer after ON or OFF event)

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
        _relays[id].pulse = getSetting({"relayPulse", id}, RELAY_PULSE_MODE);
        _relays[id].pulse_ms = 1000 * getSetting({"relayTime", id}, 0.);
    }

}

// General relay status control

bool relayStatus(unsigned char id, bool status, bool report, bool group_report) {

    if (id == RELAY_NONE) return false;
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
    #if MQTT_SUPPORT
        return relayStatus(id, status, mqttForward(), true);
    #else
        return relayStatus(id, status, false, true);
    #endif
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

    const unsigned char count = constrain(relayCount(), 0, RELAYS_MAX);

    auto statuses = std::bitset<RELAYS_MAX>(0);
    for (unsigned int id = 0; id < count; ++id) {
        statuses.set(id, relayStatus(id));
    }

    const auto mask = _relayMask(statuses.to_ulong() & 0xffffffffu);
    DEBUG_MSG_P(PSTR("[RELAY] Setting relay mask: %s\n"), mask.as_string.c_str());

    // Persist only to rtcmem, unless requested to save to the eeprom
    _relayMaskRtcmem(mask);

    // The 'eeprom' flag controls whether we are commiting this change or not.
    // It is useful to set it to 'false' if the relay change triggering the
    // save involves a relay whose boot mode is independent from current mode,
    // thus storing the last relay value is not absolutely necessary.
    // Nevertheless, we store the value in the EEPROM buffer so it will be written
    // on the next commit.
    if (eeprom) {
        _relayMaskSettings(mask);
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
    #if MQTT_SUPPORT
        relayToggle(id, mqttForward(), true);
    #else
        relayToggle(id, false, true);
    #endif
}

unsigned char relayCount() {
    return _relays.size();
}

PayloadStatus relayParsePayload(const char * payload) {
    #if MQTT_SUPPORT || API_SUPPORT
        return rpcParsePayload(payload, [](const char* payload) {
            if (_relay_rpc_payload_off.equals(payload)) return PayloadStatus::Off;
            if (_relay_rpc_payload_on.equals(payload)) return PayloadStatus::On;
            if (_relay_rpc_payload_toggle.equals(payload)) return PayloadStatus::Toggle;
            return PayloadStatus::Unknown;
        });
    #else
        return rpcParsePayload(payload);
    #endif
}

// BACKWARDS COMPATIBILITY
void _relayBackwards() {

    #if defined(EEPROM_RELAY_STATUS)
    {
        uint8_t mask = EEPROMr.read(EEPROM_RELAY_STATUS);
        if (mask != 0xff) {
            _relayMaskSettings(static_cast<uint32_t>(mask));
            EEPROMr.write(EEPROM_RELAY_STATUS, 0xff);
            eepromCommit();
        }
    }
    #endif

    for (unsigned char id = 0; id < _relays.size(); ++id) {
        const settings_key_t key {"mqttGroupInv", id};
        if (!hasSetting(key)) continue;
        setSetting({"mqttGroupSync", id}, getSetting(key));
        delSetting(key);
    }

}

void _relayBoot() {

    _relayRecursive = true;

    const auto stored_mask = rtcmemStatus()
        ? _relayMaskRtcmem()
        : _relayMaskSettings();

    DEBUG_MSG_P(PSTR("[RELAY] Retrieving mask: %s\n"), stored_mask.as_string.c_str());

    auto mask = std::bitset<RELAYS_MAX>(stored_mask.as_u32);

    // Walk the relays
    unsigned char lock;
    bool status;
    for (unsigned char i=0; i<relayCount(); ++i) {

        const auto boot_mode = getSetting({"relayBoot", i}, RELAY_BOOT_MODE);
        DEBUG_MSG_P(PSTR("[RELAY] Relay #%u boot mode %d\n"), i, boot_mode);

        status = false;
        lock = RELAY_LOCK_DISABLED;
        switch (boot_mode) {
            case RELAY_BOOT_SAME:
                status = mask.test(i);
                break;
            case RELAY_BOOT_TOGGLE:
                mask.flip(i);
                status = mask[i];
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
        _relays[i].change_delay = status
            ? _relays[i].delay_on
            : _relays[i].delay_off;

        #if RELAY_PROVIDER == RELAY_PROVIDER_STM
            // XXX hack for correctly restoring relay state on boot
            // because of broken stm relay firmware
            _relays[i].change_delay = 3000 + 1000 * i;
        #endif

        _relays[i].lock = lock;

    }

    _relayRecursive = false;

    #if TUYA_SUPPORT
        Tuya::tuyaSyncSwitchStatus();
    #endif

}

void _relayConfigure() {
    for (unsigned char i = 0, relays = _relays.size() ; (i < relays); ++i) {
        _relays[i].pulse = getSetting({"relayPulse", i}, RELAY_PULSE_MODE);
        _relays[i].pulse_ms = 1000 * getSetting({"relayTime", i}, 0.);

        _relays[i].delay_on = getSetting({"relayDelayOn", i}, _relayDelayOn(i));
        _relays[i].delay_off = getSetting({"relayDelayOff", i}, _relayDelayOff(i));

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

    _relay_flood_window = (1000 * getSetting("relayFloodTime", RELAY_FLOOD_WINDOW));
    _relay_flood_changes = getSetting("relayFloodChanges", RELAY_FLOOD_CHANGES);

    _relay_delay_interlock = getSetting("relayDelayInterlock", RELAY_DELAY_INTERLOCK);
    _relay_sync_mode = getSetting("relaySync", RELAY_SYNC);

    #if MQTT_SUPPORT || API_SUPPORT
        settingsProcessConfig({
            {_relay_rpc_payload_on,     "relayPayloadOn",     RELAY_MQTT_ON},
            {_relay_rpc_payload_off,    "relayPayloadOff",    RELAY_MQTT_OFF},
            {_relay_rpc_payload_toggle, "relayPayloadToggle", RELAY_MQTT_TOGGLE},
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
            uint8_t physical = _relays.size() - _relayDummy;
            if (i >= physical) {
                if (_relayDummy == lightChannels()) {
                    res = String("CH") + String(i-physical);
                } else if (_relayDummy == (lightChannels() + 1u)) {
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
        boot.add(getSetting({"relayBoot", i}, RELAY_BOOT_MODE));

        pulse.add(_relays[i].pulse);
        pulse_time.add(_relays[i].pulse_ms / 1000.0);

        #if SCHEDULER_SUPPORT
            sch_last.add(getSetting({"relayLastSch", i}, SCHEDULER_RESTORE_LAST_SCHEDULE));
        #endif

        #if MQTT_SUPPORT
            group.add(getSetting({"mqttGroup", i}));
            group_sync.add(getSetting({"mqttGroupSync", i}, 0));
            on_disconnect.add(getSetting({"relayOnDisc", i}, 0));
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

                if (!_relayHandlePayload(relayID, payload)) {
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

#if MQTT_SUPPORT || API_SUPPORT

const String& relayPayloadOn() {
    return _relay_rpc_payload_on;
}

const String& relayPayloadOff() {
    return _relay_rpc_payload_off;
}

const String& relayPayloadToggle() {
    return _relay_rpc_payload_toggle;
}

const char* relayPayload(PayloadStatus status) {
    switch (status) {
        case PayloadStatus::Off:
            return _relay_rpc_payload_off.c_str();
        case PayloadStatus::On:
            return _relay_rpc_payload_on.c_str();
        case PayloadStatus::Toggle:
            return _relay_rpc_payload_toggle.c_str();
        case PayloadStatus::Unknown:
        default:
            return "";
    }
}

#endif // MQTT_SUPPORT || API_SUPPORT

#if MQTT_SUPPORT

void _relayMQTTGroup(unsigned char id) {
    const String topic = getSetting({"mqttGroup", id});
    if (!topic.length()) return;

    const auto mode = getSetting({"mqttGroupSync", id}, RELAY_GROUP_SYNC_NORMAL);
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

void relayStatusWrap(unsigned char id, PayloadStatus value, bool is_group_topic) {
    #if MQTT_SUPPORT
        const auto forward = mqttForward();
    #else
        const auto forward = false;
    #endif
    switch (value) {
        case PayloadStatus::Off:
            relayStatus(id, false, forward, !is_group_topic);
            break;
        case PayloadStatus::On:
            relayStatus(id, true, forward, !is_group_topic);
            break;
        case PayloadStatus::Toggle:
            relayToggle(id, true, true);
            break;
        case PayloadStatus::Unknown:
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
        for (unsigned char i=0; i < _relays.size(); i++) {
            const auto t = getSetting({"mqttGroup", i});
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
            if (value == PayloadStatus::Unknown) return;

            relayStatusWrap(id, value, false);

            return;
        }


        // Check group topics
        for (unsigned char i=0; i < _relays.size(); i++) {

            const String t = getSetting({"mqttGroup", i});

            if ((t.length() > 0) && t.equals(topic)) {

                auto value = relayParsePayload(payload);
                if (value == PayloadStatus::Unknown) return;

                if ((value == PayloadStatus::On) || (value == PayloadStatus::Off)) {
                    if (getSetting({"mqttGroupSync", i}, RELAY_GROUP_SYNC_NORMAL) == RELAY_GROUP_SYNC_INVERSE) {
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
        for (unsigned char i=0; i < _relays.size(); i++){
            const auto reaction = getSetting({"relayOnDisc", i}, 0);
            if (1 == reaction) {     // switch relay OFF
                DEBUG_MSG_P(PSTR("[RELAY] Reset relay (%d) due to MQTT disconnection\n"), i);
                relayStatusWrap(i, PayloadStatus::Off, false);
            } else if(2 == reaction) { // switch relay ON
                DEBUG_MSG_P(PSTR("[RELAY] Set relay (%d) due to MQTT disconnection\n"), i);
                relayStatusWrap(i, PayloadStatus::On, false);
            }
        }

    }

}

void relaySetupMQTT() {
    mqttRegister(relayMQTTCallback);
}

#endif

void _relaySetupProvider() {
    // TODO: implement something like `RelayProvider tuya_provider({.setup_cb = ..., .send_cb = ...})`?
    //       note of the function call order! relay code is initialized before tuya's, and the easiest
    //       way to accomplish that is to use ctor as a way to "register" callbacks even before setup() is called
    #if TUYA_SUPPORT
        Tuya::tuyaSetupSwitch();
    #endif
}

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

// Dummy relays for virtual light switches, Sonoff Dual, Sonoff RF Bridge and Tuya
void relaySetupDummy(size_t size, bool reconfigure) {

    if (size == _relayDummy) return;

    const size_t new_size = ((_relays.size() - _relayDummy) + size);
    if (new_size > RELAYS_MAX) return;

    _relayDummy = size;
    _relays.resize(new_size);

    if (reconfigure) {
        _relayConfigure();
    }

    #if BROKER_SUPPORT
        ConfigBroker::Publish("relayDummy", String(int(size)));
    #endif

}

void _relaySetupAdhoc() {

    size_t relays = 0;

    #if RELAY1_PIN != GPIO_NONE
        ++relays;
    #endif
    #if RELAY2_PIN != GPIO_NONE
        ++relays;
    #endif
    #if RELAY3_PIN != GPIO_NONE
        ++relays;
    #endif
    #if RELAY4_PIN != GPIO_NONE
        ++relays;
    #endif
    #if RELAY5_PIN != GPIO_NONE
        ++relays;
    #endif
    #if RELAY6_PIN != GPIO_NONE
        ++relays;
    #endif
    #if RELAY7_PIN != GPIO_NONE
        ++relays;
    #endif
    #if RELAY8_PIN != GPIO_NONE
        ++relays;
    #endif

    _relays.reserve(relays);
    for (unsigned char id = 0; id < relays; ++id) {
        _relays.emplace_back(id);
    }

}

void relaySetup() {

    // Ad-hoc relays
    _relaySetupAdhoc();

    // Dummy (virtual) relays
    relaySetupDummy(getSetting("relayDummy", DUMMY_RELAY_COUNT));

    _relaySetupProvider();
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

#endif // RELAY_SUPPORT == 1
