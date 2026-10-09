#if defined(ARDUINO_ARCH_ESP32)

#include <WiFi.h>
#include <IPAddress.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_phy_init.h>
#include <lwip/dns.h>
#include <soc/soc.h>
#include <soc/rtc_cntl_reg.h>

#include <algorithm>
#include <atomic>
#include <vector>
#include <list>
#include <queue>

#include "espurna.h"
#include "wifi_orch.h"
#include "rtcmem.h"
#include "settings.h"

#if TERMINAL_SUPPORT
#include "terminal.h"
#endif

#if WEB_SUPPORT
#include "ws.h"
#endif

#if WIFI_AP_CAPTIVE_SUPPORT
#include <DNSServer.h>
#endif

namespace espurna {
namespace wifi {

namespace {

enum class Action {
    AccessPointFallback,
    AccessPointStart,
    AccessPointStop,
    Boot,
    StationConnect,
    StationReconnect,
    StationDisconnect,
    StationToggle,
    TurnOff,
    TurnOn,
    Scan,
};

using Actions = std::list<Action>;
using ActionsQueue = std::queue<Action, Actions>;

enum class State {
    Boot,
    Connect,
    WaitScan,
    TryNext,
    WaitConnected,
    Connected,
    Idle,
    Init,
    Fallback
};

namespace internal {
    bool enabled { false };
    std::atomic<bool> wifi_connected { false };
    std::atomic<bool> disconnect_triggered { false };
    std::atomic<int> last_disconnect_reason { 0 };
    std::atomic<unsigned long> last_disconnect_at { 0 };

    // Set from WiFi system task, consumed by loop task in _wifiLoop().
    // Keeps registered callbacks (MQTT/NTP/Alexa/mDNS) off the WiFi task stack.
    std::atomic<bool> pending_connect_publish { false };
    std::atomic<bool> pending_disconnect_publish { false };

    // The action queue is touched by the loop task (handle_action()/pop) and
    // by callers on terminal / web / MQTT tasks (action()/push). std::queue
    // is not thread-safe so we guard it with a portMUX critical section —
    // cheap (couple of cycles) and FreeRTOS-blessed on ESP32.
    portMUX_TYPE actions_mux = portMUX_INITIALIZER_UNLOCKED;
    ActionsQueue actions;

    State state { State::Boot };
    State last_state { state };

    uint8_t network_id { 0 };
    uint8_t connection_retries { 0 };
    unsigned long last_connect { 0 };

    // Same as esp8266 (WIFI_SCAN_NETWORKS), every connection cycle starts with a scan and
    // tries every AP of the configured networks strongest first, pinned to its BSSID & channel.
    // Without it, the driver takes the first AP it sees on the lowest channel, which with a
    // mesh / repeaters is often the weakest one.
    struct Candidate {
        uint8_t id;
        bool scanned;
        uint8_t bssid[6];
        int32_t channel;
        int32_t rssi;
    };

    std::vector<Candidate> candidates;
    size_t candidate_index { 0 };
    uint8_t candidate_attempt { 0 };
    unsigned long scan_started { 0 };
    unsigned long try_at { 0 };

    // AP fallback: last time we checked whether the AP can be stopped
    unsigned long fallback_check { 0 };

    // Set after all STA attempts failed; Idle retries after WIFI_RECONNECT_INTERVAL
    bool reconnect_pending { false };
    unsigned long reconnect_at { 0 };

    std::vector<espurna::wifi::EventCallback> callbacks;

    // Same as esp8266, StationDisconnected is only published after being connected
    // (and not for every failed attempt while connecting)
    bool station_published { false };

#if WEB_SUPPORT
    bool scan_active { false };
    unsigned long scan_requested { 0 };
    uint32_t scan_client_id { 0 };
#endif
} // namespace internal

void _wifiPublish(espurna::wifi::Event event);
void _wifiStopAp();

void action(Action value) {
    portENTER_CRITICAL(&internal::actions_mux);
    internal::actions.push(value);
    portEXIT_CRITICAL(&internal::actions_mux);
}

bool pop_action(Action& out) {
    portENTER_CRITICAL(&internal::actions_mux);
    if (internal::actions.empty()) {
        portEXIT_CRITICAL(&internal::actions_mux);
        return false;
    }
    out = internal::actions.front();
    internal::actions.pop();
    portEXIT_CRITICAL(&internal::actions_mux);
    return true;
}

void _wifiApConfigure();

template <typename T>
State handle_action(State state, T&& handler) {
    Action value;
    if (pop_action(value)) {
        if (value == Action::TurnOn || value == Action::StationConnect) {
            // _wifiStateConnect already calls WiFi.disconnect() before
            // WiFi.begin(); a second call here just thrashes the driver.
            internal::network_id = 0;
            internal::connection_retries = 0;
            internal::reconnect_pending = false;

            // Same as esp8266, radio may have been turned off. AP is brought back
            // according to the current mode (always-on AP would stay down otherwise)
            if (value == Action::TurnOn) {
                internal::enabled = true;
                _wifiApConfigure();
            }

            return State::Connect;
        }
        // Same as esp8266 wifiDisconnect(): drop the current STA connection,
        // the radio and AP stay as they are and the FSM connects again
        if (value == Action::StationReconnect) {
            DEBUG_MSG_P(PSTR("[WIFI] Reconnecting STA\n"));
            WiFi.disconnect();
            internal::wifi_connected.store(false, std::memory_order_relaxed);
            internal::network_id = 0;
            internal::connection_retries = 0;
            internal::reconnect_pending = false;
            return State::Connect;
        }
        // Same as esp8266 WIFI.STA, only the station is turned off / on. AP is left as-is
        if (value == Action::StationToggle) {
            if (WiFi.getMode() & WIFI_STA) {
                DEBUG_MSG_P(PSTR("[WIFI] Disabling STA\n"));
                internal::reconnect_pending = false;
                WiFi.disconnect();
                WiFi.enableSTA(false);
                _wifiPublish(Event::Mode);
                return State::Idle;
            }

            internal::network_id = 0;
            internal::connection_retries = 0;
            internal::reconnect_pending = false;
            return State::Connect;
        }
        if (value == Action::TurnOff) {
            internal::reconnect_pending = false;
            _wifiStopAp();
            WiFi.mode(WIFI_OFF);
            internal::enabled = false;
            _wifiPublish(Event::Mode);
            return State::Idle;
        }
        if (value == Action::Scan) {
#if WEB_SUPPORT
            if (WiFi.scanComplete() != WIFI_SCAN_RUNNING) {
                DEBUG_MSG_P(PSTR("[WIFI] Starting async scan...\n"));
                WiFi.scanNetworks(true);
            } else {
                // e.g. connection scan, results are sent to the UI as well
                DEBUG_MSG_P(PSTR("[WIFI] Scan already in progress\n"));
            }
            internal::scan_active = true;
            internal::scan_requested = millis();
#endif
            return state;
        }

        state = handler(state, value);
    }

    return state;
}

namespace sta {
namespace build {

// Same as esp8266, networks can be pre-configured with WIFI1_SSID ... WIFI5_* build flags
#define WIFI_SETTING_STRING_RESULT(FIRST, SECOND, THIRD, FOURTH, FIFTH)\
    (index == 0) ? STRING_VIEW_SETTING(FIRST) :\
    (index == 1) ? STRING_VIEW_SETTING(SECOND) :\
    (index == 2) ? STRING_VIEW_SETTING(THIRD) :\
    (index == 3) ? STRING_VIEW_SETTING(FOURTH) :\
    (index == 4) ? STRING_VIEW_SETTING(FIFTH) : StringView()

StringView ssid(size_t index) {
    return WIFI_SETTING_STRING_RESULT(WIFI1_SSID, WIFI2_SSID, WIFI3_SSID, WIFI4_SSID, WIFI5_SSID);
}

StringView pass(size_t index) {
    return WIFI_SETTING_STRING_RESULT(WIFI1_PASS, WIFI2_PASS, WIFI3_PASS, WIFI4_PASS, WIFI5_PASS);
}

StringView ip(size_t index) {
    return WIFI_SETTING_STRING_RESULT(WIFI1_IP, WIFI2_IP, WIFI3_IP, WIFI4_IP, WIFI5_IP);
}

StringView gw(size_t index) {
    return WIFI_SETTING_STRING_RESULT(WIFI1_GW, WIFI2_GW, WIFI3_GW, WIFI4_GW, WIFI5_GW);
}

StringView mask(size_t index) {
    return WIFI_SETTING_STRING_RESULT(WIFI1_MASK, WIFI2_MASK, WIFI3_MASK, WIFI4_MASK, WIFI5_MASK);
}

StringView dns(size_t index) {
    return WIFI_SETTING_STRING_RESULT(WIFI1_DNS, WIFI2_DNS, WIFI3_DNS, WIFI4_DNS, WIFI5_DNS);
}

#undef WIFI_SETTING_STRING_RESULT

} // namespace build

namespace settings {
    // Values are used as-is, same as esp8266. SSID may be UTF-8 (e.g. cyrillic), and both
    // SSID and passphrase may legitimately start or end with a space.
    String ssid(size_t index) {
        String value = getSetting(espurna::settings::Key{"ssid", index}, build::ssid(index));
        // never written (erased) storage reads back as 0xFF
        if (value.length() && (static_cast<unsigned char>(value[0]) == 0xFF)) {
            return String();
        }
        return value;
    }
    String pass(size_t index) { return getSetting(espurna::settings::Key{"pass", index}, build::pass(index)); }
    String ip(size_t index) { return getSetting(espurna::settings::Key{"ip", index}, build::ip(index)); }
    String gw(size_t index) { return getSetting(espurna::settings::Key{"gw", index}, build::gw(index)); }
    String mask(size_t index) { return getSetting(espurna::settings::Key{"mask", index}, build::mask(index)); }
    String dns(size_t index) { return getSetting(espurna::settings::Key{"dns", index}, build::dns(index)); }

    namespace query {
        static constexpr std::array<espurna::settings::query::IndexedSetting, 6> Settings PROGMEM {
            {{"ssid", ssid},
             {"pass", pass},
             {"ip", ip},
             {"gw", gw},
             {"mask", mask},
             {"dns", dns}}
        };
    }
}

size_t countNetworks() {
    size_t count = 0;
    for (size_t id = 0; id < WIFI_MAX_NETWORKS; ++id) {
        if (settings::ssid(id).length() > 0) count = id + 1;
    }
    return count;
}
} // namespace sta

namespace ap {
namespace build {

constexpr size_t SsidMax { 32 };
constexpr size_t PassphraseMin { 8 };
constexpr size_t PassphraseMax { 64 };

} // namespace build

namespace settings {
    // Same values as the esp8266 implementation ("off", "on", "fallback"), plus numeric
    // aliases and the "disabled" / "enabled" spelling that earlier ESP32 builds accepted
    ApMode mode() {
        const auto value = getSetting("wifiApMode", "");
        if (value.equalsIgnoreCase("off") || value.equalsIgnoreCase("disabled") || value == "0") return ApMode::Disabled;
        if (value.equalsIgnoreCase("on") || value.equalsIgnoreCase("enabled") || value == "1") return ApMode::Enabled;
        if (value.equalsIgnoreCase("fallback") || value == "2") return ApMode::Fallback;
        return WIFI_AP_MODE;
    }

    const char* serialize(ApMode mode) {
        switch (mode) {
        case ApMode::Disabled: return "off";
        case ApMode::Enabled: return "on";
        case ApMode::Fallback: break;
        }
        return "fallback";
    }

    // Same defaults as esp8266, hostname and admin password (unless set by build flags)
    String ssid() {
        return getSetting("wifiApSsid",
            (__builtin_strlen(WIFI_AP_SSID) > 0) ? String(WIFI_AP_SSID) : systemHostname());
    }

    String pass() {
        return getSetting("wifiApPass",
            (__builtin_strlen(WIFI_AP_PASS) > 0) ? String(WIFI_AP_PASS) : systemPassword());
    }

    // Only used when STA is not connected, otherwise AP follows the STA channel
    uint8_t channel() {
        return getSetting("wifiApChan", static_cast<uint8_t>(WIFI_AP_CHANNEL));
    }

    bool captive() {
        return getSetting("wifiApCaptive", 1 == WIFI_AP_CAPTIVE_ENABLED);
    }
}

namespace internal {
    // Started by the user (button, curtain module, terminal). Same as esp8266,
    // only the fallback AP is stopped automatically once STA is connected.
    bool manual { false };

#if WIFI_AP_CAPTIVE_SUPPORT
    // Same as esp8266, resolve every name to our AP address so phones show the
    // "sign in to network" page (ref. web.cpp /generate_204 & /fwlink)
    DNSServer dns;
    bool dns_running { false };
#endif
}

bool enabled() {
    return (WiFi.getMode() & WIFI_AP) != 0;
}
} // namespace ap

// Same settings as esp8266
namespace radio {
namespace settings {

wifi_ps_type_t sleep() {
    const auto value = getSetting("wifiSleep", "");

    int type = WIFI_SLEEP_MODE;
    if (value.equalsIgnoreCase("none") || (value == "0")) {
        type = NONE_SLEEP_T;
    } else if (value.equalsIgnoreCase("light") || (value == "1")) {
        type = LIGHT_SLEEP_T;
    } else if (value.equalsIgnoreCase("modem") || (value == "2")) {
        type = MODEM_SLEEP_T;
    }

    switch (type) {
    case LIGHT_SLEEP_T:
        return WIFI_PS_MAX_MODEM;
    case MODEM_SLEEP_T:
        return WIFI_PS_MIN_MODEM;
    }

    return WIFI_PS_NONE;
}

float tx_power() {
    return getSetting("wifiTxPwr", static_cast<float>(WIFI_OUTPUT_POWER_DBM));
}

} // namespace settings

const char* sleep_name(wifi_ps_type_t type) {
    switch (type) {
    case WIFI_PS_MIN_MODEM:
        return "modem";
    case WIFI_PS_MAX_MODEM:
        return "light";
    case WIFI_PS_NONE:
        break;
    }

    return "none";
}

// Needs the driver to be started, i.e. called after the opmode is set
void configure() {
    esp_wifi_set_ps(settings::sleep());

    // in 0.25dBm units, driver accepts 2..20dBm
    const auto dbm = settings::tx_power();
    if (!std::isnan(dbm) && !std::isinf(dbm)) {
        const auto quarters = static_cast<int>(std::lround(dbm * 4.0f));
        esp_wifi_set_max_tx_power(static_cast<int8_t>(std::clamp(quarters, 8, 80)));
    }
}

} // namespace radio

const char* authmode_name(wifi_auth_mode_t mode) {
    switch (mode) {
    case WIFI_AUTH_OPEN:
        return "OPEN";
    case WIFI_AUTH_WEP:
        return "WEP";
    case WIFI_AUTH_WPA_PSK:
        return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK:
        return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
        return "WPA_WPA2_PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE:
        return "WPA2_EAP";
    case WIFI_AUTH_WPA3_PSK:
        return "WPA3_PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK:
        return "WPA2_WPA3_PSK";
    case WIFI_AUTH_WAPI_PSK:
        return "WAPI_PSK";
    case WIFI_AUTH_MAX:
        break;
    }

    return "UNKNOWN";
}

void _wifiStopAp() {
    if (!ap::enabled()) return;
    DEBUG_MSG_P(PSTR("[WIFI] Stopping AP\n"));

#if WIFI_AP_CAPTIVE_SUPPORT
    if (ap::internal::dns_running) {
        ap::internal::dns.stop();
        ap::internal::dns_running = false;
    }
#endif

    // Not softAPdisconnect(true): it first pushes an empty AP config (channel 6)
    // through esp_wifi_set_config() and skips enableAP(false) when that fails,
    // e.g. while STA is associated on another channel. Dropping the AP bit
    // from the opmode is what actually stops the beacons.
    WiFi.enableAP(false);
    if (ap::enabled()) {
        DEBUG_MSG_P(PSTR("[WIFI] enableAP(false) failed, forcing STA opmode\n"));
        esp_wifi_set_mode(WIFI_MODE_STA);
    }

    ap::internal::manual = false;
    _wifiPublish(Event::Mode);
}

void _wifiStartAp() {
    // Same as esp8266, always generate a valid AP config. SSID falls back to the
    // identifier, invalid passphrase means an open AP (nothing is silently substituted)
    auto ssid = ap::settings::ssid();
    if (!ssid.length() || (ssid.length() >= ap::build::SsidMax)) {
        ssid = String(systemIdentifier());
    }

    const auto pass = ap::settings::pass();
    const bool secure = (pass.length() >= ap::build::PassphraseMin)
        && (pass.length() < ap::build::PassphraseMax);

    DEBUG_MSG_P(PSTR("[WIFI] Starting AP: %s (%s)\n"),
        ssid.c_str(), secure ? PSTR("secure") : PSTR("open"));

    // STA bit is kept as-is (it may have been disabled on purpose)
    WiFi.enableAP(true);
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(ssid.c_str(), secure ? pass.c_str() : nullptr, ap::settings::channel());

#if WIFI_AP_CAPTIVE_SUPPORT
    if (ap::settings::captive()) {
        ap::internal::dns.setErrorReplyCode(DNSReplyCode::NoError);
        ap::internal::dns_running = ap::internal::dns.start(53, "*", WiFi.softAPIP());
    }
#endif

    radio::configure();
    _wifiPublish(Event::Mode);
}

// Explicitly requested by the user, not managed by the fallback logic
void _wifiStartManualAp() {
    if (!ap::enabled()) {
        _wifiStartAp();
    }

    ap::internal::manual = true;
}

State _wifiStateInit(State state) {
#if WIFI_DISABLE_BROWNOUT_DETECTOR
    // Opt-in: disables hardware brownout protection. Leaving this on can
    // protect against TX-current spikes on weak PSUs but also masks real
    // brownouts and risks flash corruption mid-write.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
#endif

    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);

    WiFi.mode(WIFI_MODE_NULL);
    WiFi.setHostname(systemHostname().c_str());

    // Same as esp8266: in fallback mode the AP only comes up after STA gave up
    // (State::Fallback). Without any configured networks the AP is the only
    // way in, so start it even when disabled.
    if ((ap::settings::mode() == ApMode::Enabled) || !sta::countNetworks()) {
        _wifiStartAp();
    } else {
        WiFi.mode(WIFI_STA);
    }

    // Force EU country code for better channel 12/13 support
    wifi_country_t country = {.cc="EU", .schan=1, .nchan=13, .policy=WIFI_COUNTRY_POLICY_AUTO};
    esp_wifi_set_country(&country);

    // Same as esp8266, 802.11b/g/n with the driver defaults (rate control falls back to 'b'
    // on a weak signal by itself). wifiSleep & wifiTxPwr are applied once the driver is running
    radio::configure();

    internal::enabled = true;
    return State::Idle;
}

namespace connection {

constexpr unsigned long ScanTimeout { 15000 };
constexpr unsigned long AttemptTimeout { 15000 };
constexpr unsigned long AttemptInterval { WIFI_CONNECT_INTERVAL };
constexpr uint8_t AttemptsPerAp { WIFI_CONNECT_RETRIES };

bool expired(unsigned long since, unsigned long timeout) {
    return static_cast<int32_t>(millis() - since) >= static_cast<int32_t>(timeout);
}

// WiFi.scanComplete() has its own timeout of max_ms_per_chan * 20 (6s by default) and reports
// WIFI_SCAN_FAILED after it, even though the scan is still going and results arrive later.
// Scanning 13 channels while STA is connected (driver hops back to the home channel) easily
// takes longer than that. Keep waiting until our own timeout instead.
int16_t scan_result(unsigned long started) {
    const auto count = WiFi.scanComplete();
    if (count >= 0) {
        return count;
    }

    if (!expired(started, ScanTimeout)) {
        return WIFI_SCAN_RUNNING;
    }

    return WIFI_SCAN_FAILED;
}

String bssid_string(const uint8_t* bssid) {
    char out[18];
    snprintf_P(out, sizeof(out), PSTR("%02X:%02X:%02X:%02X:%02X:%02X"),
        bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    return out;
}

// Networks that were not seen in the scan (hidden SSID, scan failure) are still
// attempted last, without BSSID and channel, and the driver will look for them itself
void add_blind(uint8_t id) {
    auto& list = internal::candidates;
    const auto it = std::find_if(list.begin(), list.end(),
        [&](const internal::Candidate& candidate) {
            return candidate.id == id;
        });
    if (it == list.end()) {
        internal::Candidate candidate{};
        candidate.id = id;
        candidate.scanned = false;
        list.push_back(candidate);
    }
}

void build_candidates(int count) {
    auto& list = internal::candidates;
    list.clear();

    for (int index = 0; index < count; ++index) {
        const auto ssid = WiFi.SSID(index);
        for (uint8_t id = 0; id < WIFI_MAX_NETWORKS; ++id) {
            if (!ssid.length() || (ssid != sta::settings::ssid(id))) {
                continue;
            }

            internal::Candidate candidate{};
            candidate.id = id;
            candidate.scanned = true;
            std::memcpy(candidate.bssid, WiFi.BSSID(index), sizeof(candidate.bssid));
            candidate.channel = WiFi.channel(index);
            candidate.rssi = WiFi.RSSI(index);
            list.push_back(candidate);
        }
    }

    std::stable_sort(list.begin(), list.end(),
        [](const internal::Candidate& lhs, const internal::Candidate& rhs) {
            return lhs.rssi > rhs.rssi;
        });

    for (uint8_t id = 0; id < WIFI_MAX_NETWORKS; ++id) {
        if (sta::settings::ssid(id).length()) {
            add_blind(id);
        }
    }

    for (const auto& candidate : list) {
        if (candidate.scanned) {
            DEBUG_MSG_P(PSTR("[WIFI] Candidate %s BSSID %s CH %d RSSI %d\n"),
                sta::settings::ssid(candidate.id).c_str(),
                bssid_string(candidate.bssid).c_str(),
                candidate.channel, candidate.rssi);
        } else {
            DEBUG_MSG_P(PSTR("[WIFI] Candidate %s (not found in scan)\n"),
                sta::settings::ssid(candidate.id).c_str());
        }
    }

    internal::candidate_index = 0;
    internal::candidate_attempt = 0;
    internal::try_at = millis();
}

// DHCP stays disabled after WiFi.config() with a static IP, unless explicitly reset.
// Otherwise one network with a static IP makes every other one use it as well.
void configure_ip(uint8_t id) {
    IPAddress ip;
    if (ip.fromString(sta::settings::ip(id)) && ((uint32_t)ip != 0)) {
        IPAddress gw, mask, dns;
        gw.fromString(sta::settings::gw(id));
        mask.fromString(sta::settings::mask(id));
        dns.fromString(sta::settings::dns(id));
        WiFi.config(ip, gw, mask, dns);
        return;
    }

    // zero IP restarts the DHCP client (not INADDR_NONE, lwip defines it as 255.255.255.255)
    const IPAddress none(0u);
    WiFi.config(none, none, none);
}

} // namespace connection

// Same settings as esp8266 (wifiScan & wifiScanRssi), when connected network RSSI is below the
// threshold periodically look for a better AP of the configured networks.
// Unlike esp8266, scan happens in the background without dropping the connection, and we
// only switch when another AP is at least wifiRoamDelta stronger (esp8266 has no margin and
// keeps reconnecting when several APs are similarly weak). While nothing changes, scans become
// less frequent: after StableScans in a row the interval grows by a minute, up to IntervalMax.
namespace roaming {
namespace build {

constexpr unsigned long IntervalMin { 3ul * 60ul * 1000ul };
constexpr unsigned long IntervalMax { 15ul * 60ul * 1000ul };
constexpr unsigned long IntervalStep { 60ul * 1000ul };
constexpr uint8_t StableScans { 10 };

// Another AP has to be better on this many scans in a row (always the same BSSID),
// the confirming scan happens sooner than the regular one
constexpr uint8_t ConfirmScans { 2 };
constexpr unsigned long ConfirmInterval { 30ul * 1000ul };

// Arduino fixes the minimal active dwell time at 100ms, this is the maximum. Default
// is 300ms, and every channel spent away from the AP on an already weak link risks
// missed beacons (and a disconnect). Driver returns to the home channel in between
// channels while connected.
constexpr uint32_t ScanDwellMax { 120 };

// Current link RSSI is the median of periodic WiFi.RSSI() samples, single reading is too noisy
constexpr size_t RssiSamples { 7 };
constexpr unsigned long RssiSampleInterval { 2000ul };

// When a switch to another AP does not end up connected to it, that BSSID is not considered
// by roaming for a while (still used by the regular connection when the current link is lost).
// Backoff doubles on repeated failures. RAM only, forgotten on reboot.
constexpr unsigned long BackoffMin { 30ul * 60ul * 1000ul };
constexpr unsigned long BackoffMax { 4ul * 60ul * 60ul * 1000ul };
constexpr size_t BackoffSlots { 4 };

} // namespace build

namespace settings {

bool enabled() {
    return getSetting("wifiScan", 1 == WIFI_SCAN_NETWORKS);
}

int8_t threshold() {
    return getSetting("wifiScanRssi", static_cast<int8_t>(WIFI_SCAN_RSSI_THRESHOLD));
}

int8_t delta() {
    return std::clamp(
        getSetting("wifiRoamDelta", static_cast<int8_t>(WIFI_ROAM_RSSI_DELTA)),
        int8_t{ 1 }, int8_t{ 40 });
}

} // namespace settings

namespace internal {

unsigned long interval { build::IntervalMin };
unsigned long last { 0 };
uint8_t stable { 0 };
bool scanning { false };
unsigned long scan_started { 0 };

// directed scan (only when every configured network has the same SSID)
String scan_ssid;

// link RSSI samples, ring buffer
std::array<int8_t, build::RssiSamples> samples{};
size_t samples_count { 0 };
size_t samples_next { 0 };
unsigned long sample_last { 0 };

// candidate that was better on the previous scan(s), still needs to be confirmed
uint8_t pending_bssid[6] {};
uint8_t pending_count { 0 };

// AP the interval and the stable scans refer to, another one means the link changed
uint8_t bssid[6] {};

// switch to another AP in progress, checked when the FSM is connected again
bool switching { false };
uint8_t switch_bssid[6] {};

struct Backoff {
    uint8_t bssid[6];
    uint8_t failures;
    unsigned long since;
    unsigned long duration;
};

std::array<Backoff, build::BackoffSlots> backoff{};

} // namespace internal

void clear_samples() {
    internal::samples_count = 0;
    internal::samples_next = 0;
    internal::sample_last = millis() - build::RssiSampleInterval;
}

void clear_pending() {
    internal::pending_count = 0;
    std::memset(internal::pending_bssid, 0, sizeof(internal::pending_bssid));
}

void sample() {
    if (!connection::expired(internal::sample_last, build::RssiSampleInterval)) {
        return;
    }

    internal::sample_last = millis();

    const auto rssi = WiFi.RSSI();
    if (rssi >= 0) {
        return;
    }

    internal::samples[internal::samples_next] = static_cast<int8_t>(rssi);
    internal::samples_next = (internal::samples_next + 1) % internal::samples.size();
    internal::samples_count = std::min(internal::samples_count + 1, internal::samples.size());
}

// median of the collected samples, or a single fresh reading when there are none yet
int32_t link_rssi() {
    if (!internal::samples_count) {
        return WiFi.RSSI();
    }

    std::array<int8_t, build::RssiSamples> sorted{};
    std::copy(internal::samples.begin(),
        internal::samples.begin() + internal::samples_count, sorted.begin());
    std::sort(sorted.begin(), sorted.begin() + internal::samples_count);

    return sorted[internal::samples_count / 2];
}

// Entries stay after the backoff ran out (failures count is kept, so that the next failure
// doubles it) and are forgotten after twice the max backoff without another failure.
// Expired entries are marked right away (duration 0, since = expiry time), so the elapsed
// time is always small enough for the unsigned math and millis() overflow does not matter.
void backoff_housekeeping() {
    for (auto& entry : internal::backoff) {
        if (!entry.failures) {
            continue;
        }

        const auto elapsed = millis() - entry.since;
        if (entry.duration) {
            if (elapsed >= entry.duration) {
                entry.since += entry.duration;
                entry.duration = 0;
            }
        } else if (elapsed >= (2ul * build::BackoffMax)) {
            entry = internal::Backoff{};
        }
    }
}

unsigned long backoff_left(const internal::Backoff& entry) {
    if (!entry.failures || !entry.duration) {
        return 0;
    }

    const auto elapsed = millis() - entry.since;
    return (elapsed < entry.duration)
        ? (entry.duration - elapsed)
        : 0;
}

internal::Backoff* backoff_find(const uint8_t* bssid) {
    for (auto& entry : internal::backoff) {
        if (entry.failures && (std::memcmp(entry.bssid, bssid, sizeof(entry.bssid)) == 0)) {
            return &entry;
        }
    }

    return nullptr;
}

// Remaining backoff of the BSSID in ms, 0 when roaming may switch to it
unsigned long backed_off(const uint8_t* bssid) {
    const auto* entry = backoff_find(bssid);
    return entry
        ? backoff_left(*entry)
        : 0;
}

void backoff_add(const uint8_t* bssid) {
    backoff_housekeeping();

    auto* entry = backoff_find(bssid);
    if (!entry) {
        // free slot, otherwise the one with the least backoff left
        entry = &internal::backoff[0];
        for (auto& it : internal::backoff) {
            if (!it.failures) {
                entry = &it;
                break;
            }

            if (backoff_left(it) < backoff_left(*entry)) {
                entry = &it;
            }
        }

        *entry = internal::Backoff{};
        std::memcpy(entry->bssid, bssid, sizeof(entry->bssid));
    }

    if (entry->failures < 255) {
        ++entry->failures;
    }

    unsigned long duration = build::BackoffMin;
    for (uint8_t n = 1; (n < entry->failures) && (duration < build::BackoffMax); ++n) {
        duration *= 2;
    }

    entry->duration = std::min(duration, build::BackoffMax);
    entry->since = millis();

    DEBUG_MSG_P(PSTR("[WIFI] Roaming: %s on backoff for %u min (failure #%u)\n"),
        connection::bssid_string(bssid).c_str(),
        (unsigned)(entry->duration / (60ul * 1000ul)), entry->failures);
}

void backoff_forget(const uint8_t* bssid) {
    auto* entry = backoff_find(bssid);
    if (entry) {
        *entry = internal::Backoff{};
    }
}

void remember_bssid() {
    const auto* current = WiFi.BSSID();
    if (current) {
        std::memcpy(internal::bssid, current, sizeof(internal::bssid));
    } else {
        std::memset(internal::bssid, 0, sizeof(internal::bssid));
    }
}

// Connected to another AP than the one the counters refer to (e.g. driver reconnected by itself)
bool bssid_changed() {
    const auto* current = WiFi.BSSID();
    return current && (std::memcmp(internal::bssid, current, sizeof(internal::bssid)) != 0);
}

// Something actually changed (new candidate, switch, reconnect, another BSSID): short interval again
void unsettle() {
    internal::interval = build::IntervalMin;
    internal::stable = 0;
}

// (re)connected. Start over with short interval
void reset() {
    unsettle();
    internal::last = millis();
    internal::scanning = false;
    clear_samples();
    clear_pending();
    remember_bssid();
}

// Scan decided to stay (nothing better, nothing pending), slowly back off. Every such scan
// counts, after StableScans in a row each one adds IntervalStep, up to IntervalMax
void settled() {
    clear_pending();

    if (internal::stable < build::StableScans) {
        ++internal::stable;
    }

    if (internal::stable >= build::StableScans) {
        internal::interval = std::min(internal::interval + build::IntervalStep, build::IntervalMax);
    }
}

// FSM is connected (again). After a roaming switch, check that we ended up on the chosen AP.
// When we did not, that AP goes on backoff and the link is considered unchanged, so the scan
// interval keeps growing instead of starting over (and switching back and forth every few min)
void connected() {
    if (!internal::switching) {
        reset();
        return;
    }

    internal::switching = false;

    uint8_t current[6] {};
    const auto* current_ptr = WiFi.BSSID();
    if (current_ptr) {
        std::memcpy(current, current_ptr, sizeof(current));
    }

    if (current_ptr && (std::memcmp(current, internal::switch_bssid, sizeof(current)) == 0)) {
        DEBUG_MSG_P(PSTR("[WIFI] Roaming: switched to %s\n"),
            connection::bssid_string(current).c_str());
        backoff_forget(current);
        reset();
        return;
    }

    DEBUG_MSG_P(PSTR("[WIFI] Roaming: switch to %s failed, connected to %s instead\n"),
        connection::bssid_string(internal::switch_bssid).c_str(),
        connection::bssid_string(current).c_str());
    backoff_add(internal::switch_bssid);

    internal::last = millis();
    internal::scanning = false;
    clear_samples();
    remember_bssid();
    settled();

    DEBUG_MSG_P(PSTR("[WIFI] Roaming: next check in %u min (stable %u/%u)\n"),
        (unsigned)(internal::interval / (60ul * 1000ul)),
        internal::stable, build::StableScans);
}

// Connection was restarted by an action (reload, terminal, web), not a roaming failure
void cancel() {
    internal::switching = false;
}

// FSM gave up on every AP while switching. The next connection is a regular one
void abandoned() {
    if (!internal::switching) {
        return;
    }

    internal::switching = false;
    DEBUG_MSG_P(PSTR("[WIFI] Roaming: switch to %s failed, no connection\n"),
        connection::bssid_string(internal::switch_bssid).c_str());
    backoff_add(internal::switch_bssid);
}

bool configured(const String& ssid) {
    if (!ssid.length()) {
        return false;
    }

    for (uint8_t id = 0; id < WIFI_MAX_NETWORKS; ++id) {
        if (ssid == sta::settings::ssid(id)) {
            return true;
        }
    }

    return false;
}

// Only switch when another AP (never the current BSSID) of the configured networks is better
// than the current link by the margin, on ConfirmScans scans in a row.
// Current link RSSI is the average of the driver RSSI median and of the scanned RSSI of the
// current BSSID (when present), so neither a single low reading nor a single scan decides.
// Returns index of the scan result to switch to, or -1 to stay.
int better(int count) {
    uint8_t current[6] {};
    const auto* current_ptr = WiFi.BSSID();
    if (current_ptr) {
        std::memcpy(current, current_ptr, sizeof(current));
    }

    const auto link = link_rssi();
    int32_t scanned = 0;
    bool scanned_current = false;
    int best = -1;

    // APs on backoff after a failed switch, reported once per scan
    uint8_t skipped = 0;
    int skipped_index = -1;
    unsigned long skipped_left = 0;

    backoff_housekeeping();

    for (int index = 0; index < count; ++index) {
        const auto* bssid = WiFi.BSSID(index);
        if (!bssid) {
            continue;
        }

        if (current_ptr && (std::memcmp(current, bssid, sizeof(current)) == 0)) {
            scanned = WiFi.RSSI(index);
            scanned_current = true;
            continue;
        }

        if (!configured(WiFi.SSID(index))) {
            continue;
        }

        const auto left = backed_off(bssid);
        if (left) {
            if ((skipped_index < 0) || (WiFi.RSSI(index) > WiFi.RSSI(skipped_index))) {
                skipped_index = index;
                skipped_left = left;
            }
            if (skipped < 255) {
                ++skipped;
            }
            continue;
        }

        if ((best < 0) || (WiFi.RSSI(index) > WiFi.RSSI(best))) {
            best = index;
        }
    }

    if (skipped) {
        DEBUG_MSG_P(PSTR("[WIFI] Roaming: skipping %u AP(s) on backoff, strongest %s CH %d RSSI %d (%u min left)\n"),
            skipped, connection::bssid_string(WiFi.BSSID(skipped_index)).c_str(),
            WiFi.channel(skipped_index), WiFi.RSSI(skipped_index),
            (unsigned)((skipped_left + 59999ul) / (60ul * 1000ul)));
    }

    const int32_t current_rssi = scanned_current
        ? ((link + scanned) / 2)
        : link;
    const auto scan_value = scanned_current
        ? String(scanned)
        : String('-');

    if (best < 0) {
        clear_pending();
        DEBUG_MSG_P(PSTR("[WIFI] Roaming: current %s RSSI %d (link %d, scan %s), no other AP found, staying\n"),
            connection::bssid_string(current).c_str(), current_rssi, link, scan_value.c_str());
        return -1;
    }

    const auto* best_bssid = WiFi.BSSID(best);
    const int32_t best_rssi = WiFi.RSSI(best);
    const int32_t diff = best_rssi - current_rssi;
    const int32_t margin = settings::delta();

    int result = -1;
    if (diff >= margin) {
        if (internal::pending_count && (std::memcmp(internal::pending_bssid, best_bssid, sizeof(internal::pending_bssid)) == 0)) {
            ++internal::pending_count;
        } else {
            std::memcpy(internal::pending_bssid, best_bssid, sizeof(internal::pending_bssid));
            internal::pending_count = 1;
            unsettle();
        }

        if (internal::pending_count >= build::ConfirmScans) {
            result = best;
        }
    } else {
        clear_pending();
    }

    DEBUG_MSG_P(PSTR("[WIFI] Roaming: current %s RSSI %d (link %d, scan %s), best other %s CH %d RSSI %d, delta %d (margin %d, seen %u/%u), %s\n"),
        connection::bssid_string(current).c_str(), current_rssi, link, scan_value.c_str(),
        connection::bssid_string(best_bssid).c_str(), WiFi.channel(best), best_rssi,
        diff, margin, internal::pending_count, build::ConfirmScans,
        (result >= 0) ? PSTR("switching")
            : internal::pending_count ? PSTR("staying, confirming")
            : PSTR("staying"));

    return result;
}

// Same SSID for every configured network allows a directed probe, fewer responses and less time off-channel
const char* scan_ssid() {
    internal::scan_ssid = String();

    for (uint8_t id = 0; id < WIFI_MAX_NETWORKS; ++id) {
        const auto ssid = sta::settings::ssid(id);
        if (!ssid.length()) {
            continue;
        }

        if (!internal::scan_ssid.length()) {
            internal::scan_ssid = ssid;
        } else if (internal::scan_ssid != ssid) {
            internal::scan_ssid = String();
            return nullptr;
        }
    }

    return internal::scan_ssid.length()
        ? internal::scan_ssid.c_str()
        : nullptr;
}

State loop(State state) {
    if (!settings::enabled()) {
        return state;
    }

    if (internal::scanning) {
        const auto count = connection::scan_result(internal::scan_started);
        if (count == WIFI_SCAN_RUNNING) {
            return state;
        }

        internal::scanning = false;
        internal::last = millis();

        if (count < 0) {
            DEBUG_MSG_P(PSTR("[WIFI] Roaming: scan failed\n"));
            return state;
        }

        const bool moved = bssid_changed();
        if (moved) {
            DEBUG_MSG_P(PSTR("[WIFI] Roaming: AP changed to %s, starting over\n"),
                WiFi.BSSIDstr().c_str());
            remember_bssid();
            unsettle();
        }

        const auto target = better(count);
        uint8_t target_bssid[6] {};
        if (target >= 0) {
            std::memcpy(target_bssid, WiFi.BSSID(target), sizeof(target_bssid));
            connection::build_candidates(count);

            // Candidates are sorted by the scanned RSSI, which may still put the current AP
            // first. Make sure the AP we decided on is attempted first, otherwise we would
            // just drop the connection and associate with the same AP again.
            auto& list = espurna::wifi::internal::candidates;
            const auto it = std::find_if(list.begin(), list.end(),
                [&](const espurna::wifi::internal::Candidate& candidate) {
                    return candidate.scanned
                        && (std::memcmp(candidate.bssid, target_bssid, sizeof(target_bssid)) == 0);
                });
            if (it != list.end()) {
                std::rotate(list.begin(), it, it + 1);
            }
        } else if (internal::pending_count) {
            DEBUG_MSG_P(PSTR("[WIFI] Roaming: next check in %u s\n"),
                (unsigned)(build::ConfirmInterval / 1000ul));
        } else {
            if (!moved) {
                settled();
            }
            DEBUG_MSG_P(PSTR("[WIFI] Roaming: next check in %u min (stable %u/%u)\n"),
                (unsigned)(internal::interval / (60ul * 1000ul)),
                internal::stable, build::StableScans);
        }

#if WEB_SUPPORT
        // UI scan waits for the same results, it will delete them
        if (!espurna::wifi::internal::scan_active)
#endif
        {
            WiFi.scanDelete();
        }

        if (target >= 0) {
            clear_pending();
            internal::switching = true;
            std::memcpy(internal::switch_bssid, target_bssid, sizeof(internal::switch_bssid));
            espurna::wifi::internal::disconnect_triggered.store(false, std::memory_order_relaxed);
            WiFi.disconnect();
            return State::TryNext;
        }

        return state;
    }

    sample();

    const auto interval = internal::pending_count
        ? build::ConfirmInterval
        : internal::interval;
    if (!connection::expired(internal::last, interval)) {
        return state;
    }

    // Good enough, nothing to look for. The link itself did not change, so the interval and
    // the stable scans are kept (resetting them here restarted the backoff every time the
    // RSSI crossed the threshold back and forth)
    const auto rssi = link_rssi();
    if (rssi >= settings::threshold()) {
        internal::last = millis();
        clear_pending();
        return state;
    }

    // someone else is scanning right now, try again on the next loop
    if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) {
        return state;
    }

    internal::last = millis();
    if (WiFi.scanNetworks(true, false, false, build::ScanDwellMax, 0, scan_ssid()) == WIFI_SCAN_FAILED) {
        DEBUG_MSG_P(PSTR("[WIFI] Roaming: scan failed\n"));
        return state;
    }

    DEBUG_MSG_P(PSTR("[WIFI] Roaming: RSSI %d is below %d, looking for a better AP\n"),
        rssi, settings::threshold());
    internal::scanning = true;
    internal::scan_started = millis();

    return state;
}

} // namespace roaming

// Start of the connection cycle, scan for the configured networks first
State _wifiStateConnect(State state) {
    if (WiFi.status() == WL_CONNECTED) return State::Connected;

    if (!sta::countNetworks()) {
        DEBUG_MSG_P(PSTR("[WIFI] No networks configured\n"));
        return State::Fallback;
    }

    // Keep the AP bit as-is, it is managed by the fallback logic
    WiFi.enableSTA(true);
    radio::configure();

    // scan does not work while the driver is trying to connect
    internal::disconnect_triggered.store(false, std::memory_order_relaxed);
    WiFi.disconnect();

    // Same as esp8266 with wifiScan disabled, networks are tried in the configured order
    if (!roaming::settings::enabled()) {
        connection::build_candidates(0);
        return State::TryNext;
    }

    // UI scan may be running already, its results are just as good
    if (WiFi.scanComplete() != WIFI_SCAN_RUNNING) {
        DEBUG_MSG_P(PSTR("[WIFI] Scanning for the configured networks\n"));
        if (WiFi.scanNetworks(true, true) == WIFI_SCAN_FAILED) {
            DEBUG_MSG_P(PSTR("[WIFI] Scan failed, connecting without it\n"));
            connection::build_candidates(0);
            return State::TryNext;
        }
    }

    internal::scan_started = millis();
    return State::WaitScan;
}

State _wifiStateWaitScan(State state) {
    const auto count = connection::scan_result(internal::scan_started);
    if (count == WIFI_SCAN_RUNNING) {
        return state;
    }

    if (count < 0) {
        DEBUG_MSG_P(PSTR("[WIFI] Scan failed, connecting without it\n"));
        connection::build_candidates(0);
        return State::TryNext;
    }

    DEBUG_MSG_P(PSTR("[WIFI] Scan found %d network(s)\n"), count);
    connection::build_candidates(count);

#if WEB_SUPPORT
    // UI scan waits for the same results, it will delete them
    if (!internal::scan_active)
#endif
    {
        WiFi.scanDelete();
    }

    return State::TryNext;
}

State _wifiStateTryNext(State state) {
    if (!connection::expired(internal::try_at, 0)) {
        return state;
    }

    if (internal::candidate_index >= internal::candidates.size()) {
        // Same as esp8266, when every network is exhausted bring up the fallback AP.
        // STA keeps retrying every WIFI_RECONNECT_INTERVAL, starting with a new scan.
        DEBUG_MSG_P(PSTR("[WIFI] Could not connect to any of the configured networks\n"));
        return State::Fallback;
    }

    const auto& candidate = internal::candidates[internal::candidate_index];
    const auto ssid = sta::settings::ssid(candidate.id);
    const auto pass = sta::settings::pass(candidate.id);

    if (candidate.scanned) {
        DEBUG_MSG_P(PSTR("[WIFI] Connecting to %s BSSID %s CH %d (attempt %u)\n"),
            ssid.c_str(), connection::bssid_string(candidate.bssid).c_str(),
            candidate.channel, internal::candidate_attempt + 1);
    } else {
        DEBUG_MSG_P(PSTR("[WIFI] Connecting to %s (attempt %u)\n"),
            ssid.c_str(), internal::candidate_attempt + 1);
    }

    internal::network_id = candidate.id;
    connection::configure_ip(candidate.id);
    WiFi.setHostname(systemHostname().c_str());

    internal::disconnect_triggered.store(false, std::memory_order_relaxed);
    if (candidate.scanned) {
        WiFi.begin(ssid.c_str(), pass.c_str(), candidate.channel, candidate.bssid);
    } else {
        WiFi.begin(ssid.c_str(), pass.c_str());
    }

    internal::last_connect = millis();
    return State::WaitConnected;
}

// Current AP did not work out. Retry it a couple of times, then move on to the next one
State _wifiAttemptFailed() {
    WiFi.disconnect();

    ++internal::candidate_attempt;
    if (internal::candidate_attempt >= connection::AttemptsPerAp) {
        internal::candidate_attempt = 0;
        ++internal::candidate_index;
    }

    internal::try_at = millis() + connection::AttemptInterval;
    return State::TryNext;
}

// Disconnects that happen too soon after begin() are from the WiFi.disconnect() of the previous attempt
constexpr unsigned long StaleDisconnect { 500 };

// Which AP failed and why, e.g. a mesh node rejecting us has a distinct reason code
void _wifiAttemptLog(bool timeout) {
    if (internal::candidate_index >= internal::candidates.size()) {
        return;
    }

    const auto& candidate = internal::candidates[internal::candidate_index];
    const auto reason = internal::last_disconnect_reason.load(std::memory_order_relaxed);
    const auto reason_at = internal::last_disconnect_at.load(std::memory_order_relaxed);
    const bool fresh = static_cast<int32_t>(reason_at - (internal::last_connect + StaleDisconnect)) >= 0;

    char target[64];
    if (candidate.scanned) {
        snprintf_P(target, sizeof(target), PSTR("BSSID %s CH %d RSSI %d"),
            connection::bssid_string(candidate.bssid).c_str(),
            candidate.channel, candidate.rssi);
    } else {
        snprintf_P(target, sizeof(target), PSTR("(any BSSID)"));
    }

    if (timeout && !fresh) {
        DEBUG_MSG_P(PSTR("[WIFI] Connection to %s %s timed out (status %d, no disconnect reason)\n"),
            sta::settings::ssid(candidate.id).c_str(), target, (int)WiFi.status());
        return;
    }

    DEBUG_MSG_P(PSTR("[WIFI] Connection to %s %s %s (status %d, reason %d %s)\n"),
        sta::settings::ssid(candidate.id).c_str(), target,
        timeout ? PSTR("timed out") : PSTR("failed"), (int)WiFi.status(),
        reason, WiFi.disconnectReasonName(static_cast<wifi_err_reason_t>(reason)));
}

State _wifiStateWaitConnected(State state) {
    if (WiFi.status() == WL_CONNECTED || internal::wifi_connected.load(std::memory_order_acquire)) {
        DEBUG_MSG_P(PSTR("[WIFI] Successfully connected!\n"));
        return State::Connected;
    }

    // Driver explicitly told us it failed. Ignore disconnects that happen too soon
    // after begin(), those are from the WiFi.disconnect() of the previous attempt.
    if (internal::disconnect_triggered.exchange(false, std::memory_order_acq_rel)
        && connection::expired(internal::last_connect, StaleDisconnect))
    {
        _wifiAttemptLog(false);
        return _wifiAttemptFailed();
    }

    if (connection::expired(internal::last_connect, connection::AttemptTimeout)) {
        _wifiAttemptLog(true);
        return _wifiAttemptFailed();
    }

    return state;
}

State _wifiStateConnected(State state) {
    if (WiFi.status() != WL_CONNECTED) return State::Connect;
    internal::connection_retries = 0;
    return roaming::loop(state);
}

State _wifiStateFallback(State state) {
    roaming::abandoned();

    if (!ap::enabled() && (ap::settings::mode() != ApMode::Disabled)) {
        _wifiStartAp();
    }

    if (sta::countNetworks()) {
        DEBUG_MSG_P(PSTR("[WIFI] Retrying STA in %u ms\n"), (unsigned)WIFI_RECONNECT_INTERVAL);
        internal::reconnect_pending = true;
        internal::reconnect_at = millis();
    }

    return State::Idle;
}

State _wifiStateIdle(State state) {
    if (internal::reconnect_pending
        && (static_cast<int32_t>(millis() - internal::reconnect_at) >= WIFI_RECONNECT_INTERVAL))
    {
        internal::reconnect_pending = false;
        internal::network_id = 0;
        internal::connection_retries = 0;
        return State::Connect;
    }

    return state;
}

// ApMode::Fallback: once STA is connected, stop the AP after
// WIFI_FALLBACK_TIMEOUT. Unlike esp8266, connected AP clients do not keep
// it alive; they are expected to move to the STA network.
void _wifiApFallbackCheck() {
    const auto now = millis();
    if (static_cast<int32_t>(now - internal::fallback_check) < WIFI_FALLBACK_TIMEOUT) {
        return;
    }
    internal::fallback_check = now;

    if (!ap::enabled()) return;
    if (ap::internal::manual) return;
    if (ap::settings::mode() != ApMode::Fallback) return;
    if (WiFi.status() != WL_CONNECTED) return;

    DEBUG_MSG_P(PSTR("[WIFI] STA connected (%u AP clients), stopping fallback AP\n"),
        (unsigned)WiFi.softAPgetStationNum());
    _wifiStopAp();
}

void _wifiApConfigure() {
    if (WiFi.getMode() != WIFI_MODE_NULL) {
        radio::configure();
    }

    switch (ap::settings::mode()) {
    case ApMode::Enabled:
        if (!ap::enabled()) _wifiStartAp();
        break;
    case ApMode::Disabled:
        if (sta::countNetworks()) _wifiStopAp();
        break;
    case ApMode::Fallback:
        internal::fallback_check = millis();
        break;
    }
}

void _wifiPublish(espurna::wifi::Event event);

void _wifiLoop() {
    // Drain events that were latched by the WiFi system task. We dispatch
    // here so registered callbacks (MQTT publish, mDNS, Alexa, etc.) run on
    // the loop task with its larger stack and without racing internal state.
    //
    // Both flags use exchange(): rapid connect↔disconnect bursts collapse to
    // at most one of each per loop tick. To preserve causality (avoid
    // publishing "Connected" right after a real disconnect, or vice-versa)
    // we cross-check against the current radio status before firing.
    const bool pending_connect    = internal::pending_connect_publish.exchange(false, std::memory_order_acq_rel);
    const bool pending_disconnect = internal::pending_disconnect_publish.exchange(false, std::memory_order_acq_rel);
    const bool link_up            = (WiFi.status() == WL_CONNECTED);

    if (pending_disconnect && !link_up && internal::station_published) {
        internal::station_published = false;
        _wifiPublish(espurna::wifi::Event::StationDisconnected);
    }
    if (pending_connect && link_up) {
        internal::station_published = true;
        _wifiPublish(espurna::wifi::Event::StationConnected);
    }

#if WIFI_AP_CAPTIVE_SUPPORT
    // Same as esp8266, captive portal only queues requests and those are handled asap
    if (ap::internal::dns_running) {
        ap::internal::dns.processNextRequest();
    }
#endif
    // If link_up disagrees with the pending flag (stale event), we silently
    // drop it — the current state is authoritative.

    auto next_state = internal::state;

    next_state = handle_action(internal::state, [](State state, Action action) {
        if (action == Action::Boot) return State::Connect;
        return state;
    });

    if (next_state != internal::state) {
        roaming::cancel();
    } else {
        switch (internal::state) {
            case State::Boot:
            case State::Init:
                next_state = _wifiStateInit(internal::state);
                if (internal::state == State::Boot) action(Action::Boot);
                break;
            case State::Connect:
                next_state = _wifiStateConnect(internal::state);
                break;
            case State::WaitScan:
                next_state = _wifiStateWaitScan(internal::state);
                break;
            case State::TryNext:
                next_state = _wifiStateTryNext(internal::state);
                break;
            case State::WaitConnected:
                next_state = _wifiStateWaitConnected(internal::state);
                break;
            case State::Connected:
                next_state = _wifiStateConnected(internal::state);
                if (next_state == State::Connected) {
                    _wifiApFallbackCheck();
                }
                break;
            case State::Fallback:
                next_state = _wifiStateFallback(internal::state);
                break;
            case State::Idle:
                next_state = _wifiStateIdle(internal::state);
                break;
        }
    }

    if (next_state != internal::state) {
        // Give AP clients the full timeout after the STA link comes up
        if (next_state == State::Connected) {
            internal::fallback_check = millis();
            roaming::connected();
        }
        internal::state = next_state;
    }

#if WEB_SUPPORT
    // Check scan_active first to skip the IDF call into esp_wifi when no
    // scan has been requested — 99.9% of loop ticks.
    if (internal::scan_active) {
        int scan_count = connection::scan_result(internal::scan_requested);
        if (scan_count == WIFI_SCAN_FAILED) {
            DEBUG_MSG_P(PSTR("[WIFI] Scan failed\n"));
            internal::scan_active = false;
            wsPost(internal::scan_client_id, [](JsonObject& root) {
                root["scanError"] = F("Scan failed, WiFi may be busy connecting");
            });
        } else if (scan_count == 0) {
            internal::scan_active = false;
            wsPost(internal::scan_client_id, [](JsonObject& root) {
                root["scanError"] = F("No networks found");
            });
        } else if (scan_count > 0) {
            DEBUG_MSG_P(PSTR("[WIFI] Scan found %d network(s)\n"), scan_count);
            internal::scan_active = false;

            // Limit how many entries we surface to the UI — the table stays
            // readable and we avoid blasting 50+ small WS frames in a row.
            constexpr int kMaxScanResults = 20;
            const int count = std::min((int)scan_count, kMaxScanResults);

            // One WS frame per network — html/src/wifi.mjs scanResult()
            // appends a single table row per message, so the wire format is
            // a flat [bssid, authmode, rssi, channel, ssid] array.
            for (int i = 0; i < count; ++i) {
                // Snapshot per-entry values before scanDelete() so the
                // captured lambda (run later on the loop task) doesn't
                // reach into freed scan buffers.
                String bssid = WiFi.BSSIDstr(i);
                String ssid = WiFi.SSID(i);
                int rssi = WiFi.RSSI(i);
                int channel = WiFi.channel(i);
                const char* auth = authmode_name(WiFi.encryptionType(i));
                // same as esp8266, only the client that requested the scan receives it
                wsPost(internal::scan_client_id, [bssid, ssid, rssi, channel, auth](JsonObject& root) {
                    JsonArray& network = root.createNestedArray("scanResult");
                    network.add(bssid);
                    network.add(auth);
                    network.add(rssi);
                    network.add(channel);
                    network.add(ssid);
                });
            }
            WiFi.scanDelete();
        }
    }
#endif
}

#if WEB_SUPPORT
void onConnected(JsonObject& root) {
    root["wifiApSsid"] = getSetting("wifiApSsid");
    root["wifiApPass"] = getSetting("wifiApPass");
    root["wifiApMode"] = ap::settings::serialize(ap::settings::mode());

    root["wifiScan"] = roaming::settings::enabled();
    root["wifiScanRssi"] = roaming::settings::threshold();
    root["wifiRoamDelta"] = roaming::settings::delta();

    auto& config = root.createNestedObject("wifiConfig");
    auto& schema = config.createNestedArray("schema");
    schema.add("ssid"); schema.add("pass"); schema.add("ip"); 
    schema.add("gw"); schema.add("mask"); schema.add("dns");

    auto& networks = config.createNestedArray("networks");
    size_t active = sta::countNetworks();
    size_t show = active;
    for (size_t i = 0; i < show; ++i) {
        auto& net = networks.createNestedArray();
        net.add(sta::settings::ssid(i));
        net.add(sta::settings::pass(i));
        net.add(getSetting(espurna::settings::Key{"ip", i}, ""));
        net.add(getSetting(espurna::settings::Key{"gw", i}, ""));
        net.add(getSetting(espurna::settings::Key{"mask", i}, ""));
        net.add(getSetting(espurna::settings::Key{"dns", i}, ""));
    }
    config["max"] = WIFI_MAX_NETWORKS;
}

void _onAction(uint32_t client_id, const char* action_name, JsonObject& data) {
    if (strcmp(action_name, "reconnect") == 0) {
        DEBUG_MSG_P(PSTR("[WIFI] Reconnect requested via WebUI\n"));
        action(Action::TurnOn);
    } else if (strcmp(action_name, "scan") == 0) {
        DEBUG_MSG_P(PSTR("[WIFI] Scan requested via WebUI\n"));
        action(Action::Scan);
        internal::scan_client_id = client_id;
    }
}

// Same as esp8266, generic 'wifi...' keys and indexed network settings
bool onKeyCheck(StringView key, const JsonVariant&) {
    return key.startsWith(STRING_VIEW("wifi"))
        || espurna::settings::query::hasSamePrefix(sta::settings::query::Settings, key);
}
#endif

// Same as esp8266, default values for the `get` command and the settings dump
// (only the keys that exist on ESP32)
namespace query {
namespace keys {

STRING_VIEW_INLINE(Prefix, "wifi");

STRING_VIEW_INLINE(ApSsid, "wifiApSsid");
STRING_VIEW_INLINE(ApPass, "wifiApPass");
STRING_VIEW_INLINE(ApCaptive, "wifiApCaptive");
STRING_VIEW_INLINE(ApChannel, "wifiApChan");
STRING_VIEW_INLINE(ApMode, "wifiApMode");
STRING_VIEW_INLINE(Scan, "wifiScan");
STRING_VIEW_INLINE(ScanRssi, "wifiScanRssi");
STRING_VIEW_INLINE(RoamDelta, "wifiRoamDelta");
STRING_VIEW_INLINE(TxPower, "wifiTxPwr");
STRING_VIEW_INLINE(Sleep, "wifiSleep");

} // namespace keys

namespace internal {

String apSsid() { return ap::settings::ssid(); }
String apPass() { return ap::settings::pass(); }
String apCaptive() { return espurna::settings::internal::serialize(ap::settings::captive()); }
String apChannel() { return espurna::settings::internal::serialize(ap::settings::channel()); }
String apMode() { return ap::settings::serialize(ap::settings::mode()); }
String scan() { return espurna::settings::internal::serialize(roaming::settings::enabled()); }
String scanRssi() { return espurna::settings::internal::serialize(roaming::settings::threshold()); }
String roamDelta() { return espurna::settings::internal::serialize(roaming::settings::delta()); }
String txPower() { return espurna::settings::internal::serialize(radio::settings::tx_power()); }
String sleep() { return radio::sleep_name(radio::settings::sleep()); }

} // namespace internal

static constexpr std::array<espurna::settings::query::Setting, 10> Settings PROGMEM {{
    {keys::ApSsid, internal::apSsid},
    {keys::ApPass, internal::apPass},
    {keys::ApCaptive, internal::apCaptive},
    {keys::ApChannel, internal::apChannel},
    {keys::ApMode, internal::apMode},
    {keys::Scan, internal::scan},
    {keys::ScanRssi, internal::scanRssi},
    {keys::RoamDelta, internal::roamDelta},
    {keys::TxPower, internal::txPower},
    {keys::Sleep, internal::sleep},
}};

// indexed settings for 'sta' connections
bool checkIndexedPrefix(StringView key) {
    return espurna::settings::query::hasSamePrefix(
        sta::settings::query::Settings, key);
}

// generic 'ap', 'scan' and 'radio' configuration
bool checkExactPrefix(StringView key) {
    return key.startsWith(keys::Prefix);
}

espurna::settings::query::Result findIndexedFrom(StringView key) {
    return espurna::settings::query::findFrom(
        sta::countNetworks(),
        sta::settings::query::Settings, key);
}

espurna::settings::query::Result findFrom(StringView key) {
    return espurna::settings::query::findFrom(Settings, key);
}

void setup() {
    settingsRegisterQueryHandler({
        .check = checkIndexedPrefix,
        .get = findIndexedFrom,
    });

    settingsRegisterQueryHandler({
        .check = checkExactPrefix,
        .get = findFrom,
    });
}

} // namespace query

void _wifiPublish(espurna::wifi::Event event) {
    for (auto& callback : internal::callbacks) {
        callback(event);
    }
}

#if TERMINAL_SUPPORT
// RF calibration is done once and then stored in NVS, following boots only do a partial one
// on top of it. When the first boot happened with a poor supply (e.g. powered from the
// USB-UART adapter while flashing), the stored data keeps degrading the radio from then on.
// Erasing it forces a full calibration on the next boot. Settings are not affected.
STRING_VIEW_INLINE(RecalibrateCommand, "WIFI.RECALIBRATE");

void _wifiCommandRecalibrate(::terminal::CommandContext&& ctx) {
    const auto err = esp_phy_erase_cal_data_in_nvs();
    if (err != ESP_OK) {
        terminalError(ctx, F("Could not erase RF calibration data"));
        return;
    }

    ctx.output.print(F("RF calibration data erased, full calibration will run on the next boot\n"));
    prepareReset(CustomResetReason::Terminal);
    terminalOK(ctx);
}

// Same set of commands as esp8266
namespace terminal_commands {

const char* opmode_name(wifi_mode_t mode) {
    switch (mode) {
    case WIFI_MODE_STA:
        return "STA";
    case WIFI_MODE_AP:
        return "AP";
    case WIFI_MODE_APSTA:
        return "AP+STA";
    case WIFI_MODE_NULL:
    case WIFI_MODE_MAX:
        break;
    }

    return "OFF";
}

const char* state_name(State state) {
    switch (state) {
    case State::Boot:
        return "boot";
    case State::Init:
        return "init";
    case State::Connect:
        return "connect";
    case State::WaitScan:
        return "scanning";
    case State::TryNext:
    case State::WaitConnected:
        return "connecting";
    case State::Connected:
        return "connected";
    case State::Fallback:
        return "fallback";
    case State::Idle:
        break;
    }

    return "idle";
}

STRING_VIEW_INLINE(Wifi, "WIFI");

void wifi(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() == 2) {
        const auto id = espurna::settings::internal::convert<size_t>(ctx.argv[1]);
        if (id < WIFI_MAX_NETWORKS) {
            settingsDump(ctx, sta::settings::query::Settings, id);
            terminalOK(ctx);
            return;
        }

        terminalError(ctx, F("Network ID out of configurable range"));
        return;
    }

    const auto mode = WiFi.getMode();
    ctx.output.printf_P(PSTR("OPMODE: %s STATE: %s\n"),
        opmode_name(mode), state_name(internal::state));

    wifi_ps_type_t ps;
    if (esp_wifi_get_ps(&ps) == ESP_OK) {
        ctx.output.printf_P(PSTR("SLEEP: %s\n"), radio::sleep_name(ps));
    }

    if (mode & WIFI_AP) {
        ctx.output.printf_P(PSTR("SoftAP: ssid \"%s\" ip %s channel %u stations %u (%s)\n"),
            WiFi.softAPSSID().c_str(),
            WiFi.softAPIP().toString().c_str(),
            WiFi.channel(),
            WiFi.softAPgetStationNum(),
            ap::internal::manual ? "manual" : ap::settings::serialize(ap::settings::mode()));
    }

    if (mode & WIFI_STA) {
        if (WiFi.status() == WL_CONNECTED) {
            ctx.output.printf_P(PSTR("STA: bssid %s rssi %d channel %u ssid \"%s\" ip %s\n"),
                WiFi.BSSIDstr().c_str(),
                WiFi.RSSI(),
                WiFi.channel(),
                WiFi.SSID().c_str(),
                WiFi.localIP().toString().c_str());
        } else {
            ctx.output.printf_P(PSTR("STA: %s (last disconnect reason %d)\n"),
                state_name(internal::state),
                internal::last_disconnect_reason.load(std::memory_order_relaxed));
        }
    }

    ctx.output.printf_P(PSTR("wifiApMode: %s\nwifiApChan: %u\nwifiApCaptive: %s\nwifiSleep: %s\nwifiTxPwr: %.2f\n"),
        ap::settings::serialize(ap::settings::mode()),
        ap::settings::channel(),
        ap::settings::captive() ? "true" : "false",
        radio::sleep_name(radio::settings::sleep()),
        radio::settings::tx_power());

    ctx.output.printf_P(PSTR("wifiScan: %s\nwifiScanRssi: %d\nwifiRoamDelta: %d\n"),
        roaming::settings::enabled() ? "true" : "false",
        roaming::settings::threshold(),
        roaming::settings::delta());

    if (roaming::settings::enabled() && (internal::state == State::Connected)) {
        const auto elapsed = millis() - roaming::internal::last;
        const auto left = (elapsed < roaming::internal::interval)
            ? (roaming::internal::interval - elapsed) / 1000ul
            : 0ul;
        ctx.output.printf_P(PSTR("roaming: interval %u min, %u stable scan(s), next check in %u s\n"),
            (unsigned)(roaming::internal::interval / (60ul * 1000ul)),
            roaming::internal::stable, (unsigned)left);
    }

    terminalOK(ctx);
}

STRING_VIEW_INLINE(Roam, "WIFI.ROAM");

void roam(::terminal::CommandContext&& ctx) {
    roaming::backoff_housekeeping();

    ctx.output.printf_P(PSTR("wifiScan: %s wifiScanRssi: %d wifiRoamDelta: %d state: %s\n"),
        roaming::settings::enabled() ? "true" : "false",
        roaming::settings::threshold(),
        roaming::settings::delta(),
        state_name(internal::state));

    const auto interval = roaming::internal::pending_count
        ? roaming::build::ConfirmInterval
        : roaming::internal::interval;
    const auto elapsed = millis() - roaming::internal::last;
    const auto left = (elapsed < interval)
        ? (interval - elapsed) / 1000ul
        : 0ul;
    ctx.output.printf_P(PSTR("interval: %u min, stable scans: %u/%u, scanning: %s, next check in %u s\n"),
        (unsigned)(roaming::internal::interval / (60ul * 1000ul)),
        roaming::internal::stable, roaming::build::StableScans,
        roaming::internal::scanning ? "yes" : "no",
        (unsigned)left);

    if (internal::state == State::Connected) {
        ctx.output.printf_P(PSTR("link: %s RSSI %d (median of %u sample(s))\n"),
            WiFi.BSSIDstr().c_str(), roaming::link_rssi(),
            (unsigned)roaming::internal::samples_count);
    }

    if (roaming::internal::pending_count) {
        ctx.output.printf_P(PSTR("pending: %s seen %u/%u\n"),
            connection::bssid_string(roaming::internal::pending_bssid).c_str(),
            roaming::internal::pending_count, roaming::build::ConfirmScans);
    } else {
        ctx.output.printf_P(PSTR("pending: none\n"));
    }

    if (roaming::internal::switching) {
        ctx.output.printf_P(PSTR("switching to: %s\n"),
            connection::bssid_string(roaming::internal::switch_bssid).c_str());
    }

    size_t entries = 0;
    for (const auto& entry : roaming::internal::backoff) {
        if (!entry.failures) {
            continue;
        }

        ++entries;
        const auto backoff_left = roaming::backoff_left(entry);
        if (backoff_left) {
            ctx.output.printf_P(PSTR("backoff: %s failures %u, %u s left\n"),
                connection::bssid_string(entry.bssid).c_str(),
                entry.failures, (unsigned)(backoff_left / 1000ul));
        } else {
            ctx.output.printf_P(PSTR("backoff: %s failures %u, expired\n"),
                connection::bssid_string(entry.bssid).c_str(), entry.failures);
        }
    }

    if (!entries) {
        ctx.output.printf_P(PSTR("backoff: none\n"));
    }

    terminalOK(ctx);
}

STRING_VIEW_INLINE(Reset, "WIFI.RESET");

void reset(::terminal::CommandContext&& ctx) {
    action(Action::StationReconnect);
    _wifiApConfigure();
    terminalOK(ctx);
}

STRING_VIEW_INLINE(Station, "WIFI.STA");

void station(::terminal::CommandContext&& ctx) {
    action(Action::StationToggle);
    terminalOK(ctx);
}

STRING_VIEW_INLINE(AccessPoint, "WIFI.AP");

void access_point(::terminal::CommandContext&& ctx) {
    if (ap::enabled()) {
        _wifiStopAp();
    } else {
        _wifiStartManualAp();
    }
    terminalOK(ctx);
}

STRING_VIEW_INLINE(Off, "WIFI.OFF");

void off(::terminal::CommandContext&& ctx) {
    action(Action::TurnOff);
    terminalOK(ctx);
}

STRING_VIEW_INLINE(On, "WIFI.ON");

void on(::terminal::CommandContext&& ctx) {
    action(Action::TurnOn);
    terminalOK(ctx);
}

STRING_VIEW_INLINE(Scan, "WIFI.SCAN");

void scan(::terminal::CommandContext&& ctx) {
    if (WiFi.scanComplete() != WIFI_SCAN_RUNNING) {
        if (WiFi.scanNetworks(true, true) == WIFI_SCAN_FAILED) {
            terminalError(ctx, F("Scan failed, WiFi may be busy connecting"));
            return;
        }
    }

    // loop is blocked here, blockingDelay() still lets network tasks run and feeds the WDT
    const auto started = millis();
    int16_t count = WIFI_SCAN_RUNNING;
    espurna::time::blockingDelay(
        espurna::duration::Milliseconds(connection::ScanTimeout),
        espurna::duration::Milliseconds(100),
        [&]() {
            count = connection::scan_result(started);
            return count == WIFI_SCAN_RUNNING;
        });

    if (count < 0) {
        terminalError(ctx, F("Scan failed"));
        return;
    }

    for (int index = 0; index < count; ++index) {
        ctx.output.printf_P(PSTR("BSSID: %s AUTH: %13s RSSI: %4d CH: %2d SSID: %s\n"),
            WiFi.BSSIDstr(index).c_str(),
            authmode_name(WiFi.encryptionType(index)),
            WiFi.RSSI(index),
            WiFi.channel(index),
            WiFi.SSID(index).c_str());
    }

    // results may still be waited for by the connection routine or the WebUI
    const bool shared = (internal::state == State::WaitScan)
#if WEB_SUPPORT
        || internal::scan_active
#endif
        ;
    if (!shared) {
        WiFi.scanDelete();
    }

    if (!count) {
        terminalError(ctx, F("No networks found"));
        return;
    }

    terminalOK(ctx);
}

STRING_VIEW_INLINE(Stations, "WIFI.STATIONS");

void stations(::terminal::CommandContext&& ctx) {
    wifi_sta_list_t list{};
    esp_netif_sta_list_t netif_list{};
    if ((esp_wifi_ap_get_sta_list(&list) != ESP_OK)
        || (esp_netif_get_sta_list(&list, &netif_list) != ESP_OK)
        || !netif_list.num)
    {
        terminalError(ctx, F("No stations connected"));
        return;
    }

    for (int index = 0; index < netif_list.num; ++index) {
        const auto& sta = netif_list.sta[index];
        ctx.output.printf_P(PSTR("%02X:%02X:%02X:%02X:%02X:%02X %s\n"),
            sta.mac[0], sta.mac[1], sta.mac[2], sta.mac[3], sta.mac[4], sta.mac[5],
            IPAddress(sta.ip.addr).toString().c_str());
    }

    terminalOK(ctx);
}

STRING_VIEW_INLINE(Network, "NETWORK");

void network(::terminal::CommandContext&& ctx) {
    const auto mode = WiFi.getMode();
    if (mode & WIFI_STA) {
        ctx.output.printf_P(PSTR("sta %4s ip %s gateway %s mask %s\n"),
            (WiFi.status() == WL_CONNECTED) ? "up" : "down",
            WiFi.localIP().toString().c_str(),
            WiFi.gatewayIP().toString().c_str(),
            WiFi.subnetMask().toString().c_str());
    }

    if (mode & WIFI_AP) {
        ctx.output.printf_P(PSTR("ap     up ip %s\n"),
            WiFi.softAPIP().toString().c_str());
    }

    for (int n = 0; n < DNS_MAX_SERVERS; ++n) {
        const auto* addr = dns_getserver(n);
        if (!addr || ip_addr_isany(addr)) {
            break;
        }
        ctx.output.printf_P(PSTR("dns %s\n"), IPAddress(ip_2_ip4(addr)->addr).toString().c_str());
    }

    terminalOK(ctx);
}

} // namespace terminal_commands

static constexpr ::terminal::Command WifiCommands[] PROGMEM {
    {RecalibrateCommand, _wifiCommandRecalibrate},
    {terminal_commands::Wifi, terminal_commands::wifi},
    {terminal_commands::Reset, terminal_commands::reset},
    {terminal_commands::Roam, terminal_commands::roam},
    {terminal_commands::Station, terminal_commands::station},
    {terminal_commands::AccessPoint, terminal_commands::access_point},
    {terminal_commands::Off, terminal_commands::off},
    {terminal_commands::On, terminal_commands::on},
    {terminal_commands::Scan, terminal_commands::scan},
    {terminal_commands::Stations, terminal_commands::stations},
    {terminal_commands::Network, terminal_commands::network},
};
#endif

void _wifiSetup() {
    query::setup();

#if TERMINAL_SUPPORT
    espurna::terminal::add(WifiCommands);
#endif

    WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
        switch (event) {
            case ARDUINO_EVENT_WIFI_STA_START:
                DEBUG_MSG_P(PSTR("[WIFI] Station started\n"));
                break;
            case ARDUINO_EVENT_WIFI_STA_STOP:
                DEBUG_MSG_P(PSTR("[WIFI] Station stopped\n"));
                break;
            case ARDUINO_EVENT_WIFI_STA_CONNECTED:
                DEBUG_MSG_P(PSTR("[WIFI] Station connected\n"));
                break;
            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                internal::wifi_connected.store(true, std::memory_order_release);
                internal::disconnect_triggered.store(false, std::memory_order_relaxed);
                internal::pending_connect_publish.store(true, std::memory_order_release);
                DEBUG_MSG_P(PSTR("[WIFI] CONNECTED! IP: %s\n"), WiFi.localIP().toString().c_str());
                break;
            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                {
                    const int reason = (int)info.wifi_sta_disconnected.reason;
                    internal::wifi_connected.store(false, std::memory_order_relaxed);
                    internal::last_disconnect_reason.store(reason, std::memory_order_relaxed);
                    internal::last_disconnect_at.store(millis(), std::memory_order_relaxed);
                    internal::disconnect_triggered.store(true, std::memory_order_release);
                    internal::pending_disconnect_publish.store(true, std::memory_order_release);
                    uint8_t* b = info.wifi_sta_disconnected.bssid;
                    DEBUG_MSG_P(PSTR("[WIFI] DISCONNECTED! Reason: %d (%s), BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n"),
                        reason, WiFi.disconnectReasonName((wifi_err_reason_t)reason),
                        b[0], b[1], b[2], b[3], b[4], b[5]);
                }
                break;
        }
    });

#if WEB_SUPPORT
    wsRegister()
        .onConnected(onConnected)
        .onAction(_onAction)
        .onKeyCheck(onKeyCheck, ::espurna::web::ws::Callbacks::Prepend{});
#endif

    espurnaRegisterReload([]() {
        _wifiApConfigure();

        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_MSG_P(PSTR("[WIFI] Reload: not connected, initiating connection...\n"));
            action(Action::TurnOn);
            return;
        }

        // Already connected: only reconnect if the active SSID no longer
        // matches any of the configured networks (user changed WiFi settings).
        const auto active_ssid = WiFi.SSID();
        bool found = false;
        for (size_t i = 0; i < WIFI_MAX_NETWORKS; ++i) {
            if (sta::settings::ssid(i) == active_ssid) {
                found = true;
                break;
            }
        }
        if (!found) {
            DEBUG_MSG_P(PSTR("[WIFI] Reload: active SSID not in config, reconnecting...\n"));
            action(Action::TurnOn);
        } else {
            DEBUG_MSG_P(PSTR("[WIFI] Reload: WiFi config unchanged, skipping reconnect\n"));
        }
    });

    espurnaRegisterLoop([]() {
        _wifiLoop();
    });
}

} // namespace
} // namespace wifi
} // namespace espurna

// Global API
void wifiReload() { espurna::wifi::action(espurna::wifi::Action::TurnOn); }
void wifiDisconnect() {
    // Funnel through the action queue (mutex-guarded) so the FSM
    // transition happens on the loop task — not whichever task
    // (MQTT / terminal / web) the caller is on.
    // Not TurnOff! That shuts the radio down and nothing brings it back.
    espurna::wifi::action(espurna::wifi::Action::StationReconnect);
}
void wifiRegister(espurna::wifi::EventCallback callback) { espurna::wifi::internal::callbacks.push_back(callback); }
bool wifiConnected() { return WiFi.status() == WL_CONNECTED; }
void wifiSetup() { espurna::wifi::_wifiSetup(); }
// Same as esp8266, i.e. whether clients can connect to our AP
bool wifiConnectable() { return espurna::wifi::ap::enabled(); }
// Through the FSM, same as esp8266 (otherwise it immediately reconnects)
void wifiTurnOff() { espurna::wifi::action(espurna::wifi::Action::TurnOff); }
void wifiTurnOn() { espurna::wifi::action(espurna::wifi::Action::TurnOn); }
IPAddress wifiStaIp() { return WiFi.localIP(); }
String wifiStaSsid() { return WiFi.SSID(); }
void wifiToggleAp() { if (espurna::wifi::ap::enabled()) espurna::wifi::_wifiStopAp(); else espurna::wifi::_wifiStartManualAp(); }
void wifiToggleSta() { espurna::wifi::action(espurna::wifi::Action::StationToggle); }
void wifiStartAp() { espurna::wifi::_wifiStartManualAp(); }
bool wifiDisabled() { return !espurna::wifi::internal::enabled; }
void wifiDisable() { espurna::wifi::action(espurna::wifi::Action::TurnOff); }
// Same as esp8266, check right away whether the fallback AP is still needed (e.g. WebUI client left)
void wifiApCheck() {
    espurna::wifi::internal::fallback_check = millis() - WIFI_FALLBACK_TIMEOUT;
}
size_t wifiApStations() { return WiFi.softAPgetStationNum(); }
IPAddress wifiApIp() { return WiFi.softAPIP(); }

espurna::wifi::StaNetwork wifiStaInfo() {
    espurna::wifi::StaNetwork info;
    info.ssid = WiFi.SSID();
    uint8_t* bssid = WiFi.BSSID();
    if (bssid) {
        memcpy(info.bssid.data(), bssid, 6);
    } else {
        info.bssid.fill(0);
    }
    info.channel = WiFi.channel();
    info.rssi = WiFi.RSSI();
    return info;
}

espurna::wifi::SoftApNetwork wifiApInfo() {
    espurna::wifi::SoftApNetwork info;
    info.ssid = WiFi.softAPSSID();
    uint8_t mac[6];
    if (esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP) == ESP_OK) {
        memcpy(info.bssid.data(), mac, 6);
    } else {
        info.bssid.fill(0);
    }
    info.channel = WiFi.channel();
    return info;
}

#endif
