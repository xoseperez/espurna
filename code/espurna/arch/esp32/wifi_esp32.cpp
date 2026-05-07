#if defined(ARDUINO_ARCH_ESP32)

#include <WiFi.h>
#include <IPAddress.h>

#include "espurna.h"
#include "wifi_orch.h"
#include "rtcmem.h"
#include "settings.h"

#if WEB_SUPPORT
#include "ws.h"
#endif

namespace espurna {
namespace wifi {

static bool _wifi_connected = false;
static std::vector<espurna::wifi::EventCallback> _callbacks;
static uint8_t _wifi_network_id = 0;
static unsigned long _wifi_last_connect = 0;

#if WEB_SUPPORT
static bool _wifi_scan_active = false;
static uint32_t _wifi_scan_client_id = 0;

void onConnected(JsonObject& root) {
    root["wifiApSsid"] = getSetting("wifiApSsid");
    root["wifiApPass"] = getSetting("wifiApPass");
}

void _onAction(uint32_t client_id, const char* action, JsonObject& data) {
    if (strcmp(action, "scan") == 0) {
        WiFi.scanNetworks(true);
        _wifi_scan_active = true;
        _wifi_scan_client_id = client_id;
    }
}

void _wifiScanCheck() {
    if (!_wifi_scan_active) return;

    int16_t n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) {
        _wifi_scan_active = false;
    } else if (n >= 0) {
        for (int i = 0; i < n; ++i) {
            String bssid = WiFi.BSSIDstr(i);
            String ssid = WiFi.SSID(i);
            int32_t rssi = WiFi.RSSI(i);
            uint8_t ch = WiFi.channel(i);
            String auth = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) ? "yes" : "no";

            wsPost(_wifi_scan_client_id, [bssid, ssid, rssi, ch, auth](JsonObject& root) {
                JsonArray& network = root.createNestedArray("scanResult");
                network.add(bssid);
                network.add(auth);
                network.add(rssi);
                network.add(ch);
                network.add(ssid);
            });
        }
        WiFi.scanDelete();
        _wifi_scan_active = false;
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
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid.c_str(), pass.c_str());
}

void _wifiConnect() {
    if (WiFi.isConnected()) return;

    bool found = false;
    for (uint8_t i = 0; i < WIFI_MAX_NETWORKS; ++i) {
        uint8_t id = (_wifi_network_id + i) % WIFI_MAX_NETWORKS;
        String ssid = getSetting(espurna::settings::Key{"ssid", id}, "");
        if (ssid.length() > 0) {
            found = true;
            String pass = getSetting(espurna::settings::Key{"pass", id}, "");
            _wifi_network_id = (id + 1) % WIFI_MAX_NETWORKS;
            Serial.printf("[WIFI] Trying to connect to %s (id:%u)\n", ssid.c_str(), id);

            if (hasSetting(espurna::settings::Key{"ip", id})) {
                IPAddress ip, gw, mask, dns;
                ip.fromString(getSetting(espurna::settings::Key{"ip", id}, ""));
                gw.fromString(getSetting(espurna::settings::Key{"gw", id}, ""));
                mask.fromString(getSetting(espurna::settings::Key{"mask", id}, ""));
                dns.fromString(getSetting(espurna::settings::Key{"dns", id}, ""));
                WiFi.config(ip, gw, mask, dns);
            }

            WiFi.begin(ssid.c_str(), pass.c_str());
            _wifi_last_connect = millis();
            return;
        }
    }

    if (!found) {
        Serial.println("[WIFI] No configured networks found in memory.");
        _wifiStartAp();
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

    WiFi.disconnect(true, true);
    ::delay(100);

    WiFi.persistent(false);
    WiFi.mode(WIFI_AP_STA);

    String hostname = systemHostname();
    WiFi.setHostname(hostname.c_str());

    // Configure SoftAP IP
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));

    String ap_ssid = getSetting("wifiApSsid", "ESPURNA_" + String((uint32_t)ESP.getEfuseMac(), HEX));
    String ap_pass = getSetting("wifiApPass", "fibonacci");

    if (WiFi.softAP(ap_ssid.c_str(), ap_pass.c_str())) {
        Serial.printf("[WIFI] AP Started! SSID: %s\n", ap_ssid.c_str());
    }

    WiFi.onEvent([=](arduino_event_id_t event, arduino_event_info_t info) {
        Event e = Event::Initial;
        switch (event) {
            case ARDUINO_EVENT_WIFI_STA_START:
                Serial.println("[WIFI] Station started");
                break;
            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                _wifi_connected = true;
                e = Event::StationConnected;
                Serial.print("[WIFI] Connected! IP: ");
                Serial.println(WiFi.localIP());
                Rtcmem->sys &= ~WIFI_CRASH_BIT;
                
                // Disable AP if connected to STA
                if (WiFi.getMode() & WIFI_AP) {
                    Serial.println("[WIFI] Disabling SoftAP...");
                    WiFi.mode(WIFI_STA);
                }
                break;
            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                _wifi_connected = false;
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
            for (auto& cb : _callbacks) {
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
        if (!_wifi_connected && (WiFi.getMode() & WIFI_MODE_STA)) {
            if (millis() - _wifi_last_connect > 15000) {
                _wifiConnect();
            }
        }
    });

    _wifiConnect();
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

} // namespace wifi
} // namespace espurna

void wifiReload() {
    espurna::wifi::_wifi_last_connect = 0;
    WiFi.disconnect(false, false);
    espurna::wifi::_wifi_connected = false;
}

void wifiDisconnect() {
    WiFi.disconnect(false, false);
    espurna::wifi::_wifi_connected = false;
    espurna::wifi::_wifi_last_connect = millis();
}

void wifiRegister(espurna::wifi::EventCallback callback) {
    espurna::wifi::_callbacks.push_back(callback);
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
void wifiTurnOff() { WiFi.mode(WIFI_OFF); }
void wifiTurnOn() { WiFi.mode(WIFI_STA); }
IPAddress wifiStaIp() { return WiFi.localIP(); }
String wifiStaSsid() { return WiFi.SSID(); }
void wifiToggleAp() {}
void wifiToggleSta() {}
void wifiStartAp() { espurna::wifi::_wifiStartAp(); }
bool wifiDisabled() { return WiFi.getMode() == WIFI_MODE_NULL; }
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
