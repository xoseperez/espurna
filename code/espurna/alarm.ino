/*

ALARM MODULE

Copyright (C) 2019-2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#if ALARM_SUPPORT

#include <stack>
#include <vector>
#include <functional>
#include <memory>

#include <cstdlib>

#include "broker.h"
#include "terminal.h"

// -----------------------------------------------------------------------------
// Basic alarm ON OFF pattern
// -----------------------------------------------------------------------------

struct alarm_pattern_data_t {
    unsigned long delay_on;
    unsigned long delay_off;
    unsigned char repeats;
};

class alarm_pattern_t {

    enum state_t {
        PATTERN_START,
        PATTERN_ON,
        PATTERN_OFF,
        DONE
    };

    public:

        alarm_pattern_t() = delete;
        alarm_pattern_t(unsigned long delay_on, unsigned long delay_off, unsigned char repeats) :
            state(PATTERN_START),
            times(repeats),
            delay_for(0),
            last(0),
            status(true),
            delay_on(delay_on),
            delay_off(delay_off),
            repeats(repeats)
        {}
        alarm_pattern_t(const alarm_pattern_data_t& data) :
            alarm_pattern_t(data.delay_on, data.delay_off, data.repeats)
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

class alarm_t {

    using data_t = std::vector<alarm_pattern_data_t>;
    using patterns_t = std::vector<alarm_pattern_t>;
    using callback_t = std::function<void(bool status)>;

    public:

        alarm_t() = delete;
        alarm_t(callback_t callback, data_t& data) :
            callback(callback),
            status(false),
            data(data)
        {}
        alarm_t(callback_t callback, data_t&& data) :
            callback(callback),
            status(false),
            data(std::move(data))
        {}
        ~alarm_t() {
            callback(false);
        }

        void arm() {
            for (size_t index = data.size() - 1; index < data.size(); --index) {
                patterns.emplace(data[index]);
            }
        }

        void disarm() {
            while (!patterns.empty()) {
                patterns.pop();
            }
            callback(false);
        }

        bool trigger() {
            if (done()) return false;
            auto& pattern = patterns.top();
            if (pattern.trigger()) {
                callback(pattern.status);
            }
            if (pattern.done()) {
                patterns.pop();
            }
        }

        bool done() {
            return patterns.empty();
        }

        callback_t callback;
        bool status;

    private:

        data_t data;
        std::stack<alarm_pattern_t, patterns_t> patterns;
};

// -----------------------------------------------------------------------------
// Parse strings such as DELAY_ON,DELAY_OFF,N_REPEATS,...again...
// -----------------------------------------------------------------------------

std::vector<alarm_pattern_data_t> _alarmDeserialize(const char* pattern) {
    std::vector<alarm_pattern_data_t> out;

    char buffer[16] = {0};
    const char* comma = nullptr;
    char* err = nullptr;
   
    alarm_pattern_data_t data;
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

std::vector<alarm_t> _alarms;

void _alarmDigitalWrite(unsigned char pin, bool inverse, bool status) {
    status = inverse ? !status : status;
    digitalWrite(pin, status ? HIGH : LOW);
}

void _alarmConfigure() {

    _alarms.clear();

    for (size_t index = 0; index < SETTINGS_MAX_LIST_COUNT; ++index) {
        const unsigned char gpio = getSetting("alarmGPIO", index, GPIO_NONE).toInt();
        if (!gpioValid(gpio)) {
            break;
        }

        const String pattern = getSetting("alarmPattern", index, ALARM_PATTERN);
        if (!pattern.length()) {
            break;
        }

        auto patterns = _alarmDeserialize(pattern.c_str());
        if (!patterns.size()) {
            break;
        }

        const bool inverse = getSetting("alarmInv", index, 0).toInt() == 1;

        if (!gpioGetLock(gpio)) {
            break;
        }

        auto callback = [gpio, inverse](bool status) {
            _alarmDigitalWrite(gpio, inverse, status);
        };
        pinMode(gpio, OUTPUT);
        callback(false);

        _alarms.emplace_back(
            callback, std::move(patterns)
        );
    }

}

void _alarmBrokerCallback(const String& topic, unsigned char id, unsigned int status) {
    if (topic != MQTT_TOPIC_RELAY) return;
    if (id >= _alarms.size()) return;

    auto& alarm = _alarms[id];
    if (status) {
        if (alarm.done()) {
            DEBUG_MSG_P(PSTR("[ALARM] Armed #%u\n"), id);
            alarm.arm();
        }
    } else {
        if (!alarm.done()) {
            DEBUG_MSG_P(PSTR("[ALARM] Disarmed #%u\n"), id);
            alarm.disarm();
        }
    }
}

void _alarmLoop() {
    for (auto& alarm : _alarms) {
        if (!alarm.done()) {
            alarm.trigger();
        }
    }
}

void _alarmInfo() {
    const auto alarms = _alarms.size();
    if (alarms) {
        DEBUG_MSG_P(PSTR("[ALARM] %u alarm(s) configured\n"), alarms);
        for (size_t index = 0; index < alarms; ++index) {
            DEBUG_MSG_P(PSTR("[ALARM] %02u: GPIO%02u %s\n"),
                index,
                getSetting("alarmGPIO", index, "").toInt(),
                getSetting("alarmPattern", index, "").c_str()
            );
        }
    }
}

void alarmSetup() {
    #if DEBUG_SUPPORT && TERMINAL_SUPPORT
        terminalRegisterCommand(F("ALARM.INFO"), [](Embedis* e) {
            _alarmInfo();
        });
    #endif

    _alarmConfigure();
    _alarmInfo();

    StatusBroker::Register(_alarmBrokerCallback);
    espurnaRegisterLoop(_alarmLoop);
}

#endif // ALARM_SUPPORT
