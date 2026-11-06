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
#include <mbedtls/sha256.h>
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

    // Only into our own safeboot: it joins Wi-Fi with the ESPurna settings and points the next boot back to app0.
    // Anything else there may never come back (Tasmota's safeboot without its own settings boots with Wi-Fi off and
    // stays there: 10.0.5.66, 2026-11-05). An erased partition (merged image without safeboot) is refused as well
    esp_app_desc_t desc;
    if ((esp_ota_get_partition_description(part, &desc) != ESP_OK)
        || (std::strcmp(desc.project_name, "espurna-safeboot") != 0))
    {
        error = F("the factory partition does not hold the ESPurna safeboot image, install it first (/safeboot)");
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

// ESPurna's own safeboot image (code/safeboot-app) reads Wi-Fi, hostname, the admin password and webFwUrl straight
// from these settings. NVS "safeboot" passes the rest: "url" + "sha256" - the image to download into app0 on its next
// boot; "image" - ESPURNA_IMAGE, its release list shows the files of that image; "result" - written back by safeboot
namespace request {

constexpr size_t UrlMax { 240 };

bool write(const String& url, const String& sha256, String& error) {
    nvs_handle_t handle;
    if (nvs_open("safeboot", NVS_READWRITE, &handle) != ESP_OK) {
        error = F("cannot open NVS");
        return false;
    }

    auto result = nvs_set_str(handle, "url", url.c_str());
    if (result == ESP_OK) {
        result = sha256.length()
            ? nvs_set_str(handle, "sha256", sha256.c_str())
            : nvs_erase_key(handle, "sha256");
        if (result == ESP_ERR_NVS_NOT_FOUND) {
            result = ESP_OK;
        }
    }
    if (result == ESP_OK) {
        nvs_erase_key(handle, "result");
        result = nvs_commit(handle);
    }
    nvs_close(handle);

    if (result != ESP_OK) {
        error = String(F("cannot store the URL: ")) + esp_err_to_name(result);
        return false;
    }

    return true;
}

String read(const char* key) {
    String out;

    nvs_handle_t handle;
    if (nvs_open("safeboot", NVS_READONLY, &handle) == ESP_OK) {
        size_t size = 0;
        if ((nvs_get_str(handle, key, nullptr, &size) == ESP_OK) && (size > 1)) {
            std::unique_ptr<char[]> value(new (std::nothrow) char[size]);
            if (value && (nvs_get_str(handle, key, value.get(), &size) == ESP_OK)) {
                out = value.get();
            }
        }
        nvs_close(handle);
    }

    return out;
}

// once per image change, not on every boot. "app" - what the safeboot page shows for app0: the app description of
// an Arduino build names the prebuilt IDF libraries ("arduino-lib-builder"), not ESPurna
void image() {
    const String current(F(ESPURNA_IMAGE));

    const auto app = buildApp();
    String description;
    description += app.name.toString();
    description += ' ';
    description += app.version.toString();
    if (current.length()) {
        description += ' ';
        description += current;
    }
    description += F(", built ");
    description += app.build_time.toString();

    if ((current == read("image")) && (description == read("app"))) {
        return;
    }

    nvs_handle_t handle;
    if (nvs_open("safeboot", NVS_READWRITE, &handle) == ESP_OK) {
        if (current.length()) {
            nvs_set_str(handle, "image", current.c_str());
        } else {
            nvs_erase_key(handle, "image");
        }
        nvs_set_str(handle, "app", description.c_str());
        nvs_commit(handle);
        nvs_close(handle);
    }
}

// the safeboot image downloads http:// and https:// (ESP-IDF certificate bundle)
bool validUrl(const String& url) {
    return (url.length() > 10) && (url.length() <= UrlMax)
        && (url.startsWith(F("http://")) || url.startsWith(F("https://")));
}

// empty, or 64 lower case hex digits
bool validSha256(const String& value) {
    if (!value.length()) {
        return true;
    }

    if (value.length() != 64) {
        return false;
    }

    for (const auto c : value) {
        if (!(((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')))) {
            return false;
        }
    }

    return true;
}

} // namespace request

// SHA-256 of the first `size` bytes of a partition (the image file as it was written), hex
String partitionSha256(const esp_partition_t* part, size_t size) {
    if (!part || !size || (size > part->size)) {
        return String();
    }

    std::unique_ptr<uint8_t[]> buffer(new (std::nothrow) uint8_t[4096]);
    if (!buffer) {
        return String();
    }

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);

    bool ok = true;
    for (size_t offset = 0; ok && (offset < size); offset += 4096) {
        const size_t len = std::min<size_t>(4096, size - offset);
        ok = esp_partition_read(part, offset, buffer.get(), len) == ESP_OK;
        if (ok) {
            mbedtls_sha256_update(&ctx, buffer.get(), len);
        }
    }

    uint8_t digest[32];
    mbedtls_sha256_finish(&ctx, digest);
    mbedtls_sha256_free(&ctx);
    if (!ok) {
        return String();
    }

    String out;
    out.reserve(64);
    char hex[3];
    for (const auto value : digest) {
        snprintf(hex, sizeof(hex), "%02x", value);
        out += hex;
    }

    return out;
}

// From the two-slot layouts of the Arduino core (min_spiffs.csv, default.csv) to esp32_4mb_safeboot.csv over Wi-Fi.
// ESPurna has to run from the second slot: the safeboot image goes over the first one (0x10000, below the new app0),
// then the partition table at 0x8000 is replaced and otadata erased, so the next boot starts the factory partition.
// The safeboot image (code/safeboot-app) joins Wi-Fi with the ESPurna settings and downloads ESPurna into app0 from the
// stored URL, or waits for an upload in its own web UI. NVS (0x9000) and otadata (0xE000) keep their place
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
String pending_sha256;
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
        upload.error = F("not an ESP32 image that fits the safeboot partition");
        return false;
    }

    esp_app_desc_t desc;
    if ((esp_ota_get_partition_description(upload.part, &desc) != ESP_OK)
        || (std::strcmp(desc.project_name, "espurna-safeboot") != 0))
    {
        upload.error = F("not the ESPurna safeboot image (espurna-esp32-safeboot.bin)");
        return false;
    }

    return true;
}

// what a partition holds: "espurna-safeboot 1.0, built ..." or an empty string
String describe(const esp_partition_t* part) {
    esp_app_desc_t desc;
    if (!part || (esp_ota_get_partition_description(part, &desc) != ESP_OK)) {
        return String();
    }

    String out;
    out += desc.project_name;
    out += ' ';
    out += desc.version;
    out += F(", built ");
    out += desc.date;
    out += ' ';
    out += desc.time;
    return out;
}

// in loop(): nothing else touches the flash while the table is replaced
void migrate() {
    String error;
    if (pending_url.length() && !request::write(pending_url, pending_sha256, error)) {
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

    DEBUG_MSG_P(PSTR("[SAFEBOOT] Layout migrated, restarting into safeboot\n"));
    prepareReset(CustomResetReason::Web);
}

} // namespace layout

// Update through safeboot from an URL (safeboot layout): the same settings, factory as the next boot
bool updateFromUrl(const String& url, const String& sha256, String& error) {
    if (!layout::current()) {
        error = F("not the safeboot layout");
        return false;
    }

    if (!request::validUrl(url)) {
        error = F("invalid URL (http:// or https://, up to 240 characters)");
        return false;
    }

    if (!request::validSha256(sha256)) {
        error = F("invalid SHA-256 (64 lower case hex digits)");
        return false;
    }

    if (!request::write(url, sha256, error)) {
        return false;
    }

    return switchToSafeboot(error);
}

#if WEB_SUPPORT
PROGMEM_STRING(NoOtaMessage,
    "ERROR: this flash layout (safeboot) has no second OTA slot. "
    "Updates go through safeboot, see the /safeboot page\n");

void onNoOta(AsyncWebServerRequest* request) {
    request->send(400, "text/plain", NoOtaMessage);
}

void onNoOtaUpload(AsyncWebServerRequest*, String, size_t, uint8_t*, size_t, bool) {
}

PROGMEM_STRING(Page, R"(<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESPurna safeboot</title><style>
body{font-family:sans-serif;max-width:640px;margin:16px auto;padding:0 16px;line-height:1.4}
input[type=text]{width:100%;box-sizing:border-box}button{margin-top:8px;padding:6px 16px}
fieldset{margin-top:16px}.err{color:#b00}.ok{color:#070}#log{white-space:pre-wrap;background:#f4f4f4;padding:8px;margin-top:12px}
</style></head><body><h2>Flash layout and updates</h2><div id="info">...</div>
<div id="old" hidden><fieldset><legend>Migrate to the safeboot layout</legend><p>This board has the old two-slot layout:
the images with Matter (2.1 MB) no longer fit its 1.9 MB slots. The migration puts the ESPurna safeboot image in front
and one 2880 KB slot for ESPurna after it, then restarts into safeboot. With an URL safeboot downloads ESPurna by itself,
without one it waits for an upload in its own web UI (same address, user admin, the admin password). Settings stay.
Keep the power on until ESPurna is back.</p>
<label>ESPurna safeboot image (espurna-esp32-safeboot.bin)<input type="file" id="mfile" accept=".bin"></label>
<label>ESPurna image URL, optional (http:// or https://, the -ota.bin one)<input type="text" id="murl"></label>
<button id="migrate">Migrate</button></fieldset></div>
<div id="new" hidden><fieldset><legend>Update ESPurna</legend><p>ESPurna restarts into safeboot, which writes the new
image into the ESPurna slot and starts it.</p>
<label>From an URL (http:// or https://, the -ota.bin one)<input type="text" id="url"></label><button id="byurl">Update from URL</button>
<p>Or with a file: <button id="byfile">Restart into safeboot</button> and upload the image there.</p></fieldset>
<fieldset><legend>Install or replace safeboot</legend><label>ESPurna safeboot image (espurna-esp32-safeboot.bin)
<input type="file" id="sfile" accept=".bin"></label><button id="install">Install</button></fieldset></div>
<div id="log" hidden></div>
<script>
const $=id=>document.getElementById(id);let st;
function log(t,c){const l=$('log');l.hidden=false;l.className=c||'';l.textContent=t}
async function status(){const r=await fetch('safeboot/status',{cache:'no-store'});if(!r.ok)throw r.status;return r.json()}
const wait=ms=>new Promise(r=>setTimeout(r,ms));
async function espurna(t0,was){for(;;){await wait(5000);const s=Math.round((Date.now()-t0)/1000);
 try{const n=await status();if(n.layout===was&&n.last&&was==='old'){log('Stopped: '+n.last,'err');return}
 if(n.layout==='safeboot'&&n.running!=='safeboot'){log('Done after '+s+' s, ESPurna is back. Reload the WebUI.','ok');return}}
 catch(e){}log('Waiting for ESPurna ('+s+' s): safeboot joins Wi-Fi, downloads and writes the image. If nothing after '
 +'5 minutes, open this address again (safeboot UI) or join its access point <hostname>-SAFEBOOT, 192.168.4.1.')}}
async function safeboot(t0){for(;;){await wait(3000);const s=Math.round((Date.now()-t0)/1000);
 try{const r=await fetch('/',{cache:'no-store'});if(r.status==401&&(r.headers.get('WWW-Authenticate')||'').startsWith('Basic')){location.href='/';return}}
 catch(e){}log('Restarting into safeboot ('+s+' s)...')}}
function send(path,body){return new Promise((ok,no)=>{const x=new XMLHttpRequest();x.open('POST',path);
 x.upload.onprogress=e=>{if(e.lengthComputable)log('Uploading '+Math.round(e.loaded*100/e.total)+'%')};
 x.onload=()=>(x.status==200?ok:no)(x.responseText);x.onerror=()=>no('connection error');x.send(body)})}
function form(fields){const f=new FormData();for(const k in fields)f.append(k,fields[k]);return f}
$('migrate').onclick=async()=>{const file=$('mfile').files[0];if(!file){log('Pick the safeboot image','err');return}
 const f=form({url:$('murl').value.trim()});f.append('file',file,file.name);
 try{log(await send('safeboot/migrate',f));if($('murl').value.trim())espurna(Date.now(),'old');else safeboot(Date.now())}catch(e){log(e,'err')}};
$('byurl').onclick=async()=>{try{log(await send('safeboot/update',form({url:$('url').value.trim()})));espurna(Date.now(),'safeboot')}catch(e){log(e,'err')}};
$('byfile').onclick=async()=>{try{log(await send('safeboot/switch',form({})));safeboot(Date.now())}catch(e){log(e,'err')}};
$('install').onclick=async()=>{const file=$('sfile').files[0];if(!file){log('Pick the safeboot image','err');return}
 const f=new FormData();f.append('file',file,file.name);try{log(await send('safeboot/install',f),'ok');show(await status())}catch(e){log(e,'err')}};
function show(s){st=s;$('url').value=s.url;$('murl').value=s.url;
 $('info').innerHTML='Running: '+s.running+'<br>Layout: '+(s.layout==='old'?'two OTA slots (old)':s.layout)
  +(s.layout==='safeboot'?'<br>Safeboot: '+(s.safeboot||'missing'):'');
 $('old').hidden=s.layout!=='old';$('new').hidden=s.layout!=='safeboot';
 if(s.last)log('Last attempt: '+s.last,'err');if(s.error){log(s.error,'err');$('migrate').disabled=true}}
status().then(show).catch(e=>log(e,'err'));
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
    out.reserve(512);
    out += F("{\"layout\":\"");
    out += current ? F("safeboot") : (factory() ? F("other") : F("old"));
    out += F("\",\"running\":\"");
    out += running ? running->label : "?";
    out += F("\",\"safeboot\":\"");
    out += current ? layout::describe(factory()) : String();
    out += F("\",\"url\":\"");
    out += getSetting("sbUrl");
    out += F("\",\"error\":\"");
    out += error;
    out += F("\",\"last\":\"");
    out += layout::last_error;
    // what safeboot did with the last URL ("failed: ..." when it gave up)
    out += F("\",\"result\":\"");
    out += request::read("result");
    // ?size=<bytes of espurna-esp32-safeboot.bin>: SHA-256 of the installed one, to compare with the release list
    const auto* size = request->getParam("size");
    if (current && size) {
        out += F("\",\"sha256\":\"");
        out += partitionSha256(factory(), size->value().toInt());
    }
    out += F("\"}");

    request->send(200, "application/json", out);
}

String formValue(AsyncWebServerRequest* request, const char* name) {
    const auto* param = request->getParam(name, true);
    return param ? param->value() : String();
}

String formUrl(AsyncWebServerRequest* request) {
    return formValue(request, "url");
}

void onUpdate(AsyncWebServerRequest* request) {
    if (!authenticated(request)) {
        return;
    }

    const auto url = formUrl(request);
    String error;
    if (!updateFromUrl(url, formValue(request, "sha256"), error)) {
        request->send(400, "text/plain", error);
        return;
    }

    setSetting("sbUrl", url);
    request->send(200, "text/plain", F("Restarting into safeboot"));
    prepareReset(CustomResetReason::Web);
}

void onSwitch(AsyncWebServerRequest* request) {
    if (!authenticated(request)) {
        return;
    }

    String error;
    if (!switchToSafeboot(error)) {
        request->send(400, "text/plain", error);
        return;
    }

    request->send(200, "text/plain", F("Restarting into safeboot"));
    prepareReset(CustomResetReason::Web);
}

// One handler for both images that go into the safeboot slot: the migration (old layout, into the first OTA slot
// that becomes the factory partition) and install / replace (safeboot layout, into the factory partition itself)
void onSafebootUpload(AsyncWebServerRequest* request, String, size_t index, uint8_t* data, size_t len, bool final) {
    if (!webAuthenticate(request)) {
        return;
    }

    if (!index) {
        layout::upload = layout::Upload{};
        String error;
        const bool current = layout::current();
        if (!current && !layout::possible(error)) {
            layout::upload.error = error;
            return;
        }

        if (!len || (data[0] != 0xE9)) {
            layout::upload.error = F("not an ESP32 image (no 0xE9 magic byte)");
            return;
        }

        layout::upload.part = current ? factory() : layout::target();
        if (!layout::upload.part) {
            layout::upload.error = F("no partition for the safeboot image");
            return;
        }
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

bool uploadDone(AsyncWebServerRequest* request) {
    if (!layout::upload.done) {
        request->send(400, "text/plain",
            layout::upload.error.length() ? layout::upload.error : String(F("no safeboot image received")));
        return false;
    }

    layout::upload.done = false;
    return true;
}

void onInstall(AsyncWebServerRequest* request) {
    if (!authenticated(request)) {
        return;
    }

    if (!layout::current()) {
        request->send(400, "text/plain", F("not the safeboot layout, migrate first"));
        return;
    }

    if (uploadDone(request)) {
        request->send(200, "text/plain", String(F("Safeboot installed: ")) + layout::describe(factory()));
    }
}

void onMigrate(AsyncWebServerRequest* request) {
    if (!authenticated(request)) {
        return;
    }

    const auto url = formUrl(request);
    if (url.length() && !request::validUrl(url)) {
        request->send(400, "text/plain", F("invalid URL (http:// or https://, up to 240 characters)"));
        return;
    }

    const auto sha256 = formValue(request, "sha256");
    if (!request::validSha256(sha256)) {
        request->send(400, "text/plain", F("invalid SHA-256 (64 lower case hex digits)"));
        return;
    }

    if (!uploadDone(request)) {
        return;
    }

    if (url.length()) {
        setSetting("sbUrl", url);
    }
    layout::pending_url = url;
    layout::pending_sha256 = sha256;
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
    request::image();
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
    server.on("/safeboot/switch", HTTP_POST, onSwitch);
    server.on("/safeboot/migrate", HTTP_POST, onMigrate, onSafebootUpload);
    server.on("/safeboot/install", HTTP_POST, onInstall, onSafebootUpload);
    server.on("/safeboot", HTTP_GET, onPage);

    if (!otaPossible()) {
        server.rewrite("/upgrade", "/safeboot-noota");
        server.on("/safeboot-noota", HTTP_ANY, onNoOta, onNoOtaUpload);
        DEBUG_MSG_P(PSTR("[SAFEBOOT] No second OTA slot, /upgrade is disabled (use /safeboot)\n"));
    }
}
#endif

#endif // SAFEBOOT_SUPPORT
