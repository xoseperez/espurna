/*

SCHEDULER MODULE

Copyright (C) 2017 by faina09
Adapted by Xose Pérez <xose dot perez at gmail dot com>

*/

#if SCHEDULER_SUPPORT

#include <TimeLib.h>

// -----------------------------------------------------------------------------

#if WEB_SUPPORT

bool _schWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "sch", 3) == 0);
}

void _schWebSocketOnSend(JsonObject &root){

    if (relayCount() > 0) {

        root["schVisible"] = 1;
        root["maxSchedules"] = SCHEDULER_MAX_SCHEDULES;
        JsonArray &sch = root.createNestedArray("schedule");
        for (byte i = 0; i < SCHEDULER_MAX_SCHEDULES; i++) {
            if (!hasSetting("schSwitch", i)) break;
            JsonObject &scheduler = sch.createNestedObject();
            scheduler["schEnabled"] = getSetting("schEnabled", i, 1).toInt() == 1;
            scheduler["schSwitch"] = getSetting("schSwitch", i, 0).toInt();
            scheduler["schAction"] = getSetting("schAction", i, 0).toInt();
            scheduler["schType"] = getSetting("schType", i, 0).toInt();
            scheduler["schHour"] = getSetting("schHour", i, 0).toInt();
            scheduler["schMinute"] = getSetting("schMinute", i, 0).toInt();
            scheduler["schWDs"] = getSetting("schWDs", i, "");
        }

    }

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

        } else {

            #if DEBUG_SUPPORT

                int sch_enabled = getSetting("schEnabled", i, 1).toInt() == 1;
                int sch_action = getSetting("schAction", i, 0).toInt();
                int sch_hour = getSetting("schHour", i, 0).toInt();
                int sch_minute = getSetting("schMinute", i, 0).toInt();
                String sch_weekdays = getSetting("schWDs", i, "");
                unsigned char sch_type = getSetting("schType", i, SCHEDULER_TYPE_SWITCH).toInt();

                DEBUG_MSG_P(
                    PSTR("[SCH] Schedule #%d: %s #%d to %d at %02d:%02d on %s%s\n"),
                    i, SCHEDULER_TYPE_SWITCH == sch_type ? "switch" : "channel", sch_switch,
                    sch_action, sch_hour, sch_minute, (char *) sch_weekdays.c_str(),
                    sch_enabled ? "" : " (disabled)"
                );

            #endif // DEBUG_SUPPORT

        }

    }

}

bool _schIsThisWeekday(String weekdays){

    // Convert from Sunday to Monday as day 1
    int w = weekday(now()) - 1;
    if (w == 0) w = 7;

    char pch;
    char * p = (char *) weekdays.c_str();
    unsigned char position = 0;
    while (pch = p[position++]) {
        if ((pch - '0') == w) return true;
    }
    return false;

}

int _schMinutesLeft(unsigned char schedule_hour, unsigned char schedule_minute){
    time_t t = now();
    unsigned char now_hour = hour(t);
    unsigned char now_minute = minute(t);
    return (schedule_hour - now_hour) * 60 + schedule_minute - now_minute;
}

void _schCheck() {

    // Check schedules
    for (unsigned char i = 0; i < SCHEDULER_MAX_SCHEDULES; i++) {

        int sch_switch = getSetting("schSwitch", i, 0xFF).toInt();
        if (sch_switch == 0xFF) break;

        // Skip disabled schedules
        if (getSetting("schEnabled", i, 1).toInt() == 0) continue;

        String sch_weekdays = getSetting("schWDs", i, "");
        if (_schIsThisWeekday(sch_weekdays)) {

            int sch_hour = getSetting("schHour", i, 0).toInt();
            int sch_minute = getSetting("schMinute", i, 0).toInt();
            int minutes_to_trigger = _schMinutesLeft(sch_hour, sch_minute);

            if (minutes_to_trigger == 0) {

                unsigned char sch_type = getSetting("schType", i, SCHEDULER_TYPE_SWITCH).toInt();

                if (SCHEDULER_TYPE_SWITCH == sch_type) {
                    int sch_action = getSetting("schAction", i, 0).toInt();
                    DEBUG_MSG_P(PSTR("[SCH] Switching switch %d to %d\n"), sch_switch, sch_action);
                    if (sch_action == 2) {
                        relayToggle(sch_switch);
                    } else {
                        relayStatus(sch_switch, sch_action);
                    }
                }

                #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
                    if (SCHEDULER_TYPE_DIM == sch_type) {
                        int sch_brightness = getSetting("schAction", i, -1).toInt();
                        DEBUG_MSG_P(PSTR("[SCH] Set channel %d value to %d\n"), sch_switch, sch_brightness);
                        lightChannel(sch_switch, sch_brightness);
                        lightUpdate(true, true);
                    }
                #endif

                DEBUG_MSG_P(PSTR("[SCH] Schedule #%d TRIGGERED!!\n"), i);

            // Show minutes to trigger every 15 minutes
            // or every minute if less than 15 minutes to scheduled time.
            // This only works for schedules on this same day.
            // For instance, if your scheduler is set for 00:01 you will only
            // get one notification before the trigger (at 00:00)
            } else if (minutes_to_trigger > 0) {

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

}

void _schLoop() {

    // Check time has been sync'ed
    if (!ntpSynced()) return;

    // Check schedules every minute at hh:mm:00
    static unsigned long last_minute = 60;
    unsigned char current_minute = minute();
    if (current_minute != last_minute) {
        last_minute = current_minute;
        _schCheck();
    }

}

// -----------------------------------------------------------------------------

void schSetup() {

    _schConfigure();

    // Update websocket clients
    #if WEB_SUPPORT
        wsOnSendRegister(_schWebSocketOnSend);
        wsOnReceiveRegister(_schWebSocketOnReceive);
        wsOnAfterParseRegister(_schConfigure);
    #endif

    // Register loop
    espurnaRegisterLoop(_schLoop);

}

#endif // SCHEDULER_SUPPORT
