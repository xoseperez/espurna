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
#include "mqtt.h"
#include "rpc.h"
#include "rtcmem.h"
#include "settings.h"
#include "storage_eeprom.h"
#include "utils.h"
#include "ws.h"

#include "libs/BasePin.h"
#include "relay_config.h"

// Relay statuses are kept in a mutable bitmask struct
// TODO: u32toString should be convert(...) ?

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

using RelayMask = std::bitset<RelaysMax>;

struct RelayMaskHelper {
    RelayMaskHelper() = default;

    explicit RelayMaskHelper(uint32_t mask) :
        _mask(mask)
    {}

    explicit RelayMaskHelper(RelayMask&& mask) :
        _mask(std::move(mask))
    {}

    uint32_t toUnsigned() const {
        return _mask.to_ulong();
    }

    String toString() const {
        return u32toString(toUnsigned(), 2);
    }

    const RelayMask& mask() const {
        return _mask;
    }

    void reset() {
        _mask.reset();
    }

    void set(unsigned char id, bool status) {
        _mask.set(id, status);
    }

    bool operator[](size_t id) const {
        return _mask[id];
    }

private:
    RelayMask _mask { 0ul };
};

} // namespace

template <typename T>
T _relayPayloadToTristate(const char* payload) {
    auto len = strlen(payload);
    if (len == 1) {
        switch (payload[0]) {
        case '0':
            return T::None;
        case '1':
            return T::Off;
        case '2':
            return T::On;
        }
    } else if (len > 1) {
        String cmp(payload);
        if (cmp == "none") {
            return T::None;
        } else if (cmp == "off") {
            return T::Off;
        } else if (cmp == "on") {
            return T::On;
        }
    }

    return T::None;
}

template <typename T>
const char* _relayTristateToPayload(T tristate) {
    static_assert(std::is_enum<T>::value, "");
    switch (tristate) {
    case T::Off:
        return "off";
    case T::On:
        return "on";
    case T::None:
        break;
    }

    return "none";
}

const char* _relayPulseToPayload(RelayPulse pulse) {
    return _relayTristateToPayload(pulse);
}

const char* _relayLockToPayload(RelayLock lock) {
    return _relayTristateToPayload(lock);
}

namespace settings {
namespace internal {

template <>
RelayPulse convert(const String& value) {
    return _relayPayloadToTristate<RelayPulse>(value.c_str());
}

template <>
RelayLock convert(const String& value) {
    return _relayPayloadToTristate<RelayLock>(value.c_str());
}

template <>
RelayProvider convert(const String& value) {
    auto type = static_cast<RelayProvider>(value.toInt());
    switch (type) {
    case RelayProvider::None:
    case RelayProvider::Dummy:
    case RelayProvider::Gpio:
    case RelayProvider::Dual:
    case RelayProvider::Stm:
        return type;
    }

    return RelayProvider::None;
}

template <>
RelayType convert(const String& value) {
    auto type = static_cast<RelayType>(value.toInt());
    switch (type) {
    case RelayType::Normal:
    case RelayType::Inverse:
    case RelayType::Latched:
    case RelayType::LatchedInverse:
        return type;
    }

    return RelayType::Normal;
}

template <>
RelayMaskHelper convert(const String& value) {
    return RelayMaskHelper(convert<unsigned long>(value));
}

template <>
String serialize(const RelayMaskHelper& mask) {
    return mask.toString();
}

} // namespace internal
} // namespace settings

// -----------------------------------------------------------------------------
// RELAY CONTROL
// -----------------------------------------------------------------------------

RelayProviderBase* _relayDummyProvider();

struct relay_t {
public:
    // Struct defaults to empty relay configuration, as we allow switches to exist without real GPIOs
    relay_t() = default;

    relay_t(RelayProviderBasePtr&& provider_) :
        provider(provider_.release())
    {}

    relay_t(RelayProviderBase* provider_) :
        provider(provider_)
    {}

    // ON / OFF actions implementation
    RelayProviderBase* provider { _relayDummyProvider() };

    // Timers
    unsigned long delay_on { 0ul };                // Delay to turn relay ON
    unsigned long delay_off { 0ul };               // Delay to turn relay OFF

    RelayPulse pulse { RelayPulse::None };      // Sets up a timer for the opposite mode
    unsigned long pulse_ms { 0ul };                // Pulse length in millis
    Ticker* pulseTicker { nullptr };               // Holds the pulse back timer

    unsigned long fw_start { 0ul };                // Flood window start time
    unsigned char fw_count { 0u };                 // Number of changes within the current flood window

    unsigned long change_start { 0ul };            // Time when relay was scheduled to change
    unsigned long change_delay { 0ul };            // Delay until the next change

    // Status
    bool current_status { false };                 // Holds the current (physical) status of the relay
    bool target_status { false };                  // Holds the target status
    RelayLock lock { RelayLock::None };        // Holds the value of target status that persists and cannot be changed from.

    // MQTT
    bool report { false };                         // Whether to report to own topic
    bool group_report { false };                   // Whether to report to group topic
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

RelayStatusCallback _relay_status_notify { nullptr };
RelayStatusCallback _relay_status_change { nullptr };

#if WEB_SUPPORT

bool _relay_report_ws = false;

#endif // WEB_SUPPORT

#if MQTT_SUPPORT || API_SUPPORT

String _relay_rpc_payload_on;
String _relay_rpc_payload_off;
String _relay_rpc_payload_toggle;

#endif // MQTT_SUPPORT || API_SUPPORT

// -----------------------------------------------------------------------------
// RELAY PROVIDERS
// -----------------------------------------------------------------------------

// 'anchor' default virtual implementations to the relay.cpp.o

RelayProviderBase::~RelayProviderBase() {
}

void RelayProviderBase::dump() {
}

bool RelayProviderBase::setup() {
    return true;
}

void RelayProviderBase::boot(bool) {
}

void RelayProviderBase::notify(bool) {
}

// Direct status notifications

void relaySetStatusNotify(RelayStatusCallback callback) {
    _relay_status_notify = callback;
}

void relaySetStatusChange(RelayStatusCallback callback) {
    _relay_status_change = callback;
}

// No-op provider, available for purely virtual relays that are controlled only via API

struct DummyProvider : public RelayProviderBase {
    const char* id() const override {
        return "dummy";
    }

    void change(bool) override {
    }
};

RelayProviderBase* _relayDummyProvider() {
    static DummyProvider provider;
    return &provider;
}

// Real GPIO provider, using BasePin interface to implement writers

struct GpioProvider : public RelayProviderBase {
    GpioProvider(unsigned char id, RelayType type, std::unique_ptr<BasePin>&& pin, std::unique_ptr<BasePin>&& reset_pin) :
        _id(id),
        _type(type),
        _pin(std::move(pin)),
        _reset_pin(std::move(reset_pin))
    {}

    const char* id() const override {
        return "gpio";
    }

    bool setup() override {
        if (!_pin) {
            return false;
        }

        _pin->pinMode(OUTPUT);
        if (_reset_pin) {
            _reset_pin->pinMode(OUTPUT);
        }

        if (_type == RelayType::Inverse) {
            _pin->digitalWrite(HIGH);
        }

        return true;
    }

    void change(bool status) override {
        switch (_type) {
        case RelayType::Normal:
            _pin->digitalWrite(status);
            break;
        case RelayType::Inverse:
            _pin->digitalWrite(!status);
            break;
        case RelayType::Latched:
        case RelayType::LatchedInverse: {
            bool pulse = (_type == RelayType::Latched) ? HIGH : LOW;
            _pin->digitalWrite(!pulse);
            if (_reset_pin) {
                _reset_pin->digitalWrite(!pulse);
            }
            if (status || (!_reset_pin)) {
                _pin->digitalWrite(pulse);
            } else {
                _reset_pin->digitalWrite(pulse);
            }
            nice_delay(RELAY_LATCHING_PULSE);
            // TODO: note that we stall loop() execution
            // need to ensure only relay task is active
            _pin->digitalWrite(!pulse);
            if (_reset_pin) {
                _reset_pin->digitalWrite(!pulse);
            }
        }
        }
    }

private:
    unsigned char _id { RELAY_NONE };
    RelayType _type { RelayType::Normal };
    std::unique_ptr<BasePin> _pin;
    std::unique_ptr<BasePin> _reset_pin;
};

// Special provider for Sonoff Dual, using serial protocol

#if RELAY_PROVIDER_DUAL_SUPPORT

class DualProvider : public RelayProviderBase {
public:
    DualProvider() = delete;
    explicit DualProvider(unsigned char id) : _id(id) {
        _instances.push_back(this);
    }

    ~DualProvider() {
        _instances.erase(
            std::remove(_instances.begin(), _instances.end(), this),
            _instances.end());
    }

    const char* id() const override {
        return "dual";
    }

    bool setup() override {
        static bool once { false };
        if (!once) {
            once = true;
            Serial.begin(SERIAL_BAUDRATE);
            espurnaRegisterLoop(loop);
        }
        return true;
    }

    void change(bool) override {
        static bool scheduled { false };
        if (!scheduled) {
            schedule_function([]() {
                flush();
                scheduled = false;
            });
        }
    }

    unsigned char relayId() const {
        return _id;
    }

    static std::vector<DualProvider*>& instances() {
        return _instances;
    }

    // Porting the old masking code from buttons
    // (no guarantee that this actually works, based on hearsay and some 3rd-party code)
    // | first | second |  mask |
    // |  OFF  |  OFF   |  0x0  |
    // |  ON   |  OFF   |  0x1  |
    // |  OFF  |  ON    |  0x2  |
    // |  ON   |  ON    |  0x3  |
    // i.e. set status bit mask[INSTANCE] for each relay
    // unless everything is ON, then *only* send mask[SIZE] bit and erase the rest

    static void flush() {
        bool sync { true };
        RelayMaskHelper mask;
        for (unsigned char index = 0; index < _instances.size(); ++index) {
            bool status { relayStatus(_instances[index]->relayId()) };
            sync = sync && status;
            mask.set(index, status);
        }

        if (sync) {
            mask.reset();
            mask.set(_instances.size(), true);
        }

        DEBUG_MSG_P(PSTR("[RELAY] Sending DUAL mask: %s\n"), mask.toString().c_str());

        uint8_t buffer[4] { 0xa0, 0x04, static_cast<unsigned char>(mask.toUnsigned()), 0xa1 };
        Serial.write(buffer, sizeof(buffer));
        Serial.flush();
    }

    static void loop() {
        if (Serial.available() < 4) {
            return;
        }

        unsigned char bytes[4] = {0};
        Serial.readBytes(bytes, 4);
        if ((bytes[0] != 0xA0) && (bytes[1] != 0x04) && (bytes[3] != 0xA1)) {
            return;
        }

        // RELAYs and BUTTONs are synchonized in the SIL F330
        // Make sure we handle SYNC action first
        RelayMaskHelper mask(bytes[2]);
        if (mask[_instances.size()]) {
            for (auto& instance : _instances) {
                relayStatus(instance->relayId(), true);
            }
            return;
        }

        // Then, manage relays individually
        for (unsigned char index = 0; index < _instances.size(); ++index) {
            relayStatus(_instances[index]->relayId(), mask[index]);
        }
    }

private:
    unsigned char _id { 0 };

    static std::vector<DualProvider*> _instances;
};

std::vector<DualProvider*> DualProvider::_instances;

#endif // RELAY_PROVIDER_DUAL_SUPPORT

// Special provider for ESP01-relays with STM co-MCU driving the relays

#if RELAY_PROVIDER_STM_SUPPORT

class StmProvider : public RelayProviderBase {
public:
    StmProvider() = delete;
    explicit StmProvider(unsigned char id) :
        _id(id)
    {}

    const char* id() const override {
        return "stm";
    }

    bool setup() override {
        static bool once { false };
        if (!once) {
            once = true;
            Serial.begin(SERIAL_BAUDRATE);
        }
        return true;
    }

    void boot(bool) override {
        // XXX: this was part of the legacy implementation
        // "because of broken stm relay firmware"
        _relays[_id].change_delay = 3000 + 1000 * _id;
    }

    void change(bool status) {
        Serial.flush();
        Serial.write(0xA0);
        Serial.write(_id + 1);
        Serial.write(status);
        Serial.write(0xA1 + status + _id);

        // TODO: is this really solved via interlock delay, so we don't have to switch contexts here?
        //delay(100);

        Serial.flush();
    }

private:
    unsigned char _id;
};

#endif // RELAY_PROVIDER_STM_SUPPORT

// -----------------------------------------------------------------------------
// UTILITY
// -----------------------------------------------------------------------------

bool _relayTryParseId(const char* p, unsigned char& relayID) {
    char* endp { nullptr };
    const unsigned long result { strtoul(p, &endp, 10) };
    if ((endp == p) || (*endp != '\0') || (result >= relayCount())) {
        return false;
    }

    relayID = result;
    return true;
}

bool _relayTryParseIdFromPath(const String& endpoint, unsigned char& relayID) {
    int next_slash { endpoint.lastIndexOf('/') };
    if (next_slash < 0) {
        return false;
    }

    const char* p { endpoint.c_str() + next_slash + 1 };
    if (*p == '\0') {
        DEBUG_MSG_P(PSTR("[RELAY] relayID was not specified\n"));
        return false;
    }

    return _relayTryParseId(p, relayID);
}

void _relayHandleStatus(unsigned char relayID, PayloadStatus status) {
    switch (status) {
    case PayloadStatus::Off:
        relayStatus(relayID, false);
        break;
    case PayloadStatus::On:
        relayStatus(relayID, true);
        break;
    case PayloadStatus::Toggle:
        relayToggle(relayID);
        break;
    case PayloadStatus::Unknown:
        break;
    }
}

bool _relayHandlePayload(unsigned char relayID, const char* payload) {
    auto status = relayParsePayload(payload);
    if (status != PayloadStatus::Unknown) {
        _relayHandleStatus(relayID, status);
        return true;
    }

    DEBUG_MSG_P(PSTR("[RELAY] Invalid API payload (%s)\n"), payload);
    return false;
}

bool _relayHandlePayload(unsigned char relayID, const String& payload) {
    return _relayHandlePayload(relayID, payload.c_str());
}

bool _relayHandlePulsePayload(unsigned char id, const char* payload) {
    unsigned long pulse = 1000 * atof(payload);
    if (!pulse) {
        return false;
    }

    if (RelayPulse::None != _relays[id].pulse) {
        DEBUG_MSG_P(PSTR("[RELAY] Overriding relayID %u pulse settings\n"), id);
    }

    _relays[id].pulse_ms = pulse;
    _relays[id].pulse = relayStatus(id) ? RelayPulse::On : RelayPulse::Off;
    relayToggle(id, true, false);

    return true;
}

bool _relayHandlePulsePayload(unsigned char id, const String& payload) {
    return _relayHandlePulsePayload(id, payload.c_str());
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
        relay.lock = relay.target_status ? RelayLock::On : RelayLock::Off;
    }
    _relay_sync_locked = true;
}

void _relayUnlockAll() {
    for (auto& relay : _relays) {
        relay.lock = RelayLock::None;
    }
    _relay_sync_locked = false;
}

bool _relayStatusLock(unsigned char id, bool status) {
    if (_relays[id].lock != RelayLock::None) {
        bool lock = _relays[id].lock == RelayLock::On;
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
        _relays[id].current_status = target;
        _relays[id].provider->change(target);
        if (_relay_status_change) {
            _relay_status_change(id, target);
        }

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
            const auto boot_mode = getSetting({"relayBoot", id}, _relayBootMode(id));
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

// -----------------------------------------------------------------------------
// RELAY
// -----------------------------------------------------------------------------

namespace {

inline RelayMaskHelper _relayMaskRtcmem() {
    return RelayMaskHelper(Rtcmem->relay);
}

inline void _relayMaskRtcmem(uint32_t mask) {
    Rtcmem->relay = mask;
}

inline void _relayMaskRtcmem(const RelayMask& mask) {
    _relayMaskRtcmem(mask.to_ulong());
}

inline void _relayMaskRtcmem(const RelayMaskHelper& mask) {
    _relayMaskRtcmem(mask.toUnsigned());
}

RelayMaskHelper _relayMaskSettings() {
    static RelayMaskHelper defaultMask;
    return getSetting("relayBootMask", defaultMask);
}

void _relayMaskSettings(const String& mask) {
    setSetting("relayBootMask", mask);
}

inline void _relayMaskSettings(const RelayMaskHelper& mask) {
    _relayMaskSettings(settings::internal::serialize(mask));
}

} // namespace

// Pulse timers (timer after ON or OFF event)
// TODO: integrate with scheduled ON or OFF

void relayPulse(unsigned char id) {

    auto& relay = _relays[id];
    if (!relay.pulseTicker) {
        relay.pulseTicker = new Ticker();
    }

    relay.pulseTicker->detach();
    auto mode = relay.pulse;
    if (mode == RelayPulse::None) {
        return;
    }

    auto ms = relay.pulse_ms;
    if (ms == 0) {
        return;
    }

    // TODO: drive ticker on a lower 'tick rate', allow delays longer than 114 minutes
    //       we don't necessarily need millisecond precision. which is also not achievable, most likely,
    //       because of the SDK scheduler. or, at least not for every available provider.

    // limit is per https://www.espressif.com/sites/default/files/documentation/2c-esp8266_non_os_sdk_api_reference_en.pdf
    // > 3.1.1 os_timer_arm
    // > the timer value allowed ranges from 5 to 0x68D7A3.
    if ((ms < 5) || (ms >= 0x68D7A3)) {
        DEBUG_MSG_P(PSTR("[RELAY] Unable to schedule the delay %lums (longer than 114 minutes)\n"), ms);
        return;
    }

    if ((mode == RelayPulse::On) != relay.current_status) {
        DEBUG_MSG_P(PSTR("[RELAY] Scheduling relay #%d back in %lums (pulse)\n"), id, ms);
        relay.pulseTicker->once_ms(ms, relayToggle, id);
        // Reconfigure after dynamic pulse
        relay.pulse = getSetting({"relayPulse", id}, _relayPulseMode(id));
        relay.pulse_ms = static_cast<unsigned long>(1000.0 * getSetting({"relayTime", id}, _relayPulseTime(id)));
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

        _relays[id].provider->notify(status);
        if (_relay_status_notify) {
            _relay_status_notify(id, status);
        }

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
        _relays[id].report = report;
        _relays[id].group_report = group_report;

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

bool relayStatusTarget(unsigned char id) {
    if (id >= _relays.size()) return false;
    return _relays[id].target_status;
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

void relaySave(bool persist) {
    RelayMaskHelper mask;
    for (unsigned char id = 0; id < _relays.size(); ++id) {
        mask.set(id, _relays[id].current_status);
    }

    // Persist only to rtcmem, unless requested to save to settings
    DEBUG_MSG_P(PSTR("[RELAY] Relay mask: %s\n"), mask.toString().c_str());
    _relayMaskRtcmem(mask);

    // The 'persist' flag controls whether we are commiting this change or not.
    // It is useful to set it to 'false' if the relay change triggering the
    // save involves a relay whose boot mode is independent from current mode,
    // thus storing the last relay value is not absolutely necessary.
    // Nevertheless, we store the value in the EEPROM buffer so it will be written
    // on the next commit.
    if (persist) {
        _relayMaskSettings(mask);
        eepromCommit(); // TODO: should this respect settings auto-save?
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

void _relaySettingsMigrate(int version) {
    if (!version || (version >= 5)) {
        return;
    }

    delSettingPrefix({
        "relayGPIO",
        "relayProvider",
        "relayType",
    });
    delSetting("relays");
}

void _relayBoot(unsigned char index, const RelayMaskHelper& mask) {
    const auto boot_mode = getSetting({"relayBoot", index}, _relayBootMode(index));

    auto status = false;
    auto lock = RelayLock::None;

    switch (boot_mode) {
    case RELAY_BOOT_SAME:
        status = mask[index];
        break;
    case RELAY_BOOT_TOGGLE:
        status = !mask[index];
        break;
    case RELAY_BOOT_ON:
        status = true;
        break;
    case RELAY_BOOT_LOCKED_ON:
        status = true;
        lock = RelayLock::On;
        break;
    case RELAY_BOOT_OFF:
        status = false;
        break;
    case RELAY_BOOT_LOCKED_OFF:
        status = false;
        lock = RelayLock::Off;
        break;
    }

    auto& relay = _relays[index];

    relay.current_status = !status;
    relay.target_status = status;
    relay.lock = lock;

    relay.change_start = millis();
    relay.change_delay = status
        ? relay.delay_on
        : relay.delay_off;

    relay.provider->boot(status);
}

void _relayBootAll() {
    auto mask = rtcmemStatus()
        ? _relayMaskRtcmem()
        : _relayMaskSettings();

    _relayRecursive = true;

    bool once { true };
    static RelayMask done;
    for (unsigned char id = 0; id < relayCount(); ++id) {
        if (done[id]) {
            continue;
        }

        if (once) {
            DEBUG_MSG_P(PSTR("[RELAY] Number of relays: %u, boot mask: %s\n"),
                _relays.size(), mask.toString().c_str());
            once = false;
        }

        done.set(id, true);
        _relayBoot(id, mask);
    }

    _relayRecursive = false;
}

void _relayConfigure() {
    for (unsigned char i = 0, relays = _relays.size() ; (i < relays); ++i) {
        _relays[i].pulse = getSetting({"relayPulse", i}, _relayPulseMode(i));
        _relays[i].pulse_ms = static_cast<unsigned long>(1000.0 * getSetting({"relayTime", i}, _relayPulseTime(i)));

        _relays[i].delay_on = getSetting({"relayDelayOn", i}, _relayDelayOn(i));
        _relays[i].delay_off = getSetting({"relayDelayOff", i}, _relayDelayOff(i));
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

    // Note: we use byte instead of bool to ever so slightly compress json output
    for (unsigned char i=0; i<relayCount(); i++) {
        status.add<uint8_t>(_relays[i].target_status);
        lock.add(static_cast<uint8_t>(_relays[i].lock));
    }
}

void _relayWebSocketSendRelays(JsonObject& root) {
    JsonObject& config = root.createNestedObject("relayConfig");

    config["size"] = relayCount();
    config["start"] = 0;

    const char* keys[] = {
        "prov", "name", "boot", "pulse", "pulse_time"
    };
    JsonArray& schema = config.createNestedArray("schema");
    schema.copyFrom(keys, sizeof(keys) / sizeof(*keys));

    #if SCHEDULER_SUPPORT
        schema.add("sch_last");
    #endif

    #if MQTT_SUPPORT
        schema.add("group");
        schema.add("group_sync");
        schema.add("on_disc");
    #endif

    JsonArray& relays = config.createNestedArray("relays");

    for (unsigned char id = 0;  id < relayCount(); ++id) {
        JsonArray& relay = relays.createNestedArray();
        relay.add(_relays[id].provider->id());
        relay.add(getSetting({"relayName", id}));
        relay.add(getSetting({"relayBoot", id}, _relayBootMode(id)));

        relay.add(static_cast<uint8_t>(_relays[id].pulse));
        relay.add(_relays[id].pulse_ms / 1000.0);

        #if SCHEDULER_SUPPORT
            relay.add(getSetting({"relayLastSch", id}, SCHEDULER_RESTORE_LAST_SCHEDULE));
        #endif

        #if MQTT_SUPPORT
            relay.add(getSetting({"mqttGroup", id}));
            relay.add(getSetting({"mqttGroupSync", id}, 0));
            relay.add(getSetting({"relayOnDisc", id}, 0));
        #endif
    }
}

void _relayWebSocketOnVisible(JsonObject& root) {
    if (relayCount() == 0) return;

    if (relayCount() > 1) {
        root["multirelayVisible"] = 1;
        root["relaySync"] = getSetting("relaySync", RELAY_SYNC);
        root["relayDelayInterlock"] = getSetting("relayDelayInterlock", RELAY_DELAY_INTERLOCK);
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

        _relayHandlePayload(relayID, data["status"].as<const char*>());

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

template <typename T>
bool _relayApiTryHandle(ApiRequest& request, T&& callback) {
    auto id_param = request.wildcard(0);
    unsigned char id;
    if (!_relayTryParseId(id_param.c_str(), id)) {
        return false;
    }

    return callback(id);
}

void relaySetupAPI() {

    if (!relayCount()) {
        return;
    }

    apiRegister(F(MQTT_TOPIC_RELAY),
        [](ApiRequest&, JsonObject& root) {
            JsonArray& relays = root.createNestedArray("relayStatus");
            for (unsigned char id = 0; id < relayCount(); ++id) {
                relays.add(_relays[id].target_status ? 1 : 0);
            }
            return true;
        },
        nullptr
    );

    apiRegister(F(MQTT_TOPIC_RELAY "/+"),
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](unsigned char id) {
                request.send(String(_relays[id].target_status ? 1 : 0));
                return true;
            });
        },
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](unsigned char id) {
                return _relayHandlePayload(id, request.param(F("value")));
            });
        }
    );

    apiRegister(F(MQTT_TOPIC_PULSE "/+"),
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](unsigned char id) {
                request.send(String(static_cast<double>(_relays[id].pulse_ms) / 1000));
                return true;
            });
        },
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](unsigned char id) {
                return _relayHandlePulsePayload(id, request.param(F("value")));
            });
        }
    );

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

bool _relayMqttHeartbeat(heartbeat::Mask mask) {
    if (mask & heartbeat::Report::Relay)
        relayMQTT();

    return mqttConnected();
}

void relayMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (!relayCount()) {
        return;
    }

    if (type == MQTT_CONNECT_EVENT) {
        // Subscribe to own /set topic
        char relay_topic[strlen(MQTT_TOPIC_RELAY) + 3];
        snprintf_P(relay_topic, sizeof(relay_topic), PSTR("%s/+"), MQTT_TOPIC_RELAY);
        mqttSubscribe(relay_topic);

        // Subscribe to pulse topic
        char pulse_topic[strlen(MQTT_TOPIC_PULSE) + 3];
        snprintf_P(pulse_topic, sizeof(pulse_topic), PSTR("%s/+"), MQTT_TOPIC_PULSE);
        mqttSubscribe(pulse_topic);

        // Subscribe to group topics
        for (unsigned char i=0; i < _relays.size(); i++) {
            const auto t = getSetting({"mqttGroup", i});
            if (t.length() > 0) mqttSubscribeRaw(t.c_str());
        }
    }

    if (type == MQTT_MESSAGE_EVENT) {

        String t = mqttMagnitude((char *) topic);
        unsigned char id;
        if (!_relayTryParseIdFromPath(t.c_str(), id)) {
            return;
        }

        if (t.startsWith(MQTT_TOPIC_PULSE)) {
            _relayHandlePulsePayload(id, payload);
            _relays[id].report = mqttForward();
            return;
        }

        if (t.startsWith(MQTT_TOPIC_RELAY)) {
            _relayHandlePayload(id, payload);
            _relays[id].report = mqttForward();
            return;
        }

        // TODO: cache group topics instead of reading settings each time?
        // TODO: this is another kvs::foreach case, since we slow down MQTT when settings grow
        for (unsigned char i=0; i < _relays.size(); i++) {

            const String t = getSetting({"mqttGroup", i});
            if (!t.length()) break;

            if (t == topic) {

                auto value = relayParsePayload(payload);
                if (value == PayloadStatus::Unknown) return;

                if ((value == PayloadStatus::On) || (value == PayloadStatus::Off)) {
                    if (getSetting({"mqttGroupSync", i}, RELAY_GROUP_SYNC_NORMAL) == RELAY_GROUP_SYNC_INVERSE) {
                        value = _relayStatusInvert(value);
                    }
                }

                DEBUG_MSG_P(PSTR("[RELAY] Matched group topic for relayID %d\n"), i);
                _relayHandleStatus(i, value);
                _relays[i].group_report = false;

            }
        }

    }

    // TODO: safeguard against network issues. this one has good intentions, but we may end up
    // switching relays back and forth when connection is unstable but reconnects very fast after the failure

    if (type == MQTT_DISCONNECT_EVENT) {
        for (unsigned char i=0; i < _relays.size(); i++) {
            const auto reaction = getSetting({"relayOnDisc", i}, 0);

            bool status;
            switch (reaction) {
            case 1:
                status = false;
                break;
            case 2:
                status = true;
                break;
            default:
                return;
            }

            DEBUG_MSG_P(PSTR("[RELAY] Turn %s relay #%u due to MQTT disconnection\n"), status ? "ON" : "OFF", i);
            relayStatus(i, status);
        }
    }

}

void relaySetupMQTT() {
    mqttHeartbeat(_relayMqttHeartbeat);
    mqttRegister(relayMQTTCallback);
}

#endif

//------------------------------------------------------------------------------
// Settings
//------------------------------------------------------------------------------

#if TERMINAL_SUPPORT

void _relayInitCommands() {

    terminalRegisterCommand(F("RELAY"), [](const terminal::CommandContext& ctx) {
        auto showRelays = [&](unsigned char start, unsigned char stop, bool full = true) {
            for (unsigned char index = start; index < stop; ++index) {
                auto& relay = _relays[index];

                char pulse_info[64] = "";
                if ((relay.pulse != RelayPulse::None) && (relay.pulse_ms)) {
                    snprintf_P(pulse_info, sizeof(pulse_info), PSTR(" Pulse=%s Time=%u"),
                        _relayPulseToPayload(relay.pulse), relay.pulse_ms);
                }

                char extended_info[64] = "";
                if (full) {
                    int index = 0;
                    if (index >= 0 && relay.delay_on) {
                        index += snprintf_P(extended_info + index, sizeof(extended_info),
                                PSTR(" DelayOn=%u"), relay.delay_on);
                    }
                    if (index >= 0 && relay.delay_off) {
                        index += snprintf_P(extended_info + index, sizeof(extended_info),
                                PSTR(" DelayOff=%u"), relay.delay_off);
                    }
                    if (index >= 0 && relay.lock != RelayLock::None) {
                        index += snprintf_P(extended_info + index, sizeof(extended_info),
                                PSTR(" Lock=%s"), _relayLockToPayload(relay.lock));
                    }
                }

                ctx.output.printf_P(PSTR("relay%u {Prov=%s Current=%s Target=%s%s%s}\n"),
                    index, relay.provider->id(),
                    relay.current_status ? "ON" : "OFF",
                    relay.target_status ? "ON" : "OFF",
                    pulse_info,
                    extended_info
                );
            }
        };

        if (ctx.argc == 1) {
            showRelays(0, _relays.size());
            terminalOK(ctx);
            return;
        }

        unsigned char id;
        if (!_relayTryParseId(ctx.argv[1].c_str(), id)) {
            terminalError(ctx, F("Invalid relayID"));
            return;
        }

        if (ctx.argc > 2) {
            auto status = relayParsePayload(ctx.argv[2].c_str());
            if (PayloadStatus::Unknown == status) {
                terminalError(ctx, F("Invalid status"));
                return;
            }

            _relayHandleStatus(id, status);
        }

        showRelays(id, id + 1, false);
        terminalOK(ctx);
    });

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

// Dummy relays for virtual light switches (hardware-less), Sonoff Dual, Sonoff RF Bridge and Tuya

void relaySetupDummy(size_t size, bool reconfigure) {

    if (size == _relayDummy) return;

    const size_t new_size = ((_relays.size() - _relayDummy) + size);
    if (new_size > RelaysMax) return;

    _relayDummy = size;
    _relays.resize(new_size);

    if (reconfigure) {
        _relayConfigure();
    }

    #if BROKER_SUPPORT
        ConfigBroker::Publish("relayDummy", String(int(size)));
    #endif

}

constexpr size_t _relayAdhocPins() {
    return 0
    #if RELAY1_PIN != GPIO_NONE
        + 1
    #endif
    #if RELAY2_PIN != GPIO_NONE
        + 1
    #endif
    #if RELAY3_PIN != GPIO_NONE
        + 1
    #endif
    #if RELAY4_PIN != GPIO_NONE
        + 1
    #endif
    #if RELAY5_PIN != GPIO_NONE
        + 1
    #endif
    #if RELAY6_PIN != GPIO_NONE
        + 1
    #endif
    #if RELAY7_PIN != GPIO_NONE
        + 1
    #endif
    #if RELAY8_PIN != GPIO_NONE
        + 1
    #endif
    ;
}

struct RelayGpioProviderCfg {
    GpioBase* base;
    unsigned char main;
    unsigned char reset;
};

RelayGpioProviderCfg _relayGpioProviderCfg(unsigned char index) {
    return {
        gpioBase(getSetting({"relayGPIOType", index}, _relayPinType(index))),
        getSetting({"relayGPIO", index}, _relayPin(index)),
        getSetting({"relayResetGPIO", index}, _relayResetPin(index))};
}

using GpioCheck = bool(*)(unsigned char);

std::unique_ptr<GpioProvider> _relayGpioProvider(unsigned char index, RelayType type) {
    auto cfg = _relayGpioProviderCfg(index);
    if (!cfg.base) {
        return nullptr;
    }

    auto main = gpioRegister(*cfg.base, cfg.main);
    if (!main) {
        return nullptr;
    }

    auto reset = gpioRegister(*cfg.base, cfg.reset);
    return std::make_unique<GpioProvider>(
        index, type, std::move(main), std::move(reset)
    );
}

RelayProviderBasePtr _relaySetupProvider(unsigned char index) {
    auto provider = getSetting({"relayProv", index}, _relayProvider(index));
    auto type = getSetting({"relayType", index}, _relayType(index));

    RelayProviderBasePtr result;

    switch (provider) {
    case RelayProvider::Dummy:
        result = std::make_unique<DummyProvider>();
        break;
    case RelayProvider::Gpio:
        result = _relayGpioProvider(index, type);
        break;
    case RelayProvider::Stm:
#if RELAY_PROVIDER_STM_SUPPORT
        result = std::make_unique<StmProvider>(index);
#endif
        break;
    case RelayProvider::Dual:
#if RELAY_PROVIDER_DUAL_SUPPORT
        result = std::make_unique<DualProvider>(index);
#endif
        break;
    case RelayProvider::None:
        break;
    }

    return result;
}

void _relaySetup() {
    _relays.reserve(_relayAdhocPins());

    for (unsigned char id = 0; id < RelaysMax; ++id) {
        auto impl = _relaySetupProvider(id);
        if (!impl) {
            break;
        }
        if (!impl->setup()) {
            break;
        }
        _relays.emplace_back(std::move(impl));
    }

    relaySetupDummy(getSetting("relayDummy", DUMMY_RELAY_COUNT));
}

void relaySetup() {
    _relaySettingsMigrate(migrateVersion());

    _relaySetup();

    _relayConfigure();
    _relayBootAll();
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

}

bool relayAdd(RelayProviderBasePtr&& provider) {
    if (provider && provider->setup()) {
        static bool scheduled { false };
        _relays.emplace_back(std::move(provider));
        if (!scheduled) {
            schedule_function([]() {
                _relayConfigure();
                _relayBootAll();
                scheduled = false;
            });
        }
        return true;
    }

    return false;
}

#endif // RELAY_SUPPORT == 1
