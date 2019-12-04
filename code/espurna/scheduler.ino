/*

SCHEDULER MODULE

Copyright (C) 2017 by faina09
Adapted by Xose Pérez <xose dot perez at gmail dot com>

*/

#if SCHEDULER_SUPPORT

#include "relay.h"

#include <TimeLib.h>

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

    for (byte i = 0; i < SCHEDULER_MAX_SCHEDULES; i++) {
        if (!hasSetting("schSwitch", i)) break;
        ++size;

        enabled.add<uint8_t>(getSetting("schEnabled", i, 0).toInt() == 1);
        utc.add<uint8_t>(getSetting("schUTC", i, 0).toInt() == 1);

        switch_.add(getSetting("schSwitch", i, 0).toInt());
        action.add(getSetting("schAction", i, 0).toInt());
        type.add(getSetting("schType", i, SCHEDULER_TYPE_SWITCH).toInt());
        hour.add(getSetting("schHour", i, 0).toInt());
        minute.add(getSetting("schMinute", i, 0).toInt());
        weekdays.add(getSetting("schWDs", i, SCHEDULER_WEEKDAYS));
    }

    schedules["size"] = size;
    schedules["start"] = 0;

}

#endif // WEB_SUPPORT

// -----------------------------------------------------------------------------

void _schConfigure() {

    bool delete_flag = false;

    for (unsigned char i = 0; i < SCHEDULER_MAX_SCHEDULES; i++) {

        int sch_switch = getSetting("schSwitch", i, 0xFF).toInt();
        if (sch_switch == 0xFF) delete_flag = true;

        if (delete_flag) {

            delSetting("schEnabled", i);
            delSetting("schSwitch", i);
            delSetting("schAction", i);
            delSetting("schHour", i);
            delSetting("schMinute", i);
            delSetting("schWDs", i);
            delSetting("schType", i);
            delSetting("schUTC", i);

        } else {

            #if DEBUG_SUPPORT

                bool sch_enabled = getSetting("schEnabled", i, 0).toInt() == 1;
                int sch_action = getSetting("schAction", i, 0).toInt();
                int sch_hour = getSetting("schHour", i, 0).toInt();
                int sch_minute = getSetting("schMinute", i, 0).toInt();
                bool sch_utc = getSetting("schUTC", i, 0).toInt() == 1;
                String sch_weekdays = getSetting("schWDs", i, SCHEDULER_WEEKDAYS);
                unsigned char sch_type = getSetting("schType", i, SCHEDULER_TYPE_SWITCH).toInt();

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

bool _schIsThisWeekday(time_t t, String weekdays){

    // Convert from Sunday to Monday as day 1
    int w = weekday(t) - 1;
    if (0 == w) w = 7;

    char pch;
    char * p = (char *) weekdays.c_str();
    unsigned char position = 0;
    while ((pch = p[position++])) {
        if ((pch - '0') == w) return true;
    }
    return false;

}

int _schMinutesLeft(time_t t, unsigned char schedule_hour, unsigned char schedule_minute){
    unsigned char now_hour = hour(t);
    unsigned char now_minute = minute(t);
    return (schedule_hour - now_hour) * 60 + schedule_minute - now_minute;
}

void _schAction(int sch_id, int sch_action, int sch_switch) {
    unsigned char sch_type = getSetting("schType", sch_id, SCHEDULER_TYPE_SWITCH).toInt();

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

// If daybefore and relay is -1, check with current timestamp
// Otherwise, modify it by moving 'daybefore' days back and only use the 'relay' id
void _schCheck(int relay, int daybefore) {

    time_t local_time = now();
    time_t utc_time = ntpLocal2UTC(local_time);
    int minimum_restore_time = -1440;
    int saved_action = -1;
    int saved_sch = -1;

    // Check schedules
    for (unsigned char i = 0; i < SCHEDULER_MAX_SCHEDULES; i++) {

        int sch_switch = getSetting("schSwitch", i, 0xFF).toInt();
        if (sch_switch == 0xFF) break;

        // Skip disabled schedules
        if (getSetting("schEnabled", i, 0).toInt() == 0) continue;

        // Get the datetime used for the calculation
        bool sch_utc = getSetting("schUTC", i, 0).toInt() == 1;
        time_t t = sch_utc ? utc_time : local_time;

        if (daybefore > 0) {
          unsigned char now_hour = hour(t);
          unsigned char now_minute = minute(t);
          unsigned char now_sec = second(t);
          t = t - ((now_hour * 3600) + ((now_minute + 1) * 60) + now_sec + (daybefore * 86400));
        }

        String sch_weekdays = getSetting("schWDs", i, SCHEDULER_WEEKDAYS);
        if (_schIsThisWeekday(t, sch_weekdays)) {

            int sch_hour = getSetting("schHour", i, 0).toInt();
            int sch_minute = getSetting("schMinute", i, 0).toInt();
            int minutes_to_trigger = _schMinutesLeft(t, sch_hour, sch_minute);
            int sch_action = getSetting("schAction", i, 0).toInt();
            unsigned char sch_type = getSetting("schType", i, SCHEDULER_TYPE_SWITCH).toInt();

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

    if (daybefore >= 0 && daybefore < 7 && minimum_restore_time == -1440 && saved_action == -1) {
      _schCheck(relay, ++daybefore);
      return;
    }

    if (minimum_restore_time != -1440 && saved_action != -1 && saved_sch != -1) {
        _schAction(saved_sch, saved_action, relay);
    }

}

void _schLoop() {

    // Check time has been sync'ed
    if (!ntpSynced()) return;

    if (_sch_restore == 0) {
        for (unsigned char i = 0; i < relayCount(); i++){
            if (getSetting("relayLastSch", i, SCHEDULER_RESTORE_LAST_SCHEDULE).toInt() == 1)
                _schCheck(i, 0);
        }
        _sch_restore = 1;
    }

    // Check schedules every minute at hh:mm:00
    static unsigned long last_minute = 60;
    unsigned char current_minute = minute();
    if (current_minute != last_minute) {
        last_minute = current_minute;
        _schCheck(-1, -1);
    }

}

// -----------------------------------------------------------------------------

void schSetup() {

    _schConfigure();

    // Update websocket clients
    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_schWebSocketOnVisible)
            .onConnected(_schWebSocketOnConnected)
            .onKeyCheck(_schWebSocketOnKeyCheck);
    #endif

    // Main callbacks
    espurnaRegisterLoop(_schLoop);
    espurnaRegisterReload(_schConfigure);

}

#endif // SCHEDULER_SUPPORT
