/*

ALARM MODULE

Copyright (C) 2019-2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#if ALARM_SUPPORT

#include <functional>
#include <limits>
#include <memory>
#include <stack>
#include <vector>

#include <cstdlib>

#include "broker.h"
#include "relay.h"
#include "terminal.h"

namespace alarms {

// -----------------------------------------------------------------------------
// Basic alarm ON OFF pattern
// -----------------------------------------------------------------------------

struct pattern_data_t {
    unsigned long delay_on;
    unsigned long delay_off;
    unsigned char repeats;
};

class pattern_t {

    enum state_t {
        PATTERN_START,
        PATTERN_ON,
        PATTERN_OFF,
        DONE
    };

    public:

        pattern_t() = delete;
        pattern_t(unsigned long delay_on, unsigned long delay_off, unsigned char repeats) :
            state(PATTERN_START),
            times(repeats),
            delay_for(0),
            last(0),
            status(true),
            delay_on(delay_on),
            delay_off(delay_off),
            repeats(repeats)
        {}
        pattern_t(const pattern_data_t& data) :
            pattern_t(data.delay_on, data.delay_off, data.repeats)
        {}

        bool trigger() {
            if (repeats != 0 && !times) {
                return false;
            }

            bool result = false;
            switch (state) {
                case PATTERN_START:
                    status = true;
                    result = true;
                    state = PATTERN_ON;
                    delay_for = delay_on;
                    break;
                case PATTERN_ON: {
                    if (millis() - last < delay_for) {
                        return false;
                    }
                    status = false;
                    result = true;
                    state = PATTERN_OFF;
                    delay_for = delay_off;
                    break;
                }
                case PATTERN_OFF: {
                    if (millis() - last < delay_for) {
                        return false;
                    }
                    if (repeats != 0) {
                        if (!--times) {
                            state = DONE;
                            return false;
                        }
                    }
                    status = true;
                    result = true;
                    state = PATTERN_ON;
                    delay_for = delay_on;
                    break;
                }
                case DONE:
                default:
                    result = false;
                    break;
            }

            last = millis();

            return result;
        }

        void reset() {
            state = PATTERN_ON;
            times = repeats;
        }

        bool done() const {
            return (state == DONE);
        }

        state_t state;
        unsigned char times;
        unsigned long delay_for;
        unsigned long last;
        bool status;

        const unsigned long delay_on;
        const unsigned long delay_off;
        const unsigned char repeats;

};

// -----------------------------------------------------------------------------
// Alarm helper structure to track pattern triggers
// -----------------------------------------------------------------------------

class alarm_debug_t {

};

class alarm_t {

    friend class alarm_debug_t;

    using data_t = std::vector<pattern_data_t>;
    using patterns_t = std::vector<pattern_t>;
    using callback_t = std::function<void(bool status)>;

    public:

        alarm_t() = delete;
        alarm_t(callback_t callback, data_t& data) :
            callback(callback),
            status(false),
            _triggered(false),
            _data(data)
        {}
        alarm_t(callback_t callback, data_t&& data) :
            callback(callback),
            status(false),
            _triggered(false),
            _data(std::move(data))
        {}
        ~alarm_t() {
            callback(false);
        }

        void arm() {
            for (size_t index = _data.size() - 1; index < _data.size(); --index) {
                _patterns.emplace(_data[index]);
            }
        }

        void disarm() {
            while (!_patterns.empty()) {
                _patterns.pop();
            }
            callback(false);
        }

        bool trigger() {
            if (!armed()) return false;
            auto& pattern = _patterns.top();
            if (pattern.trigger()) {
                callback(pattern.status);
            }
            if (pattern.done()) {
                _patterns.pop();
            }
        }

        bool armed() {
            return !_patterns.empty();
        }

        bool triggered() {
            return _triggered;
        }

        void trigger(bool value) {
            _triggered = value;
        }

        callback_t callback;
        bool status;

    private:

        bool _triggered;
        data_t _data;
        std::stack<pattern_t, patterns_t> _patterns;
};

// -----------------------------------------------------------------------------
// Parse strings such as DELAY_ON,DELAY_OFF,N_REPEATS,...again...
// -----------------------------------------------------------------------------

std::vector<pattern_data_t> _deserializePattern(const char* pattern) {
    std::vector<pattern_data_t> out;

    char buffer[16] = {0};
    const char* comma = nullptr;
    char* err = nullptr;
   
    pattern_data_t data;
    int index = 0;

    while ((comma = strchr(pattern, ','))) {
        const auto diff = comma - pattern;
        if (!diff) break;
        memcpy(buffer, pattern, diff);
        buffer[diff] = '\0';
        if (index == 0) {
            data.delay_on = strtoul(buffer, &err, 10);
            if (err == buffer || err[0] != '\0') {
                break;
            }
            ++index;
        } else if (index == 1) {
            data.delay_off = strtoul(buffer, &err, 10);
            if (err == buffer || err[0] != '\0') {
                break;
            }
            ++index;
        } else if (index == 2) {
            data.repeats = strtoul(buffer, &err, 10);
            if (err == buffer || err[0] != '\0') {
                index = 0;
                break;
            }
            out.push_back(data);
            data.delay_on = 0;
            data.delay_off = 0;
            data.repeats = 0;
            index = 0;
        }
        pattern = comma + 1;
    }
    if (index == 2) {
        data.repeats = strtoul(pattern, nullptr, 10);
        out.push_back(data);
    }

    return out;
}

// -----------------------------------------------------------------------------

std::vector<alarm_t> _alarms;

using alarm_id_t = char;
using switch_to_alarm_id_t = std::vector<alarm_id_t>;
static_assert(
    (SETTINGS_MAX_LIST_COUNT <= std::numeric_limits<alarm_id_t>::max()),
    "ALARM ID maximum value cannot fit into this type!"
);

switch_to_alarm_id_t _arming_switch(RELAYS_MAX, -1);
switch_to_alarm_id_t _trigger_switch(RELAYS_MAX, -1);

// -----------------------------------------------------------------------------

void _digitalWrite(unsigned char pin, bool inverse, bool status) {
    status = inverse ? !status : status;
    digitalWrite(pin, status ? HIGH : LOW);
}

void _configure() {

    _alarms.clear();

    for (size_t index = 0; index < SETTINGS_MAX_LIST_COUNT; ++index) {
        const unsigned char gpio = getSetting("alarmGPIO", index, GPIO_NONE).toInt();
        if (!gpioValid(gpio)) {
            break;
        }

        const auto arm = getSetting("alarmArm", index, -1).toInt();
        const auto trigger = getSetting("alarmTrigger", index, -1).toInt();
        if (arm < 0 || trigger < 0 || arm >= relayCount() || trigger >= relayCount()) {
            break;
        }

        const String pattern = getSetting("alarmPattern", index, ALARM_PATTERN);
        if (!pattern.length()) {
            break;
        }

        auto patterns = _deserializePattern(pattern.c_str());
        if (!patterns.size()) {
            break;
        }

        const bool inverse = getSetting("alarmInv", index, 0).toInt() == 1;

        if (!gpioGetLock(gpio)) {
            break;
        }

        auto callback = [gpio, inverse](bool status) {
            _digitalWrite(gpio, inverse, status);
        };
        pinMode(gpio, OUTPUT);
        callback(false);

        _alarms.emplace_back(
            callback, std::move(patterns)
        );

        _arming_switch[arm] = index;
        _trigger_switch[trigger] = index;
    }

}

void _brokerCallback(const String& topic, unsigned char id, unsigned int status) {
    if (topic != MQTT_TOPIC_RELAY) return;

    auto id_trigger = _trigger_switch[id];
    if (id_trigger >= 0) {
        auto& alarm = _alarms[id_trigger];
        if (status == alarm.triggered()) return;
        if (!alarm.armed()) return;
        _alarms[id_trigger].trigger(status);
        DEBUG_MSG_P(PSTR("[ALARM] Alarm #%u %s\n"), id_trigger, status ? "ON" : "OFF");
    }

    auto id_arming = _arming_switch[id];
    if (id_arming >= 0) {
        auto& alarm = _alarms[id_arming];
        if (status == alarm.armed()) return;
        if (status) {
            _alarms[id_arming].arm();
        } else {
            _alarms[id_arming].disarm();
        }
        DEBUG_MSG_P(PSTR("[ALARM] Alarm #%u %s via Switch #%u\n"),
            id_arming,
            status ? "ARMED" : "DISARMED",
            id
        );
    }
}

void _terminalInfo() {
    const auto alarms = _alarms.size();
    if (alarms) {
        DEBUG_MSG_P(PSTR("[ALARM] %u alarm(s) configured\n"), alarms);
        for (size_t index = 0; index < alarms; ++index) {
            DEBUG_MSG_P(PSTR("[ALARM] %02u: GPIO:%02d ARM:SW#%02d TRIGGER:SW#%02d PATTERN:%s\n"),
                index,
                getSetting("alarmGPIO", index, GPIO_NONE).toInt(),
                getSetting("alarmArm", index, -1).toInt(),
                getSetting("alarmTrigger", index, -1).toInt(),
                getSetting("alarmPattern", index, ALARM_PATTERN).c_str()
            );
        }
    }
}

void _loop() {
    for (auto& alarm : _alarms) {
        if (alarm.armed() && alarm.triggered()) {
            alarm.trigger();
        }
    }
}

} // namespace alarms

void alarmSetup() {
    #if DEBUG_SUPPORT
    #if TERMINAL_SUPPORT
        terminalRegisterCommand(F("ALARM.INFO"), [](Embedis* e) {
            alarms::_terminalInfo();
        });
    #endif
    #endif

    alarms::_configure();

    #if DEBUG_SUPPORT
        alarms::_terminalInfo();
    #endif

    StatusBroker::Register(alarms::_brokerCallback);
    espurnaRegisterLoop(alarms::_loop);
}

#endif // ALARM_SUPPORT
