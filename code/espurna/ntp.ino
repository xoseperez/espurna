/*

NTP MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if NTP_SUPPORT

#include <TimeLib.h>
#include <NtpClientLib.h>
#include <WiFiClient.h>
#include <Ticker.h>

bool _ntp_update = false;
bool _ntp_configure = false;

// -----------------------------------------------------------------------------
// NTP
// -----------------------------------------------------------------------------

void _ntpWebSocketOnSend(JsonObject& root) {
    root["time"] = ntpDateTime();
    root["ntpVisible"] = 1;
    root["ntpStatus"] = ntpSynced();
    root["ntpServer1"] = getSetting("ntpServer1", NTP_SERVER);
    root["ntpServer2"] = getSetting("ntpServer2");
    root["ntpServer3"] = getSetting("ntpServer3");
    root["ntpOffset"] = getSetting("ntpOffset", NTP_TIME_OFFSET).toInt();
    root["ntpDST"] = getSetting("ntpDST", NTP_DAY_LIGHT).toInt() == 1;
}

void _ntpUpdate() {

    _ntp_update = false;

    #if WEB_SUPPORT
        wsSend(_ntpWebSocketOnSend);
    #endif

    DEBUG_MSG_P(PSTR("[NTP] Time: %s\n"), (char *) ntpDateTime().c_str());

}

void _ntpConfigure() {

    _ntp_configure = false;

    int offset = getSetting("ntpOffset", NTP_TIME_OFFSET).toInt();
    int sign = offset > 0 ? 1 : -1;
    offset = abs(offset);

    NTP.begin(
        getSetting("ntpServer", 1, NTP_SERVER).c_str(),
        sign * (offset / 60),
        getSetting("ntpDST", NTP_DAY_LIGHT).toInt() == 1,
        sign * (offset % 60)
    );

    if (hasSetting("ntpServer", 2)) NTP.setNtpServerName(getSetting("ntpServer", 2).c_str(), 1);
    if (hasSetting("ntpServer", 3)) NTP.setNtpServerName(getSetting("ntpServer", 3).c_str(), 2);
    NTP.setInterval(NTP_UPDATE_INTERVAL);

    _ntp_update = true;

}

void _ntpLoop() {

    if (_ntp_configure) _ntpConfigure();
    if (_ntp_update) _ntpUpdate();

    now();

    #if BROKER_SUPPORT
        static unsigned char last_minute = 60;
        if (ntpSynced() && (minute() != last_minute)) {
            last_minute = minute();
            brokerPublish(MQTT_TOPIC_DATETIME, ntpDateTime().c_str());
        }
    #endif

}

void _ntpBackwards() {
    int offset = getSetting("ntpOffset", NTP_TIME_OFFSET).toInt();
    if (-30 < offset && offset < 30) {
        offset *= 60;
        setSetting("ntpOffset", offset);
    }
}

// -----------------------------------------------------------------------------

bool ntpSynced() {
    return (timeStatus() == timeSet);
}

String ntpDateTime() {
    if (!ntpSynced()) return String();
    char buffer[20];
    time_t t = now();
    snprintf_P(buffer, sizeof(buffer),
        PSTR("%04d-%02d-%02d %02d:%02d:%02d"),
        year(t), month(t), day(t), hour(t), minute(t), second(t)
    );
    return String(buffer);
}

// -----------------------------------------------------------------------------

void ntpSetup() {

    _ntpBackwards();

    NTP.onNTPSyncEvent([](NTPSyncEvent_t error) {
        if (error) {
            #if WEB_SUPPORT
                wsSend_P(PSTR("{\"ntpStatus\": false}"));
            #endif
            if (error == noResponse) {
                DEBUG_MSG_P(PSTR("[NTP] Error: NTP server not reachable\n"));
            } else if (error == invalidAddress) {
                DEBUG_MSG_P(PSTR("[NTP] Error: Invalid NTP server address\n"));
            }
        } else {
            _ntp_update = true;
        }
    });

    wifiRegister([](justwifi_messages_t code, char * parameter) {
        if (code == MESSAGE_CONNECTED) _ntp_configure = true;
    });

    #if WEB_SUPPORT
        wsOnSendRegister(_ntpWebSocketOnSend);
        wsOnAfterParseRegister([]() { _ntp_configure = true; });
    #endif

    // Register loop
    espurnaRegisterLoop(_ntpLoop);

}

#endif // NTP_SUPPORT
