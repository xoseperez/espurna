#if !NTP_LEGACY_SUPPORT && NTP_SUPPORT

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
#include "ntp.h"

// Arduino/esp8266 lwip2 custom functions that can be redefined
// Must return time in milliseconds, legacy settings are in seconds.

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

static void _ntpTmCache(time_t ts) {
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

int weekday(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_local.tm_wday;
}

int month(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_local.tm_mon + 1;
}

int year(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_local.tm_year + 1900;
}

int utc_weekday(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_utc.tm_wday;
}
int utc_hour(time_t ts) {
    _ntpTmCache(ts);
    return _ntp_tm_utc.tm_hour;
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
    root["ntplegacyVisible"] = 0;
}

void _ntpWebSocketOnData(JsonObject& root) {
    root["ntpStatus"] = ntpSynced();
}

void _ntpWebSocketOnConnected(JsonObject& root) {
    root["ntpServer"] = getSetting("ntpServer", NTP_SERVER);
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

    DEBUG_MSG_P(PSTR("[NTP] Server     : %s\n"), _ntpGetServer().c_str());
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

bool ntpSynced() {
    return _ntp_synced;
}

void _ntpSetTimeOfDayCallback() {
    _ntp_synced = true;
    _ntp_last = time(nullptr);
    #if BROKER_SUPPORT
        // XXX: Nonos docs for some reason mention 100 micros as minimum time. Schedule next second in case this is 0
        _ntp_broker_timer.once((60 - second(_ntp_last)) ?: 1, _ntpBrokerCallback);
    #endif
    schedule_function(_ntpReport);
}

void _ntpConfigure() {
    const auto tz = getSetting("ntpTZ", F(NTP_TIMEZONE));
    const auto server = getSetting("ntpServer", F(NTP_SERVER));
    if (!tz.length() || !server.length()) {
        return;
    }
    
    const auto cfg_server = _ntpGetServer();
    const char* cfg_tz = getenv("TZ");

    if (!server.equals(cfg_server) || !tz.equals(cfg_tz)) {
        DEBUG_MSG_P(PSTR("[NTP] Server: %s, TZ: %s\n"), server.c_str(), tz.c_str());
        configTime(tz.c_str(), server.c_str());
        if (ntpSynced()) _ntp_synced = false;
    }
}

// --- public

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

#if BROKER_SUPPORT

void _ntpBrokerCallback() {
    if (ntpSynced()) {
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

        // notify subscribers about each tick interval (note that both can happen simultaiously)
        if (last_hour != now_hour) {
            last_hour = now_hour;
            NtpBroker::Publish(NtpTick::EveryHour, ts, datetime.c_str());
        }

        if (last_minute != now_minute) {
            last_minute = now_minute;
            NtpBroker::Publish(NtpTick::EveryMinute, ts, datetime.c_str());
        }

        // try to autocorrect each invocation
        _ntp_broker_timer.once((60 - second(ts)) ?: 1, _ntpBrokerCallback);

        return;
    }

    _ntp_broker_timer.once(60, _ntpBrokerCallback);
}

#endif

void _ntpSetTimestamp(time_t ts) {
    timeval tv { ts, 0 };
    timezone tz { 0, 0 };
    settimeofday(&tv, &tz);
}

void ntpSetup() {

    // Randomized in time to avoid clogging the server with simultaious requests from multiple devices
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
    schedule_function(_ntpConfigure);

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
    #endif

}

#endif
