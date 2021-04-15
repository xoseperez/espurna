/*

SCHEDULER MODULE

Copyright (C) 2017 by faina09
Adapted by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "scheduler.h"

#if SCHEDULER_SUPPORT

#include "api.h"
#include "light.h"
#include "mqtt.h"
#include "ntp.h"
#include "ntp_timelib.h"
#include "relay.h"
#include "ws.h"
#include "curtain_kingart.h"

constexpr int SchedulerDummySwitchId { 0xff };

#if RELAY_SUPPORT

size_t schedulableCount() {
    return relayCount();
}

#elif CURTAIN_SUPPORT

size_t schedulableCount() {
    return curtainCount()
}

#endif

// -----------------------------------------------------------------------------

namespace scheduler {
namespace build {

constexpr size_t max() {
    return SCHEDULER_MAX_SCHEDULES;
}

constexpr int defaultType() {
    return SCHEDULER_TYPE_SWITCH;
}

constexpr const char* const weekdays() {
    return SCHEDULER_WEEKDAYS;
}

constexpr bool restoreLast() {
    return 1 == SCHEDULER_RESTORE_LAST_SCHEDULE;
}

} // namespace build
} // namespace scheduler

// -----------------------------------------------------------------------------

#if API_SUPPORT

// UTILS

template <typename V = const char*>
bool _setJsonVariant(const SettingsKey& k, const JsonVariant& v) {
    if (!v.is<V>()) {
        return false;
    }
    return setSetting(k, v.as<V>());
}

template <>
bool _setJsonVariant<const char*>(const SettingsKey& k, const JsonVariant& v) {
    const auto& tmp = v.as<const char*>();
    if (tmp == nullptr) {
        return false;
    }
    return setSetting(k, tmp);
}

template <typename T = const char*, typename ObjKey>
bool _setJsonKey(const SettingsKey& k, const JsonObject& o, const ObjKey& key) {
    if (!o.containsKey(key)) {
        return false;
    }
    return _setJsonVariant<T>(k, o[key]);
}

//ENDPOINTS

void _schApiPrintSchedule(unsigned char id, JsonObject& root) {
    root["enabled"]     = getSetting({"schEnabled", id}, false);
    root["utc"]         = getSetting({"schUTC", id}, false);
    root["switch"]      = getSetting({"schSwitch", id}, 0);
    root["action"]      = getSetting({"schAction", id}, 0);
    root["type"]        = getSetting({"schType", id}, scheduler::build::defaultType());
    root["hour"]        = getSetting({"schHour", id}, 0);
    root["minute"]      = getSetting({"schMinute", id}, 0);
    root["weekdays"]    = getSetting({"schWDs", id}, scheduler::build::weekdays());
}


bool _schApiSetSchedule(const unsigned char id, JsonObject& sched) {
    if (!_setJsonKey<int>({"schSwitch", id}, sched, "switch")) {
        return false;
    }
    if (sched.containsKey("enabled")) {
        const auto& enabled = sched["enabled"];
        if (!_setJsonVariant<bool>({"schEnabled", id}, enabled)) {
            setSetting({"schEnabled", id}, enabled.as<String>());
        }
    }
    if (sched.containsKey("utc")) {
        const auto& utc = sched["utc"];
        if (!_setJsonVariant<bool>({"schUTC", id}, utc)) {
            setSetting({"schUTC", id}, utc.as<String>());
        }
    }

    if (sched.containsKey("action")) {
        const auto& action = sched["action"];
        if (action.is<const char*>()) {
            setSetting({"schAction", id}, int(relayParsePayload(action.as<const char*>())));
        } else {
            _setJsonVariant<int>({"schAction", id}, action);
        }
    }

    _setJsonKey<int>({"schType", id}, sched, "type");
    _setJsonKey<int>({"schHour", id}, sched, "hour");
    _setJsonKey<int>({"schMinute", id}, sched, "minute");
    _setJsonKey({"schWDs", id}, sched, "weekdays");
    return true;
}

#endif  // API_SUPPORT

#if WEB_SUPPORT

bool _schWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "sch", 3) == 0);
}

void _schWebSocketOnVisible(JsonObject& root) {
    if (!schedulableCount()) return;
    root["schVisible"] = 1;
}

void _schWebSocketOnConnected(JsonObject &root){

    if (!schedulableCount()) return;

    JsonObject& config = root.createNestedObject("schConfig");
    config["max"] = scheduler::build::max();

    {
        static const char* const schema_keys[] PROGMEM = {
            "schEnabled",
            "schRestore",
            "schType",
            "schSwitch",
            "schAction",
            "schHour",
            "schMinute",
            "schUTC",
            "schWDs"
        };

        JsonArray& schema = config.createNestedArray("schema");
        schema.copyFrom(schema_keys, sizeof(schema_keys) / sizeof(*schema_keys));
    }

    uint8_t size = 0;

    JsonArray& schedules = config.createNestedArray("schedules");

    for (size_t id = 0; id < scheduler::build::max(); ++id) {
        auto schedulerSwitch = getSetting({"schSwitch", id});
        if (!schedulerSwitch.length()) {
            break;
        }

        JsonArray& entry = schedules.createNestedArray();
        ++size;

        entry.add(getSetting({"schEnabled", id}, false) ? 1 : 0);
        entry.add(getSetting({"schRestore", id}, scheduler::build::restoreLast()) ? 1 : 0);

        entry.add(getSetting({"schType", id}, scheduler::build::defaultType()));
        entry.add(schedulerSwitch);
        entry.add(getSetting({"schAction", id}, 0));

        entry.add(getSetting({"schHour", id}, 0));
        entry.add(getSetting({"schMinute", id}, 0));

        entry.add(getSetting({"schWDs", id}, scheduler::build::weekdays()));
        entry.add(getSetting({"schUTC", id}, 0));
    }

    config["size"] = size;
    config["start"] = 0;

}

#endif // WEB_SUPPORT

// -----------------------------------------------------------------------------

void _schConfigure() {
    for (unsigned char i = 0; i < scheduler::build::max(); i++) {
        int sch_switch = getSetting({"schSwitch", i}, SchedulerDummySwitchId);
        if (sch_switch == SchedulerDummySwitchId) {
            delSetting({"schEnabled", i});
            delSetting({"schRestore", i});
            delSetting({"schSwitch", i});
            delSetting({"schAction", i});
            delSetting({"schHour", i});
            delSetting({"schMinute", i});
            delSetting({"schWDs", i});
            delSetting({"schType", i});
            delSetting({"schUTC", i});
        } else {
#if DEBUG_SUPPORT
            bool sch_enabled = getSetting({"schEnabled", i}, false);
            int sch_action = getSetting({"schAction", i}, 0);
            int sch_hour = getSetting({"schHour", i}, 0);
            int sch_minute = getSetting({"schMinute", i}, 0);
            bool sch_utc = getSetting({"schUTC", i}, false);
            String sch_weekdays = getSetting({"schWDs", i}, SCHEDULER_WEEKDAYS);

            int type = getSetting({"schType", i}, SCHEDULER_TYPE_SWITCH);
            const auto sch_type =
                (SCHEDULER_TYPE_SWITCH == type) ? "switch" :
                (SCHEDULER_TYPE_CURTAIN == type) ? "curtain" :
                (SCHEDULER_TYPE_DIM == type) ? "channel" : "unknown";

            DEBUG_MSG_P(
                PSTR("[SCH] Schedule #%d: %s #%d to %d at %02d:%02d %s on %s%s\n"),
                i, sch_type, sch_switch,
                sch_action, sch_hour, sch_minute, sch_utc ? "UTC" : "local time",
                sch_weekdays.c_str(),
                sch_enabled ? "" : " (disabled)"
            );
#endif // DEBUG_SUPPORT
        }
    }
}

bool _schIsThisWeekday(int day, const String& weekdays){

    // Convert from Sunday to Monday as day 1
    int w = day - 1;
    if (0 == w) w = 7;

    char pch;
    char * p = (char *) weekdays.c_str();
    unsigned char position = 0;
    while ((pch = p[position++])) {
        if ((pch - '0') == w) return true;
    }
    return false;

}

int _schMinutesLeft(int current_hour, int current_minute, int schedule_hour, int schedule_minute) {
    return (schedule_hour - current_hour) * 60 + schedule_minute - current_minute;
}

void _schAction(unsigned char sch_id, int sch_action, int sch_switch) {
    const auto sch_type = getSetting({"schType", sch_id}, SCHEDULER_TYPE_SWITCH);

    if (SCHEDULER_TYPE_SWITCH == sch_type) {
        DEBUG_MSG_P(PSTR("[SCH] Switching switch %d to %d\n"), sch_switch, sch_action);
        if (sch_action == 2) {
            relayToggle(sch_switch);
        } else {
            relayStatus(sch_switch, sch_action);
        }
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    } else if (SCHEDULER_TYPE_DIM == sch_type) {
        DEBUG_MSG_P(PSTR("[SCH] Set channel %d value to %d\n"), sch_switch, sch_action);
        lightChannel(sch_switch, sch_action);
        lightUpdate();
#elif CURTAIN_SUPPORT
    } else if (SCHEDULER_TYPE_CURTAIN == sch_type) {
    DEBUG_MSG_P(PSTR("[SCH] Set curtain %d value to %d\n"), sch_switch, sch_action);
    curtainSetPosition(sch_switch, sch_action);
#endif
    }
}

constexpr time_t secondsPerMinute = 60;
constexpr time_t secondsPerHour = 3600;
constexpr time_t secondsPerDay = secondsPerHour * 24;

NtpCalendarWeekday _schGetWeekday(time_t timestamp, int daybefore) {
    tm utc_time;
    tm local_time;

    gmtime_r(&timestamp, &utc_time);
    if (daybefore > 0) {
        timestamp = timestamp - ((utc_time.tm_hour * secondsPerHour) + ((utc_time.tm_min + 1) * secondsPerMinute) + utc_time.tm_sec + (daybefore * secondsPerDay));
        gmtime_r(&timestamp, &utc_time);
        localtime_r(&timestamp, &local_time);
    } else {
        localtime_r(&timestamp, &local_time);
    }

    // TimeLib sunday is 1 instead of 0
    return NtpCalendarWeekday {
        local_time.tm_wday + 1, local_time.tm_hour, local_time.tm_min,
        utc_time.tm_wday + 1, utc_time.tm_hour, utc_time.tm_min
    };
}

// If daybefore and target is -1, check with current timestamp
// Otherwise, modify it by moving 'daybefore' days back and only use the 'target' id
void _schCheck(int target, int daybefore) {
    time_t timestamp = now();
    auto calendar_weekday = _schGetWeekday(timestamp, daybefore);

    int minimum_restore_time = -(60 * 24);
    int saved_action = -1;
    int saved_sch = -1;

    // Check schedules
    for (unsigned char i = 0; i < scheduler::build::max(); i++) {

        int sch_switch = getSetting({"schSwitch", i}, SchedulerDummySwitchId);
        if (sch_switch == SchedulerDummySwitchId) break;

        // Skip disabled schedules
        if (!getSetting({"schEnabled", i}, false)) continue;

        // Get the datetime used for the calculation
        const bool sch_utc = getSetting({"schUTC", i}, false);

        String sch_weekdays = getSetting({"schWDs", i}, SCHEDULER_WEEKDAYS);
        if (_schIsThisWeekday(sch_utc ? calendar_weekday.utc_wday : calendar_weekday.local_wday, sch_weekdays)) {

            int sch_hour = getSetting({"schHour", i}, 0);
            int sch_minute = getSetting({"schMinute", i}, 0);
            int sch_action = getSetting({"schAction", i}, 0);
            int sch_type = getSetting({"schType", i}, SCHEDULER_TYPE_SWITCH);

            int minutes_to_trigger = _schMinutesLeft(
                sch_utc ? calendar_weekday.utc_hour : calendar_weekday.local_hour,
                sch_utc ? calendar_weekday.utc_minute : calendar_weekday.local_minute,
                sch_hour, sch_minute
            );

            if (sch_type == SCHEDULER_TYPE_SWITCH && sch_switch == target && sch_action != 2 && minutes_to_trigger < 0 && minutes_to_trigger > minimum_restore_time) {
                minimum_restore_time = minutes_to_trigger;
                saved_action = sch_action;
                saved_sch = i;
            }

            #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
                if (SCHEDULER_TYPE_DIM == sch_type && sch_switch == target && minutes_to_trigger < 0 && minutes_to_trigger > minimum_restore_time) {
                    minimum_restore_time = minutes_to_trigger;
                    saved_action = sch_action;
                    saved_sch = i;
                }
            #endif

             #if CURTAIN_SUPPORT == 1
                if (SCHEDULER_TYPE_CURTAIN == sch_type && sch_switch == target && minutes_to_trigger < 0 && minutes_to_trigger > minimum_restore_time) {
                    minimum_restore_time = minutes_to_trigger;
                    saved_action = sch_action;
                    saved_sch = i;
                }
            #endif

            if (minutes_to_trigger == 0 && target == -1) {

                _schAction(i, sch_action, sch_switch);
                DEBUG_MSG_P(PSTR("[SCH] Schedule #%u TRIGGERED!!\n"), i);

            // Show minutes to trigger every 15 minutes
            // or every minute if less than 15 minutes to scheduled time.
            // This only works for schedules on this same day.
            // For instance, if your scheduler is set for 00:01 you will only
            // get one notification before the trigger (at 00:00)
            } else if (minutes_to_trigger > 0 && target == -1) {

                #if DEBUG_SUPPORT
                    if ((minutes_to_trigger % 15 == 0) || (minutes_to_trigger < 15)) {
                        DEBUG_MSG_P(
                            PSTR("[SCH] %d minutes to trigger schedule #%u\n"),
                            minutes_to_trigger, i
                        );
                    }
                #endif

            }

        }

    }

    if (daybefore >= 0 && daybefore < 7 && minimum_restore_time == -(60 * 24) && saved_action == -1) {
        _schCheck(target, ++daybefore);
        return;
    }

    if (minimum_restore_time != -(60 * 24) && saved_action != -1 && saved_sch != -1) {
        _schAction(saved_sch, saved_action, target);
    }

}

// -----------------------------------------------------------------------------

void schSetup() {

    _schConfigure();

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_schWebSocketOnVisible)
            .onConnected(_schWebSocketOnConnected)
            .onKeyCheck(_schWebSocketOnKeyCheck);
    #endif

    #if API_SUPPORT
        apiRegister(
            F(MQTT_TOPIC_SCHEDULE),
            [](ApiRequest&, JsonObject& root) {
                JsonArray& scheds = root.createNestedArray("schedules");
                for (unsigned char i = 0; i < SCHEDULER_MAX_SCHEDULES; ++i) {
                    if (!hasSetting({"schSwitch", i})) continue;
                    auto& sched = scheds.createNestedObject();
                    _schApiPrintSchedule(i, sched);
                }
                return true;
            },
            [](ApiRequest&, JsonObject& root) {
                unsigned char id = 0;
                while (hasSetting({"schSwitch", id})) id++;
                if (id > scheduler::build::max()) {
                    return false;
                }
                _schApiSetSchedule(id, root);
                _schConfigure();
                return true;
            });
        apiRegister(
            F(MQTT_TOPIC_SCHEDULE "/+"),
            [](ApiRequest& r, JsonObject& root) {
                const auto& id_param = r.wildcard(0);
                size_t id;
                if (!tryParseId(id_param.c_str(), scheduler::build::max, id)) {
                    return false;
                }
                _schApiPrintSchedule(id, root);
                return true;
            },
            [](ApiRequest& r, JsonObject& root) {
                const auto& id_param = r.wildcard(0);
                size_t id;
                if (!tryParseId(id_param.c_str(), scheduler::build::max, id)) {
                    return false;
                }
                _schApiSetSchedule(id, root);
                return true;
            });
    #endif

    static bool restore_once = true;
    ntpOnTick([](NtpTick tick) {
        switch (tick) {
        case NtpTick::EveryHour:
            return;
        case NtpTick::EveryMinute:
            if (restore_once) {
                auto targets = schedulableCount();
                for (size_t i = 0; i < targets; i++) {
                    if (getSetting({"schRestore", i}, scheduler::build::restoreLast())) {
                        _schCheck(i, 0);
                    }
                }
            }
            restore_once = false;
        }
        _schCheck(-1, -1);
    });

    espurnaRegisterReload(_schConfigure);

}

#endif // SCHEDULER_SUPPORT
