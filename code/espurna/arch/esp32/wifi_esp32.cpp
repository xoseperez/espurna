#if defined(ARDUINO_ARCH_ESP32)

#include <WiFi.h>
#include <IPAddress.h>

#include <algorithm>
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

enum class ScanError {
    None,
    AlreadyScanning,
    Busy,
    NoNetworks,
    System,
};

enum class Action {
    AccessPointFallback,
    AccessPointFallbackCheck,
    AccessPointStart,
    AccessPointStop,
    Boot,
    StationConnect,
    StationContinueConnect,
    StationDisconnect,
    StationTryConnectBetter,
    TurnOff,
    TurnOn,
};

using Actions = std::list<Action>;
using ActionsQueue = std::queue<Action, Actions>;

enum class State {
    Boot,
    Connect,
    TryConnectBetter,
    Connected,
    Idle,
    Init,
    Timeout,
    Fallback,
    WaitScan,
    WaitScanWithoutCurrent,
    WaitConnected
};

struct IpSettings {
    IpSettings() = default;
    
    IpSettings(IPAddress ip, IPAddress netmask, IPAddress gateway, IPAddress dns) :
        _ip(ip), _netmask(netmask), _gateway(gateway), _dns(dns)
    {}

    const IPAddress& ip() const { return _ip; }
    const IPAddress& netmask() const { return _netmask; }
    const IPAddress& gateway() const { return _gateway; }
    const IPAddress& dns() const { return _dns; }

    explicit operator bool() const {
        return ((uint32_t)_ip != 0) && ((uint32_t)_netmask != 0) && ((uint32_t)_gateway != 0);
    }

private:
    IPAddress _ip;
    IPAddress _netmask;
    IPAddress _gateway;
    IPAddress _dns;
};

struct Network {
    Network() = delete;
    Network(String ssid, String passphrase) :
        _ssid(ssid), _passphrase(passphrase)
    {}

    Network(String ssid, String passphrase, IpSettings settings) :
        _ssid(ssid), _passphrase(passphrase), _ipSettings(settings)
    {}

    bool dhcp() const { return !_ipSettings; }
    const String& ssid() const { return _ssid; }
    const String& passphrase() const { return _passphrase; }
    const IpSettings& ipSettings() const { return _ipSettings; }

private:
    String _ssid;
    String _passphrase;
    IpSettings _ipSettings;
};

using Networks = std::list<Network>;

namespace internal {

bool enabled { false };
bool wifi_connected { false };
ActionsQueue actions;

State state { State::Boot };
State last_state { state };

uint8_t network_id { 0 };
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
        state = handler(state, internal::actions.front());
        internal::actions.pop();
    }

    return state;
}

#if WEB_SUPPORT
void onConnected(JsonObject& root) {
    root["wifiApSsid"] = getSetting("wifiApSsid");
    root["wifiApPass"] = getSetting("wifiApPass");
}

void _onAction(uint32_t client_id, const char* action, JsonObject& data) {
    if (strcmp(action, "scan") == 0) {
        WiFi.scanNetworks(true);
        internal::scan_active = true;
        internal::scan_client_id = client_id;
    }
}

void _wifiScanCheck() {
    if (!internal::scan_active) return;

    int16_t n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) {
        internal::scan_active = false;
    } else if (n >= 0) {
        for (int i = 0; i < n; ++i) {
            String bssid = WiFi.BSSIDstr(i);
            String ssid = WiFi.SSID(i);
            int32_t rssi = WiFi.RSSI(i);
            uint8_t ch = WiFi.channel(i);
            String auth = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) ? "yes" : "no";

            wsPost(internal::scan_client_id, [bssid, ssid, rssi, ch, auth](JsonObject& root) {
                JsonArray& network = root.createNestedArray("scanResult");
                network.add(bssid);
                network.add(auth);
                network.add(rssi);
                network.add(ch);
                network.add(ssid);
            });
        }
        WiFi.scanDelete();
        internal::scan_active = false;
    }
}

bool onKeyCheck(StringView key, const JsonVariant& value) {
    return key.startsWith("wifi") 
        || key.startsWith("ssid") 
        || key.startsWith("pass")
        || key.startsWith("ip")
        || key.startsWith("gw")
        || key.startsWith("mask")
        || key.startsWith("dns")
        || key.startsWith("hostname")
        || key.startsWith("desc")
        || key.startsWith("hb");
}
#endif

#define WIFI_CRASH_BIT 0x80000000

void _wifiStartAp() {
    String ssid = getSetting("wifiApSsid", "");
    if (!ssid.length()) {
        ssid = String(APP_NAME) + "_" + systemShortChipId().c_str();
    }
    String pass = getSetting("wifiApPass", "fibonacci");
    
    Serial.printf("[WIFI] Starting AP: %s\n", ssid.c_str());
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid.c_str(), pass.c_str());
}

State _wifiStateInit(State state) {
    WiFi.persistent(false);
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    internal::enabled = true;
    return State::Idle;
}

State _wifiStateIdle(State state) {
    return handle_action(state, [](State state, Action action) {
        switch (action) {
            case Action::Boot:
            case Action::TurnOn:
                return State::Connect;
            case Action::TurnOff:
                WiFi.mode(WIFI_OFF);
                return State::Idle;
            default:
                return state;
        }
    });
}

State _wifiStateConnect(State state) {
    if (WiFi.isConnected()) return State::Connected;

    for (uint8_t i = 0; i < WIFI_MAX_NETWORKS; ++i) {
        uint8_t id = (internal::network_id + i) % WIFI_MAX_NETWORKS;
        String ssid = getSetting(espurna::settings::Key{"ssid", id}, "");
        if (ssid.length() > 0) {
            String pass = getSetting(espurna::settings::Key{"pass", id}, "");
            internal::network_id = (id + 1) % WIFI_MAX_NETWORKS;
            
            Serial.printf("[WIFI] Trying to connect to %s (id:%u)\n", ssid.c_str(), id);

            if (hasSetting(espurna::settings::Key{"ip", id})) {
                IPAddress ip, gw, mask, dns;
                ip.fromString(getSetting(espurna::settings::Key{"ip", id}, ""));
                gw.fromString(getSetting(espurna::settings::Key{"gw", id}, ""));
                mask.fromString(getSetting(espurna::settings::Key{"mask", id}, ""));
                dns.fromString(getSetting(espurna::settings::Key{"dns", id}, ""));
                WiFi.config(ip, gw, mask, dns);
            } else {
                WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
            }

            WiFi.begin(ssid.c_str(), pass.c_str());
            internal::last_connect = millis();
            return State::WaitConnected;
        }
    }

    Serial.println("[WIFI] No configured networks found.");
    return State::Fallback;
}

State _wifiStateWaitConnected(State state) {
    if (WiFi.isConnected()) return State::Connected;
    
    if (millis() - internal::last_connect > 15000) {
        Serial.println("[WIFI] Connection timeout");
        WiFi.disconnect();
        return State::Connect;
    }

    return handle_action(state, [](State state, Action action) {
        if (action == Action::StationDisconnect) {
            WiFi.disconnect();
            return State::Connect;
        }
        return state;
    });
}

State _wifiStateConnected(State state) {
    if (!WiFi.isConnected()) return State::Connect;

    return handle_action(state, [](State state, Action action) {
        if (action == Action::StationDisconnect) {
            WiFi.disconnect();
            return State::Connect;
        }
        return state;
    });
}

State _wifiStateFallback(State state) {
    _wifiStartAp();
    return State::Idle;
}

void _wifiLoop() {
    auto next_state = internal::state;

    switch (internal::state) {
        case State::Boot:
            next_state = _wifiStateInit(internal::state);
            action(Action::Boot);
            break;
        case State::Init:
            next_state = _wifiStateInit(internal::state);
            break;
        case State::Idle:
            next_state = _wifiStateIdle(internal::state);
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

    if (next_state != internal::state) {
        internal::last_state = internal::state;
        internal::state = next_state;
    }
}

void _wifiSetup() {
    bool crashed = (Rtcmem->sys & WIFI_CRASH_BIT);
    if (crashed) {
        Serial.println("[WIFI] Crash protection triggered! WiFi is OFF.");
        return;
    }

    Serial.println("[WIFI] Starting WiFi...");
    Rtcmem->sys |= WIFI_CRASH_BIT;

    WiFi.onEvent([=](arduino_event_id_t event, arduino_event_info_t info) {
        Event e = Event::Initial;
        switch (event) {
            case ARDUINO_EVENT_WIFI_STA_START:
                Serial.println("[WIFI] Station started");
                break;
            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                internal::wifi_connected = true;
                e = Event::StationConnected;
                Serial.print("[WIFI] Connected! IP: ");
                Serial.println(WiFi.localIP());
                Rtcmem->sys &= ~WIFI_CRASH_BIT;
                break;
            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                internal::wifi_connected = false;
                e = Event::StationDisconnected;
                Serial.println("[WIFI] Disconnected!");
                break;
            case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
                Serial.println("[WIFI] AP Client connected");
                break;
            case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
                Serial.println("[WIFI] AP Client disconnected");
                break;
            default:
                break;
        }

        if (e != Event::Initial) {
            for (auto& cb : internal::callbacks) {
                cb(e);
            }
        }
    });

#if WEB_SUPPORT
    wsRegister()
        .onConnected(onConnected)
        .onAction(_onAction)
        .onKeyCheck(onKeyCheck);
#endif

    espurnaRegisterLoop([]() {
#if WEB_SUPPORT
        _wifiScanCheck();
#endif
        _wifiLoop();
    });
}

#if TERMINAL_SUPPORT
namespace terminal {

void wifi(::terminal::CommandContext&& ctx) {
    ctx.output.printf_P(PSTR("WiFi Mode: %d\n"), WiFi.getMode());
    ctx.output.printf_P(PSTR("WiFi Connected: %s\n"), WiFi.isConnected() ? "YES" : "NO");
    if (WiFi.isConnected()) {
        ctx.output.printf_P(PSTR("STA SSID: %s\n"), WiFi.SSID().c_str());
        ctx.output.printf_P(PSTR("STA IP:   %s\n"), WiFi.localIP().toString().c_str());
    }
    ctx.output.printf_P(PSTR("AP SSID:  %s\n"), WiFi.softAPSSID().c_str());
    ctx.output.printf_P(PSTR("AP IP:    %s\n"), WiFi.softAPIP().toString().c_str());
    terminalOK(ctx);
}

void scan(::terminal::CommandContext&& ctx) {
    int n = WiFi.scanNetworks();
    if (n == 0) {
        ctx.output.println(F("No networks found"));
    } else {
        ctx.output.printf_P(PSTR("Found %d networks:\n"), n);
        for (int i = 0; i < n; ++i) {
            ctx.output.printf_P(PSTR("%d: %s (%d) %s\n"), i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
        }
    }
    terminalOK(ctx);
}

void setup() {
    static constexpr ::terminal::Command List[] PROGMEM {
        {"WIFI", wifi},
        {"WIFI.SCAN", scan},
    };
    espurna::terminal::add(List);
}

} // namespace terminal
#endif

} // namespace
} // namespace wifi
} // namespace espurna

void wifiReload() {
    espurna::wifi::action(espurna::wifi::Action::StationDisconnect);
}

void wifiDisconnect() {
    espurna::wifi::action(espurna::wifi::Action::StationDisconnect);
}

void wifiRegister(espurna::wifi::EventCallback callback) {
    espurna::wifi::internal::callbacks.push_back(callback);
}

bool wifiConnected() { return WiFi.status() == WL_CONNECTED; }

void wifiSetup() {
#if defined(ESP32)
    espurna::wifi::_wifiSetup();
#if TERMINAL_SUPPORT
    espurna::wifi::terminal::setup();
#endif
#endif
}

bool wifiConnectable() { return true; }
void wifiTurnOff() { espurna::wifi::action(espurna::wifi::Action::TurnOff); }
void wifiTurnOn() { espurna::wifi::action(espurna::wifi::Action::TurnOn); }
IPAddress wifiStaIp() { return WiFi.localIP(); }
String wifiStaSsid() { return WiFi.SSID(); }
void wifiToggleAp() {}
void wifiToggleSta() {}
void wifiStartAp() { espurna::wifi::action(espurna::wifi::Action::AccessPointStart); }
bool wifiDisabled() { return !espurna::wifi::internal::enabled; }
void wifiDisable() { espurna::wifi::action(espurna::wifi::Action::TurnOff); }
void wifiApCheck() {}
size_t wifiApStations() { return WiFi.softAPgetStationNum(); }
IPAddress wifiApIp() { return WiFi.softAPIP(); }

espurna::wifi::StaNetwork wifiStaInfo() {
    espurna::wifi::StaNetwork info;
    info.ssid = WiFi.SSID();
    uint8_t* bssid = WiFi.BSSID();
    if (bssid) {
        memcpy(info.bssid.data(), bssid, 6);
    }
    info.channel = WiFi.channel();
    info.rssi = WiFi.RSSI();
    return info;
}

espurna::wifi::SoftApNetwork wifiApInfo() {
    espurna::wifi::SoftApNetwork info;
    info.ssid = WiFi.softAPSSID();
    return info;
}

#endif
