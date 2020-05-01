/*

NTP MODULE (based on NtpClientLib)

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "ntp.h"

#if NTP_LEGACY_SUPPORT && NTP_SUPPORT

#include <Ticker.h>

#include "debug.h"
#include "broker.h"
#include "ws.h"

Ticker _ntp_defer;

bool _ntp_report = false;
bool _ntp_configure = false;
bool _ntp_want_sync = false;

// -----------------------------------------------------------------------------
// NtpClient overrides to avoid triggering network sync
// -----------------------------------------------------------------------------

class NTPClientWrap : public NTPClient {

public:

    NTPClientWrap() : NTPClient() {
        udp = new WiFiUDP();
        _lastSyncd = 0;
    }

    bool setInterval(int shortInterval, int longInterval) {
        _shortInterval = shortInterval;
        _longInterval = longInterval;
        return true;
    }

};

// NOTE: original NTP should be discarded by the linker
// TODO: allow NTP client object to be destroyed
static NTPClientWrap NTPw;

// -----------------------------------------------------------------------------
// NTP
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

bool _ntpWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "ntp", 3) == 0);
}

void _ntpWebSocketOnVisible(JsonObject& root) {
    root["ntpVisible"] = 1;
    root["ntplegacyVisible"] = 1;
}

void _ntpWebSocketOnData(JsonObject& root) {
    root["ntpStatus"] = (timeStatus() == timeSet);
}

void _ntpWebSocketOnConnected(JsonObject& root) {
    root["ntpServer"] = getSetting("ntpServer", NTP_SERVER);
    root["ntpOffset"] = getSetting("ntpOffset", NTP_TIME_OFFSET);
    root["ntpDST"] = getSetting("ntpDST", 1 == NTP_DAY_LIGHT);
    root["ntpRegion"] = getSetting("ntpRegion", NTP_DST_REGION);
}

#endif

time_t _ntpSyncProvider() {
    _ntp_want_sync = true;
    return 0;
}

void _ntpWantSync() {
    _ntp_want_sync = true;
}

// Randomized in time to avoid clogging the server with simultaious requests from multiple devices
// (for example, when multiple devices start up at the same time)
int _ntpSyncInterval() {
    return secureRandom(NTP_SYNC_INTERVAL, NTP_SYNC_INTERVAL * 2);
}

int _ntpUpdateInterval() {
    return secureRandom(NTP_UPDATE_INTERVAL, NTP_UPDATE_INTERVAL * 2);
}

void _ntpConfigure() {

    _ntp_configure = false;

    int offset = getSetting("ntpOffset", NTP_TIME_OFFSET);
    int sign = offset > 0 ? 1 : -1;
    offset = abs(offset);
    int tz_hours = sign * (offset / 60);
    int tz_minutes = sign * (offset % 60);
    if (NTPw.getTimeZone() != tz_hours || NTPw.getTimeZoneMinutes() != tz_minutes) {
        NTPw.setTimeZone(tz_hours, tz_minutes);
        _ntp_report = true;
    }

    const bool daylight = getSetting("ntpDST", 1 == NTP_DAY_LIGHT);
    if (NTPw.getDayLight() != daylight) {
        NTPw.setDayLight(daylight);
        _ntp_report = true;
    }

    const auto server = getSetting("ntpServer", NTP_SERVER);
    if (!NTPw.getNtpServerName().equals(server)) {
        NTPw.setNtpServerName(server);
    }

    uint8_t dst_region = getSetting("ntpRegion", NTP_DST_REGION);
    NTPw.setDSTZone(dst_region);

    // Some remote servers can be slow to respond, increase accordingly
    // TODO does this need upper constrain?
    NTPw.setNTPTimeout(getSetting("ntpTimeout", NTP_TIMEOUT));

}

void _ntpStart() {

    _ntpConfigure();

    // short (initial) and long (after sync) intervals
    NTPw.setInterval(_ntpSyncInterval(), _ntpUpdateInterval());
    DEBUG_MSG_P(PSTR("[NTP] Update intervals: %us / %us\n"),
        NTPw.getShortInterval(), NTPw.getLongInterval());

    // setSyncProvider will immediatly call given function by setting next sync time to the current time.
    // Avoid triggering sync immediatly by canceling sync provider flag and resetting sync interval again
    setSyncProvider(_ntpSyncProvider);
    _ntp_want_sync = false;

    setSyncInterval(NTPw.getShortInterval());

}


void _ntpReport() {

    _ntp_report = false;

    #if DEBUG_SUPPORT
    if (ntpSynced()) {
        time_t t = now();
        DEBUG_MSG_P(PSTR("[NTP] UTC Time  : %s\n"), ntpDateTime(ntpLocal2UTC(t)).c_str());
        DEBUG_MSG_P(PSTR("[NTP] Local Time: %s\n"), ntpDateTime(t).c_str());
    }
    #endif

}

#if BROKER_SUPPORT

void inline _ntpBroker() {
    static unsigned char last_minute = 60;
    if (ntpSynced() && (minute() != last_minute)) {
        last_minute = minute();
        NtpBroker::Publish(NtpTick::EveryMinute, now(), ntpDateTime());
    }
}

#endif

void _ntpLoop() {

    // Disable ntp sync when softAP is active. This will not crash, but instead spam debug-log with pointless sync failures.
    if (!wifiConnected()) return;

    if (_ntp_configure) _ntpConfigure();

    // NTPClientLib will trigger callback with sync status
    // see: NTPw.onNTPSyncEvent([](NTPSyncEvent_t error){ ... }) below
    if (_ntp_want_sync) {
        _ntp_want_sync = false;
        NTPw.getTime();
    }

    // Print current time whenever configuration changes or after successful sync
    if (_ntp_report) _ntpReport();

    #if BROKER_SUPPORT
        _ntpBroker();
    #endif

}

// TODO: remove me!
void _ntpBackwards() {
    moveSetting("ntpServer1", "ntpServer");
    delSetting("ntpServer2");
    delSetting("ntpServer3");
    int offset = getSetting("ntpOffset", NTP_TIME_OFFSET);
    if (-30 < offset && offset < 30) {
        offset *= 60;
        setSetting("ntpOffset", offset);
    }
}

// -----------------------------------------------------------------------------

bool ntpSynced() {
    #if NTP_WAIT_FOR_SYNC
        // Has synced at least once
        return (NTPw.getFirstSync() > 0);
    #else
        // TODO: runtime setting?
        return true;
    #endif
}

String ntpDateTime(time_t t) {
    char buffer[20];
    snprintf_P(buffer, sizeof(buffer),
        PSTR("%04d-%02d-%02d %02d:%02d:%02d"),
        year(t), month(t), day(t), hour(t), minute(t), second(t)
    );
    return String(buffer);
}

String ntpDateTime() {
    if (ntpSynced()) return ntpDateTime(now());
    return String();
}

// XXX: returns garbage during DST switch
time_t ntpLocal2UTC(time_t local) {
    int offset = getSetting("ntpOffset", NTP_TIME_OFFSET);
    if (NTPw.isSummerTime()) offset += 60;
    return local - offset * 60;
}

// -----------------------------------------------------------------------------

void ntpSetup() {

    _ntpBackwards();

    #if TERMINAL_SUPPORT
        terminalRegisterCommand(F("NTP"), [](Embedis* e) {
            if (ntpSynced()) {
                _ntpReport();
                terminalOK();
            } else {
                DEBUG_MSG_P(PSTR("[NTP] Not synced\n"));
            }
        });

        terminalRegisterCommand(F("NTP.SYNC"), [](Embedis* e) {
            _ntpWantSync();
            terminalOK();
        });
    #endif

    NTPw.onNTPSyncEvent([](NTPSyncEvent_t error) {
        if (error) {
            if (error == noResponse) {
                DEBUG_MSG_P(PSTR("[NTP] Error: NTP server not reachable\n"));
            } else if (error == invalidAddress) {
                DEBUG_MSG_P(PSTR("[NTP] Error: Invalid NTP server address\n"));
            }
            #if WEB_SUPPORT
                wsPost(_ntpWebSocketOnData);
            #endif
        } else {
            _ntp_report = true;
            setTime(NTPw.getLastNTPSync());
        }
    });

    wifiRegister([](justwifi_messages_t code, char * parameter) {
        if (code == MESSAGE_CONNECTED) {
            if (!ntpSynced()) {
                _ntp_defer.once(secureRandom(NTP_START_DELAY, NTP_START_DELAY * 2), _ntpWantSync);
            }
        }
    });

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_ntpWebSocketOnVisible)
            .onConnected(_ntpWebSocketOnConnected)
            .onData(_ntpWebSocketOnData)
            .onKeyCheck(_ntpWebSocketOnKeyCheck);
    #endif

    // Main callbacks
    espurnaRegisterLoop(_ntpLoop);
    espurnaRegisterReload([]() { _ntp_configure = true; });

    // Sets up NTP instance, installs ours sync provider
    _ntpStart();

}

#endif // NTP_SUPPORT && NTP_LEGACY_SUPPORT
