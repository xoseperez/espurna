/*

SCHEDULER MODULE

Copyright (C) 2017 by faina09
Adapted by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "scheduler.h"

#if SCHEDULER_SUPPORT

#include "broker.h"
#include "light.h"
#include "ntp.h"
#include "relay.h"
#include "ws.h"

constexpr const int SchedulerDummySwitchId = 0xff;

int _sch_restore = 0;

// -----------------------------------------------------------------------------

#if WEB_SUPPORT

bool _schWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "sch", 3) == 0);
}

void _schWebSocketOnVisible(JsonObject& root) {
    if (!relayCount()) return;
    root["schVisible"] = 1;
}

void _schWebSocketOnConnected(JsonObject &root){

    if (!relayCount()) return;

    JsonObject &schedules = root.createNestedObject("schedules");
    schedules["max"] = SCHEDULER_MAX_SCHEDULES;

    JsonArray& enabled = schedules.createNestedArray("schEnabled");
    JsonArray& switch_ = schedules.createNestedArray("schSwitch");
    JsonArray& action = schedules.createNestedArray("schAction");
    JsonArray& type = schedules.createNestedArray("schType");
    JsonArray& hour = schedules.createNestedArray("schHour");
    JsonArray& minute = schedules.createNestedArray("schMinute");
    JsonArray& utc = schedules.createNestedArray("schUTC");
    JsonArray& weekdays = schedules.createNestedArray("schWDs");

    uint8_t size = 0;

    for (unsigned char i = 0; i < SCHEDULER_MAX_SCHEDULES; i++) {
        if (!getSetting({"schSwitch", i}).length()) break;
        ++size;

        enabled.add(getSetting({"schEnabled", i}, false) ? 1 : 0);
        utc.add(getSetting({"schUTC", i}, 0));

        switch_.add(getSetting({"schSwitch", i}, 0));
        action.add(getSetting({"schAction", i}, 0));
        type.add(getSetting({"schType", i}, SCHEDULER_TYPE_SWITCH));
        hour.add(getSetting({"schHour", i}, 0));
        minute.add(getSetting({"schMinute", i}, 0));
        weekdays.add(getSetting({"schWDs", i}, SCHEDULER_WEEKDAYS));
    }

    schedules["size"] = size;
    schedules["start"] = 0;

}

#endif // WEB_SUPPORT

// -----------------------------------------------------------------------------

void _schConfigure() {

    bool delete_flag = false;

    for (unsigned char i = 0; i < SCHEDULER_MAX_SCHEDULES; i++) {

        int sch_switch = getSetting({"schSwitch", i}, SchedulerDummySwitchId);
        if (sch_switch == SchedulerDummySwitchId) delete_flag = true;

        if (delete_flag) {

            delSetting({"schEnabled", i});
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
                int sch_type = getSetting({"schType", i}, SCHEDULER_TYPE_SWITCH);

                DEBUG_MSG_P(
                    PSTR("[SCH] Schedule #%d: %s #%d to %d at %02d:%02d %s on %s%s\n"),
                    i, SCHEDULER_TYPE_SWITCH == sch_type ? "switch" : "channel", sch_switch,
                    sch_action, sch_hour, sch_minute, sch_utc ? "UTC" : "local time",
                    (char *) sch_weekdays.c_str(),
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
    }

    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        if (SCHEDULER_TYPE_DIM == sch_type) {
            DEBUG_MSG_P(PSTR("[SCH] Set channel %d value to %d\n"), sch_switch, sch_action);
            lightChannel(sch_switch, sch_action);
            lightUpdate(true, true);
        }
    #endif
}

#if NTP_LEGACY_SUPPORT

NtpCalendarWeekday _schGetWeekday(time_t timestamp, int daybefore) {
    if (daybefore > 0) {
        timestamp = timestamp - ((hour(timestamp) * SECS_PER_HOUR) + ((minute(timestamp) + 1) * SECS_PER_MIN) + second(timestamp) + (daybefore * SECS_PER_DAY));
    }

    // XXX: no
    time_t utc_timestamp = ntpLocal2UTC(timestamp);
    return NtpCalendarWeekday {
        weekday(timestamp), hour(timestamp), minute(timestamp),
        weekday(utc_timestamp), hour(utc_timestamp), minute(utc_timestamp)
    };
}

#else

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

#endif

// If daybefore and relay is -1, check with current timestamp
// Otherwise, modify it by moving 'daybefore' days back and only use the 'relay' id
void _schCheck(int relay, int daybefore) {

    time_t timestamp = now();
    auto calendar_weekday = _schGetWeekday(timestamp, daybefore);

    int minimum_restore_time = -(60 * 24);
    int saved_action = -1;
    int saved_sch = -1;

    // Check schedules
    for (unsigned char i = 0; i < SCHEDULER_MAX_SCHEDULES; i++) {

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

            if (sch_type == SCHEDULER_TYPE_SWITCH && sch_switch == relay && sch_action != 2 && minutes_to_trigger < 0 && minutes_to_trigger > minimum_restore_time) {
                minimum_restore_time = minutes_to_trigger;
                saved_action = sch_action;
                saved_sch = i;
            }

            #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
                if (SCHEDULER_TYPE_DIM == sch_type && sch_switch == relay && minutes_to_trigger < 0 && minutes_to_trigger > minimum_restore_time) {
                    minimum_restore_time = minutes_to_trigger;
                    saved_action = sch_action;
                    saved_sch = i;
                }
            #endif

            if (minutes_to_trigger == 0 && relay == -1) {

                _schAction(i, sch_action, sch_switch);
                DEBUG_MSG_P(PSTR("[SCH] Schedule #%d TRIGGERED!!\n"), i);

            // Show minutes to trigger every 15 minutes
            // or every minute if less than 15 minutes to scheduled time.
            // This only works for schedules on this same day.
            // For instance, if your scheduler is set for 00:01 you will only
            // get one notification before the trigger (at 00:00)
            } else if (minutes_to_trigger > 0 && relay == -1) {

                #if DEBUG_SUPPORT
                    if ((minutes_to_trigger % 15 == 0) || (minutes_to_trigger < 15)) {
                        DEBUG_MSG_P(
                            PSTR("[SCH] %d minutes to trigger schedule #%d\n"),
                            minutes_to_trigger, i
                        );
                    }
                #endif

            }

        }

    }

    if (daybefore >= 0 && daybefore < 7 && minimum_restore_time == -(60 * 24) && saved_action == -1) {
        _schCheck(relay, ++daybefore);
        return;
    }

    if (minimum_restore_time != -(60 * 24) && saved_action != -1 && saved_sch != -1) {
        _schAction(saved_sch, saved_action, relay);
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

    NtpBroker::Register([](const NtpTick tick, time_t, const String&) {
        if (NtpTick::EveryMinute != tick) return;

        static bool restore_once = true;
        if (restore_once) {
            for (unsigned char i = 0; i < relayCount(); i++) {
                if (getSetting({"relayLastSch", i}, 1 == SCHEDULER_RESTORE_LAST_SCHEDULE)) {
                    _schCheck(i, 0);
                }
            }
            restore_once = false;
        }
        _schCheck(-1, -1);
    });

    espurnaRegisterReload(_schConfigure);

}

#endif // SCHEDULER_SUPPORT
