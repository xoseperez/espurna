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

#include <algorithm>
#include <forward_list>

static_assert(
    (SNTP_SERVER_DNS == 1),
    "lwip must be configured with SNTP_SERVER_DNS"
);

#include "config/buildtime.h"

#include "ntp.h"
#include "ntp_timelib.h"
#include "utils.h"
#include "ws.h"

namespace espurna {
namespace ntp {
namespace {
namespace build {

// not strictly necessary, but allow ::time() to return something else than 0
// (but, still report as 'not synced' to the outside APIs related to time)
static constexpr time_t InitialTimestamp { __UNIX_TIMESTAMP__ };

// per the lwip recommendations, we delay the actual sntp app start
// by default, this is expected to be `LWIP_RAND % 5000` (aka [0...5) seconds)
static constexpr auto StartMin = espurna::duration::Seconds { 1 };
static constexpr auto StartMax = espurna::duration::Seconds { espurna::duration::Minutes { 5 } };
static constexpr espurna::duration::Seconds StartDelay { NTP_START_DELAY };
static_assert((StartMin <= StartDelay) && (StartDelay <= StartMax), "");

// per the https://datatracker.ietf.org/doc/html/rfc4330
// > 10. Best practices
// > 1. A client MUST NOT under any conditions use a poll interval less
// >    than 15 seconds.
// (and notice that in case things break, sntp app itself will handle retries)
static constexpr auto UpdateMin = espurna::duration::Seconds { 15 };
static constexpr auto UpdateMax = espurna::duration::Seconds { espurna::duration::Days { 30 } };
static constexpr espurna::duration::Seconds UpdateInterval { NTP_UPDATE_INTERVAL };
static_assert((UpdateMin <= UpdateInterval) && (UpdateInterval <= UpdateMax), "");

// We don't adjust update time(s) more than once, unlike NTP clients such as chrony or timesyncd.
// These offsets are applied to both values on boot and persist until the device is reset.
static constexpr auto StartRandomOffset = espurna::duration::Seconds { 10 };
static constexpr auto UpdateRandomOffset = espurna::duration::Seconds { 300 };

const __FlashStringHelper* server() {
    return F(NTP_SERVER);
}

const char* tz() {
    return NTP_TIMEZONE;
}

constexpr bool dhcp() {
    return 1 == (NTP_DHCP_SERVER);
}

} // namespace build

namespace settings {
namespace internal {

template <typename T>
T randomizeDuration(T base, T offset) {
    return T(::randomNumber(base.count(), (base + offset).count()));
}

} // namespace internal

espurna::duration::Seconds startDelay() {
    return std::clamp(
        getSetting("ntpStartDelay", build::StartDelay),
        build::StartMin, build::StartMax);
}

espurna::duration::Seconds randomStartDelay() {
    return internal::randomizeDuration(startDelay(), build::StartRandomOffset);
}

espurna::duration::Seconds updateInterval() {
    return std::clamp(
        getSetting("ntpUpdateIntvl", build::UpdateInterval),
        build::UpdateMin, build::UpdateMax);
}

espurna::duration::Seconds randomUpdateInterval() {
    return internal::randomizeDuration(updateInterval(), build::UpdateRandomOffset);
}

// as either DNS name or IP address
String server() {
    return getSetting("ntpServer", build::server());
}

void server(const String& value) {
    setSetting("ntpServer", value);
}

// POSIX TZ variable, ref.
// - https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap08.html#tag_08_03
// - https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
String tz() {
    return getSetting("ntpTZ", build::tz());
}

// in case DHCP packet contains a SNTP option, switch to that server instead of the one from settings
bool dhcp() {
    return getSetting("ntpDhcp", build::dhcp());
}

void dhcp(bool value) {
    setSetting("ntpDhcp", value);
}

} // namespace settings

namespace internal {

espurna::duration::Seconds start_delay { build::StartDelay };
espurna::duration::Seconds update_interval { build::UpdateInterval };

} // namespace internal

} // namespace
} // namespace ntp
} // namespace espurna

// With esp8266's lwipopts.h, we are allowed to configure SNTP delays at runtime
// These are weak, if we don't redefine it will fallback to Core's default ones
// (notice that our settings are in *seconds*, while SNTP expects *milliseconds*)

static_assert(sizeof(decltype(::espurna::ntp::internal::start_delay)::rep) <= sizeof(uint32_t), "");
static_assert(sizeof(decltype(::espurna::ntp::internal::update_interval)::rep) <= sizeof(uint32_t), "");

// aka `SNTP_STARTUP_DELAY_FUNC`
uint32_t sntp_startup_delay_MS_rfc_not_less_than_60000() {
    return espurna::duration::Milliseconds(::espurna::ntp::internal::start_delay).count();
}

// aka `SNTP_UPDATE_DELAY`
uint32_t sntp_update_delay_MS_rfc_not_less_than_15000() {
    return espurna::duration::Milliseconds(::espurna::ntp::internal::update_interval).count();
}

// -----------------------------------------------------------------------------

namespace espurna {
namespace ntp {
namespace {

bool setTimestamp(time_t ts) {
    const timeval tv {
        .tv_sec = ts,
        .tv_usec = 0
    };

    return EINVAL != settimeofday(&tv, nullptr);
}

namespace internal {

struct Status {
    Status() = default;

    void update(time_t timestamp) {
        _synced = true;
        _timestamp = timestamp;
    }

    bool synced() const {
        return _synced;
    }

    time_t timestamp() const {
        return _timestamp;
    }

private:
    bool _synced { false };
    time_t _timestamp { 0 };
};

Status status;
String server;

} // namespace internal

bool synced() {
    return internal::status.synced();
}

namespace parse {

struct Result {
    Result() = default;
    explicit Result(time_t value) :
        _result(true),
        _timestamp(value)
    {}

    Result& operator=(time_t value) {
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

namespace internal {

template <typename T>
T convert(const char*, char**, int);

template <>
[[gnu::unused]] inline long convert(const char* p, char** endp, int base) {
    return strtol(p, endp, base);
}

template <>
[[gnu::unused]] inline long long convert(const char* p, char** endp, int base) {
    return strtoll(p, endp, base);
}

} // namespace internal

Result timestamp(const String& value) {
    Result out;

    const char* p { value.c_str() };
    char* endp { nullptr };

    auto result = internal::convert<time_t>(p, &endp, 10);
    if (!endp || (endp == p)) {
        return out;
    }

    out = result;
    return out;
}

} // namespace parse

namespace timelib {
namespace internal {

time_t last_timestamp { 0 };
tm local;
tm utc;

void cache(time_t value) {
    if (last_timestamp != value) {
        last_timestamp = value;
        localtime_r(&last_timestamp, &local);
        gmtime_r(&last_timestamp, &utc);
    }
}

} // namespace internal

// This is based on the Timelib implementation, which is slightly different from POSIX
// This remains (mostly) for historical reasons, since we don't want to break existing config for no reason

int hour(time_t ts) {
    internal::cache(ts);
    return internal::local.tm_hour;
}

int minute(time_t ts) {
    internal::cache(ts);
    return internal::local.tm_min;
}

int second(time_t ts) {
    internal::cache(ts);
    return internal::local.tm_sec;
}

int day(time_t ts) {
    internal::cache(ts);
    return internal::local.tm_mday;
}

// `tm.tm_wday` range is 0..6, TimeLib is 1..7
int weekday(time_t ts) {
    internal::cache(ts);
    return internal::local.tm_wday + 1;
}

// `tm.tm_mon` range is 0..11, TimeLib range is 1..12
int month(time_t ts) {
    internal::cache(ts);
    return internal::local.tm_mon + 1;
}

int year(time_t ts) {
    internal::cache(ts);
    return internal::local.tm_year + 1900;
}

int utc_hour(time_t ts) {
    internal::cache(ts);
    return internal::utc.tm_hour;
}

int utc_minute(time_t ts) {
    internal::cache(ts);
    return internal::utc.tm_min;
}

int utc_second(time_t ts) {
    internal::cache(ts);
    return internal::utc.tm_sec;
}

int utc_day(time_t ts) {
    internal::cache(ts);
    return internal::utc.tm_mday;
}

int utc_weekday(time_t ts) {
    internal::cache(ts);
    return internal::utc.tm_wday + 1;
}

int utc_month(time_t ts) {
    internal::cache(ts);
    return internal::utc.tm_mon + 1;
}

int utc_year(time_t ts) {
    internal::cache(ts);
    return internal::utc.tm_year + 1900;
}

time_t now() {
    return ::time(nullptr);
}

} // namespace timelib

#if WEB_SUPPORT
namespace web {

bool onKeyCheck(const char * key, JsonVariant&) {
    return (strncmp(key, "ntp", 3) == 0);
}

void onVisible(JsonObject& root) {
    wsPayloadModule(root, "ntp");
}

void onData(JsonObject& root) {
    root["ntpStatus"] = ::espurna::ntp::internal::status.synced();
}

void onConnected(JsonObject& root) {
    root["ntpServer"] = ::espurna::ntp::settings::server();
    root["ntpTZ"] = ::espurna::ntp::settings::tz();
}

} // namespace web
#endif

String activeServer() {
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

String datetime(tm* timestruct) {
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

String datetime(time_t ts) {
    tm timestruct;
    localtime_r(&ts, &timestruct);
    return datetime(&timestruct);
}

String datetime() {
    return synced() ? datetime(timelib::now()) : String();
}

NtpInfo makeInfo() {
    NtpInfo result;

    const auto sync = internal::status.timestamp();
    tm sync_datetime{};
    gmtime_r(&sync, &sync_datetime);
    result.sync = datetime(&sync_datetime);

    const auto now = timelib::now();
    result.now = now;

    tm now_datetime{};
    gmtime_r(&now, &now_datetime);
    result.utc = datetime(&now_datetime);

    const char* cfg_tz = getenv("TZ");
    if ((cfg_tz != nullptr) && (strcmp(cfg_tz, "UTC0") != 0)) {
        tm local_datetime{};
        localtime_r(&now, &local_datetime);
        result.local = datetime(&local_datetime);
        result.tz = cfg_tz;
    }

    return result;
}

#if TERMINAL_SUPPORT
namespace terminal {

void report(Print& out) {
    const auto info = makeInfo();
    out.printf_P(
        PSTR("server: %s\n"
             "update every: %u (s)\n"
             "last synced: %s\n"
             "utc time: %s\n"),
        internal::server.c_str(),
        internal::update_interval.count(),
        info.sync.c_str(),
        info.utc.c_str());

    if (info.tz.length()) {
        out.printf_P(PSTR("local time: %s (%s)\n"),
            info.local.c_str(),
            info.tz.c_str());
    }
}

void setup() {
    terminalRegisterCommand(F("NTP"), [](::terminal::CommandContext&& ctx) {
        if (synced()) {
            report(ctx.output);
            terminalOK(ctx);
            return;
        }

        terminalError(ctx, F("NTP not synced"));
    });

    terminalRegisterCommand(F("NTP.SYNC"), [](::terminal::CommandContext&& ctx) {
        if (synced()) {
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

        auto value = parse::timestamp(ctx.argv[1]);
        if (value && setTimestamp(value.timestamp())) {
            internal::status.update(value.timestamp());
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
            datetime(&out).c_str());

        auto ts = mktime(&out);
        setTimestamp(ts);
        internal::status.update(ts);

        terminalOK(ctx);
    });
#endif
}

} // namespace terminal
#endif

#if DEBUG_SUPPORT
namespace debug {

void report() {
    if (!synced()) {
        DEBUG_MSG_P(PSTR("[NTP] Not synced\n"));
        return;
    }

    const auto info = makeInfo();
    DEBUG_MSG_P(PSTR("[NTP] Server    %s\n"), internal::server.c_str());
    DEBUG_MSG_P(PSTR("[NTP] Last Sync %s (UTC)\n"), info.sync.c_str());
    DEBUG_MSG_P(PSTR("[NTP] UTC Time  %s\n"), info.utc.c_str());

    if (info.tz.length()) {
        DEBUG_MSG_P(PSTR("[NTP] Local Time %s (%s)\n"),
            info.local.c_str(), info.tz.c_str());
    }
}

} // namespace debug
#endif

namespace tick {

static constexpr espurna::duration::Seconds OffsetMin { 1 };
static constexpr espurna::duration::Seconds OffsetMax { 60 };

using Callbacks = std::forward_list<NtpTickCallback>;

namespace internal {

Callbacks callbacks;
Ticker timer;

} // namespace internal

void add(NtpTickCallback callback) {
    internal::callbacks.push_front(callback);
}

void schedule(espurna::duration::Seconds offset);

void callback() {
    if (!synced()) {
        schedule(OffsetMax);
        return;
    }

    const auto now = timelib::now();
    tm local_tm;
    localtime_r(&now, &local_tm);

    int now_hour = local_tm.tm_hour;
    int now_minute = local_tm.tm_min;

    static int last_hour { -1 };
    static int last_minute { -1 };

    // notify subscribers about each tick interval (note that both can happen simultaneously)
    if (last_hour != now_hour) {
        last_hour = now_hour;
        for (auto& callback : internal::callbacks) {
            callback(NtpTick::EveryHour);
        }
    }

    if (last_minute != now_minute) {
        last_minute = now_minute;
        for (auto& callback : internal::callbacks) {
            callback(NtpTick::EveryMinute);
        }
    }

    // try to autocorrect each invocation
    schedule(OffsetMax - espurna::duration::Seconds(local_tm.tm_sec));
}

void schedule(espurna::duration::Seconds offset) {
    static bool scheduled { false };

    // Never allow delays less than a second, or greater than a minute
    // (ref. Non-OS 3.1.1 os_timer_arm, actual minimal value is 5ms)
    if (!scheduled) {
        scheduled = true;
        internal::timer.once_scheduled(
            std::clamp(offset, OffsetMin, OffsetMax).count(),
            []() {
                scheduled = false;
                callback();
            });
    }
}

void init() {
    static bool initialized { false };
    if (!initialized) {
        schedule_function(callback);
        initialized = true;
    }
}

} // namespace tick

void onSystemTimeSynced() {
    internal::status.update(::time(nullptr));
    tick::init();

#if WEB_SUPPORT
    wsPost(web::onData);
#endif
#if DEBUG_SUPPORT
    schedule_function(debug::report);
#endif
}

namespace settings {

void convertLegacyOffsets() {
    bool save { true };
    bool found { false };

    bool europe { true };
    bool dst { true };
    int offset { 60 };

    ::settings::internal::foreach([&](::settings::kvs_type::KeyValueResult&& kv) {
        using namespace ::settings::internal;
        const auto key = kv.key.read();
        if (key == F("ntpTZ")) {
            save = false;
        } else if (key == F("ntpOffset")) {
            offset = convert<int>(kv.value.read());
            found = true;
        } else if (key == F("ntpDST")) {
            dst = convert<bool>(kv.value.read());
            found = true;
        } else if (key == F("ntpRegion")) {
            europe = (0 == convert<int>(kv.value.read()));
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

        setSetting(F("ntpTZ"), custom);
    }

    delSetting(F("ntpOffset"));
    delSetting(F("ntpDST"));
    delSetting(F("ntpRegion"));
}

} // namespace settings

void configure() {
    // Ignore or accept the DHCP SNTP option
    // When enabled, it is possible that lwip will replace the NTP server pointer from under us
    sntp_servermode_dhcp(settings::dhcp());

    // Note: TZ_... provided by the Core are already wrapped with PSTR(...)
    // but, String() already handles every char pointer as a flash-string
    auto cfg_tz = settings::tz();
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

    const auto cfg_server = settings::server();
    const auto active_server = activeServer();
    changed = (cfg_server != active_server) || changed;

    // We skip configTime() API since we already set the TZ just above
    // (and most of the time we expect NTP server to proxy to multiple servers instead of defining more than one here)
    if (changed) {
        sntp_stop();
        internal::server = cfg_server;
        sntp_setservername(0, internal::server.c_str());
        sntp_init();
        DEBUG_MSG_P(PSTR("[NTP] Server: %s, TZ: %s\n"), cfg_server.c_str(),
                cfg_tz.length() ? cfg_tz.c_str() : "UTC0");
    }
}

void onStationModeGotIP(WiFiEventStationModeGotIP) {
    if (!sntp_enabled()) {
        return;
    }

    auto server = activeServer();
    if (!server.length()) {
        DEBUG_MSG_P(PSTR("[NTP] Updating `ntpDhcp` to ignore the DHCP values\n"));
        settings::dhcp(false);
        sntp_servermode_dhcp(0);
        schedule_function(configure);
        return;
    }

    if (!internal::server.length() || (server != internal::server)) {
        DEBUG_MSG_P(PSTR("[NTP] Updating from DHCP option - \"ntpServer\" => \"%s\"\n"), server.c_str());
        internal::server = server;
        settings::server(server);
    }
}

void setup() {
    // Randomize both times to avoid simultaneous requests from multiple devices
    internal::start_delay = settings::randomStartDelay();
    internal::update_interval = settings::randomUpdateInterval();
    DEBUG_MSG_P(PSTR("[NTP] Startup delay: %u (s), Update interval: %u (s)\n"),
        internal::start_delay.count(), internal::update_interval.count());

    // start up with some reasonable timestamp already available
    // (notice that this may not be in UTC, if the machine used to build this has a local timezone set)
    setTimestamp(build::InitialTimestamp);

    // will be called every time after ntp syncs AND loop() finishes
    settimeofday_cb(onSystemTimeSynced);

    // make sure our logic does know about the actual server
    // in case dhcp sends out ntp settings
    static auto track_active_server = WiFi.onStationModeGotIP(onStationModeGotIP);

    // generic configuration, always handled
    ::espurnaRegisterReload(configure);
    settings::convertLegacyOffsets();
    configure();

    // optional modules, depends on the build flags
#if TERMINAL_SUPPORT
    terminal::setup();
#endif
#if WEB_SUPPORT
    wsRegister()
        .onVisible(web::onVisible)
        .onConnected(web::onConnected)
        .onData(web::onData)
        .onKeyCheck(web::onKeyCheck);
#endif
}

} // namespace
} // namespace ntp
} // namespace espurna

// -----------------------------------------------------------------------------

int utc_hour(time_t ts) {
    return ::espurna::ntp::timelib::utc_hour(ts);
}

int utc_minute(time_t ts) {
    return ::espurna::ntp::timelib::utc_minute(ts);
}

int utc_second(time_t ts) {
    return ::espurna::ntp::timelib::utc_second(ts);
}

int utc_day(time_t ts) {
    return ::espurna::ntp::timelib::utc_day(ts);
}

int utc_weekday(time_t ts) {
    return ::espurna::ntp::timelib::utc_weekday(ts);
}

int utc_month(time_t ts) {
    return ::espurna::ntp::timelib::utc_month(ts);
}

int utc_year(time_t ts) {
    return ::espurna::ntp::timelib::utc_year(ts);
}

int hour(time_t ts) {
    return ::espurna::ntp::timelib::hour(ts);
}

int minute(time_t ts) {
    return ::espurna::ntp::timelib::minute(ts);
}

int second(time_t ts) {
    return ::espurna::ntp::timelib::second(ts);
}

int day(time_t ts) {
    return ::espurna::ntp::timelib::day(ts);
}

int weekday(time_t ts) {
    return ::espurna::ntp::timelib::weekday(ts);
}

int month(time_t ts) {
    return ::espurna::ntp::timelib::month(ts);
}

int year(time_t ts) {
    return ::espurna::ntp::timelib::year(ts);
}

time_t now() {
    return ::espurna::ntp::timelib::now();
}

// -----------------------------------------------------------------------------

void ntpOnTick(NtpTickCallback callback) {
    ::espurna::ntp::tick::add(callback);
}

NtpInfo ntpInfo() {
    return ::espurna::ntp::makeInfo();
}

String ntpDateTime(tm* timestruct) {
    return ::espurna::ntp::datetime(timestruct);
}

String ntpDateTime(time_t ts) {
    return ::espurna::ntp::datetime(ts);
}

String ntpDateTime() {
    return ::espurna::ntp::datetime();
}

bool ntpSynced() {
    return ::espurna::ntp::synced();
}

void ntpSetup() {
    ::espurna::ntp::setup();
}

#endif // NTP_SUPPORT
