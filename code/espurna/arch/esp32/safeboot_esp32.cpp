/*

SAFEBOOT (ESP32)

ref. safeboot_esp32.h

*/

#include "espurna.h"

#if SAFEBOOT_SUPPORT

#include "safeboot_esp32.h"
#include "terminal.h"
#include "web.h"

#include <esp_attr.h>
#include <esp_flash.h>
#include <esp_image_format.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_rom_md5.h>
#include <esp_system.h>
#include <nvs.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>

namespace espurna {
namespace safeboot {
namespace {

constexpr uint32_t Magic { 0x53414642 }; // SAFB
constexpr uint32_t CrashLoopMax { 5 };
constexpr uint32_t CrashLoopUptimeMs { 60000 };

RTC_NOINIT_ATTR uint32_t rtc_magic;
RTC_NOINIT_ATTR uint32_t rtc_count;

uint32_t count_at_boot { 0 };
bool cleared { false };

// Only crashes count, plus software restarts that ESPurna did not request itself
// (ESPurna always stores a custom reason before its own restarts)
bool counted(esp_reset_reason_t reason) {
    switch (reason) {
    case ESP_RST_PANIC:
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:
    case ESP_RST_UNKNOWN:
        return true;
    case ESP_RST_SW:
        return customResetReason() == CustomResetReason::None;
    default:
        break;
    }

    return false;
}

const esp_partition_t* factory() {
    return esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, nullptr);
}

// esp_ota_get_next_update_partition() falls back to the first OTA slot, which is the running
// one when the table has a single OTA app. Update would then erase the running image
bool otaPossible() {
    const auto* running = esp_ota_get_running_partition();
    const auto* next = esp_ota_get_next_update_partition(nullptr);
    return next && running && (next->address != running->address);
}

bool switchToSafeboot(String& error) {
    const auto* part = factory();
    if (!part) {
        error = F("no factory (safeboot) app partition");
        return false;
    }

    const auto* running = esp_ota_get_running_partition();
    if (running && (running->address == part->address)) {
        error = F("already running from the factory partition");
        return false;
    }

    // a merged image flashed without the safeboot one leaves the factory partition erased
    uint8_t magic { 0 };
    if ((esp_partition_read(part, 0, &magic, sizeof(magic)) != ESP_OK) || (magic != 0xE9)) {
        error = F("factory (safeboot) partition holds no image");
        return false;
    }

    const auto result = esp_ota_set_boot_partition(part);
    if (result != ESP_OK) {
        error = esp_err_to_name(result);
        return false;
    }

    return true;
}

void guard() {
    const auto reason = esp_reset_reason();
    if ((rtc_magic != Magic) || (reason == ESP_RST_POWERON)) {
        rtc_magic = Magic;
        rtc_count = 0;
    }

    if (counted(reason)) {
        ++rtc_count;
    }

    count_at_boot = rtc_count;
    if (rtc_count >= CrashLoopMax) {
        rtc_count = 0;
        DEBUG_MSG_P(PSTR("[SAFEBOOT] %u unstable boots in a row, switching to safeboot\n"), count_at_boot);

        String error;
        if (switchToSafeboot(error)) {
            delay(100);
            esp_restart();
        }

        DEBUG_MSG_P(PSTR("[SAFEBOOT] Switch failed: %s\n"), error.c_str());
    }
}

void tick() {
    if (!cleared && (millis() >= CrashLoopUptimeMs)) {
        rtc_count = 0;
        cleared = true;
    }
}

// The settings Tasmota's safeboot image reads at boot (NVS namespace "main", blob "Settings", TSettings of
// Tasmota 16.0.0, tasmota/include/tasmota_types.h). The safeboot build takes the blob as it is (no defaults
// for a FIRMWARE_MINIMAL build), so everything it needs to join Wi-Fi is filled in here. NVS "otal"/"otal" = 1
// makes it download OtaUrl into app0 as soon as the network is up, then it boots that image.
// The blob stays, so later updates through safeboot (`safeboot` command, crash-loop guard) find Wi-Fi as well
namespace tasmota {

constexpr size_t SettingsSize { 4096 };
constexpr uint32_t Version { 0x10000000 };   // 16.0.0
constexpr uint16_t CfgHolder { 4617 };
constexpr size_t TextPool { 0x017 };
constexpr size_t TextPoolSize { 699 };
constexpr size_t Baudrate { 0x778 };         // / 300
constexpr size_t StaConfig { 0xEC7 };        // WIFI_MANAGER: AP at 192.168.4.1 when Wi-Fi fails
constexpr size_t Webserver { 0xECD };        // 2 = admin
constexpr size_t Flag4 { 0xEF8 };            // bit 13 network_wifi
constexpr size_t SerialConfig { 0xEFE };     // TS_SERIAL_8N1
constexpr size_t WifiOutputPower { 0xEFF };  // MAX_TX_PWR_DBM_54g of the ESP32

constexpr size_t UrlMax { 140 };             // OtaUrl is copied into a TOPSZ (151) buffer

void put16(uint8_t* out, size_t offset, uint16_t value) {
    std::memcpy(out + offset, &value, sizeof(value));
}

void put32(uint8_t* out, size_t offset, uint32_t value) {
    std::memcpy(out + offset, &value, sizeof(value));
}

bool nvsBlob(const char* space, const char* key, const void* data, size_t size) {
    nvs_handle_t handle;
    if (nvs_open(space, NVS_READWRITE, &handle) != ESP_OK) {
        return false;
    }

    auto result = nvs_set_blob(handle, key, data, size);
    if (result == ESP_OK) {
        result = nvs_commit(handle);
    }

    nvs_close(handle);
    return result == ESP_OK;
}

// Text pool: \0-separated strings in SettingsTextIndex order
// SET_OTAURL, SET_MQTTPREFIX1..3, SET_STASSID1, SET_STASSID2, SET_STAPWD1, SET_STAPWD2, SET_HOSTNAME,
// SET_SYSLOG_HOST, SET_WEBPWD (the rest stays empty)
bool write(const String& url, String& error) {
    std::unique_ptr<uint8_t[]> blob(new (std::nothrow) uint8_t[SettingsSize]());
    if (!blob) {
        error = F("out of memory");
        return false;
    }

    auto* out = blob.get();
    put16(out, 0x000, CfgHolder);
    put16(out, 0x002, SettingsSize);
    put32(out, 0x008, Version);

    const String texts[] {
        url, String(), String(), String(),
        getSetting(espurna::settings::Key{"ssid", 0}), getSetting(espurna::settings::Key{"ssid", 1}),
        getSetting(espurna::settings::Key{"pass", 0}), getSetting(espurna::settings::Key{"pass", 1}),
        systemHostname(), String(),
        systemPassword(),
    };

    size_t pos { TextPool };
    for (const auto& text : texts) {
        if ((pos + text.length() + 1) > (TextPool + TextPoolSize)) {
            error = F("Wi-Fi / hostname / password too long for the safeboot settings");
            return false;
        }

        std::memcpy(out + pos, text.c_str(), text.length());
        pos += text.length() + 1;
    }

    put16(out, Baudrate, 115200 / 300);
    out[StaConfig] = 2;
    out[Webserver] = 2;
    put32(out, Flag4, 1ul << 13);
    out[SerialConfig] = 3;
    out[WifiOutputPower] = 160;

    if (!nvsBlob("main", "Settings", out, SettingsSize)) {
        error = F("cannot write the safeboot settings (NVS full?)");
        return false;
    }

    const uint32_t loader { 1 };
    if (!nvsBlob("otal", "otal", &loader, sizeof(loader))) {
        error = F("cannot write the safeboot OTA flag");
        return false;
    }

    return true;
}

bool validUrl(const String& url) {
    return (url.length() > 10) && (url.length() <= UrlMax)
        && (url.startsWith(F("http://")) || url.startsWith(F("https://")))
        && (url.indexOf('%') < 0);
}

} // namespace tasmota

// From the two-slot layouts of the Arduino core (min_spiffs.csv, default.csv) to esp32_4mb_safeboot.csv over Wi-Fi.
// ESPurna has to run from the second slot: the safeboot image goes over the first one (0x10000, below the new app0),
// then the partition table at 0x8000 is replaced and otadata erased, so the next boot starts the factory partition.
// Tasmota's safeboot joins Wi-Fi with the settings above and downloads ESPurna into app0. NVS (0x9000) and otadata
// (0xE000) keep their place, ESPurna settings stay
namespace layout {

constexpr uint32_t TableOffset { 0x8000 };
constexpr size_t TableSize { 0xC00 };
constexpr uint32_t SafebootOffset { 0x10000 };
constexpr uint32_t SafebootSize { 0xD0000 };
constexpr uint32_t AppOffset { 0xE0000 };
constexpr uint32_t FlashSize { 0x400000 };

struct Entry {
    uint8_t type;
    uint8_t subtype;
    uint32_t offset;
    uint32_t size;
    const char* label;
};

// esp32_4mb_safeboot.csv
constexpr Entry Entries[] {
    {0x01, 0x02, 0x9000, 0x5000, "nvs"},
    {0x01, 0x00, 0xE000, 0x2000, "otadata"},
    {0x00, 0x00, SafebootOffset, SafebootSize, "safeboot"},
    {0x00, 0x10, AppOffset, 0x2D0000, "app0"},
    {0x01, 0x82, 0x3B0000, 0x40000, "spiffs"},
    {0x01, 0x03, 0x3F0000, 0x10000, "coredump"},
};

// ESP-IDF partition table: 32-byte entries (0xAA 0x50, type, subtype, offset, size, label[16], flags), then
// the MD5 entry (0xEB 0xEB, 14 x 0xFF, MD5 of the entries above), 0xFF up to the end
void table(uint8_t* out) {
    std::memset(out, 0xFF, TableSize);

    size_t pos { 0 };
    for (const auto& entry : Entries) {
        auto* row = out + pos;
        std::memset(row, 0, 32);
        row[0] = 0xAA;
        row[1] = 0x50;
        row[2] = entry.type;
        row[3] = entry.subtype;
        std::memcpy(row + 4, &entry.offset, 4);
        std::memcpy(row + 8, &entry.size, 4);
        std::strncpy(reinterpret_cast<char*>(row + 12), entry.label, 16);
        pos += 32;
    }

    md5_context_t ctx;
    esp_rom_md5_init(&ctx);
    esp_rom_md5_update(&ctx, out, pos);

    auto* row = out + pos;
    row[0] = 0xEB;
    row[1] = 0xEB;
    std::memset(row + 2, 0xFF, 14);
    esp_rom_md5_final(row + 16, &ctx);
}

bool current() {
    const auto* app = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr);
    return factory() && app && (app->address == AppOffset);
}

// the old slot that becomes the safeboot partition
const esp_partition_t* target() {
    const auto* running = esp_ota_get_running_partition();
    const auto* part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr);
    if (part && running && (part != running)
        && (part->address == SafebootOffset) && (part->size >= SafebootSize))
    {
        return part;
    }

    return nullptr;
}

bool possible(String& error) {
    if (current()) {
        error = F("the flash layout is already the safeboot one");
        return false;
    }

    if (factory()) {
        error = F("unknown layout with a factory partition");
        return false;
    }

    uint32_t size { 0 };
    if ((esp_flash_get_size(nullptr, &size) != ESP_OK) || (size < FlashSize)) {
        error = F("needs 4 MB of flash");
        return false;
    }

    const auto* nvs = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, nullptr);
    const auto* otadata = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, nullptr);
    if (!nvs || (nvs->address != 0x9000) || (nvs->size != 0x5000)
        || !otadata || (otadata->address != 0xE000) || (otadata->size != 0x2000))
    {
        error = F("NVS / otadata are not at 0x9000 / 0xE000, the settings would be lost");
        return false;
    }

    const auto* running = esp_ota_get_running_partition();
    if (!running || (running->address < AppOffset)) {
        error = F("ESPurna runs from the first slot. Update it once more (any image, it goes to the second slot), then migrate");
        return false;
    }

    if (!target()) {
        error = F("no first OTA slot at 0x10000 big enough for the safeboot image");
        return false;
    }

    return true;
}

// esp_flash_write() refuses (aborts with the Arduino sdkconfig) writes over the partition table. Same chip,
// without the region check, for the one table write
bool writeTable(const uint8_t* data) {
    esp_flash_os_functions_t os = *esp_flash_default_chip->os_func;
    os.region_protected = nullptr;

    esp_flash_t chip = *esp_flash_default_chip;
    chip.os_func = &os;

    std::unique_ptr<uint8_t[]> check(new (std::nothrow) uint8_t[TableSize]);
    if (!check) {
        return false;
    }

    for (int attempt = 0; attempt < 3; ++attempt) {
        if ((esp_flash_erase_region(&chip, TableOffset, 0x1000) == ESP_OK)
            && (esp_flash_write(&chip, data, TableOffset, TableSize) == ESP_OK)
            && (esp_flash_read(&chip, check.get(), TableOffset, TableSize) == ESP_OK)
            && (std::memcmp(check.get(), data, TableSize) == 0))
        {
            return true;
        }
    }

    return false;
}

struct Upload {
    const esp_partition_t* part { nullptr };
    size_t written { 0 };
    size_t erased { 0 };
    bool done { false };
    String error;
};

Upload upload;
String pending_url;
String last_error;

bool uploadWrite(const uint8_t* data, size_t len) {
    if ((upload.written + len) > SafebootSize) {
        upload.error = F("the safeboot image is bigger than its partition");
        return false;
    }

    const size_t end = upload.written + len;
    if (end > upload.erased) {
        const size_t erase = ((end - upload.erased) + 0xFFF) & ~size_t(0xFFF);
        if (esp_partition_erase_range(upload.part, upload.erased, erase) != ESP_OK) {
            upload.error = F("erase failed");
            return false;
        }
        upload.erased += erase;
    }

    if (esp_partition_write(upload.part, upload.written, data, len) != ESP_OK) {
        upload.error = F("write failed");
        return false;
    }

    upload.written = end;
    return true;
}

bool uploadVerify() {
    const esp_partition_pos_t pos {
        .offset = upload.part->address,
        .size = upload.part->size,
    };

    esp_image_metadata_t meta;
    if (esp_image_verify(ESP_IMAGE_VERIFY_SILENT, &pos, &meta) != ESP_OK) {
        upload.error = F("the safeboot image does not verify (checksum / hash)");
        return false;
    }

    if ((meta.image.chip_id != ESP_CHIP_ID_ESP32) || (meta.image_len > SafebootSize)) {
        upload.error = F("not a safeboot image for the ESP32 (tasmota32-safeboot.bin)");
        return false;
    }

    return true;
}

// in loop(): nothing else touches the flash while the table is replaced
void migrate() {
    String error;
    if (!tasmota::write(pending_url, error)) {
        DEBUG_MSG_P(PSTR("[SAFEBOOT] Migration stopped: %s\n"), error.c_str());
        last_error = error;
        return;
    }

    std::unique_ptr<uint8_t[]> data(new (std::nothrow) uint8_t[TableSize]);
    if (!data) {
        DEBUG_MSG_P(PSTR("[SAFEBOOT] Migration stopped: out of memory\n"));
        last_error = F("out of memory");
        return;
    }

    table(data.get());
    DEBUG_MSG_P(PSTR("[SAFEBOOT] Writing the partition table, do not power off\n"));
    if (!writeTable(data.get())) {
        // nothing the bootloader can use at 0x8000 now, the next boot needs a serial flash
        DEBUG_MSG_P(PSTR("[SAFEBOOT] PARTITION TABLE WRITE FAILED, do not restart, flash it over serial\n"));
        last_error = F("PARTITION TABLE WRITE FAILED, do not restart, flash it over serial");
        return;
    }

    // blank otadata: the bootloader starts the factory (safeboot) partition. A failed erase is no harm,
    // app0 holds no valid image yet and the bootloader falls back to factory as well
    const auto* otadata = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, nullptr);
    if (otadata) {
        esp_partition_erase_range(otadata, 0, otadata->size);
    }

    DEBUG_MSG_P(PSTR("[SAFEBOOT] Layout migrated, restarting into safeboot, it downloads %s\n"), pending_url.c_str());
    prepareReset(CustomResetReason::Web);
}

} // namespace layout

// Update through safeboot from an URL (safeboot layout): the same settings, factory as the next boot
bool updateFromUrl(const String& url, String& error) {
    if (!layout::current()) {
        error = F("not the safeboot layout");
        return false;
    }

    if (!tasmota::validUrl(url)) {
        error = F("invalid URL (http:// or https://, up to 140 characters, no %)");
        return false;
    }

    if (!tasmota::write(url, error)) {
        return false;
    }

    return switchToSafeboot(error);
}

#if WEB_SUPPORT
PROGMEM_STRING(NoOtaMessage,
    "ERROR: this flash layout (safeboot) has no second OTA slot. "
    "Update from an URL on the /safeboot page, or run the terminal command `safeboot` and upload the image "
    "in the safeboot web UI\n");

void onNoOta(AsyncWebServerRequest* request) {
    request->send(400, "text/plain", NoOtaMessage);
}

void onNoOtaUpload(AsyncWebServerRequest*, String, size_t, uint8_t*, size_t, bool) {
}

constexpr char DefaultUrl[] PROGMEM =
#if defined(SAFEBOOT_DEFAULT_URL)
    SAFEBOOT_DEFAULT_URL;
#else
    "https://github.com/igorufox/espurna/releases/latest/download/espurna-esp32-ota.bin";
#endif

String otaUrl() {
    return getSetting("sbUrl", DefaultUrl);
}

PROGMEM_STRING(Page, R"(<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESPurna flash layout</title><style>
body{font-family:sans-serif;max-width:640px;margin:16px auto;padding:0 16px;line-height:1.4}
input[type=text]{width:100%;box-sizing:border-box}button{margin-top:12px;padding:6px 16px}
.err{color:#b00}.ok{color:#070}#log{white-space:pre-wrap;background:#f4f4f4;padding:8px;margin-top:12px}
</style></head><body><h2>Flash layout</h2><div id="info">...</div>
<div id="migrate" hidden><p>This board has the old two-slot layout: the images with Matter (2.1 MB) no longer fit
its 1.9 MB slots. The migration below puts the Tasmota safeboot image in front, one 2880 KB slot for ESPurna after it.
ESPurna then reboots into safeboot, which joins your Wi-Fi and downloads ESPurna from the URL. Settings stay.
Keep the power on for the next two minutes.</p>
<label>Tasmota safeboot image (tasmota32-safeboot.bin, <a href="https://github.com/arendst/Tasmota/releases/latest">Tasmota releases</a>)
<input type="file" id="file" accept=".bin"></label></div>
<div id="update" hidden><p>Updates go through safeboot: ESPurna reboots into it, safeboot downloads the image
from the URL into the ESPurna slot and starts it.</p></div>
<div id="form" hidden><label>ESPurna image URL (the <code>-ota.bin</code> one)<input type="text" id="url"></label>
<button id="go"></button></div><div id="log" hidden></div>
<script>
const $=id=>document.getElementById(id);let st;
function log(t,c){const l=$('log');l.hidden=false;l.className=c||'';l.textContent=t}
async function status(){const r=await fetch('safeboot/status',{cache:'no-store'});return r.json()}
async function back(t0,was){for(;;){await new Promise(r=>setTimeout(r,5000));
 const s=Math.round((Date.now()-t0)/1000);try{const n=await status();
 if(n.layout===was&&n.last){log('Stopped: '+n.last,'err');$('go').disabled=false;return}
 if(n.layout==='safeboot'&&n.running!=='safeboot'){log('Done after '+s+' s: ESPurna runs from '+n.running+'. Reload the WebUI.','ok');return}}
 catch(e){}log('Waiting for ESPurna ('+s+' s). Safeboot works in between: Wi-Fi, download, write. '
 +'If nothing after 5 minutes: safeboot could not join Wi-Fi, it opens its own access point (tasmota-...) at 192.168.4.1.')}}
function send(path,body){return new Promise((ok,no)=>{const x=new XMLHttpRequest();x.open('POST',path);
 x.upload.onprogress=e=>{if(e.lengthComputable)log('Uploading '+Math.round(e.loaded*100/e.total)+'%')};
 x.onload=()=>(x.status==200?ok:no)(x.responseText);x.onerror=()=>no('connection error');x.send(body)})}
$('go').onclick=async()=>{const url=$('url').value.trim();const f=new FormData();f.append('url',url);
 if(st.layout==='old'){const file=$('file').files[0];if(!file){log('Pick the safeboot image','err');return}f.append('file',file,file.name)}
 $('go').disabled=true;try{const r=await send(st.layout==='old'?'safeboot/migrate':'safeboot/update',f);log(r);back(Date.now(),st.layout)}
 catch(e){log(e,'err');$('go').disabled=false}};
status().then(s=>{st=s;$('url').value=s.url;
 $('info').textContent='Running: '+s.running+'. Layout: '+(s.layout==='old'?'two OTA slots (old)':s.layout)+'.';
 if(s.layout==='old'){$('migrate').hidden=false;$('go').textContent='Migrate'}else{$('update').hidden=false;$('go').textContent='Update from URL'}
 $('form').hidden=false;if(s.last){log('Last attempt: '+s.last,'err')}if(s.error){log(s.error,'err');$('go').disabled=true}}).catch(e=>log(e,'err'));
</script></body></html>)");

bool authenticated(AsyncWebServerRequest* request) {
    if (!webAuthenticate(request)) {
        request->requestAuthentication(systemHostname().c_str());
        return false;
    }

    return true;
}

void onPage(AsyncWebServerRequest* request) {
    if (authenticated(request)) {
        request->send(200, "text/html", Page);
    }
}

void onStatus(AsyncWebServerRequest* request) {
    if (!authenticated(request)) {
        return;
    }

    const auto* running = esp_ota_get_running_partition();
    const bool current = layout::current();

    String error;
    if (!current) {
        layout::possible(error);
    }

    String out;
    out.reserve(384);
    out += F("{\"layout\":\"");
    out += current ? F("safeboot") : (factory() ? F("other") : F("old"));
    out += F("\",\"running\":\"");
    out += running ? running->label : "?";
    out += F("\",\"url\":\"");
    out += otaUrl();
    out += F("\",\"error\":\"");
    out += error;
    out += F("\",\"last\":\"");
    out += layout::last_error;
    out += F("\"}");

    request->send(200, "application/json", out);
}

String formUrl(AsyncWebServerRequest* request) {
    const auto* param = request->getParam("url", true);
    return param ? param->value() : String();
}

void onUpdate(AsyncWebServerRequest* request) {
    if (!authenticated(request)) {
        return;
    }

    const auto url = formUrl(request);
    String error;
    if (!updateFromUrl(url, error)) {
        request->send(400, "text/plain", error);
        return;
    }

    setSetting("sbUrl", url);
    request->send(200, "text/plain", F("Restarting into safeboot"));
    prepareReset(CustomResetReason::Web);
}

void onMigrateUpload(AsyncWebServerRequest* request, String, size_t index, uint8_t* data, size_t len, bool final) {
    if (!webAuthenticate(request)) {
        return;
    }

    if (!index) {
        layout::upload = layout::Upload{};
        String error;
        if (!layout::possible(error)) {
            layout::upload.error = error;
            return;
        }

        if (!len || (data[0] != 0xE9)) {
            layout::upload.error = F("not an ESP32 image (no 0xE9 magic byte)");
            return;
        }

        layout::upload.part = layout::target();
    }

    if (!layout::upload.part || layout::upload.error.length() || layout::upload.done) {
        return;
    }

    if (!layout::uploadWrite(data, len)) {
        layout::upload.part = nullptr;
        return;
    }

    if (final && layout::uploadVerify()) {
        layout::upload.done = true;
    }
}

void onMigrate(AsyncWebServerRequest* request) {
    if (!authenticated(request)) {
        return;
    }

    const auto url = formUrl(request);
    if (!tasmota::validUrl(url)) {
        request->send(400, "text/plain", F("invalid URL (http:// or https://, up to 140 characters, no %)"));
        return;
    }

    if (!layout::upload.done) {
        request->send(400, "text/plain", layout::upload.error.length() ? layout::upload.error : String(F("no safeboot image received")));
        return;
    }

    layout::upload.done = false;
    setSetting("sbUrl", url);
    layout::pending_url = url;
    espurnaRegisterOnce(layout::migrate);

    request->send(200, "text/plain",
        F("Safeboot image written. Writing the partition table, then restarting into safeboot"));
}
#endif

#if TERMINAL_SUPPORT
PROGMEM_STRING(Safeboot, "SAFEBOOT");

void command_safeboot(::terminal::CommandContext&& ctx) {
    String error;
    if (!switchToSafeboot(error)) {
        terminalError(ctx, error);
        return;
    }

    ctx.output.print(F("Next boot: factory (safeboot) partition, restarting\n"));
    terminalOK(ctx);
    prepareReset(CustomResetReason::Terminal);
}

PROGMEM_STRING(Layout, "SAFEBOOT.STATUS");

void command_status(::terminal::CommandContext&& ctx) {
    const auto* running = esp_ota_get_running_partition();
    ctx.output.printf_P(PSTR("running: %s, factory (safeboot): %s, second OTA slot: %s, unstable boots: %u\n"),
        running ? running->label : "?",
        factory() ? "present" : "missing",
        otaPossible() ? "available" : "missing",
        count_at_boot);

    if (!layout::current()) {
        String error;
        ctx.output.printf_P(PSTR("old layout, migration over Wi-Fi (web /safeboot): %s\n"),
            layout::possible(error) ? "possible" : error.c_str());
    }

    terminalOK(ctx);
}

// NVS is 20 KB in this layout (and in min_spiffs.csv): ESPurna settings (one blob, rewritten whole, so it needs
// as much free space again), Wi-Fi, Matter (chip-*), the safeboot settings. What takes it, per namespace
PROGMEM_STRING(Nvs, "NVS");

void command_nvs(::terminal::CommandContext&& ctx) {
    nvs_stats_t stats;
    if (nvs_get_stats(nullptr, &stats) == ESP_OK) {
        ctx.output.printf_P(PSTR("entries: used %u, free %u, available %u, total %u, namespaces %u\n"),
            stats.used_entries, stats.free_entries, stats.available_entries, stats.total_entries,
            stats.namespace_count);
    }

    struct Space {
        String name;
        size_t keys;
        size_t entries;
    };

    std::vector<Space> spaces;
    nvs_iterator_t it { nullptr };
    auto result = nvs_entry_find(NVS_DEFAULT_PART_NAME, nullptr, NVS_TYPE_ANY, &it);
    while (result == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);

        size_t used { 0 };
        nvs_handle_t handle;
        if (nvs_open(info.namespace_name, NVS_READONLY, &handle) == ESP_OK) {
            nvs_get_used_entry_count(handle, &used);
            nvs_close(handle);
        }

        auto found = std::find_if(spaces.begin(), spaces.end(),
            [&](const Space& space) { return space.name == info.namespace_name; });
        if (found == spaces.end()) {
            spaces.push_back(Space{info.namespace_name, 1, used});
        } else {
            ++found->keys;
        }

        result = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);

    for (const auto& space : spaces) {
        ctx.output.printf_P(PSTR("%-16s keys %3u, entries %4u (~%u bytes)\n"),
            space.name.c_str(), space.keys, space.entries, space.entries * 32);
    }

    terminalOK(ctx);
}

PROGMEM_STRING(NvsErase, "NVS.ERASE");

void command_nvs_erase(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() != 2) {
        terminalError(ctx, F("NVS.ERASE <namespace>"));
        return;
    }

    nvs_handle_t handle;
    if (nvs_open(ctx.argv[1].c_str(), NVS_READWRITE, &handle) != ESP_OK) {
        terminalError(ctx, F("no such namespace"));
        return;
    }

    const auto result = nvs_erase_all(handle);
    const auto commit = nvs_commit(handle);
    nvs_close(handle);

    if ((result != ESP_OK) || (commit != ESP_OK)) {
        terminalError(ctx, String(esp_err_to_name((result != ESP_OK) ? result : commit)));
        return;
    }

    terminalOK(ctx);
}

static constexpr ::terminal::Command Commands[] PROGMEM {
    {Safeboot, command_safeboot},
    {Layout, command_status},
    {Nvs, command_nvs},
    {NvsErase, command_nvs_erase},
};
#endif

} // namespace
} // namespace safeboot
} // namespace espurna

void safebootSetup() {
    using namespace espurna::safeboot;

    guard();
    espurnaRegisterLoop(tick);

#if TERMINAL_SUPPORT
    espurna::terminal::add(Commands);
#endif
}

#if WEB_SUPPORT
// Only after webSetup(): the server does not exist before it (and never in the unstable mode)
void safebootWebSetup() {
    using namespace espurna::safeboot;

    auto& server = webServer();
    // a handler takes its sub-paths as well ("/safeboot" would answer "/safeboot/status"): specific ones first
    server.on("/safeboot/status", HTTP_GET, onStatus);
    server.on("/safeboot/update", HTTP_POST, onUpdate);
    server.on("/safeboot/migrate", HTTP_POST, onMigrate, onMigrateUpload);
    server.on("/safeboot", HTTP_GET, onPage);

    if (!otaPossible()) {
        server.rewrite("/upgrade", "/safeboot-noota");
        server.on("/safeboot-noota", HTTP_ANY, onNoOta, onNoOtaUpload);
        DEBUG_MSG_P(PSTR("[SAFEBOOT] No second OTA slot, /upgrade is disabled (use /safeboot)\n"));
    }
}
#endif

#endif // SAFEBOOT_SUPPORT
