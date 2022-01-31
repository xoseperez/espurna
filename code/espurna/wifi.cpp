/*

WIFI MODULE

Original code based on JustWifi, Wifi Manager for ESP8266 (GPLv3+)
Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

Modified for ESPurna
Copyright (C) 2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "wifi.h"

#include "telnet.h"
#include "ws.h"

#include <IPAddress.h>
#include <AddrList.h>

#if WIFI_AP_CAPTIVE_SUPPORT
#include <DNSServer.h>
#endif

#include <algorithm>
#include <array>
#include <list>
#include <queue>
#include <vector>

// ref.
// https://github.com/d-a-v/esp82xx-nonos-linklayer/blob/master/README.md#how-it-works
//
// Current esp8266 Arduino Core is based on the NONOS SDK using the lwip1.4 APIs
// To handle static IPs, these need to be called when current IP differs from the one set via the setting.
//
// Can't include the original headers, since they refer to the ip_addr_t and IPAddress depends on a specific overload to extract v4 addresses
// (SDK layer *only* works with ipv4 addresses)

#undef netif_set_addr
extern "C" netif* eagle_lwip_getif(int);
extern "C" void netif_set_addr(netif* netif, ip4_addr_t*, ip4_addr_t*, ip4_addr_t*);

// -----------------------------------------------------------------------------
// INTERNAL
// -----------------------------------------------------------------------------

namespace wifi {

using Mac = std::array<uint8_t, 6>;

namespace build {
namespace {

constexpr float txPower() {
    return WIFI_OUTPUT_POWER_DBM;
}

constexpr WiFiSleepType_t sleep() {
    return WIFI_SLEEP_MODE;
}

} // namespace
} // namespace build

namespace ap {
namespace settings {
namespace options {
namespace {

using ::settings::options::Enumeration;

alignas(4) static constexpr char Disabled[] PROGMEM = "off";
alignas(4) static constexpr char Enabled[] PROGMEM = "on";
alignas(4) static constexpr char Fallback[] PROGMEM = "fallback";

static constexpr Enumeration<wifi::ApMode> ApModeOptions[] PROGMEM {
    {wifi::ApMode::Disabled, Disabled},
    {wifi::ApMode::Enabled, Enabled},
    {wifi::ApMode::Fallback, Fallback},
};

} // namespace
} // namespace options
} // namespace settings
} // namespace ap

namespace settings {
namespace options {
namespace {

using ::settings::options::Enumeration;

alignas(4) static constexpr char None[] PROGMEM = "none";
alignas(4) static constexpr char Modem[] PROGMEM = "modem";
alignas(4) static constexpr char Light[] PROGMEM = "light";

static constexpr Enumeration<WiFiSleepType_t> WiFiSleepTypeOptions[] PROGMEM {
    {WIFI_NONE_SLEEP, None},
    {WIFI_MODEM_SLEEP, Modem},
    {WIFI_LIGHT_SLEEP, Light},
};

} // namespace
} // namespace options
} // namespace settings
} // namespace wifi

namespace settings {
namespace internal {
namespace {

using wifi::ap::settings::options::ApModeOptions;
using wifi::settings::options::WiFiSleepTypeOptions;

} // namespace

template<>
wifi::StaMode convert(const String& value) {
    return convert<bool>(value)
        ? wifi::StaMode::Enabled
        : wifi::StaMode::Disabled;
}

String serialize(wifi::StaMode mode) {
    return serialize(mode == wifi::StaMode::Enabled);
}

template<>
wifi::ApMode convert(const String& value) {
    return convert(ApModeOptions, value, wifi::ApMode::Fallback);
}

String serialize(wifi::ApMode mode) {
    return serialize(ApModeOptions, mode);
}

template <>
WiFiSleepType_t convert(const String& value) {
    return convert(WiFiSleepTypeOptions, value, wifi::build::sleep());
}

String serialize(WiFiSleepType_t sleep) {
    return serialize(WiFiSleepTypeOptions, sleep);
}

template <>
IPAddress convert(const String& value) {
    IPAddress out;
    out.fromString(value);
    return out;
}

template <>
wifi::Mac convert(const String& value) {
    wifi::Mac out{};

    static constexpr size_t Min { 12 };
    static constexpr size_t Max { 17 };

    switch (value.length()) {
    // xxxxxxxxxx
    case Min:
        hexDecode(value.c_str(), value.length(), out.data(), out.size());
        break;

    // xx:xx:xx:xx:xx:xx
    case Max: {
        String buffer;
        buffer.reserve(value.length());

        for (auto it = value.begin(); it != value.end(); ++it) {
            if ((*it) != ':') {
                buffer += *it;
            }
        }
        if (buffer.length() == Min) {
            hexDecode(buffer.c_str(), buffer.length(), out.data(), out.size());
        }
        break;
    }

    }

    return out;
}

// XXX: "(IP unset)" when not set, no point saving these :/
// XXX: both 0.0.0.0 and 255.255.255.255 will be saved as empty string

String serialize(const IPAddress& ip) {
    return ip.isSet() ? ip.toString() : emptyString;
}

String serialize(wifi::Mac mac) {
    String out;
    out.reserve(18);

    bool delim { false };
    char buffer[3] = {0};
    for (auto& byte : mac) {
        hexEncode(&byte, 1, buffer, sizeof(buffer));
        if (delim) {
            out += ':';
        }
        out += buffer;
        delim = true;
    }

    return out;
}

} // namespace internal
} // namespace settings

namespace wifi {
namespace {

// Use SDK constants directly. Provide a constexpr version of the Core enum, since the code never
// actually uses `WiFi::mode(...)` directly, *but* opmode is retrieved using the SDK function.

static constexpr uint8_t OpmodeNull { NULL_MODE };
static constexpr uint8_t OpmodeSta { STATION_MODE };
static constexpr uint8_t OpmodeAp { SOFTAP_MODE };
static constexpr uint8_t OpmodeApSta { OpmodeSta | OpmodeAp };

enum class ScanError {
    None,
    AlreadyScanning,
    System,
    NoNetworks
};

enum class Action {
    StationConnect,
    StationContinueConnect,
    StationTryConnectBetter,
    StationDisconnect,
    AccessPointFallback,
    AccessPointFallbackCheck,
    AccessPointStart,
    AccessPointStop,
    TurnOff,
    TurnOn
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

} // namespace

namespace internal {
namespace {

// Module actions are controled in a serialzed manner, when internal loop is done with the
// current task and is free to take up another one. Allow to toggle OFF for the whole module,
// discarding any actions involving an active WiFi. Default is ON

bool enabled { true };
ActionsQueue actions;

} // namespace
} // namespace internal

namespace {

uint8_t opmode() {
    return wifi_get_opmode();
}

bool enabled() {
    return internal::enabled;
}

void enable() {
    internal::enabled = true;
}

void disable() {
    internal::enabled = false;
}

void action(Action value) {
    switch (value) {
    case Action::StationConnect:
    case Action::StationTryConnectBetter:
    case Action::StationContinueConnect:
    case Action::StationDisconnect:
    case Action::AccessPointFallback:
    case Action::AccessPointFallbackCheck:
    case Action::AccessPointStart:
    case Action::AccessPointStop:
        if (!enabled()) {
            return;
        }
        break;
    case Action::TurnOff:
    case Action::TurnOn:
        break;
    }

    internal::actions.push(value);
}

ActionsQueue& actions() {
    return internal::actions;
}

// ::forceSleepBegin() remembers the previous mode and ::forceSleepWake() calls station connect when it has STA in it :/
// while we *do* set opmode to 0 to avoid this uncertainty, preper to call wake through SDK instead of the Arduino wrapper
//
// 0xFFFFFFF is a magic number per the NONOS API reference, 3.7.5 wifi_fpm_do_sleep:
// > If sleep_time_in_us is 0xFFFFFFF, the ESP8266 will sleep till be woke up as below:
// > • If wifi_fpm_set_sleep_type is set to be LIGHT_SLEEP_T, ESP8266 can wake up by GPIO.
// > • If wifi_fpm_set_sleep_type is set to be MODEM_SLEEP_T, ESP8266 can wake up by wifi_fpm_do_wakeup.
//
// In our case, wake-up is software driven, so the MODEM sleep is the only choice available.
// This version can *only* work from CONT context, since the only consumer atm is wifi::Action handler
// TODO(esp32): Null mode turns off radio, no need for these

bool sleep() {
    if (opmode() == ::wifi::OpmodeNull) {
        wifi_fpm_set_sleep_type(MODEM_SLEEP_T);
        yield();
        wifi_fpm_open();
        yield();
        if (0 == wifi_fpm_do_sleep(0xFFFFFFF)) {
            delay(10);
            return true;
        }
    }

    return false;
}

bool wakeup() {
    if (wifi_fpm_get_sleep_type() != NONE_SLEEP_T) {
        wifi_fpm_do_wakeup();
        wifi_fpm_close();
        delay(10);
        return true;
    }

    return false;
}

} // namespace

namespace debug {
namespace {

String error(wifi::ScanError error) {
    const __FlashStringHelper* ptr { nullptr };

    switch (error) {
    case wifi::ScanError::AlreadyScanning:
        ptr = F("Scan already in progress");
        break;
    case wifi::ScanError::System:
        ptr = F("Could not start the scan");
        break;
    case wifi::ScanError::NoNetworks:
        ptr = F("No networks");
        break;
    case wifi::ScanError::None:
        ptr = F("OK");
        break;
    }

    return ptr;
}

String mac(Mac mac) {
    return ::settings::internal::serialize(mac);
}

String ip(const IPAddress& addr) {
    return addr.toString();
}

String ip(ip4_addr_t addr) {
    String out;
    out.reserve(16);

    bool delim { false };
    for (int byte = 0; byte < 4; ++byte) {
        if (delim) {
            out += '.';
        }
        out += ip4_addr_get_byte_val(addr, byte);
        delim = true;
    }

    return out;
}

String authmode(AUTH_MODE mode) {
    const __FlashStringHelper* ptr { F("UNKNOWN") };

    switch (mode) {
    case AUTH_OPEN:
        ptr = F("OPEN");
        break;
    case AUTH_WEP:
        ptr = F("WEP");
        break;
    case AUTH_WPA_PSK:
        ptr = F("WPAPSK");
        break;
    case AUTH_WPA2_PSK:
        ptr = F("WPA2PSK");
        break;
    case AUTH_WPA_WPA2_PSK:
        ptr = F("WPAWPA2-PSK");
        break;
    case AUTH_MAX:
        break;
    }

    return ptr;
}

String opmode(uint8_t mode) {
    const __FlashStringHelper* ptr { nullptr };

    switch (mode) {
    case ::wifi::OpmodeApSta:
        ptr = F("AP+STA");
        break;
    case ::wifi::OpmodeSta:
        ptr = F("STA");
        break;
    case ::wifi::OpmodeAp:
        ptr = F("AP");
        break;
    case ::wifi::OpmodeNull:
        ptr = F("NULL");
        break;
    }

    return ptr;
}

} // namespace
} // namespace debug

namespace settings {
namespace keys {
namespace {

alignas(4) static constexpr char TxPower[] PROGMEM = "wifiTxPwr";
alignas(4) static constexpr char Sleep[] PROGMEM = "wifiSleep";

} // namespace
} // namespace keys

namespace {

float txPower() {
    return getSetting(keys::TxPower, wifi::build::txPower());
}

WiFiSleepType_t sleep() {
    return getSetting(keys::Sleep, wifi::build::sleep());
}

} // namespace

namespace query {
namespace internal {
namespace {

#define EXACT_VALUE(NAME, FUNC)\
String NAME () {\
    return ::settings::internal::serialize(FUNC());\
}

#define ID_VALUE(NAME, FUNC)\
String NAME (size_t id) {\
    return ::settings::internal::serialize(FUNC(id));\
}

EXACT_VALUE(sleep, settings::sleep)
EXACT_VALUE(txPower, settings::txPower)

} // namespace
} // namespace internal
} // namespace query
} // namespace settings

namespace {

// We are guaranteed to have '\0' when <32 b/c the SDK zeroes out the data
// But, these are byte arrays, not C strings. When ssid_len is available, use it.
// When not, we are still expecting the <32 arrays to have '\0' at the end and we manually
// set the 32'nd char to '\0' to prevent conversion issues

String convertSsid(const softap_config& config) {
    String ssid;
    ssid.concat(reinterpret_cast<const char*>(config.ssid), config.ssid_len);
    return ssid;
}

String convertSsid(const bss_info& info) {
    String ssid;
    ssid.concat(reinterpret_cast<const char*>(info.ssid), info.ssid_len);
    return ssid;
}

template <typename T, size_t SsidSize = sizeof(T::ssid)>
String convertSsid(const T& config) {
    static_assert(SsidSize == 32, "");

    const char* ptr { reinterpret_cast<const char*>(config.ssid) };
    char ssid[SsidSize + 1];
    std::copy(ptr, ptr + SsidSize, ssid);
    ssid[SsidSize] = '\0';

    return ssid;
}

template <typename T, size_t PassphraseSize = sizeof(T::password)>
String convertPassphrase(const T& config) {
    static_assert(PassphraseSize == 64, "");

    const char* ptr { reinterpret_cast<const char*>(config.password) };
    char passphrase[PassphraseSize + 1];
    std::copy(ptr, ptr + PassphraseSize, passphrase);
    passphrase[PassphraseSize] = '\0';

    return passphrase;
}

template <typename T, size_t MacSize = sizeof(T::bssid)>
wifi::Mac convertBssid(const T& info) {
    static_assert(MacSize == 6, "");
    wifi::Mac mac;
    std::copy(info.bssid, info.bssid + MacSize, mac.begin());
    return mac;
}

struct Info {
    Info() = default;
    Info(const Info&) = default;
    Info(Info&&) = default;

    Info(wifi::Mac&& bssid, AUTH_MODE authmode, int8_t rssi, uint8_t channel) :
        _bssid(std::move(bssid)),
        _authmode(authmode),
        _rssi(rssi),
        _channel(channel)
    {}

    explicit Info(const bss_info& info) :
        _bssid(convertBssid(info)),
        _authmode(info.authmode),
        _rssi(info.rssi),
        _channel(info.channel)
    {}

    Info& operator=(const Info&) = default;
    Info& operator=(Info&&) = default;

    Info& operator=(const bss_info& info) {
        _bssid = convertBssid(info);
        _authmode = info.authmode;
        _channel = info.channel;
        _rssi = info.rssi;
        return *this;
    }

    explicit operator bool() const {
        return _rssi != 0 && _channel != 0;
    }

    bool operator<(const Info& rhs) const {
        return _rssi < rhs._rssi;
    }

    bool operator>(const Info& rhs) const {
        return _rssi > rhs._rssi;
    }

    const wifi::Mac& bssid() const {
        return _bssid;
    }

    AUTH_MODE authmode() const {
        return _authmode;
    }

    int8_t rssi() const {
        return _rssi;
    }

    uint8_t channel() const {
        return _channel;
    }

private:
    Mac _bssid{};
    AUTH_MODE _authmode { AUTH_OPEN };
    int8_t _rssi { 0 };
    uint8_t _channel { 0u };
};

struct SsidInfo {
    SsidInfo() = delete;

    explicit SsidInfo(const bss_info& info) :
        _ssid(convertSsid(info)),
        _info(info)
    {}

    SsidInfo(String&& ssid, wifi::Info&& info) :
        _ssid(std::move(ssid)),
        _info(std::move(info))
    {}

    const String& ssid() const {
        return _ssid;
    }

    const wifi::Info& info() const {
        return _info;
    }

    // decreasing order by rssi (default sort() order is increasing)
    bool operator<(const SsidInfo& rhs) const {
        if (!_info.rssi()) {
            return false;
        }

        return info() > rhs.info();
    }

private:
    String _ssid;
    wifi::Info _info;
};

using SsidInfos = std::forward_list<SsidInfo>;

// Note that lwip config allows up to 3 DNS servers. But, most of the time we use DHCP.
// TODO: ::dns(size_t index)? how'd that look with settings?

struct IpSettings {
    IpSettings() = default;
    IpSettings(const IpSettings&) = default;
    IpSettings(IpSettings&&) = default;

    IpSettings& operator=(const IpSettings&) = default;
    IpSettings& operator=(IpSettings&&) = default;

    template <typename Ip, typename Netmask, typename Gateway, typename Dns>
    IpSettings(Ip&& ip, Netmask&& netmask, Gateway&& gateway, Dns&& dns) :
        _ip(std::forward<Ip>(ip)),
        _netmask(std::forward<Netmask>(netmask)),
        _gateway(std::forward<Gateway>(gateway)),
        _dns(std::forward<Dns>(dns))
    {}

    const IPAddress& ip() const {
        return _ip;
    }

    const IPAddress& netmask() const {
        return _netmask;
    }

    const IPAddress& gateway() const {
        return _gateway;
    }

    const IPAddress& dns() const {
        return _dns;
    }

    explicit operator bool() const {
        return _ip.isSet()
            && _netmask.isSet()
            && _gateway.isSet()
            && _dns.isSet();
    }

    ip_info toIpInfo() const {
        ip_info info{};
        info.ip.addr = _ip.v4();
        info.netmask.addr = _netmask.v4();
        info.gw.addr = _gateway.v4();

        return info;
    }

private:
    IPAddress _ip;
    IPAddress _netmask;
    IPAddress _gateway;
    IPAddress _dns;
};

struct StaNetwork {
    Mac bssid;
    String ssid;
    String passphrase;
    int8_t rssi;
    uint8_t channel;
};

struct SoftApNetwork {
    Mac bssid;
    String ssid;
    String passphrase;
    uint8_t channel;
    AUTH_MODE authmode;
};

struct Network {
    Network() = delete;
    Network(const Network&) = default;
    Network(Network&&) = default;

    Network& operator=(Network&&) = default;

    explicit Network(String&& ssid) :
        _ssid(std::move(ssid))
    {}

    Network(String&& ssid, String&& passphrase) :
        _ssid(std::move(ssid)),
        _passphrase(std::move(passphrase))
    {}

    Network(String&& ssid, String&& passphrase, IpSettings&& settings) :
        _ssid(std::move(ssid)),
        _passphrase(std::move(passphrase)),
        _ipSettings(std::move(settings))
    {}

    // TODO(?): in case SDK API is used directly, this also could use an authmode field
    // Arduino wrapper sets WPAPSK minimum by default, so one use-case is to set it to WPA2PSK

    Network(Network other, wifi::Mac bssid, uint8_t channel) :
        _ssid(std::move(other._ssid)),
        _passphrase(std::move(other._passphrase)),
        _ipSettings(std::move(other._ipSettings)),
        _bssid(bssid),
        _channel(channel)
    {}

    bool dhcp() const {
        return !_ipSettings;
    }

    const String& ssid() const {
        return _ssid;
    }

    const String& passphrase() const {
        return _passphrase;
    }

    const IpSettings& ipSettings() const {
        return _ipSettings;
    }

    const wifi::Mac& bssid() const {
        return _bssid;
    }

    uint8_t channel() const {
        return _channel;
    }

private:
    String _ssid;
    String _passphrase;
    IpSettings _ipSettings;

    Mac _bssid {};
    uint8_t _channel { 0u };
};

using Networks = std::list<Network>;

} // namespace

// -----------------------------------------------------------------------------
// STATION
// -----------------------------------------------------------------------------

namespace sta {
namespace build {
namespace {

static constexpr size_t NetworksMax { WIFI_MAX_NETWORKS };

// aka short interval
static constexpr auto ConnectionInterval = espurna::duration::Milliseconds { WIFI_CONNECT_INTERVAL };

// aka long interval
static constexpr auto ReconnectionInterval = espurna::duration::Milliseconds { WIFI_RECONNECT_INTERVAL };

static constexpr int ConnectionRetries { WIFI_CONNECT_RETRIES };
static constexpr auto RecoveryInterval = ConnectionInterval * ConnectionRetries;

constexpr StaMode mode() {
    return WIFI_STA_MODE;
}

const __FlashStringHelper* ssid(size_t index) {
    return (
        (index == 0) ? F(WIFI1_SSID) :
        (index == 1) ? F(WIFI2_SSID) :
        (index == 2) ? F(WIFI3_SSID) :
        (index == 3) ? F(WIFI4_SSID) :
        (index == 4) ? F(WIFI5_SSID) : nullptr
    );
}

const __FlashStringHelper* passphrase(size_t index) {
    return (
        (index == 0) ? F(WIFI1_PASS) :
        (index == 1) ? F(WIFI2_PASS) :
        (index == 2) ? F(WIFI3_PASS) :
        (index == 3) ? F(WIFI4_PASS) :
        (index == 4) ? F(WIFI5_PASS) : nullptr
    );
}

const __FlashStringHelper* ip(size_t index) {
    return (
        (index == 0) ? F(WIFI1_IP) :
        (index == 1) ? F(WIFI2_IP) :
        (index == 2) ? F(WIFI3_IP) :
        (index == 3) ? F(WIFI4_IP) :
        (index == 4) ? F(WIFI5_IP) : nullptr
    );
}

const __FlashStringHelper* gateway(size_t index) {
    return (
        (index == 0) ? F(WIFI1_GW) :
        (index == 1) ? F(WIFI2_GW) :
        (index == 2) ? F(WIFI3_GW) :
        (index == 3) ? F(WIFI4_GW) :
        (index == 4) ? F(WIFI5_GW) : nullptr
    );
}

const __FlashStringHelper* netmask(size_t index) {
    return (
        (index == 0) ? F(WIFI1_MASK) :
        (index == 1) ? F(WIFI2_MASK) :
        (index == 2) ? F(WIFI3_MASK) :
        (index == 3) ? F(WIFI4_MASK) :
        (index == 4) ? F(WIFI5_MASK) : nullptr
    );
}

const __FlashStringHelper* dns(size_t index) {
    return (
        (index == 0) ? F(WIFI1_DNS) :
        (index == 1) ? F(WIFI2_DNS) :
        (index == 2) ? F(WIFI3_DNS) :
        (index == 3) ? F(WIFI4_DNS) :
        (index == 4) ? F(WIFI5_DNS) : nullptr
    );
}

const __FlashStringHelper* bssid(size_t index) {
    return (
        (index == 0) ? F(WIFI1_BSSID) :
        (index == 1) ? F(WIFI2_BSSID) :
        (index == 2) ? F(WIFI3_BSSID) :
        (index == 3) ? F(WIFI4_BSSID) :
        (index == 4) ? F(WIFI5_BSSID) : nullptr
    );
}

constexpr uint8_t channel(size_t index) {
    return (
        (index == 0) ? WIFI1_CHANNEL :
        (index == 1) ? WIFI2_CHANNEL :
        (index == 2) ? WIFI3_CHANNEL :
        (index == 3) ? WIFI4_CHANNEL :
        (index == 4) ? WIFI5_CHANNEL : 0
    );
}

} // namespace
} // namespace build

namespace settings {
namespace keys {
namespace {

alignas(4) static constexpr char Mode[] PROGMEM = "wifiStaMode";

alignas(4) static constexpr char Ssid[] PROGMEM = "ssid";
alignas(4) static constexpr char Passphrase[] PROGMEM = "pass";

alignas(4) static constexpr char Ip[] PROGMEM = "ip";
alignas(4) static constexpr char Gateway[] PROGMEM = "gw";
alignas(4) static constexpr char Netmask[] PROGMEM = "mask";
alignas(4) static constexpr char Dns[] PROGMEM = "dns";

alignas(4) static constexpr char Bssid[] PROGMEM = "bssid";
alignas(4) static constexpr char Channel[] PROGMEM = "chan";

} // namespace
} // namespace keys

namespace {

wifi::StaMode mode() {
    return getSetting(keys::Mode, build::mode());
}

String ssid(size_t index) {
    return getSetting({keys::Ssid, index}, build::ssid(index));
}

String passphrase(size_t index) {
    return getSetting({keys::Passphrase, index}, build::passphrase(index));
}

IPAddress ip(size_t index) {
    return ::settings::internal::convert<IPAddress>(
        getSetting({keys::Ip, index}, build::ip(index)));
}

IPAddress gateway(size_t index) {
    return ::settings::internal::convert<IPAddress>(
        getSetting({keys::Gateway, index}, build::gateway(index)));
}

IPAddress netmask(size_t index) {
    return ::settings::internal::convert<IPAddress>(
        getSetting({keys::Netmask, index}, build::netmask(index)));
}

IPAddress dns(size_t index) {
    return ::settings::internal::convert<IPAddress>(
        getSetting({keys::Dns, index}, build::dns(index)));
}

wifi::Mac bssid(size_t index) {
    return ::settings::internal::convert<wifi::Mac>(
        getSetting({keys::Bssid, index}, build::bssid(index)));
}

int8_t channel(size_t index) {
    return getSetting({keys::Channel, index}, build::channel(index));
}

} // namespace

namespace query {
namespace internal {
namespace {

ID_VALUE(ip, settings::ip)
ID_VALUE(gateway, settings::gateway)
ID_VALUE(netmask, settings::netmask)
ID_VALUE(dns, settings::dns)
ID_VALUE(bssid, settings::bssid)
ID_VALUE(channel, settings::channel)

EXACT_VALUE(mode, settings::mode)

} // namespace
} // namespace internal

namespace {

static constexpr std::array<::settings::query::IndexedSetting, 8> Settings PROGMEM {
    {{keys::Ssid, settings::ssid},
     {keys::Passphrase, settings::passphrase},
     {keys::Ip, internal::ip},
     {keys::Gateway, internal::gateway},
     {keys::Netmask, internal::netmask},
     {keys::Dns, internal::dns},
     {keys::Bssid, internal::bssid},
     {keys::Channel, internal::channel}}
};

} // namespace
} // namespace query
} // namespace settings

namespace {

uint8_t channel() {
    return wifi_get_channel();
}

int8_t rssi() {
    return wifi_station_get_rssi();
}

wifi::Networks networks() {
    wifi::Networks out;

    for (size_t id = 0; id < build::NetworksMax; ++id) {
        auto ssid = settings::ssid(id);
        if (!ssid.length()) {
            break;
        }

        auto pass = settings::passphrase(id);

        auto ip = settings::ip(id);
        auto ipSettings = ip.isSet()
            ? wifi::IpSettings{
                std::move(ip),
                settings::netmask(id),
                settings::gateway(id),
                settings::dns(id)}
            : wifi::IpSettings{};

        Network network(std::move(ssid), settings::passphrase(id), std::move(ipSettings));
        auto channel = settings::channel(id);
        if (channel) {
            out.emplace_back(std::move(network), settings::bssid(id), channel);
        } else {
            out.push_back(std::move(network));
        }
    }

    return out;
}

size_t countNetworks() {
    size_t networks { 0 };

    for (size_t id = 0; id < build::NetworksMax; ++id) {
        auto ssid = settings::ssid(id);
        if (!ssid.length()) {
            break;
        }

        ++networks;
    }

    return networks;
}

// Note that authmode field is a our threshold, not the one selected by an AP

wifi::Info info(const station_config& config) {
    return wifi::Info{
        convertBssid(config),
        config.threshold.authmode,
        rssi(),
        channel()};
}

wifi::Info info() {
    station_config config{};
    wifi_station_get_config(&config);
    return info(config);
}

wifi::StaNetwork current(const station_config& config) {
    return {
        convertBssid(config),
        convertSsid(config),
        convertPassphrase(config),
        rssi(),
        channel()};
}

wifi::StaNetwork current() {
    station_config config{};
    wifi_station_get_config(&config);
    return current(config);
}

} // namespace

#if WIFI_GRATUITOUS_ARP_SUPPORT
namespace garp {
namespace {
namespace build {

static constexpr auto IntervalMin = espurna::duration::Milliseconds { WIFI_GRATUITOUS_ARP_INTERVAL_MIN };
static constexpr auto IntervalMax = espurna::duration::Milliseconds { WIFI_GRATUITOUS_ARP_INTERVAL_MAX };

} // namespace build

namespace settings {
namespace internal {

template <typename T>
T randomInterval(T minimum, T maximum) {
    return T(::randomNumber(minimum.count(), maximum.count()));
}

espurna::duration::Milliseconds randomInterval() {
    return randomInterval(build::IntervalMin, build::IntervalMax);
}

} // namespace internal

espurna::duration::Milliseconds interval() {
    static const auto defaultInterval = internal::randomInterval();
    return getSetting("wifiGarpIntvl", defaultInterval);
}

} // namespace settings

namespace internal {

Ticker timer;
bool wait { false };

} // namespace internal

bool send() {
    bool result { false };

    for (netif* interface = netif_list; interface != nullptr; interface = interface->next) {
        if (
            (interface->flags & NETIF_FLAG_ETHARP)
            && (interface->hwaddr_len == ETHARP_HWADDR_LEN)
            && (!ip4_addr_isany_val(*netif_ip4_addr(interface)))
            && (interface->flags & NETIF_FLAG_LINK_UP)
            && (interface->flags & NETIF_FLAG_UP)
        ) {
            etharp_gratuitous(interface);
            result = true;
        }
    }

    return result;
}

bool wait() {
    if (internal::wait) {
        return true;
    }

    internal::wait = true;
    return false;
}

void stop() {
    internal::timer.detach();
}

void start(espurna::duration::Milliseconds next) {
    internal::timer.attach_ms(next.count(), []() {
        internal::wait = false;
    });
}

} // namespace
} // namespace garp
#endif

namespace scan {
namespace settings {
namespace keys {
namespace {

alignas(4) static constexpr char Enabled[] PROGMEM = "wifiScan";

} // namespace
} // namespace keys
} // namespace settings

namespace {

using SsidInfosPtr = std::shared_ptr<wifi::SsidInfos>;

using Success = std::function<void(bss_info*)>;
using Error = std::function<void(wifi::ScanError)>;

struct Task {
    Task() = delete;

    Task(Success&& success, Error&& error) :
        _success(std::move(success)),
        _error(std::move(error))
    {}

    void success(bss_info* info) {
        _success(info);
    }

    void error(wifi::ScanError error) {
        _error(error);
    }

private:
    Success _success;
    Error _error;
};

using TaskPtr = std::unique_ptr<Task>;

namespace internal {

TaskPtr task;

void stop() {
    task = nullptr;
}

// STATUS comes from c_types.h, and it seems this is the only place that uses it
// instead of some ESP-specific type.

void complete(void* result, STATUS status) {
    if (status) { // aka anything but OK / 0
        task->error(wifi::ScanError::System);
        stop();
        return;
    }

    size_t networks { 0ul };
    bss_info* head = reinterpret_cast<bss_info*>(result);
    for (bss_info* it = head; it; it = STAILQ_NEXT(it, next), ++networks) {
        task->success(it);
    }

    if (!networks) {
        task->error(wifi::ScanError::NoNetworks);
    }

    stop();
}

} // namespace internal

bool start(Success&& success, Error&& error) {
    if (internal::task) {
        error(wifi::ScanError::AlreadyScanning);
        return false;
    }

    // Note that esp8266 callback only reports the resulting status and will (always?) timeout all by itself
    // Default values are an active scan with some unspecified channel times.
    // (zeroed out scan_config struct or simply nullptr)

    // For example, c/p config from the current esp32 Arduino Core wrapper which are close to the values mentioned here:
    // https://github.com/espressif/ESP8266_NONOS_SDK/issues/103#issuecomment-383440370
    // Which could be useful if scanning needs to be more aggressive or switched into PASSIVE scan type

    //scan_config config{};
    //config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    //config.scan_time.active.min = 100;
    //config.scan_time.active.max = 300;

    if (wifi_station_scan(nullptr, &internal::complete)) {
        internal::task = std::make_unique<Task>(std::move(success), std::move(error));
        return true;
    }

    error(wifi::ScanError::System);
    return false;
}

// Alternative to the stock WiFi method, where we wait for the task to finish before returning
bool wait(Success&& success, Error&& error) {
    auto result = start(std::move(success), std::move(error));
    while (internal::task) {
        delay(100);
    }

    return result;
}

// Another alternative to the stock WiFi method, return a shared Info list
// Caller is expected to wait for the scan to complete before using the contents
SsidInfosPtr ssidinfos() {
    auto infos = std::make_shared<wifi::SsidInfos>();

    start(
        [infos](bss_info* found) {
            infos->emplace_front(*found);
        },
        [infos](wifi::ScanError) {
            infos->clear();
        });

    return infos;
}

} // namespace
} // namespace scan

namespace {

bool enabled() {
    return wifi::opmode() & wifi::OpmodeSta;
}

// XXX: WiFi.disconnect() also implicitly disables STA mode *and* erases the current STA config

void disconnect() {
    if (enabled()) {
        wifi_station_disconnect();
    }
}

// Some workarounds for built-in WiFi management:
// - don't *intentionally* perist current SSID & PASS even when persistance is disabled from the Arduino Core side.
// while this seems like a good idea in theory, we end up with a bunch of async actions coming our way.
// - station disconnect events are linked with the connection routine as well, single WiFi::begin() may trigger up to
// 3 events (as observed with `WiFi::waitForConnectResult()`) before the connection loop stops further attempts
// - explicit OPMODE changes to both notify the userspace when the change actually happens (alternative is SDK event, but it is SYS context),
// since *all* STA & AP start-up methods will implicitly change the mode (`WiFi.begin()`, `WiFi.softAP()`, `WiFi.config()`)

void enable() {
    if (WiFi.enableSTA(true)) {
        disconnect();
        if (wifi_station_get_reconnect_policy()) {
            wifi_station_set_reconnect_policy(false);
        }
        if (wifi_station_get_auto_connect()) {
            wifi_station_set_auto_connect(false);
        }
        return;
    }

    // `std::abort()` calls are the to ensure the mode actually changes, but it should be extremely rare
    // it may be also wise to add these for when the mode is already the expected one,
    // since we should enforce mode changes to happen *only* through the configuration loop
    abort();
}

void disable() {
    if (!WiFi.enableSTA(false)) {
        abort();
    }
}

} // namespace

namespace connection {
namespace internal {
namespace {

struct Task {
    static constexpr size_t SsidMax { sizeof(station_config::ssid) };

    static constexpr size_t PassphraseMin { 8ul };
    static constexpr size_t PassphraseMax { sizeof(station_config::password) };

    static constexpr int8_t RssiThreshold { -127 };

    using Iterator = wifi::Networks::iterator;

    Task() = delete;
    Task(const Task&) = delete;
    Task(Task&&) = delete;

    explicit Task(String&& hostname, Networks&& networks, int retries) :
        _hostname(std::move(hostname)),
        _networks(std::move(networks)),
        _begin(_networks.begin()),
        _end(_networks.end()),
        _current(_begin),
        _retries(retries),
        _retry(_retries)
    {}

    bool empty() const {
        return _networks.empty();
    }

    size_t count() const {
        return _networks.size();
    }

    bool done() const {
        return _current == _end;
    }

    bool next() {
        if (!done()) {
            if (--_retry < 0) {
                _retry = _retries;
                _current = std::next(_current);
            }
            return !done();
        }

        return false;
    }

    bool connect() const {
        if (!done() && wifi::sta::enabled()) {
            // Need to call this to cancel SDK tasks (previous scan, connection, etc.)
            // Otherwise, it will fail the initial attempt and force a retry.
            wifi::sta::disconnect();

            // SDK sends EVENT_STAMODE_DISCONNECTED right after the disconnect() call, which is likely to happen
            // after being connected and disconnecting for the first time. Not doing this will cause the connection loop
            // to cancel the `wait` lock too early, forcing the Timeout state despite the EVENT_STAMODE_GOTIP coming in later.
            // Allow the event to come in right now to allow `wifi_station_connect()` down below trigger a real one.
            yield();

            auto& network = *_current;
            if (!network.dhcp()) {
                auto& ipsettings = network.ipSettings();

                wifi_station_dhcpc_stop();

                ip_info current;
                wifi_get_ip_info(STATION_IF, &current);

                ip_info info = ipsettings.toIpInfo();
                if (!wifi_set_ip_info(STATION_IF, &info)) {
                    return false;
                }

                dns_setserver(0, ipsettings.dns());

                if ((current.ip.addr != 0) && (current.ip.addr != info.ip.addr)) {
#undef netif_set_addr
                    netif_set_addr(eagle_lwip_getif(STATION_IF), &info.ip, &info.netmask, &info.gw);
                }
            }

            // Only the STA cares about the hostname setting
            // esp8266 specific Arduino-specific - this sets lwip internal structs related to the DHCPc
            WiFi.hostname(_hostname);

            // The rest is related to the connection routine
            // SSID & Passphrase are u8 arrays, with 0 at the end when the string is less than it's size
            // Perform checks earlier, before calling SDK config functions, since it would not reflect in the connection
            // state correctly, and we would need to use the Event API once again.

            station_config config{};

            auto& ssid = network.ssid();
            if (!ssid.length() || (ssid.length() > SsidMax)) {
                return false;
            }

            std::copy(ssid.c_str(), ssid.c_str() + ssid.length(),
                    reinterpret_cast<char*>(config.ssid));
            if (ssid.length() < SsidMax) {
                config.ssid[ssid.length()] = 0;
            }

            auto& pass = network.passphrase();
            if (pass.length()) {
                if ((pass.length() < PassphraseMin) || (pass.length() > PassphraseMax)) {
                    return false;
                }
                config.threshold.authmode = AUTH_WPA_PSK;
                std::copy(pass.c_str(), pass.c_str() + pass.length(),
                        reinterpret_cast<char*>(config.password));
                if (pass.length() < PassphraseMax) {
                    config.password[pass.length()] = 0;
                }
            } else {
                config.threshold.authmode = AUTH_OPEN;
                config.password[0] = 0;
            }

            config.threshold.rssi = RssiThreshold;

            if (network.channel()) {
                auto& bssid = network.bssid();
                std::copy(bssid.begin(), bssid.end(), config.bssid);
                config.bssid_set = 1;
            }

            // TODO: check every return value?
            // TODO: is it sufficient for the event to fire? otherwise,
            // there needs to be a manual timeout code after this returns true

            wifi_station_set_config_current(&config);
            if (!wifi_station_connect()) {
                return false;
            }

            if (network.channel()) {
                wifi_set_channel(network.channel());
            }

            if (network.dhcp() && (wifi_station_dhcpc_status() != DHCP_STARTED)) {
                wifi_station_dhcpc_start();
            }

            return true;
        }

        return false;
    }

    Networks& networks() {
        return _networks;
    }

private:
    String _hostname;

    Networks _networks;
    Iterator _begin;
    Iterator _end;
    Iterator _current;

    const int _retries;
    int _retry;
};

using ActionPtr = void(*)();

void action_next() {
    wifi::action(wifi::Action::StationContinueConnect);
}

void action_new() {
    wifi::action(wifi::Action::StationConnect);
}

wifi::sta::scan::SsidInfosPtr scanResults;
wifi::Networks preparedNetworks;

bool connected { false };
bool wait { false };

Ticker timer;
bool persist { false };

using TaskPtr = std::unique_ptr<Task>;
TaskPtr task;

} // namespace
} // namespace internal

namespace {

void persist(bool value) {
    internal::persist = value;
}

bool persist() {
    return internal::persist;
}

void stop() {
    internal::task.reset();
    internal::timer.detach();
}

bool start(String&& hostname) {
    if (!internal::task) {
        internal::task = std::make_unique<internal::Task>(
            std::move(hostname),
            std::move(internal::preparedNetworks),
            build::ConnectionRetries);
        internal::timer.detach();
        return true;
    }

    internal::preparedNetworks.clear();
    return false;
}

void schedule(espurna::duration::Milliseconds next, internal::ActionPtr ptr) {
    internal::timer.once_ms(next.count(), ptr);
    DEBUG_MSG_P(PSTR("[WIFI] Next connection attempt in %u (ms)\n"), next.count());
}

void schedule_next() {
    schedule(build::ConnectionInterval, internal::action_next);
}

void schedule_new(espurna::duration::Milliseconds next) {
    schedule(next, internal::action_new);
}

void schedule_new() {
    schedule_new(build::ReconnectionInterval);
}

bool next() {
    return internal::task->next();
}

bool connect() {
    if (internal::task->connect()) {
        internal::wait = true;
        return true;
    }

    return false;
}

// Note that `wifi_station_get_connect_status()` may never actually change the state from CONNECTING when AP is not available.
// Wait for the WiFi stack event instead (handled on setup with a static object) and continue after it is either connected or disconnected

bool wait() {
    if (internal::wait) {
        return true;
    }

    return false;
}

// TODO(Core 2.7.4): `WiFi.isConnected()` is a simple `wifi_station_get_connect_status() == STATION_GOT_IP`,
// Meaning, it will never detect link up / down updates when AP silently kills the connection or something else unexpected happens.
// Running JustWiFi with autoconnect + reconnect enabled, it silently avoided the issue b/c the SDK reconnect routine disconnected the STA,
// causing our state machine to immediately cancel it (since `WL_CONNECTED != WiFi.status()`) and then try to connect again using it's own loop.
// We could either (* is used currently):
// - (*) listen for the SDK event through the `WiFi.onStationModeDisconnected()`
// - ( ) poll NETIF_FLAG_LINK_UP for the lwip's netif, since the SDK will bring the link down on disconnection
//   find the `interface` in the `netif_list`, where `interface->num == STATION_IF`
// - ( ) use lwip's netif event system from the recent Core, track UP and DOWN for a specific interface number
//   this one is probably only used internally, thus should be treated as a private API
// - ( ) poll whether `wifi_get_ip_info(STATION_IF, &ip);` is set to something valid
//   (tuple of ip, gw and mask)
// - ( ) poll `WiFi.localIP().isSet()`
//   (will be unset when the link is down)

// placing status into a simple bool to avoid extracting ip info every time someone needs to check the connection

bool connected() {
    return internal::connected;
}

bool connecting() {
    return static_cast<bool>(internal::task);
}

bool lost() {
    static bool last { internal::connected };

    if (internal::connected != last) {
        last = internal::connected;
        return !last;
    }

    return false;
}

void prepare(Networks&& networks) {
    internal::preparedNetworks = std::move(networks);
}

bool prepared() {
    return internal::preparedNetworks.size();
}

} // namespace
} // namespace connection

namespace {

bool connected() {
    return connection::connected();
}

bool connecting() {
    return connection::connecting();
}

bool scanning() {
    return static_cast<bool>(scan::internal::task);
}

// TODO: generic onEvent is deprecated on esp8266 in favour of the event-specific
// methods returning 'cancelation' token. Right now it is a basic shared_ptr with an std function inside of it.
// esp32 only has a generic onEvent, but event names are not compatible with the esp8266 version.
//
// TODO: instead of bool, do a state object that is 'armed' before use and it is possible to make sure there's an expected value swap between `true` and `false`
// (i.e. 'disarmed', 'armed-for', 'received-success', 'received-failure'. where 'armed-for' only reacts on a specific assignment, and the consumer
// checks whether 'received-success' had happend, and also handles 'received-failure'. when 'disarmed', value status does not change)
// TODO: ...and a timeout? most of the time, these happen right after switch into the system task. but, since the sdk funcs don't block until success
// (or at all, for anything), it might be nice to have some safeguards.

void init() {
    static auto disconnected = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected&) {
        connection::internal::wait = false;
        connection::internal::connected = false;
    });
    static auto connected = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP&) {
        connection::internal::wait = false;
        connection::internal::connected = true;
    });
    disconnect();
    disable();
    yield();
}

void toggle() {
    auto current = enabled();
    connection::persist(!current);
    wifi::action(current
        ? wifi::Action::StationDisconnect
        : wifi::Action::StationConnect);
}

} // namespace

namespace scan {
namespace build {
namespace {

constexpr bool enabled() {
    return 1 == WIFI_SCAN_NETWORKS;
}

} // namespace
} // namespace build

namespace settings {
namespace {

bool enabled() {
    return getSetting(keys::Enabled, build::enabled());
}

} // namespace

namespace query {
namespace {

EXACT_VALUE(enabled, settings::enabled)

} // namespace
} // namespace query
} // namespace settings

namespace periodic {
namespace build {
namespace {

static constexpr auto Interval = espurna::duration::Milliseconds { WIFI_SCAN_RSSI_CHECK_INTERVAL };
static constexpr int8_t Checks { WIFI_SCAN_RSSI_CHECKS };

constexpr int8_t threshold() {
    return WIFI_SCAN_RSSI_THRESHOLD;
}

} // namespace
} // namespace build

namespace settings {
namespace keys {
namespace {

alignas(4) static constexpr char Threshold[] PROGMEM = "wifiScanRssi";

} // namespace
} // namespace keys

namespace {

int8_t threshold() {
    return getSetting(FPSTR(keys::Threshold), build::threshold());
}

} // namespace

namespace query {
namespace {

EXACT_VALUE(threshold, settings::threshold)

} // namespace
} // namespace query
} // namespace settings

namespace internal {
namespace {

int8_t threshold { build::threshold() };
int8_t counter { build::Checks };
Ticker timer;

void task() {
    if (!wifi::sta::connected()) {
        counter = build::Checks;
        return;
    }

    auto rssi = wifi::sta::rssi();
    if (rssi > threshold) {
        counter = build::Checks;
    } else if (rssi < threshold) {
        if (counter < 0) {
            return;
        }

        if (!--counter) {
            wifi::action(wifi::Action::StationTryConnectBetter);
        }
    }
}

void start() {
    counter = build::Checks;
    timer.attach_ms(build::Interval.count(), task);
}

void stop() {
    counter = build::Checks;
    timer.detach();
}

} // namespace
} // namespace internal

namespace {

void threshold(int8_t value) {
    internal::threshold = value;
}

void stop() {
    internal::stop();
}

void start() {
    internal::start();
}

bool check() {
    if (internal::counter <= 0) {
        internal::counter = build::Checks;
        return true;
    }

    return false;
}

} // namespace
} // namespace periodic
} // namespace scan

namespace connection {
namespace {

// After scan attempt, generate a new networks list based on the results sorted by the rssi value.
// For the initial connection, add every matching network with the scan result bssid and channel info.
// For the attempt to find a better network, filter out every network with worse than the current network's rssi

void scanNetworks() {
    internal::scanResults = wifi::sta::scan::ssidinfos();
}

bool suitableNetwork(const Network& network, const SsidInfo& ssidInfo) {
    return (ssidInfo.ssid() == network.ssid())
        && ((ssidInfo.info().authmode() != AUTH_OPEN)
                ? network.passphrase().length()
                : !network.passphrase().length());
}

bool scanProcessResults(int8_t threshold) {
    if (internal::scanResults) {
        auto results = std::move(internal::scanResults);
        results->sort();

        if (threshold < 0) {
            results->remove_if([threshold](const wifi::SsidInfo& result) {
                return result.info().rssi() < threshold;
            });

        }

        Networks networks(std::move(internal::preparedNetworks));
        Networks sortedNetworks;

        for (auto& result : *results) {
            for (auto& network : networks) {
                if (suitableNetwork(network, result)) {
                    sortedNetworks.emplace_back(network, result.info().bssid(), result.info().channel());
                    break;
                }
            }
        }

        internal::preparedNetworks = std::move(sortedNetworks);
        internal::scanResults.reset();
    }

    return internal::preparedNetworks.size();
}

bool scanProcessResults(const wifi::Info& info) {
    return scanProcessResults(info.rssi());
}

bool scanProcessResults() {
    return scanProcessResults(0);
}

} // namespace
} // namespace connection

namespace {

void configure() {
    auto enabled = (wifi::StaMode::Enabled == wifi::sta::settings::mode());
    connection::persist(enabled);
    wifi::action(enabled
        ? wifi::Action::StationConnect
        : wifi::Action::StationDisconnect);

    scan::periodic::threshold(
        scan::periodic::settings::threshold());

#if WIFI_GRATUITOUS_ARP_SUPPORT
    auto interval = garp::settings::interval();
    if (interval.count()) {
        garp::start(interval);
    } else {
        garp::stop();
    }
#endif
}

} // namespace
} // namespace sta

// -----------------------------------------------------------------------------
// ACCESS POINT
// -----------------------------------------------------------------------------

namespace ap {
namespace build {
namespace {

static constexpr size_t SsidMax { sizeof(softap_config::ssid) };

static constexpr size_t PassphraseMin { 8u };
static constexpr size_t PassphraseMax { sizeof(softap_config::password) };

static constexpr int Hidden { 0 };
static constexpr uint8_t ConnectionsMax { 4u };

constexpr bool hasSsid() {
    return __builtin_strlen(WIFI_AP_SSID);
}

const __FlashStringHelper* ssid() {
    return F(WIFI_AP_SSID);
}

constexpr bool hasPassphrase() {
    return __builtin_strlen(WIFI_AP_PASS);
}

const __FlashStringHelper* passphrase() {
    return F(WIFI_AP_PASS);
}

constexpr bool captive() {
    return 1 == WIFI_AP_CAPTIVE_ENABLED;
}

constexpr ApMode mode() {
    return WIFI_AP_MODE;
}

constexpr uint8_t channel() {
    return WIFI_AP_CHANNEL;
}

} // namespace
} // namespace build

namespace settings {
namespace keys {
namespace {

alignas(4) static constexpr char Mode[] PROGMEM = "wifiApMode";

alignas(4) static constexpr char Ssid[] PROGMEM = "wifiApSsid";
alignas(4) static constexpr char Passphrase[] PROGMEM = "wifiApPass";

alignas(4) static constexpr char Channel[] PROGMEM = "wifiApChan";

[[gnu::unused]] alignas(4) static constexpr char Captive[] PROGMEM = "wifiApCaptive";

} // namespace
} // namespace keys

namespace {

wifi::ApMode mode() {
    return getSetting(FPSTR(keys::Mode), build::mode());
}

String defaultSsid() {
    return getIdentifier();
}

String ssid() {
    return getSetting(FPSTR(keys::Ssid), build::hasSsid()
        ? build::ssid()
        : getHostname());
}

String passphrase() {
    return getSetting(FPSTR(keys::Passphrase), build::hasPassphrase()
        ? build::passphrase()
        : getAdminPass());
}

int8_t channel() {
    return getSetting(FPSTR(keys::Channel), build::channel());
}

[[gnu::unused]]
bool captive() {
    return getSetting(FPSTR(keys::Captive), build::captive());
}

namespace query {
namespace internal {

EXACT_VALUE(captive, wifi::ap::settings::captive)
EXACT_VALUE(channel, wifi::ap::settings::channel)
EXACT_VALUE(mode, wifi::ap::settings::mode)

#undef ID_VALUE
#undef EXACT_VALUE

} // namespace internal
} // namespace query
} // namespace
} // namespace settings

namespace internal {
namespace {

#if WIFI_AP_CAPTIVE_SUPPORT
bool captive { build::captive() };
DNSServer dns;
#endif

void start(String&& defaultSsid, String&& ssid, String&& passphrase, uint8_t channel) {
    // Always generate valid AP config, even when user-provided credentials fail to comply with the requirements
    // TODO: configuration routine depends on a lwip dhcpserver, which is a custom module made specifically for the ESP.
    // while it's possible to hijack this and control the process manually, right now it's easier to delegate this to the Core helpers
    // (plus, it makes it not compatible with the esp-idf stack anyway, since wifi_softap_dhcps_... calls don't do anything here)

    const char* apSsid {
        (ssid.length() && (ssid.length() < build::SsidMax))
        ? ssid.c_str() : defaultSsid.c_str() };

    const char* apPass {
        (passphrase.length() \
         && (passphrase.length() >= build::PassphraseMin) \
         && (passphrase.length() < build::PassphraseMax))
        ? passphrase.c_str() : nullptr };

    // TODO: when using `softap_config`, can also tweak the beacon intvl
    // static constexpr uint16_t BeaconInterval { 100u };
    WiFi.softAP(apSsid, apPass, channel, build::Hidden, build::ConnectionsMax);
}

} // namespace
} // namespace internal

namespace {

#if WIFI_AP_CAPTIVE_SUPPORT

void captive(bool value) {
    internal::captive = value;
}

bool captive() {
    return internal::captive;
}

void dnsLoop() {
    internal::dns.processNextRequest();
}

#endif

void enable() {
    if (!WiFi.enableAP(true)) {
        abort();
    }
}

void disable() {
    if (!WiFi.enableAP(false)) {
        abort();
    }
}

bool enabled() {
    return wifi::opmode() & WIFI_AP;
}

void toggle() {
    wifi::action(wifi::ap::enabled()
        ? wifi::Action::AccessPointStop
        : wifi::Action::AccessPointStart);
}

void stop() {
#if WIFI_AP_CAPTIVE_SUPPORT
    internal::dns.stop();
#endif
    WiFi.softAPdisconnect();
}

void start(String&& defaultSsid, String&& ssid, String&& passphrase, uint8_t channel) {
    internal::start(std::move(defaultSsid), std::move(ssid),
        std::move(passphrase), channel);

#if WIFI_AP_CAPTIVE_SUPPORT
    if (internal::captive) {
        internal::dns.setErrorReplyCode(DNSReplyCode::NoError);
        internal::dns.start(53, "*", WiFi.softAPIP());
    } else {
        internal::dns.stop();
    }
#endif
}

wifi::SoftApNetwork current() {
    softap_config config{};
    wifi_softap_get_config(&config);

    wifi::Mac mac;
    WiFi.softAPmacAddress(mac.data());

    return {
        mac,
        convertSsid(config),
        convertPassphrase(config),
        config.channel,
        config.authmode};
}

void init() {
    disable();
}

size_t stations() {
    return WiFi.softAPgetStationNum();
}

} // namespace

namespace fallback {
namespace {
namespace build {

constexpr auto Timeout = espurna::duration::Milliseconds { WIFI_FALLBACK_TIMEOUT };

} // namespace build

namespace internal {

auto timeout = build::Timeout;
bool enabled { false };
Ticker timer;

} // namespace internal

void enable() {
    internal::enabled = true;
}

void disable() {
    internal::enabled = false;
}

bool enabled() {
    return internal::enabled;
}

void remove() {
    internal::timer.detach();
}

void check();

void schedule() {
    internal::timer.once_ms(internal::timeout.count(), check);
}

void check() {
    if (wifi::ap::enabled()
        && wifi::sta::connected()
        && !wifi::ap::stations())
    {
        remove();
        wifi::action(wifi::Action::AccessPointStop);
        return;
    }

    schedule();
}

} // namespace
} // namespace fallback

namespace {

void configure() {
    auto current = settings::mode();
    if (wifi::ApMode::Fallback == current) {
        fallback::enable();
    } else {
        fallback::disable();
        fallback::remove();
        wifi::action((wifi::ApMode::Enabled == current)
                ? wifi::Action::AccessPointStart
                : wifi::Action::AccessPointStop);
    }

#if WIFI_AP_CAPTIVE_SUPPORT
    captive(settings::captive());
#endif
}

}
} // namespace ap

// -----------------------------------------------------------------------------
// SETTINGS
// -----------------------------------------------------------------------------

namespace settings {
namespace query {
namespace {

static constexpr std::array<::settings::query::Setting, 10> Settings PROGMEM {
    {{wifi::ap::settings::keys::Ssid, wifi::ap::settings::ssid},
     {wifi::ap::settings::keys::Passphrase, wifi::ap::settings::passphrase},
     {wifi::ap::settings::keys::Captive, wifi::ap::settings::query::internal::captive},
     {wifi::ap::settings::keys::Channel, wifi::ap::settings::query::internal::channel},
     {wifi::ap::settings::keys::Mode, wifi::ap::settings::query::internal::mode},
     {wifi::sta::settings::keys::Mode, wifi::sta::settings::query::internal::mode},
     {wifi::sta::scan::settings::keys::Enabled, wifi::sta::scan::settings::query::enabled},
     {wifi::sta::scan::periodic::settings::keys::Threshold, wifi::sta::scan::periodic::settings::query::threshold},
     {wifi::settings::keys::TxPower, ::wifi::settings::query::internal::txPower},
     {wifi::settings::keys::Sleep, ::wifi::settings::query::internal::sleep}}
};

// indexed settings for 'sta' connections
bool checkIndexedPrefix(::settings::StringView key) {
    return ::settings::query::IndexedSetting::findSamePrefix(
        ::wifi::sta::settings::query::Settings, key);
}

// generic 'ap' and 'modem' configuration
bool checkExactPrefix(::settings::StringView key) {
    alignas(4) static constexpr char Prefix[] PROGMEM = "wifi";
    if (::settings::query::samePrefix(key, Prefix)) {
        return true;
    }

    return false;
}

String findIndexedValueFrom(::settings::StringView key) {
    using ::settings::query::IndexedSetting;
    return IndexedSetting::findValueFrom(
        wifi::sta::countNetworks(),
        wifi::sta::settings::query::Settings, key);
}

String findValueFrom(::settings::StringView key) {
    using ::settings::query::Setting;
    return Setting::findValueFrom(Settings, key);
}

void setup() {
    // TODO: small implementation detail - when searching, these
    // should be registered like this so the 'exact' is processed first
    settingsRegisterQueryHandler({
        .check = checkIndexedPrefix,
        .get = findIndexedValueFrom
    });

    settingsRegisterQueryHandler({
        .check = checkExactPrefix,
        .get = findValueFrom
    });
}

} // namespace
} // namespace query

namespace {

void configure() {
    wifi::ap::configure();
    wifi::sta::configure();

    WiFi.setSleepMode(wifi::settings::sleep());
    WiFi.setOutputPower(wifi::settings::txPower());
}

} // namespace
} // namespace settings

// -----------------------------------------------------------------------------
// TERMINAL
// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT
namespace terminal {
namespace {

void init() {

    terminalRegisterCommand(F("WIFI.STATIONS"), [](::terminal::CommandContext&& ctx) {
        size_t stations { 0ul };
        for (auto* it = wifi_softap_get_station_info(); it; it = STAILQ_NEXT(it, next), ++stations) {
            ctx.output.printf_P(PSTR("%s %s\n"),
                wifi::debug::mac(convertBssid(*it)).c_str(),
                wifi::debug::ip(it->ip).c_str());
        }

        wifi_softap_free_station_info();

        if (!stations) {
            terminalError(ctx, F("No stations connected"));
            return;
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("NETWORK"), [](::terminal::CommandContext&& ctx) {
        for (auto& addr : addrList) {
            ctx.output.printf_P(PSTR("%s%d %4s %6s "),
                addr.ifname().c_str(),
                addr.ifnumber(),
                addr.ifUp() ? "up" : "down",
                addr.isLocal() ? "local" : "global");

#if LWIP_IPV6
            if (addr.isV4()) {
#endif
                ctx.output.printf_P(PSTR("ip %s gateway %s mask %s\n"),
                    wifi::debug::ip(addr.ipv4()).c_str(),
                    wifi::debug::ip(addr.gw()).c_str(),
                    wifi::debug::ip(addr.netmask()).c_str());
#if LWIP_IPV6
            } else {
                // TODO: ip6_addr[...] array is included in the list
                // we'll just see another entry
                // TODO: routing info is not attached to the netif :/
                // ref. nd6.h (and figure out what it does)
                ctx.output.printf_P(PSTR("ip %s\n"),
                    wifi::debug::ip(netif->ip6_addr[i]).c_str());
            }
#endif

        }

        for (int n = 0; n < DNS_MAX_SERVERS; ++n) {
            auto ip = IPAddress(dns_getserver(n));
            if (!ip.isSet()) {
                break;
            }
            ctx.output.printf_P(PSTR("dns %s\n"), wifi::debug::ip(ip).c_str());
        }
    });

    terminalRegisterCommand(F("WIFI"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() == 2) {
            auto id = ::settings::internal::convert<size_t>(ctx.argv[1]);
            if (id < wifi::sta::build::NetworksMax) {
                settingsDump(ctx, wifi::sta::settings::query::Settings, id);
                return;
            }

            terminalError(ctx, F("Network ID out of configurable range"));
            return;
        }

        const auto mode = wifi::opmode();
        ctx.output.printf_P(PSTR("OPMODE: %s\n"), wifi::debug::opmode(mode).c_str());

        if (mode & OpmodeAp) {
            auto current = wifi::ap::current();

            ctx.output.printf_P(PSTR("SoftAP: bssid %s channel %hhu auth %s\n"),
                wifi::debug::mac(current.bssid).c_str(),
                current.channel,
                wifi::debug::authmode(current.authmode).c_str(),
                current.ssid.c_str(),
                current.passphrase.c_str());
        }

        if (mode & OpmodeSta) {
            if (wifi::sta::connected()) {
                station_config config{};
                wifi_station_get_config(&config);

                auto network = wifi::sta::current(config);
                ctx.output.printf_P(PSTR("STA: bssid %s rssi %hhd channel %hhu ssid \"%s\"\n"),
                    wifi::debug::mac(network.bssid).c_str(),
                    network.rssi, network.channel, network.ssid.c_str());
            } else {
                ctx.output.printf_P(PSTR("STA: %s\n"),
                        wifi::sta::connecting() ? "connecting" : "disconnected");
            }
        }

        settingsDump(ctx, wifi::settings::query::Settings);
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("WIFI.RESET"), [](::terminal::CommandContext&& ctx) {
        wifiDisconnect();
        wifi::settings::configure();
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("WIFI.STA"), [](::terminal::CommandContext&& ctx) {
        wifi::sta::toggle();
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("WIFI.AP"), [](::terminal::CommandContext&& ctx) {
        wifi::ap::toggle();
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("WIFI.SCAN"), [](::terminal::CommandContext&& ctx) {
        wifi::sta::scan::wait(
            [&](bss_info* info) {
                ctx.output.printf_P(PSTR("BSSID: %s AUTH: %11s RSSI: %3hhd CH: %2hhu SSID: %s\n"),
                    wifi::debug::mac(convertBssid(*info)).c_str(),
                    wifi::debug::authmode(info->authmode).c_str(),
                    info->rssi,
                    info->channel,
                    convertSsid(*info).c_str()
                );
            },
            [&](wifi::ScanError error) {
                terminalError(ctx, wifi::debug::error(error));
            }
        );
    });

}

} // namespace
} // namespace terminal
#endif

// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

namespace web {
namespace {

#if WEB_SUPPORT

void onConnected(JsonObject& root) {
    for (const auto& setting : wifi::settings::query::Settings) {
        root[FPSTR(setting.key().c_str())] = setting.value();
    }

    ::web::ws::EnumerableConfig config{root, STRING_VIEW("wifiConfig")};
    config(STRING_VIEW("networks"), wifi::sta::countNetworks(), wifi::sta::settings::query::Settings);

    auto& container = config.root();
    container[F("max")] = wifi::sta::build::NetworksMax;
}

bool onKeyCheck(const char* key, JsonVariant&) {
    const auto key_view = ::settings::StringView { key };
    return wifi::settings::query::checkExactPrefix(key_view)
        || wifi::settings::query::checkIndexedPrefix(key_view);
}

void onScan(uint32_t client_id) {
    if (wifi::sta::scanning()) {
        return;
    }

    wifi::sta::scan::start([client_id](bss_info* found) {
        wifi::SsidInfo result(*found);
        wsPost(client_id, [result](JsonObject& root) {
            JsonArray& scan = root.createNestedArray("scanResult");

            auto& info = result.info();
            scan.add(wifi::debug::mac(info.bssid()));
            scan.add(wifi::debug::authmode(info.authmode()));
            scan.add(info.rssi());
            scan.add(info.channel());

            scan.add(result.ssid());
        });
    },
    [client_id](wifi::ScanError error) {
        wsPost(client_id, [error](JsonObject& root) {
            root["scanError"] = wifi::debug::error(error);
        });
    });
}

void onAction(uint32_t client_id, const char* action, JsonObject&) {
    if (strcmp(action, "scan") == 0) {
        onScan(client_id);
    }
}

#endif

} // namespace
} // namespace web

// -----------------------------------------------------------------------------
// INITIALIZATION
// -----------------------------------------------------------------------------

namespace settings {
namespace {

void migrate(int version) {
    if (version < 5) {
        moveSetting(F("apmode"), ap::settings::keys::Mode);
    }
}

} // namespace
} // namespace settings

namespace debug {
namespace {

[[gnu::unused]]
String event(wifi::Event value) {
    String out;

    switch (value) {
    case wifi::Event::Initial:
        out = F("Initial");
        break;
    case wifi::Event::Mode: {
        const auto mode = wifi::opmode();
        out = F("Mode changed to ");
        out += wifi::debug::opmode(mode);
        break;
    }
    case wifi::Event::StationInit:
        out = F("Station init");
        break;
    case wifi::Event::StationScan:
        out = F("Scanning");
        break;
    case wifi::Event::StationConnecting:
        out = F("Connecting");
        break;
    case wifi::Event::StationConnected: {
        auto current = wifi::sta::current();
        out += F("Connected to BSSID ");
        out += wifi::debug::mac(current.bssid);
        out += F(" SSID ");
        out += current.ssid;
        break;
    }
    case wifi::Event::StationTimeout:
        out = F("Connection timeout");
        break;
    case wifi::Event::StationDisconnected: {
        auto current = wifi::sta::current();
        out += F("Disconnected from ");
        out += current.ssid;
        break;
    }
    case wifi::Event::StationReconnect:
        out = F("Reconnecting");
        break;
    }

    return out;
}

[[gnu::unused]]
const char* state(wifi::State value) {
    switch (value) {
    case wifi::State::Boot:
        return "Boot";
    case wifi::State::Connect:
        return "Connect";
    case wifi::State::TryConnectBetter:
        return "TryConnectBetter";
    case wifi::State::Fallback:
        return "Fallback";
    case wifi::State::Connected:
        return "Connected";
    case wifi::State::Idle:
        return "Idle";
    case wifi::State::Init:
        return "Init";
    case wifi::State::Timeout:
        return "Timeout";
    case wifi::State::WaitScan:
        return "WaitScan";
    case wifi::State::WaitScanWithoutCurrent:
        return "WaitScanWithoutCurrent";
    case wifi::State::WaitConnected:
        return "WaitConnected";
    }

    return "";
}

} // namespace
} // namespace debug

namespace internal {
namespace {

// STA + AP FALLBACK:
// - try connection
// - if ok, stop existing AP
// - if not, keep / start AP
//
// STA:
// - try connection
// - don't do anything on completion
//
// TODO? WPS / SMARTCONFIG + STA + AP FALLBACK
// - same as above
// - when requested, make sure there are no active connections
//   abort when sta connected or ap is connected
// - run autoconf, receive credentials and store in a free settings slot

// TODO: provide a clearer 'unroll' of the current state?

using EventCallbacks = std::forward_list<wifi::EventCallback>;
EventCallbacks callbacks;

void publish(wifi::Event event) {
    for (auto& callback : callbacks) {
        callback(event);
    }
}

void subscribe(wifi::EventCallback callback) {
    callbacks.push_front(callback);
}

State handleAction(State& state, Action action) {
    switch (action) {
    case Action::StationConnect:
        if (!wifi::sta::enabled()) {
            wifi::sta::enable();
            publish(wifi::Event::Mode);
        }

        if (!wifi::sta::connected()) {
            if (wifi::sta::connecting()) {
                wifi::sta::connection::schedule_next();
            } else {
                state = State::Init;
            }
        }
        break;

    case Action::StationContinueConnect:
        if (wifi::sta::connecting()) {
            state = State::Connect;
        }
        break;

    case Action::StationDisconnect:
        if (wifi::sta::connected()) {
            wifi::ap::fallback::remove();
            wifi::sta::disconnect();
        }

        wifi::sta::connection::stop();

        if (wifi::sta::enabled()) {
            wifi::sta::disable();
            publish(wifi::Event::Mode);
        }
        break;

    case Action::StationTryConnectBetter:
        if (!wifi::sta::connected() || wifi::sta::connecting()) {
            wifi::sta::scan::periodic::stop();
            break;
        }

        if (wifi::sta::scan::periodic::check()) {
            state = State::TryConnectBetter;
        }
        break;

    case Action::AccessPointFallback:
    case Action::AccessPointStart:
        if (!wifi::ap::enabled()) {
            wifi::ap::enable();
            wifi::ap::start(
                wifi::ap::settings::defaultSsid(),
                wifi::ap::settings::ssid(),
                wifi::ap::settings::passphrase(),
                wifi::ap::settings::channel());
            publish(wifi::Event::Mode);
            if ((Action::AccessPointFallback == action)
                    && wifi::ap::fallback::enabled()) {
                wifi::ap::fallback::schedule();
            }
        }
        break;

    case Action::AccessPointFallbackCheck:
        if (wifi::ap::fallback::enabled()) {
            wifi::ap::fallback::check();
        }
        break;

    case Action::AccessPointStop:
        if (wifi::ap::enabled()) {
            wifi::ap::fallback::remove();
            wifi::ap::stop();
            wifi::ap::disable();
            publish(wifi::Event::Mode);
        }
        break;

    case Action::TurnOff:
        if (wifi::enabled()) {
            wifi::ap::fallback::remove();
            wifi::ap::stop();
            wifi::ap::disable();
            wifi::sta::scan::periodic::stop();
            wifi::sta::connection::stop();
            wifi::sta::disconnect();
            wifi::sta::disable();
            wifi::disable();
            publish(wifi::Event::Mode);
            if (!wifi::sleep()) {
                wifi::action(wifi::Action::TurnOn);
                break;
            }
        }
        break;

    case Action::TurnOn:
        if (!wifi::enabled()) {
            wifi::enable();
            wifi::wakeup();
            wifi::settings::configure();
        }
        break;

    }

    return state;
}

bool prepareConnection() {
    if (wifi::sta::enabled()) {
        wifi::sta::connection::prepare(wifi::sta::networks());
        return wifi::sta::connection::prepared();
    }

    return false;
}

void loop() {
    static State state { State::Boot };
    static State last_state { state };

    if (last_state != state) {
        DEBUG_MSG_P(PSTR("[WIFI] State %s -> %s\n"),
            debug::state(last_state),
            debug::state(state));
        last_state = state;
    }

    switch (state) {

    case State::Boot:
        state = State::Idle;
        publish(wifi::Event::Initial);
        break;

    case State::Init: {
        if (!prepareConnection()) {
            state = State::Fallback;
            break;
        }

        wifi::sta::scan::periodic::stop();
        if (wifi::sta::scan::settings::enabled()) {
            if (wifi::sta::scanning()) {
                break;
            }
            wifi::sta::connection::scanNetworks();
            state = State::WaitScan;
            break;
        }

        state = State::Connect;
        break;
    }

    case State::TryConnectBetter:
        if (wifi::sta::scan::settings::enabled()) {
            if (wifi::sta::scanning()) {
                break;
            }

            if (!prepareConnection()) {
                state = State::Idle;
                break;
            }

            wifi::sta::scan::periodic::stop();
            wifi::sta::connection::scanNetworks();
            state = State::WaitScanWithoutCurrent;
            break;
        }
        state = State::Idle;
        break;

    case State::Fallback:
        state = State::Idle;
        wifi::sta::connection::schedule_new();
        if (wifi::ApMode::Fallback == wifi::ap::settings::mode()) {
            wifi::action(wifi::Action::AccessPointFallback);
        }
        publish(wifi::Event::StationReconnect);
        break;

    case State::WaitScan:
        if (wifi::sta::scanning()) {
            break;
        }

        wifi::sta::connection::scanProcessResults();
        state = State::Connect;
        break;

    case State::WaitScanWithoutCurrent:
        if (wifi::sta::scanning()) {
            break;
        }

        if (wifi::sta::connection::scanProcessResults(wifi::sta::info())) {
            wifi::sta::disconnect();
            state = State::Connect;
            break;
        }

        state = State::Idle;
        break;

    case State::Connect: {
        if (!wifi::sta::connecting()) {
            if (!wifi::sta::connection::start(getHostname())) {
                state = State::Timeout;
                break;
            }
        }

        if (wifi::sta::connection::connect()) {
            state = State::WaitConnected;
            publish(wifi::Event::StationConnecting);
        } else {
            state = State::Timeout;
        }
        break;
    }

    case State::WaitConnected:
        if (wifi::sta::connection::wait()) {
            break;
        }

        if (wifi::sta::connected()) {
            state = State::Connected;
            break;
        }

        state = State::Timeout;
        break;

    // Current logic closely follows the SDK connection routine with reconnect enabled,
    // and will retry the same network multiple times before giving up.
    case State::Timeout:
        if (wifi::sta::connecting() && wifi::sta::connection::next()) {
            state = State::Idle;
            wifi::sta::connection::schedule_next();
            publish(wifi::Event::StationTimeout);
        } else {
            wifi::sta::connection::stop();
            state = State::Fallback;
        }
        break;

    case State::Connected:
        wifi::sta::connection::stop();
        if (wifi::sta::scan::settings::enabled()) {
            wifi::sta::scan::periodic::start();
        }
        state = State::Idle;
        publish(wifi::Event::StationConnected);
        break;

    case State::Idle: {
        auto& actions = wifi::actions();
        if (!actions.empty()) {
            state = handleAction(state, actions.front());
            actions.pop();
        }
        break;
    }

    }

    // SDK disconnection event is specific to the phy layer. i.e. it will happen all the same
    // when trying to connect and being unable to find the AP, being forced out by the AP with bad credentials
    // or being disconnected when the wireless signal is lost.
    // Thus, provide a specific connected -> disconnected event specific to the IP network availability.
    if (wifi::sta::connection::lost()) {
        wifi::sta::scan::periodic::stop();
        if (wifi::sta::connection::persist()) {
            wifi::sta::connection::schedule_new(wifi::sta::build::RecoveryInterval);
        }
        publish(wifi::Event::StationDisconnected);
    }

#if WIFI_AP_CAPTIVE_SUPPORT
    // Captive portal only queues packets and those need to be processed asap
    if (wifi::ap::enabled() && wifi::ap::captive()) {
        wifi::ap::dnsLoop();
    }
#endif
#if WIFI_GRATUITOUS_ARP_SUPPORT
    // ref: https://github.com/xoseperez/espurna/pull/1877#issuecomment-525612546
    // Periodically send out ARP, even if no one asked
    if (wifi::sta::connected() && !wifi::sta::garp::wait()) {
        wifi::sta::garp::send();
    }
#endif
}

// XXX: With Arduino Core 3.0.0, WiFi is asleep on boot
// It will wake up when calling WiFi::mode(...):
// - WiFi.begin(...)
// - WiFi.softAP(...)
// - WiFi.enableSTA(...)
// - WiFi.enableAP(...)
// ref. https://github.com/esp8266/Arduino/pull/7902

void init() {
    WiFi.persistent(false);
    wifi::ap::init();
    wifi::sta::init();
}

} // namespace
} // namespace internal
} // namespace wifi

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

void wifiRegister(wifi::EventCallback callback) {
    wifi::internal::subscribe(callback);
}

bool wifiConnectable() {
    return wifi::ap::enabled();
}

bool wifiConnected() {
    return wifi::sta::connected();
}

IPAddress wifiStaIp() {
    if (wifi::opmode() & wifi::OpmodeSta) {
        return WiFi.localIP();
    }

    return {};
}

String wifiStaSsid() {
    if (wifi::opmode() & wifi::OpmodeSta) {
        auto current = wifi::sta::current();
        return current.ssid;
    }

    return emptyString;
}

void wifiDisconnect() {
    wifi::sta::disconnect();
}

void wifiToggleAp() {
    wifi::ap::toggle();
}

void wifiToggleSta() {
    wifi::sta::toggle();
}

void wifiStartAp() {
    wifi::action(wifi::Action::AccessPointStart);
}

void wifiTurnOff() {
    wifi::action(wifi::Action::TurnOff);
}

void wifiTurnOn() {
    wifi::action(wifi::Action::TurnOn);
}

void wifiApCheck() {
    wifi::action(wifi::Action::AccessPointFallbackCheck);
}

size_t wifiApStations() {
    if (wifi::ap::enabled()) {
        return wifi::ap::stations();
    }

    return 0;
}

void wifiSetup() {
    wifi::internal::init();

    migrateVersion(wifi::settings::migrate);
    wifi::settings::configure();
    wifi::settings::query::setup();

#if SYSTEM_CHECK_ENABLED
    if (!systemCheck()) {
        wifi::actions() = wifi::ActionsQueue{};
        wifi::action(wifi::Action::AccessPointStart);
    }
#endif

#if DEBUG_SUPPORT
    wifiRegister([](wifi::Event event) {
        DEBUG_MSG_P(PSTR("[WIFI] %s\n"), wifi::debug::event(event).c_str());
    });
#endif

#if WEB_SUPPORT
    wsRegister()
        .onAction(wifi::web::onAction)
        .onConnected(wifi::web::onConnected)
        .onKeyCheck(wifi::web::onKeyCheck);
#endif

#if TERMINAL_SUPPORT
    wifi::terminal::init();
#endif

    espurnaRegisterLoop(wifi::internal::loop);
    espurnaRegisterReload(wifi::settings::configure);
}
