/*

SCHEDULER MODULE

Copyright (C) 2017 by faina09
Adapted by Xose Pérez <xose dot perez at gmail dot com>

*/

#if SCHEDULER_SUPPORT

#include <NtpClientLib.h>

#if WEB_SUPPORT

void _schWebSocketOnSend(JsonObject &root){

    root["maxScheduled"] = MAX_SCHEDULED;
    JsonArray &sch = root.createNestedArray("schedule");
    for (byte i = 0; i < MAX_SCHEDULED; i++) {
        if (!hasSetting("schSwitch", i)) break;
        JsonObject &scheduler = sch.createNestedObject();
        scheduler["schSwitch"] = getSetting("schSwitch", i, "");
        scheduler["schAction"] = getSetting("schAction", i, "");
        scheduler["schHour"] = getSetting("schHour", i, "");
        scheduler["schMinute"] = getSetting("schMinute", i, "");
        scheduler["schWDs"] = getSetting("schWDs", i, "");
    }
}

#endif // WEB_SUPPORT

void _schConfigure() {

    bool delete_flag = false;

    for (unsigned char i = 0; i < MAX_SCHEDULED; i++) {

        int sch_switch = getSetting("schSwitch", i, 0xFF).toInt();
        if (sch_switch == 0xFF) delete_flag = true;

        if (delete_flag) {

            delSetting("schSwitch", i);
            delSetting("schAction", i);
            delSetting("schHour", i);
            delSetting("schMinute", i);
            delSetting("schWDs", i);

        } else {

            #if DEBUG_SUPPORT

                int sch_action = getSetting("schAction", i, 0).toInt();
                int sch_hour = getSetting("schHour", i, 0).toInt();
                int sch_minute = getSetting("schMinute", i, 0).toInt();
                String sch_weekdays = getSetting("schWDs", i, "");
                DEBUG_MSG_P(
                    PSTR("[SCH] Schedule #%d: %s switch #%d at %02d:%02d on %s\n"),
                    i, sch_action == 0 ? "turn OFF" : sch_action == 1 ? "turn ON" : "toggle", sch_switch,
                    sch_hour, sch_minute, (char *) sch_weekdays.c_str()
                );

            #endif // DEBUG_SUPPORT

        }

    }

}

bool _isThisWeekday(String weekdays){

    // Monday = 1, Tuesday = 2 ... Sunday = 7
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

int _diffTime(unsigned char schedule_hour, unsigned char schedule_minute){

    unsigned char now_hour;
    unsigned char now_minute;

    if (ntpConnected()) {
        String value = NTP.getTimeDateString();
        now_hour = value.substring(0, 2).toInt();
        now_minute = value.substring(3, 5).toInt();
    } else {
        time_t t = now();
        now_hour = hour(t);
        now_minute = minute(t);
    }

    return (schedule_hour - now_hour) * 60 + schedule_minute - now_minute;

}

// -----------------------------------------------------------------------------

void schSetup() {

    _schConfigure();

    // Update websocket clients
    #if WEB_SUPPORT
        wsOnSendRegister(_schWebSocketOnSend);
        wsOnAfterParseRegister(_schConfigure);
    #endif

}

void schLoop(){

    static unsigned long last_update = 0;
    static int update_time = 0;

    // Check if we should compare scheduled and actual times
    if ((millis() - last_update > update_time) || (last_update == 0)) {
        last_update = millis();

        // Calculate next update time
        unsigned char current_second = ntpConnected() ?
            NTP.getTimeDateString().substring(6, 8).toInt() :
            second(now())
            ;
        update_time = (SCH_UPDATE_SEC + 60 - current_second) * 1000;

        for (unsigned char i = 0; i < MAX_SCHEDULED; i++) {

            int sch_switch = getSetting("schSwitch", i, 0xFF).toInt();
            if (sch_switch == 0xFF) break;

            String sch_weekdays = getSetting("schWDs", i, "");
            if (_isThisWeekday(sch_weekdays)) {

                int sch_hour = getSetting("schHour", i, 0).toInt();
                int sch_minute = getSetting("schMinute", i, 0).toInt();
                int minutes_to_trigger = _diffTime(sch_hour, sch_minute);
                if (minutes_to_trigger == 0) {
                    int sch_action = getSetting("schAction", i, 0).toInt();
                    if (sch_action == 2) {
                        relayToggle(sch_switch);
                    } else {
                        relayStatus(sch_switch, sch_action);
                    }
                    DEBUG_MSG_P(PSTR("[SCH] Schedule #%d TRIGGERED!!\n"), sch_switch);
                } else if (minutes_to_trigger > 0) {
                    DEBUG_MSG_P(
                        PSTR("[SCH] %d minutes to trigger schedule #%d\n"),
                        minutes_to_trigger, sch_switch
                    );
                }
            }
        }
     }
}

#endif // SCHEDULER_SUPPORT
