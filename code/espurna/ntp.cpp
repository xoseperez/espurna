/*

NTP MODULE

Based on esp8266 / esp32 configTime and C date and time functions:
- https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/NTP-TZ-DST/NTP-TZ-DST.ino
- https://www.nongnu.org/lwip/2_1_x/group__sntp.html
- man 3 ctime

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "ntp.h"

#if NTP_SUPPORT && !NTP_LEGACY_SUPPORT

#include <Arduino.h>
#include <coredecls.h>
#include <Ticker.h>

static_assert(
    (SNTP_SERVER_DNS == 1),
    "lwip must be configured with SNTP_SERVER_DNS"
);

#include "config/buildtime.h"
#include "debug.h"
#include "broker.h"
#include "ws.h"

// Arduino/esp8266 lwip2 custom functions that can be redefined
// Must return time in milliseconds, legacy settings are in seconds.

String _ntp_server;

uint32_t _ntp_startup_delay = (NTP_START_DELAY * 1000);
uint32_t _ntp_update_delay = (NTP_UPDATE_INTERVAL * 1000);

uint32_t sntp_startup_delay_MS_rfc_not_less_than_60000() {
    return _ntp_startup_delay;
}

uint32_t sntp_update_delay_MS_rfc_not_less_than_15000() {
    return _ntp_update_delay;
}

// We also must shim TimeLib functions until everything else is ported.
// We can't sometimes avoid TimeLib as dependancy though, which would be really bad

static Ticker _ntp_broker_timer;
static bool _ntp_synced = false;

static time_t _ntp_last = 0;
static time_t _ntp_ts = 0;

static tm _ntp_tm_local;
static tm _ntp_tm_utc;

void _ntpTmCache(time_t ts) {
    if (_ntp_ts != ts) {
        _ntp_ts = ts;
        localtime_r(&_ntp_ts, &_ntp_tm_local);
        gmtime_r(&_ntp_ts, &_ntp_tm_utc);
    }
}

int hour(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_local.tm_hour;
}

int minute(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_local.tm_min;
}

int second(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_local.tm_sec;
}

int day(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_local.tm_mday;
}

// `tm.tm_wday` range is 0..6, TimeLib is 1..7
int weekday(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_local.tm_wday + 1;
}

// `tm.tm_mon` range is 0..11, TimeLib range is 1..12
int month(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_local.tm_mon + 1;
}

int year(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_local.tm_year + 1900;
}

int utc_hour(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_utc.tm_hour;
}

int utc_minute(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_utc.tm_min;
}

int utc_second(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_utc.tm_sec;
}

int utc_day(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_utc.tm_mday;
}

int utc_weekday(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_utc.tm_wday + 1;
}

int utc_month(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_utc.tm_mon + 1;
}

int utc_year(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_utc.tm_year + 1900;
}

time_t now() {
    return time(nullptr);
}

// -----------------------------------------------------------------------------

#if WEB_SUPPORT

bool _ntpWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "ntp", 3) == 0);
}

void _ntpWebSocketOnVisible(JsonObject& root) {
    root["ntpVisible"] = 1;
    root["ntplwipVisible"] = 1;
}

void _ntpWebSocketOnData(JsonObject& root) {
    root["ntpStatus"] = ntpSynced();
}

void _ntpWebSocketOnConnected(JsonObject& root) {
    root["ntpServer"] = getSetting("ntpServer", F(NTP_SERVER));
    root["ntpTZ"] = getSetting("ntpTZ", NTP_TIMEZONE);
}

#endif

// TODO: mention possibility of multiple servers
String _ntpGetServer() {
    String server;

    server = sntp_getservername(0);
    if (!server.length()) {
        server = IPAddress(sntp_getserver(0)).toString();
    }

    return server;
}

void _ntpReport() {
    if (!ntpSynced()) {
        DEBUG_MSG_P(PSTR("[NTP] Not synced\n")); 
        return;
    }

    tm utc_tm;
    tm sync_tm;

    auto ts = now();
    gmtime_r(&ts, &utc_tm);
    gmtime_r(&_ntp_last, &sync_tm);

    DEBUG_MSG_P(PSTR("[NTP] Server     : %s\n"), _ntp_server.c_str());
    DEBUG_MSG_P(PSTR("[NTP] Sync Time  : %s (UTC)\n"), ntpDateTime(&sync_tm).c_str());
    DEBUG_MSG_P(PSTR("[NTP] UTC Time   : %s\n"), ntpDateTime(&utc_tm).c_str());

    const char* cfg_tz = getenv("TZ");
    if ((cfg_tz != nullptr) && (strcmp(cfg_tz, "UTC0") != 0)) {
        tm local_tm;
        localtime_r(&ts, &local_tm);
        DEBUG_MSG_P(PSTR("[NTP] Local Time : %s (%s)\n"),
            ntpDateTime(&local_tm).c_str(), cfg_tz
        );
    }
}

void _ntpConfigure() {
    // Note: TZ_... provided by the Core are already wrapped with PSTR(...)
    const auto cfg_tz = getSetting("ntpTZ", NTP_TIMEZONE);
    const char* active_tz = getenv("TZ");
    if (cfg_tz != active_tz) {
        setenv("TZ", cfg_tz.c_str(), 1);
        tzset();
    }
    
    const auto cfg_server = getSetting("ntpServer", F(NTP_SERVER));
    const auto active_server = _ntpGetServer();
    if (cfg_tz != active_tz) {
        _ntp_server = cfg_server;
        configTime(cfg_tz.c_str(), _ntp_server.c_str());
        DEBUG_MSG_P(PSTR("[NTP] Server: %s, TZ: %s\n"), cfg_server.c_str(), cfg_tz.length() ? cfg_tz.c_str() : "UTC0");
    }
}

// -----------------------------------------------------------------------------

bool ntpSynced() {
    return _ntp_synced;
}

String ntpDateTime(tm* timestruct) {
    char buffer[20];
    snprintf_P(buffer, sizeof(buffer),
        PSTR("%04d-%02d-%02d %02d:%02d:%02d"),
        timestruct->tm_year + 1900,
        timestruct->tm_mon + 1,
        timestruct->tm_mday,
        timestruct->tm_hour,
        timestruct->tm_min,
        timestruct->tm_sec
    );
    return String(buffer);
}

String ntpDateTime(time_t ts) {
    tm timestruct;
    localtime_r(&ts, &timestruct);
    return ntpDateTime(&timestruct);
}

String ntpDateTime() {
    if (ntpSynced()) {
        return ntpDateTime(now());
    }
    return String();
}

// -----------------------------------------------------------------------------

#if BROKER_SUPPORT

void _ntpBrokerSchedule(int offset);

void _ntpBrokerCallback() {

    if (!ntpSynced()) {
        _ntpBrokerSchedule(60);
        return;
    }

    const auto ts = now();

    // current  time and formatter string is in local TZ
    tm local_tm;
    localtime_r(&ts, &local_tm);

    int now_hour = local_tm.tm_hour;
    int now_minute = local_tm.tm_min;

    static int last_hour = -1;
    static int last_minute = -1;

    String datetime;
    if ((last_minute != now_minute) || (last_hour != now_hour)) {
        datetime = ntpDateTime(&local_tm);
    }

    // notify subscribers about each tick interval (note that both can happen simultaneously)
    if (last_hour != now_hour) {
        last_hour = now_hour;
        NtpBroker::Publish(NtpTick::EveryHour, ts, datetime.c_str());
    }

    if (last_minute != now_minute) {
        last_minute = now_minute;
        NtpBroker::Publish(NtpTick::EveryMinute, ts, datetime.c_str());
    }

    // try to autocorrect each invocation
    _ntpBrokerSchedule(60 - local_tm.tm_sec);

}

// XXX: Nonos docs for some reason mention 100 micros as minimum time. Schedule next second in case this is 0
void _ntpBrokerSchedule(int offset) {
    _ntp_broker_timer.once_scheduled(offset ?: 1, _ntpBrokerCallback);
}

#endif

void _ntpSetTimeOfDayCallback() {
    _ntp_synced = true;
    _ntp_last = time(nullptr);
    #if BROKER_SUPPORT
    static bool once = true;
    if (once) {
        schedule_function(_ntpBrokerCallback);
        once = false;
    }
    #endif
    #if WEB_SUPPORT
        wsPost(_ntpWebSocketOnData);
    #endif
    schedule_function(_ntpReport);
}

void _ntpSetTimestamp(time_t ts) {
    timeval tv { ts, 0 };
    timezone tz { 0, 0 };
    settimeofday(&tv, &tz);
}

// -----------------------------------------------------------------------------

void ntpSetup() {

    // Randomized in time to avoid clogging the server with simultaneous requests from multiple devices
    // (for example, when multiple devices start up at the same time)
    const uint32_t startup_delay = getSetting("ntpStartDelay", NTP_START_DELAY);
    const uint32_t update_delay = getSetting("ntpUpdateIntvl", NTP_UPDATE_INTERVAL);

    _ntp_startup_delay = secureRandom(startup_delay, startup_delay * 2);
    _ntp_update_delay = secureRandom(update_delay, update_delay * 2);
    DEBUG_MSG_P(PSTR("[NTP] Startup delay: %us, Update delay: %us\n"),
        _ntp_startup_delay, _ntp_update_delay
    );

    _ntp_startup_delay = _ntp_startup_delay * 1000;
    _ntp_update_delay = _ntp_update_delay * 1000;

    // start up with some reasonable timestamp already available
    _ntpSetTimestamp(__UNIX_TIMESTAMP__);

    // will be called every time after ntp syncs AND loop() finishes
    settimeofday_cb(_ntpSetTimeOfDayCallback);

    // generic configuration, always handled
    espurnaRegisterReload(_ntpConfigure);
    _ntpConfigure();

    // make sure our logic does know about the actual server
    // in case dhcp sends out ntp settings
    static WiFiEventHandler on_sta = WiFi.onStationModeGotIP([](WiFiEventStationModeGotIP) {
        const auto server = _ntpGetServer();
        if (sntp_enabled() && (!_ntp_server.length() || (server != _ntp_server))) {
            DEBUG_MSG_P(PSTR("[NTP] Updating `ntpServer` setting from DHCP: %s\n"), server.c_str());
            _ntp_server = server;
            setSetting("ntpServer", server);
        }
    });

    // optional functionality
    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_ntpWebSocketOnVisible)
            .onConnected(_ntpWebSocketOnConnected)
            .onData(_ntpWebSocketOnData)
            .onKeyCheck(_ntpWebSocketOnKeyCheck);
    #endif

    #if TERMINAL_SUPPORT
        terminalRegisterCommand(F("NTP"), [](Embedis* e) {
            _ntpReport();
            terminalOK();
        });

        terminalRegisterCommand(F("NTP.SETTIME"), [](Embedis* e) {
            if (e->argc != 2) return;
            _ntp_synced = true;
            _ntpSetTimestamp(String(e->argv[1]).toInt());
            terminalOK();
        });

        // TODO:
        // terminalRegisterCommand(F("NTP.SYNC"), [](Embedis* e) { ... }
        //
    #endif

}

#endif // NTP_SUPPORT && !NTP_LEGACY_SUPPORT
