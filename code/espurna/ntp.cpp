/*

NTP MODULE

Based on esp8266 / esp32 configTime and C date and time functions:
- https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/NTP-TZ-DST/NTP-TZ-DST.ino
- https://www.nongnu.org/lwip/2_1_x/group__sntp.html
- man 3 ctime

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if NTP_SUPPORT

#include <Arduino.h>
#include <coredecls.h>
#include <Ticker.h>

#include <ctime>
#include <errno.h>
#include <lwip/apps/sntp.h>
#include <TZ.h>

#include <forward_list>

static_assert(
    (SNTP_SERVER_DNS == 1),
    "lwip must be configured with SNTP_SERVER_DNS"
);

#include "config/buildtime.h"

#include "ntp.h"
#include "ntp_timelib.h"
#include "ws.h"

// Arduino/esp8266 lwip2 custom functions that can be redefined
// Must return time in milliseconds, settings are in seconds.

namespace {

constexpr espurna::duration::Seconds NtpStartDelay { NTP_START_DELAY };
constexpr espurna::duration::Seconds NtpUpdateInterval { NTP_UPDATE_INTERVAL };

espurna::duration::Seconds _ntp_startup_delay { NtpStartDelay };
espurna::duration::Seconds _ntp_update_delay { NtpUpdateInterval };

espurna::duration::Seconds _ntpRandomizeDelay(espurna::duration::Seconds base) {
    return espurna::duration::Seconds(
        secureRandom(base.count(), (2 * base.count())));
}

} // namespace

uint32_t sntp_startup_delay_MS_rfc_not_less_than_60000() {
    static_assert(sizeof(decltype(_ntp_startup_delay)::rep) <= sizeof(uint32_t), "");
    return espurna::duration::Milliseconds(_ntp_startup_delay).count();
}

uint32_t sntp_update_delay_MS_rfc_not_less_than_15000() {
    static_assert(sizeof(decltype(_ntp_update_delay)::rep) <= sizeof(uint32_t), "");
    return espurna::duration::Milliseconds(_ntp_update_delay).count();
}

// We also must shim TimeLib functions until everything else is ported.
// We can't sometimes avoid TimeLib as dependancy though, which would be really bad

namespace {

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

} // namespace

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

namespace {

#if WEB_SUPPORT

bool _ntpWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "ntp", 3) == 0);
}

void _ntpWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, "ntp");
}

void _ntpWebSocketOnData(JsonObject& root) {
    root["ntpStatus"] = ntpSynced();
}

void _ntpWebSocketOnConnected(JsonObject& root) {
    root["ntpServer"] = getSetting("ntpServer", F(NTP_SERVER));
    root["ntpTZ"] = getSetting("ntpTZ", NTP_TIMEZONE);
}

#endif

String _ntpGetServer() {
    String server;

    server = sntp_getservername(0);
    if (!server.length()) {
        auto ip = IPAddress(sntp_getserver(0));
        if (ip) {
            server = ip.toString();
        }
    }

    return server;
}

} // namespace

NtpInfo ntpInfo() {
    NtpInfo result;

    auto ts = now();
    result.now = ts;

    tm sync_tm;
    gmtime_r(&_ntp_last, &sync_tm);
    result.sync = ntpDateTime(&sync_tm);

    tm utc_tm;
    gmtime_r(&ts, &utc_tm);
    result.utc = ntpDateTime(&utc_tm);

    const char* cfg_tz = getenv("TZ");
    if ((cfg_tz != nullptr) && (strcmp(cfg_tz, "UTC0") != 0)) {
        tm local_tm;
        localtime_r(&ts, &local_tm);
        result.local = ntpDateTime(&local_tm);
        result.tz = cfg_tz;
    }

    return result;
}

namespace {

String _ntp_server;

#if TERMINAL_SUPPORT

void _ntpReportTerminal(Print& out) {
    const auto info = ntpInfo();
    out.printf_P(PSTR("server: %s\nsync time: %s\nutc time: %s\n"),
        _ntp_server.c_str(),
        info.sync.c_str(),
        info.utc.c_str());

    if (info.tz.length()) {
        out.printf_P(PSTR("local time: %s (%s)\n"),
            info.local.c_str(), info.tz.c_str());
    }
}

#endif

void _ntpReport() {
    if (!ntpSynced()) {
        DEBUG_MSG_P(PSTR("[NTP] Not synced\n"));
        return;
    }

    const auto info = ntpInfo();
    DEBUG_MSG_P(PSTR("[NTP] Server     %s\n"), _ntp_server.c_str());
    DEBUG_MSG_P(PSTR("[NTP] Sync Time  %s (UTC)\n"), info.sync.c_str());
    DEBUG_MSG_P(PSTR("[NTP] UTC Time   %s\n"), info.utc.c_str());

    if (info.tz.length()) {
        DEBUG_MSG_P(PSTR("[NTP] Local Time %s (%s)\n"),
            info.local.c_str(), info.tz.c_str());
    }
}

void _ntpConfigure() {
    // Ignore or accept the DHCP SNTP option
    // When enabled, it is possible that lwip will replace the NTP server pointer from under us
    sntp_servermode_dhcp(getSetting("ntpDhcp", 1 == NTP_DHCP_SERVER));

    // Note: TZ_... provided by the Core are already wrapped with PSTR(...)
    // but, String() already handles every char pointer as a flash-string
    auto cfg_tz = getSetting("ntpTZ", NTP_TIMEZONE);
    const char* active_tz = getenv("TZ");

    bool changed = cfg_tz != active_tz;
    if (changed) {
        if (cfg_tz.length()) {
            setenv("TZ", cfg_tz.c_str(), 1);
        } else {
            unsetenv("TZ");
        }
        tzset();
    }

    const auto cfg_server = getSetting("ntpServer", F(NTP_SERVER));
    const auto active_server = _ntpGetServer();
    changed = (cfg_server != active_server) || changed;

    // We skip configTime() API since we already set the TZ just above
    // (and most of the time we expect NTP server to proxy to multiple servers instead of defining more than one here)
    if (changed) {
        sntp_stop();
        _ntp_server = cfg_server;
        sntp_setservername(0, _ntp_server.c_str());
        sntp_init();
        DEBUG_MSG_P(PSTR("[NTP] Server: %s, TZ: %s\n"), cfg_server.c_str(),
                cfg_tz.length() ? cfg_tz.c_str() : "UTC0");
    }
}

} // namespace

// -----------------------------------------------------------------------------

bool ntpSynced() {
    return _ntp_synced;
}

String ntpDateTime(tm* timestruct) {
    char buffer[32];
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

namespace {

static constexpr espurna::duration::Seconds NtpTickOffsetMin { 1 };
static constexpr espurna::duration::Seconds NtpTickOffsetMax { 60 };

using NtpTickCallbacks = std::forward_list<NtpTickCallback>;
NtpTickCallbacks _ntp_tick_callbacks;

Ticker _ntp_tick;

void _ntpTickSchedule(espurna::duration::Seconds offset);

void _ntpTickCallback() {
    if (!ntpSynced()) {
        _ntpTickSchedule(NtpTickOffsetMax);
        return;
    }

    const auto ts = now();
    tm local_tm;
    localtime_r(&ts, &local_tm);

    int now_hour = local_tm.tm_hour;
    int now_minute = local_tm.tm_min;

    static int last_hour = -1;
    static int last_minute = -1;

    // notify subscribers about each tick interval (note that both can happen simultaneously)
    if (last_hour != now_hour) {
        last_hour = now_hour;
        for (auto& callback : _ntp_tick_callbacks) {
            callback(NtpTick::EveryHour);
        }
    }

    if (last_minute != now_minute) {
        last_minute = now_minute;
        for (auto& callback : _ntp_tick_callbacks) {
            callback(NtpTick::EveryMinute);
        }
    }

    // try to autocorrect each invocation
    _ntpTickSchedule(NtpTickOffsetMax - espurna::duration::Seconds(local_tm.tm_sec));
}

void _ntpTickSchedule(espurna::duration::Seconds offset) {
    static bool scheduled { false };

    // Never allow delays less than a second, or greater than a minute
    // (ref. Non-OS 3.1.1 os_timer_arm, actual minimal value is 5ms)
    if (!scheduled) {
        scheduled = true;
        _ntp_tick.once_scheduled(
            std::clamp(offset, NtpTickOffsetMin, NtpTickOffsetMax).count(),
            []() {
                scheduled = false;
                _ntpTickCallback();
            });
    }
}

void _ntpSetTimeOfDayCallback() {
    _ntp_synced = true;
    _ntp_last = time(nullptr);
    static bool once = true;
    if (once) {
        schedule_function(_ntpTickCallback);
        once = false;
    }
#if WEB_SUPPORT
    wsPost(_ntpWebSocketOnData);
#endif
    schedule_function(_ntpReport);
}

bool _ntpSetTimestamp(time_t ts) {
    const timeval tv {
        .tv_sec = ts,
        .tv_usec = 0
    };

    return EINVAL != settimeofday(&tv, nullptr);
}

struct NtpParseResult {
    NtpParseResult() = default;
    explicit NtpParseResult(time_t value) :
        _result(true),
        _timestamp(value)
    {}

    NtpParseResult& operator=(time_t value) {
        _result = true;
        _timestamp = value;
        return *this;
    }

    explicit operator bool() const {
        return _result;
    }

    time_t timestamp() const {
        return _timestamp;
    }

private:
    bool _result { false };
    time_t _timestamp;
};

template <typename T>
struct NtpParseImpl {
};

template <>
struct NtpParseImpl<long> {
    using Type = long(*)(const char*, char**, int);
    static const Type Func;
};

const NtpParseImpl<long>::Type NtpParseImpl<long>::Func = strtol;

template <>
struct NtpParseImpl<long long> {
    using Type = long long(*)(const char*, char**, int);
    static const Type Func;
};

const NtpParseImpl<long long>::Type NtpParseImpl<long long>::Func = strtoll;

NtpParseResult _ntpParseTimestamp(const String& value) {
    NtpParseResult out;

    const char* p { value.c_str() };
    char* endp { nullptr };

    auto result = NtpParseImpl<time_t>::Func(p, &endp, 10);
    if (!endp || (endp == p)) {
        return out;
    }

    out = result;
    return out;
}

} // namespace

// -----------------------------------------------------------------------------

namespace {

void _ntpConvertLegacyOffsets() {
    bool save { true };
    bool found { false };

    bool europe { true };
    bool dst { true };
    int offset { 60 };

    settings::internal::foreach([&](settings::kvs_type::KeyValueResult&& kv) {
        const auto key = kv.key.read();
        if (key == F("ntpTZ")) {
            save = false;
        } else if (key == F("ntpOffset")) {
            offset = settings::internal::convert<int>(kv.value.read());
            found = true;
        } else if (key == F("ntpDST")) {
            dst = settings::internal::convert<bool>(kv.value.read());
            found = true;
        } else if (key == F("ntpRegion")) {
            europe = (0 == settings::internal::convert<int>(kv.value.read()));
            found = true;
        }
    });

    if (save && found) {
        // XXX: only expect offsets in hours
        String custom { europe ? F("CET") : F("CST") };
        custom.reserve(32);

        if (offset > 0) {
            custom += '-';
        }
        custom += abs(offset) / 60;

        if (dst) {
            custom += europe ? F("CEST") : F("EDT");
            if (europe) {
                custom += F(",M3.5.0,M10.5.0/3");
            } else {
                custom += F(",M3.2.0,M11.1.0");
            }
        }

        setSetting("ntpTZ", custom);
    }

    delSetting("ntpOffset");
    delSetting("ntpDST");
    delSetting("ntpRegion");
}

} // namespace

void ntpOnTick(NtpTickCallback callback) {
    _ntp_tick_callbacks.push_front(callback);
}

void ntpSetup() {

    // Randomized in time to avoid clogging the server with simultaneous requests from multiple devices
    // (for example, when multiple devices start up at the same time)
    const auto startup_delay = getSetting("ntpStartDelay", NtpStartDelay);
    const auto update_delay = getSetting("ntpUpdateIntvl", NtpUpdateInterval);

    _ntp_startup_delay = _ntpRandomizeDelay(startup_delay);
    _ntp_update_delay = _ntpRandomizeDelay(update_delay);
    DEBUG_MSG_P(PSTR("[NTP] Startup delay: %u (s), Update delay: %u (s)\n"),
        _ntp_startup_delay.count(), _ntp_update_delay.count());

    // start up with some reasonable timestamp already available
    _ntpSetTimestamp(__UNIX_TIMESTAMP__);

    // will be called every time after ntp syncs AND loop() finishes
    settimeofday_cb(_ntpSetTimeOfDayCallback);

    // generic configuration, always handled
    espurnaRegisterReload(_ntpConfigure);
    _ntpConvertLegacyOffsets();
    _ntpConfigure();

    // make sure our logic does know about the actual server
    // in case dhcp sends out ntp settings
    static WiFiEventHandler on_sta = WiFi.onStationModeGotIP([](WiFiEventStationModeGotIP) {
        if (!sntp_enabled()) {
            return;
        }

        auto server = _ntpGetServer();
        if (!server.length()) {
            DEBUG_MSG_P(PSTR("[NTP] Updating `ntpDhcp` to ignore the DHCP values\n"));
            setSetting("ntpDhcp", "0");
            sntp_servermode_dhcp(0);
            schedule_function(_ntpConfigure);
            return;
        }

        if (!_ntp_server.length() || (server != _ntp_server)) {
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
        terminalRegisterCommand(F("NTP"), [](::terminal::CommandContext&& ctx) {
            if (ntpSynced()) {
                _ntpReportTerminal(ctx.output);
                terminalOK(ctx);
                return;
            }

            terminalError(ctx, F("NTP not synced"));
        });

        terminalRegisterCommand(F("NTP.SYNC"), [](::terminal::CommandContext&& ctx) {
            if (_ntp_synced) {
                sntp_stop();
                sntp_init();
                terminalOK(ctx);
                return;
            }

            terminalError(ctx, F("NTP waiting for initial sync"));
        });

        // TODO: strptime & mktime is around ~3.7Kb
#if 1
        terminalRegisterCommand(F("NTP.SETTIME"), [](::terminal::CommandContext&& ctx) {
            if (ctx.argv.size() != 2) {
                terminalError(ctx, F("NTP.SETTIME <TIME>"));
                return;
            }

            auto value = _ntpParseTimestamp(ctx.argv[1]);
            if (value && _ntpSetTimestamp(value.timestamp())) {
                _ntp_synced = true;
                terminalOK(ctx);
                return;
            }

            terminalError(ctx, F("Invalid timestamp"));
        });
#else
        terminalRegisterCommand(F("NTP.SETTIME"), [](::terminal::CommandContext&& ctx) {
            if (ctx.argv.size() != 2) {
                terminalError(ctx, F("NTP.SETTIME <TIME>"));
                return;
            }

            const char* const fmt = (ctx.argv[1].endsWith("Z"))
                ? "%Y-%m-%dT%H:%M:%SZ" : "%s";

            tm out{};
            if (strptime(ctx.argv[1].c_str(), fmt, &out) != nullptr) {
                terminalError(ctx, F("Invalid time"));
                return;
            }

            ctx.output.printf_P(PSTR("Setting time to %s\n"),
                ntpDateTime(&out).c_str());

            _ntpSetTimestamp(mktime(&out));
            _ntp_synced = true;

            terminalOK(ctx);
        });
#endif

    #endif

}

#endif // NTP_SUPPORT
