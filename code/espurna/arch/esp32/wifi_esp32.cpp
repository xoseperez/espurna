#if defined(ARDUINO_ARCH_ESP32)

#include <WiFi.h>
#include <IPAddress.h>
#include <esp_wifi.h>
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

#if WEB_SUPPORT
#include "ws.h"
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
    StationDisconnect,
    TurnOff,
    TurnOn,
    Scan,
};

using Actions = std::list<Action>;
using ActionsQueue = std::queue<Action, Actions>;

enum class State {
    Boot,
    Connect,
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

    // Set from WiFi system task, consumed by loop task in _wifiLoop().
    // Keeps registered callbacks (MQTT/NTP/Alexa/mDNS) off the WiFi task stack.
    std::atomic<bool> pending_connect_publish { false };
    std::atomic<bool> pending_disconnect_publish { false };

    ActionsQueue actions;

    State state { State::Boot };
    State last_state { state };

    uint8_t network_id { 0 };
    uint8_t connection_retries { 0 };
    unsigned long last_connect { 0 };

    std::vector<espurna::wifi::EventCallback> callbacks;

#if WEB_SUPPORT
    bool scan_active { false };
    uint32_t scan_client_id { 0 };
#endif
} // namespace internal

void action(Action value) {
    internal::actions.push(value);
}

template <typename T>
State handle_action(State state, T&& handler) {
    if (!internal::actions.empty()) {
        auto value = internal::actions.front();
        internal::actions.pop();
        
        if (value == Action::TurnOn || value == Action::StationConnect) {
            WiFi.disconnect();
            internal::network_id = 0;
            internal::connection_retries = 0;
            return State::Connect;
        }
        if (value == Action::TurnOff) {
            WiFi.mode(WIFI_OFF);
            return State::Idle;
        }
        if (value == Action::Scan) {
#if WEB_SUPPORT
            if (WiFi.scanComplete() != WIFI_SCAN_RUNNING) {
                DEBUG_MSG_P(PSTR("[WIFI] Starting async scan...\n"));
                WiFi.scanNetworks(true); 
                internal::scan_active = true;
            } else {
                DEBUG_MSG_P(PSTR("[WIFI] Scan already in progress\n"));
            }
#endif
            return state;
        }

        state = handler(state, value);
    }

    return state;
}

namespace sta {
namespace settings {
    String ssid(size_t index) { 
        String value = getSetting(espurna::settings::Key{"ssid", index}, "");
        value.trim();
        if (value.length() > 0 && ((unsigned char)value[0] == 255 || !isprint(value[0]))) return "";
        return value;
    }
    String pass(size_t index) { 
        String value = getSetting(espurna::settings::Key{"pass", index}, "");
        value.trim();
        return value;
    }
    String ip(size_t index) { return getSetting(espurna::settings::Key{"ip", index}); }
    String gw(size_t index) { return getSetting(espurna::settings::Key{"gw", index}); }
    String mask(size_t index) { return getSetting(espurna::settings::Key{"mask", index}); }
    String dns(size_t index) { return getSetting(espurna::settings::Key{"dns", index}); }

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

void _wifiStartAp() {
    String ssid = getSetting("wifiApSsid", "");
    if (!ssid.length()) ssid = String(APP_NAME) + "_" + systemShortChipId().c_str();
    String pass = getSetting("wifiApPass", "fibonacci");
    if (pass.length() > 0 && pass.length() < 8) pass = "fibonacci";

    DEBUG_MSG_P(PSTR("[WIFI] Starting AP: %s (pass: %s)\n"), ssid.c_str(), pass.c_str());
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid.c_str(), pass.length() > 0 ? pass.c_str() : NULL);
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
    WiFi.setSleep(false); 
    WiFi.setTxPower(WIFI_POWER_19_5dBm); // Max power to improve handshake success
    
    WiFi.mode(WIFI_MODE_NULL);
    WiFi.setHostname(systemHostname().c_str());
    _wifiStartAp();

    // Force EU country code for better channel 12/13 support
    wifi_country_t country = {.cc="EU", .schan=1, .nchan=13, .policy=WIFI_COUNTRY_POLICY_AUTO};
    esp_wifi_set_country(&country);

    // Force 802.11b/g only. 'n' is often problematic in very noisy environments
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);
    
    internal::enabled = true;
    return State::Idle;
}

State _wifiStateConnect(State state) {
    if (WiFi.status() == WL_CONNECTED) return State::Connected;

    for (uint8_t i = 0; i < WIFI_MAX_NETWORKS; ++i) {
        uint8_t id = (internal::network_id + i) % WIFI_MAX_NETWORKS;
        String ssid = sta::settings::ssid(id);
        if (ssid.length() > 0) {
            String pass = sta::settings::pass(id);
            internal::network_id = (id + 1) % WIFI_MAX_NETWORKS;
            
            DEBUG_MSG_P(PSTR("[WIFI] Attempting connection to SSID: %s (id:%u)\n"), ssid.c_str(), id);

            if (WiFi.getMode() != WIFI_AP_STA) {
                WiFi.mode(WIFI_AP_STA);
            }
            esp_wifi_set_ps(WIFI_PS_NONE); 

            String ip = sta::settings::ip(id);
            if (ip.length() > 0) {
                IPAddress ipAddr, gwAddr, maskAddr, dnsAddr;
                ipAddr.fromString(ip);
                gwAddr.fromString(sta::settings::gw(id));
                maskAddr.fromString(sta::settings::mask(id));
                dnsAddr.fromString(sta::settings::dns(id));
                WiFi.config(ipAddr, gwAddr, maskAddr, dnsAddr);
            }

            internal::disconnect_triggered.store(false, std::memory_order_relaxed); // Reset flag before starting
            WiFi.disconnect();
            delay(100); // Small pause for driver to settle
            
            WiFi.setHostname(systemHostname().c_str());
            WiFi.begin(ssid.c_str(), pass.c_str());
            
            // Re-apply best AP selection settings after begin()
            wifi_config_t config;
            if (esp_wifi_get_config(WIFI_IF_STA, &config) == ESP_OK) {
                config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
                config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
                config.sta.threshold.rssi = -100;
                esp_wifi_set_config(WIFI_IF_STA, &config);
            }

            internal::last_connect = millis();
            return State::WaitConnected;
        }
    }

    DEBUG_MSG_P(PSTR("[WIFI] No networks found to connect\n"));
    return State::Fallback;
}

State _wifiStateWaitConnected(State state) {
    if (WiFi.status() == WL_CONNECTED || internal::wifi_connected.load(std::memory_order_acquire)) {
        DEBUG_MSG_P(PSTR("[WIFI] Successfully connected!\n"));
        return State::Connected;
    }

    // If driver explicitly told us it failed
    if (internal::disconnect_triggered.exchange(false, std::memory_order_acq_rel)) {
        // Ignore disconnects that happen too soon after begin (likely transient or from disconnect() call)
        if (millis() - internal::last_connect < 500) {
            return state;
        }

        DEBUG_MSG_P(PSTR("[WIFI] Connection failed (Reason: %d), retrying in 2s...\n"),
            internal::last_disconnect_reason.load(std::memory_order_relaxed));
        internal::last_connect = millis() + 2000; // Small delay before next attempt
        return State::WaitConnected;
    }

    // Artificial delay handler
    if (millis() < internal::last_connect) {
         return state;
    }

    if (millis() - internal::last_connect > 30000) { 
        DEBUG_MSG_P(PSTR("[WIFI] Connection timeout (status: %d)\n"), (int)WiFi.status());
        WiFi.disconnect();
        internal::connection_retries++;
        if (internal::connection_retries >= (WIFI_MAX_NETWORKS * 2)) {
             DEBUG_MSG_P(PSTR("[WIFI] Too many retries, falling back to AP\n"));
             return State::Fallback;
        }
        return State::Connect;
    }
    return state;
}

State _wifiStateConnected(State state) {
    if (WiFi.status() != WL_CONNECTED) return State::Connect;
    internal::connection_retries = 0;
    return state;
}

State _wifiStateFallback(State state) {
    if (!(WiFi.getMode() & WIFI_AP)) {
        _wifiStartAp();
    }
    return State::Idle;
}

void _wifiPublish(espurna::wifi::Event event);

void _wifiLoop() {
    // Drain events that were latched by the WiFi system task. We dispatch
    // here so registered callbacks (MQTT publish, mDNS, Alexa, etc.) run on
    // the loop task with its larger stack and without racing internal state.
    if (internal::pending_connect_publish.exchange(false, std::memory_order_acq_rel)) {
        _wifiPublish(espurna::wifi::Event::StationConnected);
    }
    if (internal::pending_disconnect_publish.exchange(false, std::memory_order_acq_rel)) {
        _wifiPublish(espurna::wifi::Event::StationDisconnected);
    }

    auto next_state = internal::state;

    next_state = handle_action(internal::state, [](State state, Action action) {
        if (action == Action::Boot) return State::Connect;
        return state;
    });

    if (next_state == internal::state) {
        switch (internal::state) {
            case State::Boot:
            case State::Init:
                next_state = _wifiStateInit(internal::state);
                if (internal::state == State::Boot) action(Action::Boot);
                break;
            case State::Connect:
                next_state = _wifiStateConnect(internal::state);
                break;
            case State::WaitConnected:
                next_state = _wifiStateWaitConnected(internal::state);
                break;
            case State::Connected:
                next_state = _wifiStateConnected(internal::state);
                break;
            case State::Fallback:
                next_state = _wifiStateFallback(internal::state);
                break;
            default:
                break;
        }
    }

    if (next_state != internal::state) {
        internal::state = next_state;
    }

#if WEB_SUPPORT
    int scan_count = WiFi.scanComplete();
    if (scan_count >= 0 && internal::scan_active) {
        internal::scan_active = false;
        int count = scan_count;
        if (count > 10) count = 10; // Limit to 10 networks for stability
        
        for (int i = 0; i < count; ++i) {
            String bssid = WiFi.BSSIDstr(i);
            String ssid = WiFi.SSID(i);
            int rssi = WiFi.RSSI(i);
            int channel = WiFi.channel(i);
            String enc = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) ? "yes" : "no";
            
            wsPost([bssid, ssid, rssi, channel, enc](JsonObject& root) {
                JsonArray& network = root.createNestedArray("scanResult");
                network.add(bssid);
                network.add(enc);
                network.add(rssi);
                network.add(channel);
                network.add(ssid);
            });
            yield(); // Give system time to handle background tasks and WDT
        }
        WiFi.scanDelete();
    }
#endif
}

#if WEB_SUPPORT
void onConnected(JsonObject& root) {
    root["wifiApSsid"] = getSetting("wifiApSsid");
    root["wifiApPass"] = getSetting("wifiApPass");
    root["wifiApMode"] = 2; // Enabled

    auto& config = root.createNestedObject("wifiConfig");
    auto& schema = config.createNestedArray("schema");
    schema.add("ssid"); schema.add("pass"); schema.add("ip"); 
    schema.add("gw"); schema.add("mask"); schema.add("dns");

    auto& networks = config.createNestedArray("networks");
    size_t active = sta::countNetworks();
    size_t show = std::min(size_t(WIFI_MAX_NETWORKS), active + 1);
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

bool onKeyCheck(StringView key, const JsonVariant& value) {
    return true;
}
#endif

void _wifiPublish(espurna::wifi::Event event) {
    for (auto& callback : internal::callbacks) {
        callback(event);
    }
}

void _wifiSetup() {
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
                    internal::disconnect_triggered.store(true, std::memory_order_release);
                    internal::pending_disconnect_publish.store(true, std::memory_order_release);
                    uint8_t* b = info.wifi_sta_disconnected.bssid;
                    DEBUG_MSG_P(PSTR("[WIFI] DISCONNECTED! Reason: %d, BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n"),
                        reason, b[0], b[1], b[2], b[3], b[4], b[5]);
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
        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_MSG_P(PSTR("[WIFI] Reload: WiFi not connected, initiating connection...\n"));
            action(Action::TurnOn);
        } else {
            DEBUG_MSG_P(PSTR("[WIFI] Reload: WiFi already connected, skipping reconnect\n"));
        }
    });

    espurnaRegisterLoop([]() {
        _wifiLoop();
    });
}

} // namespace
} // namespace wifi
} // namespace espurna

// ГЛОБАЛЬНЫЕ ФУНКЦИИ
void wifiReload() { espurna::wifi::action(espurna::wifi::Action::TurnOn); }
void wifiDisconnect() {
    espurna::wifi::internal::wifi_connected.store(false, std::memory_order_relaxed);
    WiFi.disconnect();
}
void wifiRegister(espurna::wifi::EventCallback callback) { espurna::wifi::internal::callbacks.push_back(callback); }
bool wifiConnected() { return WiFi.status() == WL_CONNECTED; }
void wifiSetup() { espurna::wifi::_wifiSetup(); }
bool wifiConnectable() { return true; }
void wifiTurnOff() { WiFi.mode(WIFI_OFF); }
void wifiTurnOn() { espurna::wifi::action(espurna::wifi::Action::TurnOn); }
IPAddress wifiStaIp() { return WiFi.localIP(); }
String wifiStaSsid() { return WiFi.SSID(); }
void wifiToggleAp() { if (WiFi.getMode() & WIFI_AP) WiFi.mode(WIFI_STA); else espurna::wifi::_wifiStartAp(); }
void wifiToggleSta() {}
void wifiStartAp() { espurna::wifi::_wifiStartAp(); }
bool wifiDisabled() { return !espurna::wifi::internal::enabled; }
void wifiDisable() { WiFi.mode(WIFI_OFF); }
void wifiApCheck() {}
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
