/*

A SCHEDULER MODULE

Copyright (C) 2017 by faina09

*/

#if SCHEDULER_SUPPORT

#include <NtpClientLib.h>

void _schWebSocketOnSend(JsonObject &root){
    root["maxScheduled"] = MAX_SCHEDULED;
    JsonArray &sch = root.createNestedArray("schedule");
    for (byte i = 0; i < MAX_SCHEDULED; i++) {
        if (!hasSetting("sch_switch", i))
            break;
        JsonObject &scheduler = sch.createNestedObject();
        scheduler["sch_switch"] = getSetting("sch_switch", i, "");
        scheduler["sch_operation"] = getSetting("sch_operation", i, "");
        scheduler["sch_hour"] = getSetting("sch_hour", i, "");
        scheduler["sch_minute"] = getSetting("sch_minute", i, "");
        scheduler["sch_weekdays"] = getSetting("sch_weekdays", i, "");
    }
}

void schSetup(){
    // Update websocket clients
    #if WEB_SUPPORT
            wsOnSendRegister(_schWebSocketOnSend);
    #endif

    int i;
    for (i = 0; i < MAX_SCHEDULED; i++) {
        if (getSetting("sch_switch" + String(i)).length() == 0)
            break;
        String sch_weekdays = getSetting("sch_weekdays" + String(i));
        int sch_switch = getSetting("sch_switch" + String(i)).toInt();
        int sch_operation = getSetting("sch_operation" + String(i)).toInt();
        int sch_hour = getSetting("sch_hour" + String(i)).toInt();
        int sch_minute = getSetting("sch_minute" + String(i)).toInt();
        DEBUG_MSG_P(PSTR("[SCH] Turn switch #%d %s at %02d:%02d on %s\n"), sch_switch, sch_operation ? "ON" : "OFF", sch_hour, sch_minute, (char *)sch_weekdays.c_str());
    }
}

void schLoop(){
    // Check if we should compare scheduled and actual times
    static unsigned long last_update = 0;
    static int sec = 0;
    if ((millis() - last_update > ((SCH_UPDATE_SEC + 60 - sec)*1000)) || (last_update == 0)) {
        last_update = millis();
        if (!ntpConnected()){
            time_t t = now();
            sec = second(t);
            DEBUG_MSG_P(PSTR("[SCH] no NTP, time now=%02d:%02d:%02d\n"),hour(t),minute(t),second(t));
        }
        else {
            // compare at next minute and SCH_UPDATE_SEC seconds
            sec = NTP.getTimeDateString().substring(6, 8).toInt();
        }
        int i;
        for (i = 0; i < MAX_SCHEDULED; i++) {
            if (getSetting("sch_switch" + String(i)).length() == 0)
                break;
            String sch_weekdays = getSetting("sch_weekdays" + String(i));
            if (isThisWday(sch_weekdays)) {
                int sch_switch = getSetting("sch_switch" + String(i)).toInt();
                int sch_operation = getSetting("sch_operation" + String(i)).toInt();
                int sch_hour = getSetting("sch_hour" + String(i)).toInt();
                int sch_minute = getSetting("sch_minute" + String(i)).toInt();
                //DEBUG_MSG_P(PSTR("[SCH] Today it will turn switch #%d %d @ %02d:%02d\n"), sch_switch, sch_operation, sch_hour, sch_minute);
                int minToTrigger = diffTime(sch_hour, sch_minute);
                if (minToTrigger == 0) {
                    relayStatus(sch_switch, sch_operation);
                    DEBUG_MSG_P(PSTR("[SCH] TRIGGERED!! switch #%d is %s\n"), sch_switch, sch_operation ? "ON" : "OFF");
                }
                if (minToTrigger < 0) {
                    //DEBUG_MSG_P(PSTR("[SCH] Time now: %s\n"), (char *)ntpDateTime().c_str()); // aaaa/mm/dd hh:mm:ss
                    DEBUG_MSG_P(PSTR("[SCH] Time now: %s, %d minutes to trigger %02d:%02d switch #%d %s\n"),
                      (char *)ntpDateTime().c_str(), -minToTrigger, sch_hour, sch_minute, sch_switch, sch_operation ? "ON" : "OFF");
                }
            }
        }
     }
}

bool isThisWday(String weekdays){
    //Sunday = 1, Monday = 2, ...
    int w = weekday(now());
    //DEBUG_MSG_P(PSTR("[SCH] ntp weekday: %d\n"), w);
    char * pch;
    char * p = (char *)weekdays.c_str();
    while ((pch = strtok_r(p, ",", &p)) != NULL) {
        //DEBUG_MSG_P(PSTR("[SCH] w found: %d\n"), atoi(pch));
        if (atoi(pch) == w) return true;
    }
    return false;
}

int diffTime(int schhour, int schminute){
    if (!ntpConnected()){
        time_t t = now();
        DEBUG_MSG_P(PSTR("[SCH] no NTP time = %02d:%02d:%02d\n"),hour(t),minute(t),second(t));
        return (hour(t) - schhour) * 60 + minute(t) - schminute;
    }
    else {
        String value = NTP.getTimeDateString();
        int hour = value.substring(0, 2).toInt();
        int minute = value.substring(3, 5).toInt();
        //DEBUG_MSG_P(PSTR("[SCH] ntp time: %02d:%02d\n"), hour, minute);
        //DEBUG_MSG_P(PSTR("[SCH] cmp time: %02d:%02d\n"), schhour, schminute);
        return (hour - schhour) * 60 + minute - schminute;
    }
}

#endif // SCHEDULER_SUPPORT
