/*

MATTER BACKEND FOR ESP32 (Arduino-ESP32 core 3.x)

On/Off plug endpoints of the Matter library of Arduino-ESP32 (esp_matter, connectedhomeip). ESPurna keeps Wi-Fi, the
relays and the web UI (matter.cpp). Commissioning is on-network only: ESPurna is on Wi-Fi already, and the libraries
are built without CHIPoBLE. Setup codes and device attestation are the board's own (matter_identity_esp32.cpp), under
the SDK test PAI (VID 0xFFF1, PID 0x8000)

*/

#include "espurna.h"

#if defined(ARDUINO_ARCH_ESP32) && MATTER_SUPPORT

#include "espurna_matter.h"
#include "matter_identity_esp32.h"
#include "relay.h"

#include <Matter.h>
#include <WiFi.h>

#include <app/server/Server.h>
#include <platform/CHIPDeviceLayer.h>

#include <cstring>

namespace espurna {
namespace matter {
namespace backend {
namespace {

constexpr size_t MaxPlugs { 4 };

MatterOnOffPlugin plugs[MaxPlugs];
size_t count { 0 };
bool begun { false };

// Identity strings are copied by the Matter library, but keep them for the panel
String serial;
String label;

// "https://...qrcode.html?data=MT%3A..." -> "MT:..."
bool qr_payload(const String& url, char* out, size_t size) {
    const auto start = url.indexOf(F("data="));
    if ((start < 0) || !size) {
        return false;
    }

    size_t length { 0 };
    for (size_t index = start + 5; (index < url.length()) && (length + 1 < size); ++index) {
        char c = url[index];
        if ((c == '%') && (index + 2 < url.length())) {
            const char hex[3] { url[index + 1], url[index + 2], '\0' };
            c = static_cast<char>(std::strtoul(hex, nullptr, 16));
            index += 2;
        }
        out[length++] = c;
    }
    out[length] = '\0';

    return std::strncmp(out, "MT:", 3) == 0;
}

} // namespace

size_t endpoints() {
    return std::min(relayCount(), MaxPlugs);
}

void setup() {
    // Matter runs over IPv6 (link-local at least): before ESPurna's first connection attempt
    WiFi.enableIPv6(true);
}

bool start() {
    if (begun) {
        return true;
    }

    count = endpoints();
    for (size_t index = 0; index < count; ++index) {
        plugs[index].begin(relayStatus(index) == RelayStatus::On);
        // Matter task: the relay is switched in the loop
        plugs[index].onChange([index](bool value) {
            onoff_changed(index, value);
            return true;
        });
    }

    // Basic Information: the speaker hub reads the serial number while commissioning and stops when it is missing
    serial = systemChipId();
    label = systemHostname();

    Matter.setVendorName("ESPurna");
    Matter.setProductName(String(systemDevice()).c_str());
    Matter.setDeviceName(label.c_str());
    Matter.setSerialNumber(serial.c_str());
    Matter.setHardwareVersion(1);
    Matter.setHardwareVersionString("1");

    Matter.onEvent([](matterEvent_t event, const chip::DeviceLayer::ChipDeviceEvent*) {
        switch (event) {
        case MATTER_COMMISSIONING_SESSION_STARTED:
            commissioning(true);
            break;
        case MATTER_COMMISSIONING_SESSION_STOPPED:
        case MATTER_COMMISSIONING_COMPLETE:
        case MATTER_FAIL_SAFE_TIMER_EXPIRED:
            commissioning(false);
            break;
        default:
            break;
        }
    });

    // the SDK test codes stay when NVS fails
    uint32_t passcode;
    uint16_t discriminator;
    if (identity::load(passcode, discriminator)) {
        Matter.setSetupPasscode(passcode);
        Matter.setSetupDiscriminator(discriminator);
    }

    Matter.begin();
    begun = true;

    // first start: the key and the certificate take a moment (two signatures)
    if (!identity::install()) {
        DEBUG_MSG_P(PSTR("[MATTER] Own device attestation failed, the SDK test one stays\n"));
    }
    DEBUG_MSG_P(PSTR("[MATTER] Identity: %s\n"), identity::status().c_str());

    return true;
}

void network(bool) {
}

void set_onoff(size_t index, bool on) {
    if (begun && (index < count)) {
        plugs[index].setOnOff(on);
    }
}

bool codes(char* manual, size_t manual_size, char* qr, size_t qr_size) {
    if (!begun || !manual_size || !qr_size) {
        return false;
    }

    const auto code = Matter.getManualPairingCode();
    if (!code.length()) {
        return false;
    }

    std::strncpy(manual, code.c_str(), manual_size - 1);
    manual[manual_size - 1] = '\0';

    return qr_payload(Matter.getOnboardingQRCodeUrl(), qr, qr_size);
}

int fabrics() {
    if (!begun) {
        return 0;
    }

    chip::DeviceLayer::PlatformMgr().LockChipStack();
    const auto out = chip::Server::GetInstance().GetFabricTable().FabricCount();
    chip::DeviceLayer::PlatformMgr().UnlockChipStack();

    return out;
}

void open_window() {
    if (!begun) {
        return;
    }

    chip::DeviceLayer::PlatformMgr().LockChipStack();
    auto& manager = chip::Server::GetInstance().GetCommissioningWindowManager();
    if (!manager.IsCommissioningWindowOpen()) {
        manager.OpenBasicCommissioningWindow();
    }
    chip::DeviceLayer::PlatformMgr().UnlockChipStack();
}

// A work item through the Matter task, answered from the loop
void ping() {
    if (begun) {
        chip::DeviceLayer::PlatformMgr().ScheduleWork([](intptr_t) { pong(); });
    }
}

String product() {
    uint16_t vendor { 0 };
    uint16_t product { 0 };
    // the provider is only set up by Matter.begin(): before that (the WebUI asks on every connection) it crashed
    if (begun) {
        auto* info = chip::DeviceLayer::GetDeviceInstanceInfoProvider();
        if (info) {
            info->GetVendorId(vendor);
            info->GetProductId(product);
        }
    }

    String out;
    out.reserve(64);
    out += F("ESPurna (0x");
    out += String(vendor, 16);
    out += F(") / ");
    out += String(systemDevice());
    out += F(" (0x");
    out += String(product, 16);
    out += ')';

    return out;
}

size_t heap() {
    return esp_get_free_heap_size();
}

size_t heap_min() {
    return esp_get_minimum_free_heap_size();
}

} // namespace backend
} // namespace matter
} // namespace espurna

#endif
