/*

RELAY MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if RELAY_SUPPORT

#include <Ticker.h>
#include <ArduinoJson.h>

#include <bitset>
#include <cstring>
#include <functional>
#include <vector>

#include "api.h"
#include "mqtt.h"
#include "relay.h"
#include "rpc.h"
#include "rtcmem.h"
#include "settings.h"
#include "storage_eeprom.h"
#include "terminal.h"
#include "utils.h"
#include "ws.h"

// -----------------------------------------------------------------------------

namespace relay {
namespace {

using FloodWindowDuration = std::chrono::duration<float>;
using PulseDuration = std::chrono::duration<float>;

namespace build {

constexpr espurna::duration::Milliseconds saveDelay() {
    return espurna::duration::Milliseconds(RELAY_SAVE_DELAY);
}

constexpr size_t dummyCount() {
    return DUMMY_RELAY_COUNT;
}

constexpr int syncMode() {
    return RELAY_SYNC;
}

constexpr espurna::duration::Milliseconds latchingPulse() {
    return espurna::duration::Milliseconds(RELAY_LATCHING_PULSE);
}

constexpr FloodWindowDuration floodWindow() {
    return FloodWindowDuration(RELAY_FLOOD_WINDOW);
}

static_assert(floodWindow().count() >= 0.0f, "");

constexpr espurna::duration::Milliseconds floodWindowMs() {
    return std::chrono::duration_cast<espurna::duration::Milliseconds>(floodWindow());
}

constexpr unsigned long floodChanges() {
    return RELAY_FLOOD_CHANGES;
}

constexpr espurna::duration::Milliseconds interlockDelay() {
    return espurna::duration::Milliseconds(RELAY_DELAY_INTERLOCK);
}

constexpr PulseDuration pulseTime(size_t index) {
    return PulseDuration(
        (index == 0) ? RELAY1_PULSE_TIME :
        (index == 1) ? RELAY2_PULSE_TIME :
        (index == 2) ? RELAY3_PULSE_TIME :
        (index == 3) ? RELAY4_PULSE_TIME :
        (index == 4) ? RELAY5_PULSE_TIME :
        (index == 5) ? RELAY6_PULSE_TIME :
        (index == 6) ? RELAY7_PULSE_TIME :
        (index == 7) ? RELAY8_PULSE_TIME : RELAY_PULSE_TIME
    );
}

static_assert(pulseTime(0).count() >= 0.0f, "");
static_assert(pulseTime(1).count() >= 0.0f, "");
static_assert(pulseTime(2).count() >= 0.0f, "");
static_assert(pulseTime(3).count() >= 0.0f, "");
static_assert(pulseTime(4).count() >= 0.0f, "");
static_assert(pulseTime(5).count() >= 0.0f, "");
static_assert(pulseTime(6).count() >= 0.0f, "");
static_assert(pulseTime(7).count() >= 0.0f, "");

constexpr RelayPulse pulseMode(size_t index) {
    return (
        (index == 0) ? RELAY1_PULSE_MODE :
        (index == 1) ? RELAY2_PULSE_MODE :
        (index == 2) ? RELAY3_PULSE_MODE :
        (index == 3) ? RELAY4_PULSE_MODE :
        (index == 4) ? RELAY5_PULSE_MODE :
        (index == 5) ? RELAY6_PULSE_MODE :
        (index == 6) ? RELAY7_PULSE_MODE :
        (index == 7) ? RELAY8_PULSE_MODE : RELAY_PULSE_NONE
    );
}

constexpr espurna::duration::Milliseconds delayOn(size_t index) {
    return espurna::duration::Milliseconds(
        (index == 0) ? RELAY1_DELAY_ON :
        (index == 1) ? RELAY2_DELAY_ON :
        (index == 2) ? RELAY3_DELAY_ON :
        (index == 3) ? RELAY4_DELAY_ON :
        (index == 4) ? RELAY5_DELAY_ON :
        (index == 5) ? RELAY6_DELAY_ON :
        (index == 6) ? RELAY7_DELAY_ON :
        (index == 7) ? RELAY8_DELAY_ON : 0ul
    );
}

constexpr espurna::duration::Milliseconds delayOff(size_t index) {
    return espurna::duration::Milliseconds(
        (index == 0) ? RELAY1_DELAY_OFF :
        (index == 1) ? RELAY2_DELAY_OFF :
        (index == 2) ? RELAY3_DELAY_OFF :
        (index == 3) ? RELAY4_DELAY_OFF :
        (index == 4) ? RELAY5_DELAY_OFF :
        (index == 5) ? RELAY6_DELAY_OFF :
        (index == 6) ? RELAY7_DELAY_OFF :
        (index == 7) ? RELAY8_DELAY_OFF : 0ul
    );
}

constexpr unsigned char pin(size_t index) {
    return (
        (index == 0) ? RELAY1_PIN :
        (index == 1) ? RELAY2_PIN :
        (index == 2) ? RELAY3_PIN :
        (index == 3) ? RELAY4_PIN :
        (index == 4) ? RELAY5_PIN :
        (index == 5) ? RELAY6_PIN :
        (index == 6) ? RELAY7_PIN :
        (index == 7) ? RELAY8_PIN : GPIO_NONE
    );
}

constexpr RelayType type(size_t index) {
    return (
        (index == 0) ? RELAY1_TYPE :
        (index == 1) ? RELAY2_TYPE :
        (index == 2) ? RELAY3_TYPE :
        (index == 3) ? RELAY4_TYPE :
        (index == 4) ? RELAY5_TYPE :
        (index == 5) ? RELAY6_TYPE :
        (index == 6) ? RELAY7_TYPE :
        (index == 7) ? RELAY8_TYPE : RELAY_TYPE_NORMAL
    );
}

constexpr GpioType pinType(size_t index) {
    return (
        (index == 0) ? RELAY1_PIN_TYPE :
        (index == 1) ? RELAY2_PIN_TYPE :
        (index == 2) ? RELAY3_PIN_TYPE :
        (index == 3) ? RELAY4_PIN_TYPE :
        (index == 4) ? RELAY5_PIN_TYPE :
        (index == 5) ? RELAY6_PIN_TYPE :
        (index == 6) ? RELAY7_PIN_TYPE :
        (index == 7) ? RELAY8_PIN_TYPE : GPIO_TYPE_NONE
    );
}

constexpr unsigned char resetPin(size_t index) {
    return (
        (index == 0) ? RELAY1_RESET_PIN :
        (index == 1) ? RELAY2_RESET_PIN :
        (index == 2) ? RELAY3_RESET_PIN :
        (index == 3) ? RELAY4_RESET_PIN :
        (index == 4) ? RELAY5_RESET_PIN :
        (index == 5) ? RELAY6_RESET_PIN :
        (index == 6) ? RELAY7_RESET_PIN :
        (index == 7) ? RELAY8_RESET_PIN : GPIO_NONE
    );
}

constexpr int bootMode(size_t index) {
    return (
        (index == 0) ? RELAY1_BOOT_MODE :
        (index == 1) ? RELAY2_BOOT_MODE :
        (index == 2) ? RELAY3_BOOT_MODE :
        (index == 3) ? RELAY4_BOOT_MODE :
        (index == 4) ? RELAY5_BOOT_MODE :
        (index == 5) ? RELAY6_BOOT_MODE :
        (index == 6) ? RELAY7_BOOT_MODE :
        (index == 7) ? RELAY8_BOOT_MODE : RELAY_BOOT_OFF
    );
}

constexpr RelayProvider provider(size_t index) {
    return (
        (index == 0) ? (RELAY1_PROVIDER) :
        (index == 1) ? (RELAY2_PROVIDER) :
        (index == 2) ? (RELAY3_PROVIDER) :
        (index == 3) ? (RELAY4_PROVIDER) :
        (index == 4) ? (RELAY5_PROVIDER) :
        (index == 5) ? (RELAY6_PROVIDER) :
        (index == 6) ? (RELAY7_PROVIDER) :
        (index == 7) ? (RELAY8_PROVIDER) : RELAY_PROVIDER_NONE
    );
}

constexpr RelayMqttTopicMode mqttTopicMode(size_t index) {
    return (
        (index == 0) ? (RELAY1_MQTT_TOPIC_MODE) :
        (index == 1) ? (RELAY2_MQTT_TOPIC_MODE) :
        (index == 2) ? (RELAY3_MQTT_TOPIC_MODE) :
        (index == 3) ? (RELAY4_MQTT_TOPIC_MODE) :
        (index == 4) ? (RELAY5_MQTT_TOPIC_MODE) :
        (index == 5) ? (RELAY6_MQTT_TOPIC_MODE) :
        (index == 6) ? (RELAY7_MQTT_TOPIC_MODE) :
        (index == 7) ? (RELAY8_MQTT_TOPIC_MODE) : RELAY_MQTT_TOPIC_MODE
    );
}

const __FlashStringHelper* payloadOn() {
    return F(RELAY_MQTT_ON);
}

const __FlashStringHelper* payloadOff() {
    return F(RELAY_MQTT_OFF);
}

const __FlashStringHelper* payloadToggle() {
    return F(RELAY_MQTT_TOGGLE);
}

constexpr const char* mqttTopicSub(size_t index) {
    return (
        (index == 0) ? (RELAY1_MQTT_TOPIC_SUB) :
        (index == 1) ? (RELAY2_MQTT_TOPIC_SUB) :
        (index == 2) ? (RELAY3_MQTT_TOPIC_SUB) :
        (index == 3) ? (RELAY4_MQTT_TOPIC_SUB) :
        (index == 4) ? (RELAY5_MQTT_TOPIC_SUB) :
        (index == 5) ? (RELAY6_MQTT_TOPIC_SUB) :
        (index == 6) ? (RELAY7_MQTT_TOPIC_SUB) :
        (index == 7) ? (RELAY8_MQTT_TOPIC_SUB) : ""
    );
}

constexpr const char* mqttTopicPub(size_t index) {
    return (
        (index == 0) ? (RELAY1_MQTT_TOPIC_PUB) :
        (index == 1) ? (RELAY2_MQTT_TOPIC_PUB) :
        (index == 2) ? (RELAY3_MQTT_TOPIC_PUB) :
        (index == 3) ? (RELAY4_MQTT_TOPIC_PUB) :
        (index == 4) ? (RELAY5_MQTT_TOPIC_PUB) :
        (index == 5) ? (RELAY6_MQTT_TOPIC_PUB) :
        (index == 6) ? (RELAY7_MQTT_TOPIC_PUB) :
        (index == 7) ? (RELAY8_MQTT_TOPIC_PUB) : ""
    );
}

constexpr PayloadStatus mqttDisconnectionStatus(size_t index) {
    return (
        (index == 0) ? (RELAY1_MQTT_DISCONNECT_STATUS) :
        (index == 1) ? (RELAY2_MQTT_DISCONNECT_STATUS) :
        (index == 2) ? (RELAY3_MQTT_DISCONNECT_STATUS) :
        (index == 3) ? (RELAY4_MQTT_DISCONNECT_STATUS) :
        (index == 4) ? (RELAY5_MQTT_DISCONNECT_STATUS) :
        (index == 5) ? (RELAY6_MQTT_DISCONNECT_STATUS) :
        (index == 6) ? (RELAY7_MQTT_DISCONNECT_STATUS) :
        (index == 7) ? (RELAY8_MQTT_DISCONNECT_STATUS) : RELAY_MQTT_DISCONNECT_NONE
    );
}

} // namespace build
} // namespace
} // namespace relay

namespace {

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
        return settings::internal::serialize(toUnsigned(), 2);
    }

    const RelayMask& mask() const {
        return _mask;
    }

    void reset() {
        _mask.reset();
    }

    void set(size_t id, bool status) {
        _mask.set(id, status);
    }

    bool operator[](size_t id) const {
        return _mask[id];
    }

private:
    RelayMask _mask { 0ul };
};

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

} // namespace

namespace settings {
namespace internal {

template <>
PayloadStatus convert(const String& value) {
    auto status = static_cast<PayloadStatus>(value.toInt());
    switch (status) {
    case PayloadStatus::Off:
    case PayloadStatus::On:
    case PayloadStatus::Toggle:
    case PayloadStatus::Unknown:
        return status;
    }

    return PayloadStatus::Unknown;
}

template <>
RelayMqttTopicMode convert(const String& value) {
    auto mode = static_cast<RelayMqttTopicMode>(value.toInt());
    switch (mode) {
    case RelayMqttTopicMode::Normal:
    case RelayMqttTopicMode::Inverse:
        return mode;
    }

    return RelayMqttTopicMode::Normal;
}

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

String serialize(RelayMaskHelper mask) {
    return mask.toString();
}

} // namespace internal
} // namespace settings

namespace relay {
namespace {
namespace settings {

size_t dummyCount() {
    return getSetting("relayDummy", build::dummyCount());
}

[[gnu::unused]]
String name(size_t index) {
    return getSetting({"relayName", index});
}

RelayProvider provider(size_t index) {
    return getSetting({"relayProv", index}, build::provider(index));
}

RelayType type(size_t index) {
    return getSetting({"relayType", index}, build::type(index));
}

GpioType pinType(size_t index) {
    return getSetting({"relayGpioType", index}, build::pinType(index));
}

unsigned char pin(size_t index) {
    return getSetting({"relayGpio", index}, build::pin(index));
}

unsigned char resetPin(size_t index) {
    return getSetting({"relayResetGpio", index}, build::resetPin(index));
}

int bootMode(size_t index) {
    return getSetting({"relayBoot", index}, build::bootMode(index));
}

RelayMaskHelper bootMask() {
    static RelayMaskHelper defaultMask;
    return getSetting("relayBootMask", defaultMask);
}

void bootMask(const String& mask) {
    setSetting("relayBootMask", mask);
}

void bootMask(const RelayMaskHelper& mask) {
    bootMask(::settings::internal::serialize(mask));
}

RelayPulse pulseMode(size_t index) {
    return getSetting({"relayPulse", index}, build::pulseMode(index));
}

// TODO: clean-up duration & types, pulse should probably be an external entity

PulseDuration pulseTime(size_t index) {
    return getSetting({"relayTime", index}, build::pulseTime(index));
}

espurna::duration::Milliseconds pulseTimeMs(size_t index) {
    return std::chrono::duration_cast<espurna::duration::Milliseconds>(pulseTime(index));
}

espurna::duration::Milliseconds delayOn(size_t index) {
    return getSetting({"relayDelayOn", index}, build::delayOn(index));
}

espurna::duration::Milliseconds delayOff(size_t index) {
    return getSetting({"relayDelayOff", index}, build::delayOff(index));
}

FloodWindowDuration floodWindow() {
    return getSetting("relayFloodTime", build::floodWindow());
}

espurna::duration::Milliseconds floodWindowMs() {
    return std::chrono::duration_cast<espurna::duration::Milliseconds>(floodWindow());
}

unsigned long floodChanges() {
    return getSetting("relayFloodChanges", build::floodChanges());
}

espurna::duration::Milliseconds interlockDelay() {
    return getSetting("relayIlkDelay", build::interlockDelay());
}

int syncMode() {
    return getSetting("relaySync", build::syncMode());
}

[[gnu::unused]]
String payloadOn() {
    return getSetting("relayPayloadOn", build::payloadOn());
}

[[gnu::unused]]
String payloadOff() {
    return getSetting("relayPayloadOff", build::payloadOff());
}

[[gnu::unused]]
String payloadToggle() {
    return getSetting("relayPayloadToggle", build::payloadToggle());
}

#if MQTT_SUPPORT
String mqttTopicSub(size_t index) {
    return getSetting({"relayTopicSub", index}, build::mqttTopicSub(index));
}

String mqttTopicPub(size_t index) {
    return getSetting({"relayTopicPub", index}, build::mqttTopicPub(index));
}

RelayMqttTopicMode mqttTopicMode(size_t index) {
    return getSetting({"relayTopicMode", index}, build::mqttTopicMode(index));
}

PayloadStatus mqttDisconnectionStatus(size_t index) {
    return getSetting({"relayMqttDisc", index}, build::mqttDisconnectionStatus(index));
}
#endif

} // namespace settings
} // namespace
} // namespace relay

// -----------------------------------------------------------------------------
// RELAY CONTROL
// -----------------------------------------------------------------------------

// No-op provider, available for purely virtual relays that are controlled only via API

struct DummyProvider : public RelayProviderBase {
    const char* id() const override {
        return "dummy";
    }

    void change(bool) override {
    }

    static RelayProviderBase* sharedInstance() {
        static DummyProvider provider;
        return &provider;
    }
};

class Relay {
public:
    using TimeSource = espurna::time::CoreClock;
    using Delay = espurna::duration::Milliseconds;
    using TimePoint = TimeSource::time_point;

    // Struct defaults to empty relay configuration, as we allow switches to exist without real GPIOs
    Relay() = default;

    explicit Relay(RelayProviderBasePtr&& ptr) :
        provider(ptr.release())
    {}

    explicit Relay(RelayProviderBase* ptr) :
        provider(ptr)
    {}

    // ON / OFF actions implementation
    RelayProviderBase* provider { DummyProvider::sharedInstance() };

    // Timers
    Delay delay_on { 0ul };                 // Delay to turn relay ON
    Delay delay_off { 0ul };                // Delay to turn relay OFF

    RelayPulse pulse { RelayPulse::None };  // Sets up a timer for the opposite mode
    Delay pulse_time { 0ul };               // Pulse length in millis
    Ticker* pulseTicker { nullptr };        // Holds the pulse back timer

    TimePoint fw_start{};                   // Flood window start time
    unsigned char fw_count { 0u };          // Number of changes within the current flood window

    TimePoint change_start{};               // Time when relay was scheduled to change
    Delay change_delay { 0ul };             // Delay until the next change

    // Status
    bool current_status { false };          // Holds the current (physical) status of the relay
    bool target_status { false };           // Holds the target status
    RelayLock lock { RelayLock::None };     // Holds the value of target status that persists and cannot be changed from.

    // MQTT
    bool report { false };                  // Whether to report to own topic
    bool group_report { false };            // Whether to report to group topic
};

namespace {

using Relays = std::vector<Relay>;
Relays _relays;
size_t _relayDummy { 0ul };

espurna::duration::Milliseconds _relay_flood_window { relay::build::floodWindowMs() };
unsigned long _relay_flood_changes { relay::build::floodChanges() };

espurna::duration::Milliseconds _relay_delay_interlock;
int _relay_sync_mode { RELAY_SYNC_ANY };
bool _relay_sync_reent { false };
bool _relay_sync_locked { false };

Ticker _relay_save_timer;
Ticker _relay_sync_timer;

std::forward_list<RelayStatusCallback> _relay_status_notify;
std::forward_list<RelayStatusCallback> _relay_status_change;

#if WEB_SUPPORT

bool _relay_report_ws = false;

void _relayWsReport() {
    _relay_report_ws = true;
}

#endif // WEB_SUPPORT

#if MQTT_SUPPORT || API_SUPPORT

String _relay_payload_on;
String _relay_payload_off;
String _relay_payload_toggle;

#endif // MQTT_SUPPORT || API_SUPPORT

} // namespace

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

void relayOnStatusNotify(RelayStatusCallback callback) {
    _relay_status_notify.push_front(callback);
}

void relayOnStatusChange(RelayStatusCallback callback) {
    _relay_status_change.push_front(callback);
}

// Real GPIO provider, using BasePin interface to implement writers

namespace {

struct GpioProvider : public RelayProviderBase {
    GpioProvider(RelayType type, std::unique_ptr<BasePin>&& pin, std::unique_ptr<BasePin>&& reset_pin) :
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

            // notice that this stalls loop() execution, since
            // we need to ensure only relay task is active
            espurna::time::blockingDelay(relay::build::latchingPulse());

            _pin->digitalWrite(!pulse);
            if (_reset_pin) {
                _reset_pin->digitalWrite(!pulse);
            }
        }
        }
    }

private:
    RelayType _type { RelayType::Normal };
    std::unique_ptr<BasePin> _pin;
    std::unique_ptr<BasePin> _reset_pin;
};

// Special provider for Sonoff Dual, using serial protocol

#if RELAY_PROVIDER_DUAL_SUPPORT

class DualProvider : public RelayProviderBase {
public:
    DualProvider() = delete;
    explicit DualProvider(size_t id) : _id(id) {
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

    size_t relayId() const {
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
        for (size_t index = 0; index < _instances.size(); ++index) {
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
        for (size_t index = 0; index < _instances.size(); ++index) {
            relayStatus(_instances[index]->relayId(), mask[index]);
        }
    }

private:
    size_t _id;

    static std::vector<DualProvider*> _instances;
};

std::vector<DualProvider*> DualProvider::_instances;

#endif // RELAY_PROVIDER_DUAL_SUPPORT

// Special provider for ESP01-relays with STM co-MCU driving the relays

#if RELAY_PROVIDER_STM_SUPPORT

class StmProvider : public RelayProviderBase {
public:
    StmProvider() = delete;
    explicit StmProvider(size_t id) :
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
        // XXX: does this actually help with anything? remains as part of the
        // original implementation, quoting "because of broken stm relay firmware"
        _relays[_id].change_delay = espurna::duration::Seconds(3) + espurna::duration::Seconds(1) * _id;
    }

    void change(bool status) override {
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
    size_t _id;
};

#endif // RELAY_PROVIDER_STM_SUPPORT

// -----------------------------------------------------------------------------
// UTILITY
// -----------------------------------------------------------------------------

bool _relayTryParseId(const char* p, size_t& id) {
    return tryParseId(p, relayCount, id);
}

[[gnu::unused]]
bool _relayTryParseIdFromPath(const String& endpoint, size_t& id) {
    int next_slash { endpoint.lastIndexOf('/') };
    if (next_slash < 0) {
        return false;
    }

    const char* p { endpoint.c_str() + next_slash + 1 };
    if (*p == '\0') {
        DEBUG_MSG_P(PSTR("[RELAY] relayID was not specified\n"));
        return false;
    }

    return _relayTryParseId(p, id);
}

void _relayHandleStatus(size_t id, PayloadStatus status) {
    switch (status) {
    case PayloadStatus::Off:
        relayStatus(id, false);
        break;
    case PayloadStatus::On:
        relayStatus(id, true);
        break;
    case PayloadStatus::Toggle:
        relayToggle(id);
        break;
    case PayloadStatus::Unknown:
        break;
    }
}

bool _relayHandlePayload(size_t id, const char* payload) {
    auto status = relayParsePayload(payload);
    if (status != PayloadStatus::Unknown) {
        _relayHandleStatus(id, status);
        return true;
    }

    DEBUG_MSG_P(PSTR("[RELAY] Invalid API payload (%s)\n"), payload);
    return false;
}

[[gnu::unused]]
bool _relayHandlePayload(size_t id, const String& payload) {
    return _relayHandlePayload(id, payload.c_str());
}

[[gnu::unused]]
bool _relayHandlePulsePayload(size_t id, const char* payload) {
    const auto pulse = relay::PulseDuration { atof(payload) };

    if (std::isfinite(pulse.count()) && (pulse.count() > 0.0f)) {
        if (RelayPulse::None != _relays[id].pulse) {
            DEBUG_MSG_P(PSTR("[RELAY] Overriding relayID %u pulse settings\n"), id);
        }

        _relays[id].pulse_time = std::chrono::duration_cast<Relay::Delay>(pulse);
        _relays[id].pulse = relayStatus(id)
            ? RelayPulse::On
            : RelayPulse::Off;
        relayToggle(id, true, false);

        return true;
    }

    return false;
}

[[gnu::unused]]
bool _relayHandlePulsePayload(size_t id, const String& payload) {
    return _relayHandlePulsePayload(id, payload.c_str());
}

[[gnu::unused]]
PayloadStatus _relayInvertStatus(PayloadStatus status) {
    switch (status) {
    case PayloadStatus::On:
        return PayloadStatus::Off;
    case PayloadStatus::Off:
        return PayloadStatus::On;
    case PayloadStatus::Toggle:
    case PayloadStatus::Unknown:
        break;
    }

    return PayloadStatus::Unknown;
}

[[gnu::unused]]
PayloadStatus _relayPayloadStatus(size_t id) {
    if (id < _relays.size()) {
        return _relays[id].current_status
            ? PayloadStatus::On
            : PayloadStatus::Off;
    }

    return PayloadStatus::Unknown;
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

bool _relayStatusLock(size_t id, bool status) {
    if (_relays[id].lock != RelayLock::None) {
        bool lock = _relays[id].lock == RelayLock::On;
        if ((lock != status) || (lock != _relays[id].target_status)) {
            _relays[id].target_status = lock;
            _relays[id].change_delay = Relay::Delay::zero();
            return false;
        }
    }

    return true;
}

// https://github.com/xoseperez/espurna/issues/1510#issuecomment-461894516
// completely reset timing on the other relay to sync with this one
// to ensure that they change state sequentially
void _relaySyncRelaysDelay(size_t first, size_t second) {
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
            _relayWsReport();
        #endif
    };

    if (all_off) {
        _relay_sync_timer.once_ms(_relay_delay_interlock.count(), action);
    } else {
        action();
    }
}

} // namespace

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

} // namespace

// Pulse timers (timer after ON or OFF event)
// TODO: integrate with scheduled ON or OFF

void relayPulse(size_t id) {

    auto& relay = _relays[id];
    if (!relay.pulseTicker) {
        relay.pulseTicker = new Ticker();
    }

    relay.pulseTicker->detach();
    auto mode = relay.pulse;
    if (mode == RelayPulse::None) {
        return;
    }

    auto ms = relay.pulse_time;
    if (!std::isfinite(ms.count() || (ms.count() <= 0.0f))) {
        return;
    }

    // TODO: drive ticker on a lower 'tick rate', allow delays longer than 114 minutes
    //       we don't necessarily need millisecond precision. which is also not achievable, most likely,
    //       because of the SDK scheduler. or, at least not for every available provider.

    // limit is per https://www.espressif.com/sites/default/files/documentation/2c-esp8266_non_os_sdk_api_reference_en.pdf
    // > 3.1.1 os_timer_arm
    // > with `system_timer_reinit()`, the timer value allowed ranges from 100 to 0x0x689D0.
    // > otherwise, the timer value allowed ranges from 5 to 0x68D7A3.
    static constexpr espurna::duration::Milliseconds PulseDurationMin { 5 };
    static constexpr espurna::duration::Milliseconds PulseDurationMax { 0x68D7A3 };
    if ((ms < PulseDurationMin) || (ms >= PulseDurationMax)) {
        DEBUG_MSG_P(PSTR("[RELAY] Unable to schedule the delay %lu (ms) (longer than 114 minutes)\n"), ms.count());
        return;
    }

    // Notice that we reconfigure after dynamic pulse, next relay toggle will use the configuration values
    if ((mode == RelayPulse::On) != relay.current_status) {
        relay.pulseTicker->once_ms(ms.count(), relayToggle, id);
        relay.pulse = relay::settings::pulseMode(id);
        relay.pulse_time = relay::settings::pulseTimeMs(id);
        DEBUG_MSG_P(PSTR("[RELAY] Scheduling relay #%u pulse in %lu(ms)\n"), id, ms.count());
    }

}

// General relay status control

bool relayStatus(size_t id, bool status, bool report, bool group_report) {

    if ((id >= RelaysMax) || (id >= _relays.size())) {
        return false;
    }

    if (!_relayStatusLock(id, status)) {
        DEBUG_MSG_P(PSTR("[RELAY] #%u is locked to %s\n"), id, _relays[id].current_status ? "ON" : "OFF");
        _relays[id].report = true;
        _relays[id].group_report = true;
        return false;
    }

    bool changed = false;

    if (_relays[id].current_status == status) {

        if (_relays[id].target_status != status) {
            DEBUG_MSG_P(PSTR("[RELAY] #%u scheduled change cancelled\n"), id);
            _relays[id].target_status = status;
            _relays[id].report = false;
            _relays[id].group_report = false;
            _relays[id].change_delay = Relay::Delay::zero();
            changed = true;
        }

        _relays[id].provider->notify(status);
        for (auto& notify : _relay_status_notify) {
            notify(id, status);
        }

        // Update the pulse counter if the relay is already in the non-normal state (#454)
        relayPulse(id);

    } else {

        auto current_time = Relay::TimeSource::now();

        auto change_delay = status
            ? _relays[id].delay_on
            : _relays[id].delay_off;

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

        DEBUG_MSG_P(PSTR("[RELAY] #%u scheduled %s in %u (ms)\n"),
            id, status ? "ON" : "OFF", _relays[id].change_delay.count());

        changed = true;

    }

    return changed;

}

bool relayStatus(size_t id, bool status) {
#if MQTT_SUPPORT
    return relayStatus(id, status, mqttForward(), true);
#else
    return relayStatus(id, status, false, true);
#endif
}

bool relayStatus(size_t id) {

    // Check that relay ID is valid
    if (id >= _relays.size()) return false;

    // Get status directly from storage
    return _relays[id].current_status;

}

bool relayStatusTarget(size_t id) {
    if (id >= _relays.size()) return false;
    return _relays[id].target_status;
}

void relaySync(size_t target) {
    // No sync if none or only one relay
    auto relays = _relays.size();
    if (relays < 2) {
        return;
    }

    // Only call once when coming from the relayStatus(id, status)
    if (_relay_sync_reent) {
        return;
    }
    _relay_sync_reent = true;

    bool status = _relays[target].target_status;

    // If RELAY_SYNC_SAME all relays should have the same state
    if (_relay_sync_mode == RELAY_SYNC_SAME) {
        for (decltype(relays) id = 0; id < relays; ++id) {
            if (id != target) {
                relayStatus(id, status);
            }
        }

    // If RELAY_SYNC_FIRST all relays should have the same state as first if first changes
    } else if (_relay_sync_mode == RELAY_SYNC_FIRST) {
        if (target == 0) {
            for (decltype(relays) id = 1; id < relays; ++id) {
                relayStatus(id, status);
            }
        }

    } else if ((_relay_sync_mode == RELAY_SYNC_NONE_OR_ONE) || (_relay_sync_mode == RELAY_SYNC_ONE)) {
        // If NONE_OR_ONE or ONE and setting ON we should set OFF all the others
        if (status) {
            if (_relay_sync_mode != RELAY_SYNC_ANY) {
                for (decltype(relays) id = 0; id < relays; ++id) {
                    if (id != target) {
                        relayStatus(id, false);
                        if (relayStatus(id)) {
                            _relaySyncRelaysDelay(id, target);
                        }
                    }
                }
            }
        // If ONLY_ONE and setting OFF we should set ON the other one
        } else {
            if (_relay_sync_mode == RELAY_SYNC_ONE) {
                auto id = (target + 1) % relays;
                _relaySyncRelaysDelay(target, id);
                relayStatus(id, true);
            }
        }
        _relayLockAll();
    }

    _relay_sync_reent = false;
}

namespace {

RelayMaskHelper _relayMaskCurrent() {
    RelayMaskHelper mask;
    for (size_t id = 0; id < _relays.size(); ++id) {
        mask.set(id, _relays[id].current_status);
    }
    return mask;
}

} // namespace

void relaySave(bool persist) {
    // Persist only to rtcmem, unless requested to save to settings
    auto mask = _relayMaskCurrent();
    DEBUG_MSG_P(PSTR("[RELAY] Relay mask: %s\n"), mask.toString().c_str());
    _relayMaskRtcmem(mask);

    // The 'persist' flag controls whether we are commiting this change or not.
    // It is useful to set it to 'false' if the relay change triggering the
    // save involves a relay whose boot mode is independent from current mode,
    // thus storing the last relay value is not absolutely necessary.
    // Nevertheless, we store the value in the EEPROM buffer so it will be written
    // on the next commit.
    if (persist) {
        relay::settings::bootMask(mask);
        eepromCommit(); // TODO: should this respect settings auto-save?
    }
}

void relaySave() {
    relaySave(false);
}

void relayToggle(size_t id, bool report, bool group_report) {
    if (id < _relays.size()) {
        relayStatus(id, !relayStatus(id), report, group_report);
    }
}

void relayToggle(size_t id) {
#if MQTT_SUPPORT
    relayToggle(id, mqttForward(), true);
#else
    relayToggle(id, false, true);
#endif
}

size_t relayCount() {
    return _relays.size();
}

PayloadStatus relayParsePayload(const char * payload) {
#if MQTT_SUPPORT || API_SUPPORT
    return rpcParsePayload(payload, [](const char* payload) {
        if (_relay_payload_off.equals(payload)) {
            return PayloadStatus::Off;
        } else if (_relay_payload_on.equals(payload)) {
            return PayloadStatus::On;
        } else if (_relay_payload_toggle.equals(payload)) {
            return PayloadStatus::Toggle;
        }

        return PayloadStatus::Unknown;
    });
#else
    return rpcParsePayload(payload);
#endif
}

namespace {

void _relaySettingsMigrate(int version) {
    if (version < 5) {
        // just a rename
        moveSetting("relayDelayInterlock", "relayIlkDelay");

        // groups use a new set of keys
        for (size_t index = 0; index < RelaysMax; ++index) {
            auto group = getSetting({"mqttGroup", index});
            if (!group.length()) {
                break;
            }

            auto syncKey = SettingsKey("mqttGroupSync", index);
            auto sync = getSetting(syncKey);

            setSetting({"relayTopicSub", index}, group);
            if (sync.length()) {
                if (sync != "2") { // aka RECEIVE_ONLY
                    setSetting("relayTopicMode", sync);
                    setSetting("relayTopicPub", group);
                }
            }
        }

        delSettingPrefix({
            "mqttGroup",     // migrated to relayTopic
            "mqttGroupSync", // migrated to relayTopic
            "relayOnDisc",   // replaced with relayMqttDisc
            "relayGPIO",     // avoid depending on migrate.ino
            "relayGpio",     //
            "relayProvider", // different type
            "relayType",     // different type
        });
        delSetting("relays"); // does not do anything
    }
}

void _relayBoot(size_t index, const RelayMaskHelper& mask) {
    auto status = false;
    auto lock = RelayLock::None;

    switch (relay::settings::bootMode(index)) {
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

    relay.change_start = Relay::TimeSource::now();
    relay.change_delay = status
        ? relay.delay_on
        : relay.delay_off;

    relay.provider->boot(status);
}

void _relayBootAll() {
    auto mask = rtcmemStatus()
        ? _relayMaskRtcmem()
        : relay::settings::bootMask();

    bool log { false };

    static RelayMask done;
    const auto relays = _relays.size();
    for (size_t id = 0; id < relays; ++id) {
        if (!done[id]) {
            done.set(id, true);
            _relayBoot(id, mask);
            log = true;
        }
    }

    if (log) {
        DEBUG_MSG_P(PSTR("[RELAY] Number of relays: %u, boot mask: %s\n"),
            relays, mask.toString().c_str());
    }
}

void _relayConfigure() {
    auto relays = _relays.size();
    for (decltype(relays) id = 0; id < relays; ++id) {
        _relays[id].pulse = relay::settings::pulseMode(id);
        _relays[id].pulse_time = relay::settings::pulseTimeMs(id);

        _relays[id].delay_on = relay::settings::delayOn(id);
        _relays[id].delay_off = relay::settings::delayOff(id);
    }

    _relay_flood_window = relay::settings::floodWindowMs();
    _relay_flood_changes = relay::settings::floodChanges();

    _relay_delay_interlock = relay::settings::interlockDelay();
    _relay_sync_mode = relay::settings::syncMode();

#if MQTT_SUPPORT || API_SUPPORT
    _relay_payload_on = relay::settings::payloadOn();
    _relay_payload_off = relay::settings::payloadOff();
    _relay_payload_toggle = relay::settings::payloadToggle();
#endif // MQTT_SUPPORT
}

} // namespace

//------------------------------------------------------------------------------
// WEBSOCKETS
//------------------------------------------------------------------------------

#if WEB_SUPPORT

namespace {

bool _relayWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "relay", 5) == 0);
}

void _relayWebSocketUpdate(JsonObject& root) {
    ::web::ws::EnumerableConfig config{root, F("relayState")};

    config(F("states"), _relays.size(), {
        {F("status"), [](JsonArray& out, size_t index) {
            out.add(_relays[index].target_status ? 1 : 0);
        }},
        {F("lock"), [](JsonArray& out, size_t index) {
            out.add(static_cast<uint8_t>(_relays[index].lock));
        }},
    });
}

void _relayWebSocketSendRelays(JsonObject& root) {
    if (!_relays.size()) {
        return;
    }

    ::web::ws::EnumerableConfig config{root, F("relayConfig")};

    auto& container = config.root();
    container["size"] = _relays.size();
    container["start"] = 0;

    config(F("relays"), _relays.size(), {
        {F("relayDesc"), [](JsonArray& out, size_t index) {
            out.add(_relays[index].provider->id());
        }},
        {F("relayProv"), [](JsonArray& out, size_t index) {
            out.add(static_cast<uint8_t>(relay::settings::provider(index)));
        }},
        {F("relayName"), [](JsonArray& out, size_t index) {
            out.add(relay::settings::name(index));
        }},
        {F("relayBoot"), [](JsonArray& out, size_t index) {
            out.add(relay::settings::bootMode(index));
        }},
#if MQTT_SUPPORT
        {F("relayTopicPub"), [](JsonArray& out, size_t index) {
            out.add(relay::settings::mqttTopicSub(index));
        }},
        {F("relayTopicSub"), [](JsonArray& out, size_t index) {
            out.add(relay::settings::mqttTopicPub(index));
        }},
        {F("relayTopicMode"), [](JsonArray& out, size_t index) {
            out.add(static_cast<uint8_t>(relay::settings::mqttTopicMode(index)));
        }},
        {F("relayMqttDisc"), [](JsonArray& out, size_t index) {
            out.add(static_cast<uint8_t>(relay::settings::mqttDisconnectionStatus(index)));
        }},
#endif
        {F("relayPulse"), [](JsonArray& out, size_t index) {
            out.add(static_cast<uint8_t>(_relays[index].pulse));
        }},
        {F("relayTime"), [](JsonArray& out, size_t index) {
            out.add(std::chrono::duration_cast<relay::PulseDuration>(
                _relays[index].pulse_time).count());
        }},
    });
}

void _relayWebSocketOnVisible(JsonObject& root) {
    const auto relays = _relays.size();
    if (!relays) {
        return;
    }

    if (relays > 1) {
        wsPayloadModule(root, "multirelay");
        root["relaySync"] = relay::settings::syncMode();
        root["relayIlkDelay"] = relay::settings::interlockDelay().count();
    }

    wsPayloadModule(root, "relay");
}

void _relayWebSocketOnConnected(JsonObject& root) {
    _relayWebSocketSendRelays(root);
}

void _relayWebSocketOnAction(uint32_t client_id, const char* action, JsonObject& data) {
    if (strcmp(action, "relay") == 0) {
        if (!data.is<size_t>("id") || !data.is<String>("status")) {
            return;
        }

        _relayHandlePayload(data["id"].as<size_t>(), data["status"].as<String>().c_str());
    }
}

} // namespace

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

namespace {

template <typename T>
bool _relayApiTryHandle(ApiRequest& request, T&& callback) {
    auto id_param = request.wildcard(0);
    size_t id;
    if (!_relayTryParseId(id_param.c_str(), id)) {
        return false;
    }

    return callback(id);
}

} // namespace

void relaySetupAPI() {

    if (!_relays.size()) {
        return;
    }

    apiRegister(F(MQTT_TOPIC_RELAY),
        [](ApiRequest&, JsonObject& root) {
            JsonArray& out = root.createNestedArray("relayStatus");
            for (auto& relay : _relays) {
                out.add(relay.target_status ? 1 : 0);
            }
            return true;
        },
        nullptr
    );

    apiRegister(F(MQTT_TOPIC_RELAY "/+"),
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](size_t id) {
                request.send(String(_relays[id].target_status ? 1 : 0));
                return true;
            });
        },
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](size_t id) {
                return _relayHandlePayload(id, request.param(F("value")));
            });
        }
    );

    apiRegister(F(MQTT_TOPIC_PULSE "/+"),
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](size_t id) {
                request.send(String(
                    std::chrono::duration_cast<relay::PulseDuration>(
                        _relays[id].pulse_time).count()));
                return true;
            });
        },
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](size_t id) {
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
    return _relay_payload_on;
}

const String& relayPayloadOff() {
    return _relay_payload_off;
}

const String& relayPayloadToggle() {
    return _relay_payload_toggle;
}

const char* relayPayload(PayloadStatus status) {
    switch (status) {
    case PayloadStatus::Off:
        return _relay_payload_off.c_str();
    case PayloadStatus::On:
        return _relay_payload_on.c_str();
    case PayloadStatus::Toggle:
        return _relay_payload_toggle.c_str();
    case PayloadStatus::Unknown:
        break;
    }

    return "";
}

#endif // MQTT_SUPPORT || API_SUPPORT

#if MQTT_SUPPORT

namespace {

// TODO: it *will* handle the duplicates, but we waste memory storing them
// TODO: mqttSubscribe(...) also happens multiple times
//
// this is not really an intended use-case though, but it is techically possible...

struct RelayCustomTopicBase {
    RelayCustomTopicBase() = delete;
    RelayCustomTopicBase(const RelayCustomTopicBase&) = delete;
    RelayCustomTopicBase(RelayCustomTopicBase&& other) noexcept :
        _value(std::move(other._value)),
        _mode(other._mode)
    {}

    template <typename T>
    RelayCustomTopicBase(T&& value, RelayMqttTopicMode mode) :
        _value(std::forward<T>(value)),
        _mode(mode)
    {}

    RelayCustomTopicBase& operator=(const char* const value) {
        _value = value;
        return *this;
    }

    RelayCustomTopicBase& operator=(const String& value) {
        _value = value;
        return *this;
    }

    RelayCustomTopicBase& operator=(String&& value) noexcept {
        _value = std::move(value);
        return *this;
    }

    RelayCustomTopicBase& operator=(RelayMqttTopicMode mode) noexcept {
        _mode = mode;
        return *this;
    }

    String&& get() && {
        return std::move(_value);
    }

    const String& value() const {
        return _value;
    }

    RelayMqttTopicMode mode() const {
        return _mode;
    }

private:
    String _value;
    RelayMqttTopicMode _mode;
};

struct RelayCustomTopic {
    RelayCustomTopic() = delete;
    RelayCustomTopic(const RelayCustomTopic&) = delete;
    RelayCustomTopic(RelayCustomTopic&&) = delete;

    RelayCustomTopic(size_t id, RelayCustomTopicBase&& base) :
        _id(id),
        _topic(std::move(base).get()),
        _parts(_topic),
        _mode(base.mode())
    {}

    size_t id() const {
        return _id;
    }

    const char* const c_str() const {
        return _topic.c_str();
    }

    const String& topic() const {
        return _topic;
    }

    const PathParts& parts() const {
        return _parts;
    }

    const RelayMqttTopicMode mode() const {
        return _mode;
    }

    bool match(const String& other) const {
        PathParts parts(other);
        return _parts.match(parts);
    }

    bool match(const PathParts& parts) const {
        return _parts.match(parts);
    }

private:
    size_t _id;
    String _topic;
    PathParts _parts;
    RelayMqttTopicMode _mode;
};

std::forward_list<RelayCustomTopic> _relay_custom_topics;

void _relayMqttSubscribeCustomTopics() {
    const size_t relays { _relays.size() };
    if (!relays) {
        return;
    }

    static std::vector<RelayCustomTopicBase> topics;
    for (size_t id = 0; id < relays; ++id) {
        topics.emplace_back(relay::build::mqttTopicSub(id), relay::build::mqttTopicMode(id));
    }

    settings::internal::foreach([&](settings::kvs_type::KeyValueResult&& kv) {
        const char* const SubPrefix = "relayTopicSub";
        const char* const ModePrefix = "relayTopicMode";

        if ((kv.key.length <= strlen(SubPrefix))
                && (kv.key.length <= strlen(ModePrefix))) {
            return;
        }

        if (!kv.value.length) {
            return;
        }

        const auto key = kv.key.read();
        size_t id;

        if (key.startsWith(SubPrefix)) {
            if (_relayTryParseId(key.c_str() + strlen(SubPrefix), id)) {
                topics[id] = kv.value.read();
            }
        } else if (key.startsWith(ModePrefix)) {
            if (_relayTryParseId(key.c_str() + strlen(ModePrefix), id)) {
                topics[id] = settings::internal::convert<RelayMqttTopicMode>(kv.value.read());
            }
        }
    });

    _relay_custom_topics.clear();
    for (size_t id = 0; id < relays; ++id) {
        RelayCustomTopicBase& topic = topics[id];
        auto& value = topic.value();
        if (!value.length()) {
            continue;
        }

        mqttSubscribeRaw(value.c_str());
        _relay_custom_topics.emplace_front(id, std::move(topic));
    }

    topics.clear();
}

void _relayMqttPublishCustomTopic(size_t id) {
    const String topic = relay::settings::mqttTopicPub(id);
    if (!topic.length()) {
        return;
    }

    auto status = _relayPayloadStatus(id);

    auto mode = relay::settings::mqttTopicMode(id);
    if (mode == RelayMqttTopicMode::Inverse) {
        status = _relayInvertStatus(status);
    }

    mqttSendRaw(topic.c_str(), relayPayload(status));
}

void _relayMqttReport(size_t id) {
    if (_relays[id].report) {
        _relays[id].report = false;
        mqttSend(MQTT_TOPIC_RELAY, id, relayPayload(_relayPayloadStatus(id)));
    }

    if (_relays[id].group_report) {
        _relays[id].group_report = false;
        _relayMqttPublishCustomTopic(id);
    }
}

void _relayMqttReportAll() {
    for (unsigned int id=0; id < _relays.size(); id++) {
        mqttSend(MQTT_TOPIC_RELAY, id, relayPayload(_relayPayloadStatus(id)));
    }
}

} // namespace

void relayStatusWrap(size_t id, PayloadStatus value, bool is_group_topic) {
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
            _relayMqttReport(id);
            break;
    }
}

namespace {

bool _relayMqttHeartbeat(espurna::heartbeat::Mask mask) {
    if (mask & espurna::heartbeat::Report::Relay)
        _relayMqttReportAll();

    return mqttConnected();
}

void _relayMqttHandleCustomTopic(const String& topic, const char* payload) {
    PathParts received(topic);
    for (auto& topic : _relay_custom_topics) {
        if (topic.match(received)) {
            auto status = relayParsePayload(payload);
            if (topic.mode() == RelayMqttTopicMode::Inverse) {
                status = _relayInvertStatus(status);
            }

            const auto id = topic.id();
            _relayHandleStatus(id, status);
            _relays[id].group_report = false;
        }
    }
}

void _relayMqttHandleDisconnect() {
    settings::internal::foreach([](settings::kvs_type::KeyValueResult&& kv) {
        const char* const prefix = "relayMqttDisc";
        if (kv.key.length <= strlen(prefix)) {
            return;
        }

        const auto key = kv.key.read();
        if (key.startsWith(prefix)) {
            size_t id;
            if (_relayTryParseId(key.c_str() + strlen(prefix), id)) {
                const auto value = kv.value.read();
                _relayHandleStatus(id, relayParsePayload(value.c_str()));
            }
        }
    });
}

} // namespace

void relayMQTTCallback(unsigned int type, const char* topic, char* payload) {

    static bool connected { false };

    if (!_relays.size()) {
        return;
    }

    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_RELAY "/+");
        mqttSubscribe(MQTT_TOPIC_PULSE "/+");
        _relayMqttSubscribeCustomTopics();
        connected = true;
        return;
    }

    if (type == MQTT_MESSAGE_EVENT) {
        String t = mqttMagnitude(topic);

        auto is_relay = t.startsWith(MQTT_TOPIC_RELAY);
        auto is_pulse = t.startsWith(MQTT_TOPIC_PULSE);
        if (is_relay || is_pulse) {
            size_t id;
            if (!_relayTryParseIdFromPath(t.c_str(), id)) {
                return;
            }

            if (is_relay) {
                _relayHandlePayload(id, payload);
                _relays[id].report = mqttForward();
                return;
            }

            if (is_pulse) {
                _relayHandlePulsePayload(id, payload);
                _relays[id].report = mqttForward();
                return;
            }
        }

        _relayMqttHandleCustomTopic(topic, payload);
        return;
    }

    if (type == MQTT_DISCONNECT_EVENT) {
        if (connected) {
            connected = false;
            _relayMqttHandleDisconnect();
        }
        return;
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

namespace {

void _relayInitCommands() {

    terminalRegisterCommand(F("RELAY"), [](::terminal::CommandContext&& ctx) {
        auto showRelays = [&](size_t start, size_t stop, bool full = true) {
            for (size_t index = start; index < stop; ++index) {
                auto& relay = _relays[index];

                char pulse_info[64] = "";
                if ((relay.pulse != RelayPulse::None) && (relay.pulse_time.count() > 0)) {
                    snprintf_P(pulse_info, sizeof(pulse_info), PSTR(" Pulse=%s Time=%u(ms)"),
                        _relayPulseToPayload(relay.pulse), relay.pulse_time);
                }

                char extended_info[64] = "";
                if (full) {
                    int index = 0;
                    if (index >= 0 && relay.delay_on.count()) {
                        index += snprintf_P(extended_info + index, sizeof(extended_info),
                                PSTR(" DelayOn=%u(ms)"), relay.delay_on.count());
                    }
                    if (index >= 0 && relay.delay_off.count()) {
                        index += snprintf_P(extended_info + index, sizeof(extended_info),
                                PSTR(" DelayOff=%u(ms)"), relay.delay_off.count());
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

        if (ctx.argv.size() == 1) {
            showRelays(0, _relays.size());
            terminalOK(ctx);
            return;
        }

        size_t id;
        if (!_relayTryParseId(ctx.argv[1].c_str(), id)) {
            terminalError(ctx, F("Invalid relayID"));
            return;
        }

        if (ctx.argv.size() > 2) {
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

} // namespace

#endif // TERMINAL_SUPPORT

//------------------------------------------------------------------------------

namespace {

void _relayReport(size_t id [[gnu::unused]], bool status [[gnu::unused]]) {
    for (auto& change : _relay_status_change) {
        change(id, status);
    }
#if MQTT_SUPPORT
    _relayMqttReport(id);
#endif
#if WEB_SUPPORT
    _relayWsReport();
#endif
}

/**
 * Walks the relay vector processing only those relays
 * that have to change to the requested mode
 * @bool mode Requested mode
 */
void _relayProcess(bool mode) {

    bool changed = false;

    auto relays = _relays.size();
    for (decltype(relays) id = 0; id < relays; ++id) {
        bool target = _relays[id].target_status;

        // Only process the relays:
        // - target mode in the one requested by the arg
        // - target status is different from the current one
        // - change delay has expired

        if ((target != _relays[id].current_status)
            && (target == mode)
            && ((!_relays[id].change_delay.count())
                || (Relay::TimeSource::now() - _relays[id].change_start > _relays[id].change_delay)))
        {
            // delay will be reset back to the correct value via relayStatus
            _relays[id].change_delay = Relay::Delay::zero();
            _relays[id].current_status = target;
            _relays[id].provider->change(target);

            _relayReport(id, target);
            relayPulse(id);

            {
                const auto boot_mode = relay::settings::bootMode(id);
                _relay_save_timer.once_ms(
                    relay::build::saveDelay().count(), relaySave,
                    (RELAY_BOOT_SAME == boot_mode) || (RELAY_BOOT_TOGGLE == boot_mode));
            }

            DEBUG_MSG_P(PSTR("[RELAY] #%u set to %s\n"), id, target ? "ON" : "OFF");

            changed = true;
        }
    }

    // Whenever we are using sync modes and any relay had changed the state, check if we can unlock
    switch (_relay_sync_mode) {
    case RELAY_SYNC_ONE:
    case RELAY_SYNC_NONE_OR_ONE:
        if (_relay_sync_locked && changed) {
            _relaySyncUnlock();
        }
        break;
    case RELAY_SYNC_ANY:
    case RELAY_SYNC_SAME:
    case RELAY_SYNC_FIRST:
        break;
    }
}

} // namespace

//------------------------------------------------------------------------------
// Setup
//------------------------------------------------------------------------------

namespace {

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

} // namespace

// Dummy relays for virtual light switches (hardware-less), Sonoff Dual, Sonoff RF Bridge and Tuya

void relaySetupDummy(size_t size, bool reconfigure) {
    if (size == _relayDummy) {
        return;
    }

    const size_t new_size = ((_relays.size() - _relayDummy) + size);
    if (new_size > RelaysMax) {
        return;
    }

    _relayDummy = size;
    _relays.resize(new_size);

    if (reconfigure) {
        _relayConfigure();
    }
}

namespace {

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
    uint8_t main;
    uint8_t reset;
};

RelayGpioProviderCfg _relayGpioProviderCfg(size_t index) {
    return {
        gpioBase(relay::settings::pinType(index)),
        relay::settings::pin(index),
        relay::settings::resetPin(index)};
}

std::unique_ptr<GpioProvider> _relayGpioProvider(size_t index, RelayType type) {
    auto cfg = _relayGpioProviderCfg(index);
    if (!cfg.base) {
        return nullptr;
    }

    auto main = gpioRegister(*cfg.base, cfg.main);
    if (main) {
        auto reset = gpioRegister(*cfg.base, cfg.reset);
        return std::make_unique<GpioProvider>(
            type, std::move(main), std::move(reset));
    }

    return nullptr;
}

RelayProviderBasePtr _relaySetupProvider(size_t index) {
    auto provider = relay::settings::provider(index);
    auto type = relay::settings::type(index);

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
    auto relays = _relays.size();
    _relays.reserve(relays + _relayAdhocPins());

    for (size_t id = relays; id < RelaysMax; ++id) {
        auto impl = _relaySetupProvider(id);
        if (!impl) {
            break;
        }
        if (!impl->setup()) {
            break;
        }
        _relays.emplace_back(std::move(impl));
    }

    relaySetupDummy(relay::settings::dummyCount());
}

} // namespace

void relaySetup() {
    migrateVersion(_relaySettingsMigrate);

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
