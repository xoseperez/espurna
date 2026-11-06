/*

ESPurna safeboot (ESP32, ESP-IDF)

Small recovery / update image for the factory partition of esp32_4mb_safeboot.csv (0x10000, 832 KB). ESPurna lives in
the single big OTA slot (app0) and cannot overwrite itself, so updates go through this image:
- Wi-Fi (ssid0/pass0, ssid1/pass1, static ip0/gw0/mask0/dns0), hostname and adminPass are read straight from the
  ESPurna settings (NVS "eeprom"/"eeprom", Embedis key-value format written right to left); no settings of its own
- web UI on port 80, user admin + the ESPurna admin password: pick a release (the list comes from firmware.json on
  GitHub Pages, read by the browser), upload an image into app0, download one from an URL, boot ESPurna
- update from an URL without the web UI: ESPurna stores it in NVS "safeboot"/"url" (and its SHA-256 in "sha256")
  before switching here, the image is downloaded into app0 as soon as Wi-Fi is up and ESPurna boots. http:// and
  https:// (the ESP-IDF certificate bundle); the result goes to NVS "safeboot"/"result" for ESPurna to show
- no Wi-Fi in 30 s: own access point <hostname>-SAFEBOOT (WPA2, admin password) at 192.168.4.1
- the next boot is set back to app0 right at start (when it holds a valid image): a crash or a power cut here always
  returns to ESPurna; nothing done for 15 minutes: back to ESPurna as well

*/

#include <esp_crt_bundle.h>
#include <esp_event.h>
#include <esp_http_client.h>
#include <esp_http_server.h>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <lwip/inet.h>
#include <mbedtls/base64.h>
#include <mbedtls/sha256.h>
#include <nvs.h>
#include <nvs_flash.h>

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

namespace {

constexpr char DefaultPassword[] = "fibonacci";
constexpr char DefaultManifest[] = "https://igorufox.github.io/espurna/firmware.json";
constexpr int DownloadAttempts { 3 };
constexpr int ConnectTimeoutMs { 30000 };
constexpr int64_t IdleTimeoutUs { 15LL * 60 * 1000 * 1000 };

// --- ESPurna settings ------------------------------------------------------------------------------------------

struct Settings {
    std::string ssid[2];
    std::string pass[2];
    std::string ip;
    std::string gw;
    std::string mask;
    std::string dns;
    std::string hostname;
    std::string password;
    std::string manifest;
};

Settings settings;

// Embedis: records from the end of the blob towards its beginning. An item is its data followed by the data length
// (2 bytes, big endian, the low byte last); a record is the key item, then the value item. 0xffff is free space
struct Embedis {
    const uint8_t* data;
    size_t size;

    bool item(size_t& pos, const uint8_t*& out, size_t& len) const {
        if (pos < 2) {
            return false;
        }

        const size_t value = data[pos - 1] | (data[pos - 2] << 8);
        if (value == 0xffff) {
            return false;
        }

        pos -= 2;
        if (pos < value) {
            return false;
        }

        pos -= value;
        out = data + pos;
        len = value;
        return true;
    }

    std::string get(const char* key) const {
        const size_t key_len = strlen(key);
        size_t pos = size;
        for (;;) {
            const uint8_t* k;
            size_t k_len;
            if (!item(pos, k, k_len) || !k_len) {
                break;
            }

            const uint8_t* v;
            size_t v_len;
            if (!item(pos, v, v_len)) {
                break;
            }

            if ((k_len == key_len) && (memcmp(k, key, key_len) == 0)) {
                return std::string(reinterpret_cast<const char*>(v), v_len);
            }
        }

        return std::string();
    }
};

void readSettings() {
    nvs_handle_t handle;
    if (nvs_open("eeprom", NVS_READONLY, &handle) == ESP_OK) {
        size_t size = 0;
        if ((nvs_get_blob(handle, "eeprom", nullptr, &size) == ESP_OK) && size) {
            std::unique_ptr<uint8_t[]> blob(new (std::nothrow) uint8_t[size]);
            if (blob && (nvs_get_blob(handle, "eeprom", blob.get(), &size) == ESP_OK)) {
                const Embedis kv{blob.get(), size};
                settings.ssid[0] = kv.get("ssid0");
                settings.pass[0] = kv.get("pass0");
                settings.ssid[1] = kv.get("ssid1");
                settings.pass[1] = kv.get("pass1");
                settings.ip = kv.get("ip0");
                settings.gw = kv.get("gw0");
                settings.mask = kv.get("mask0");
                settings.dns = kv.get("dns0");
                settings.hostname = kv.get("hostname");
                settings.password = kv.get("adminPass");
                settings.manifest = kv.get("webFwUrl");
            }
        }
        nvs_close(handle);
    }

    if (settings.hostname.empty()) {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        char out[24];
        snprintf(out, sizeof(out), "ESPURNA-%02X%02X%02X", mac[3], mac[4], mac[5]);
        settings.hostname = out;
    }

    if (settings.password.empty()) {
        settings.password = DefaultPassword;
    }

    if (settings.manifest.empty()) {
        settings.manifest = DefaultManifest;
    }
}

std::string nvsString(nvs_handle_t handle, const char* key) {
    std::string out;

    size_t size = 0;
    if ((nvs_get_str(handle, key, nullptr, &size) == ESP_OK) && (size > 1)) {
        std::unique_ptr<char[]> value(new (std::nothrow) char[size]);
        if (value && (nvs_get_str(handle, key, value.get(), &size) == ESP_OK)) {
            out = value.get();
        }
    }

    return out;
}

// what ESPurna asked for before switching here: the image URL and its SHA-256 (hex, optional)
struct Request {
    std::string url;
    std::string sha256;
};

Request takeRequest() {
    Request out;

    nvs_handle_t handle;
    if (nvs_open("safeboot", NVS_READWRITE, &handle) == ESP_OK) {
        out.url = nvsString(handle, "url");
        out.sha256 = nvsString(handle, "sha256");
        if (!out.url.empty()) {
            // taken once: a broken URL must not keep the board here over the next boots
            nvs_erase_key(handle, "url");
            nvs_erase_key(handle, "sha256");
            nvs_commit(handle);
        }
        nvs_close(handle);
    }

    return out;
}

// the image ESPurna in app0 was built as (ESPURNA_IMAGE, written by ESPurna): the release list shows its files
std::string runningImage() {
    std::string out;

    nvs_handle_t handle;
    if (nvs_open("safeboot", NVS_READONLY, &handle) == ESP_OK) {
        out = nvsString(handle, "image");
        nvs_close(handle);
    }

    return out;
}

void storeResult(const std::string& result) {
    nvs_handle_t handle;
    if (nvs_open("safeboot", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_str(handle, "result", result.c_str());
        nvs_commit(handle);
        nvs_close(handle);
    }
}

// --- app0 -------------------------------------------------------------------------------------------------------

const esp_partition_t* app() {
    return esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr);
}

// ESPurna writes NVS "safeboot"/"app" at boot: the app description of an Arduino build names the prebuilt IDF
// libraries ("arduino-lib-builder"), not ESPurna. Erased whenever safeboot writes app0, until ESPurna boots again
std::string appInfo() {
    esp_app_desc_t desc;
    const auto* part = app();
    if (!part || (esp_ota_get_partition_description(part, &desc) != ESP_OK)) {
        return "no valid image";
    }

    std::string written;
    nvs_handle_t handle;
    if (nvs_open("safeboot", NVS_READONLY, &handle) == ESP_OK) {
        written = nvsString(handle, "app");
        nvs_close(handle);
    }
    if (!written.empty()) {
        return written;
    }

    return std::string(desc.project_name) + " " + desc.version + ", built " + desc.date + " " + desc.time;
}

void forgetAppInfo() {
    nvs_handle_t handle;
    if (nvs_open("safeboot", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_erase_key(handle, "app");
        nvs_commit(handle);
        nvs_close(handle);
    }
}

bool bootApp(std::string& error) {
    esp_app_desc_t desc;
    const auto* part = app();
    if (!part || (esp_ota_get_partition_description(part, &desc) != ESP_OK)) {
        error = "no valid image in app0";
        return false;
    }

    const auto result = esp_ota_set_boot_partition(part);
    if (result != ESP_OK) {
        error = esp_err_to_name(result);
        return false;
    }

    return true;
}

// One image write into app0: begin with the first chunk (checks the 0xE9 magic), then end() verifies the image.
// The SHA-256 of everything written is kept for the check against the release list
struct Writer {
    esp_ota_handle_t handle { 0 };
    const esp_partition_t* part { nullptr };
    size_t written { 0 };
    size_t expected { 0 };  // Content-Length, 0 - not known (app0 is erased whole then)
    std::string error;
    mbedtls_sha256_context sha;

    Writer() {
        mbedtls_sha256_init(&sha);
        mbedtls_sha256_starts(&sha, 0);
    }

    ~Writer() {
        mbedtls_sha256_free(&sha);
    }

    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;

    std::string digest() {
        uint8_t out[32];
        mbedtls_sha256_finish(&sha, out);

        std::string hex;
        char byte[3];
        for (const auto value : out) {
            snprintf(byte, sizeof(byte), "%02x", value);
            hex += byte;
        }

        return hex;
    }

    bool write(const char* data, size_t len) {
        if (!part) {
            if (!len || (static_cast<uint8_t>(data[0]) != 0xE9)) {
                error = "not an ESP32 image (no 0xE9 magic byte)";
                return false;
            }

            part = app();
            if (part && expected && (expected > part->size)) {
                error = "the image is bigger than app0";
                part = nullptr;
                return false;
            }

            // erases only what the image takes when its size is known
            const auto result = part
                ? esp_ota_begin(part, expected ? expected : OTA_SIZE_UNKNOWN, &handle) : ESP_ERR_NOT_FOUND;
            if (result != ESP_OK) {
                error = std::string("begin: ") + esp_err_to_name(result);
                part = nullptr;
                return false;
            }
        }

        const auto result = esp_ota_write(handle, data, len);
        if (result != ESP_OK) {
            error = std::string("write: ") + esp_err_to_name(result);
            return false;
        }

        mbedtls_sha256_update(&sha, reinterpret_cast<const unsigned char*>(data), len);
        written += len;
        return true;
    }

    bool finish() {
        if (!part) {
            if (error.empty()) {
                error = "no data";
            }
            return false;
        }

        auto result = esp_ota_end(handle);
        part = nullptr;
        if (result == ESP_OK) {
            forgetAppInfo();
            result = esp_ota_set_boot_partition(app());
        }

        if (result != ESP_OK) {
            error = std::string("image check failed: ") + esp_err_to_name(result);
            return false;
        }

        return true;
    }

    void abort() {
        if (part) {
            esp_ota_abort(handle);
            part = nullptr;
        }
    }
};

std::string last_result;

bool validHex(const std::string& value) {
    if (value.size() != 64) {
        return false;
    }

    for (const auto c : value) {
        if (!(((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')))) {
            return false;
        }
    }

    return true;
}

// http:// or https:// (servers checked against the ESP-IDF certificate bundle). sha256: hex, lower case, or empty
bool download(const std::string& url, const std::string& sha256) {
    const bool https = url.rfind("https://", 0) == 0;
    if (!https && (url.rfind("http://", 0) != 0)) {
        last_result = "only http:// and https:// URLs";
        return false;
    }

    if (!sha256.empty() && !validHex(sha256)) {
        last_result = "the SHA-256 is not 64 lower case hex digits";
        return false;
    }

    esp_http_client_config_t config {};
    config.url = url.c_str();
    config.timeout_ms = 15000;
    config.buffer_size = 2048;
    config.max_redirection_count = 5;
    if (https) {
        config.crt_bundle_attach = esp_crt_bundle_attach;
    }

    auto* client = esp_http_client_init(&config);
    if (!client) {
        last_result = "invalid URL";
        return false;
    }

    Writer writer;
    bool ok = false;
    if (esp_http_client_open(client, 0) == ESP_OK) {
        const auto length = esp_http_client_fetch_headers(client);
        const int status = esp_http_client_get_status_code(client);
        if (status == 200) {
            writer.expected = (length > 0) ? static_cast<size_t>(length) : 0;
            std::unique_ptr<char[]> buffer(new char[4096]);
            for (;;) {
                const int len = esp_http_client_read(client, buffer.get(), 4096);
                if (len < 0) {
                    writer.error = "read error";
                    break;
                }
                if (!len) {
                    if (writer.expected && (writer.written != writer.expected)) {
                        writer.error = "connection closed after " + std::to_string(writer.written) + " of "
                            + std::to_string(writer.expected) + " bytes";
                        break;
                    }
                    if (!sha256.empty() && writer.part && (writer.digest() != sha256)) {
                        writer.error = "SHA-256 mismatch, not the file of the release list";
                        break;
                    }
                    ok = writer.finish();
                    break;
                }
                if (!writer.write(buffer.get(), len)) {
                    break;
                }
            }
        } else {
            writer.error = "HTTP " + std::to_string(status);
        }
    } else {
        writer.error = https ? "cannot connect (or the server certificate is not trusted)" : "cannot connect";
    }

    esp_http_client_cleanup(client);
    if (!ok) {
        writer.abort();
        last_result = writer.error;
        return false;
    }

    last_result = "downloaded " + std::to_string(writer.written) + " bytes"
        + (sha256.empty() ? "" : ", SHA-256 checked");
    return true;
}

// --- Wi-Fi ------------------------------------------------------------------------------------------------------

EventGroupHandle_t events;
constexpr EventBits_t Connected = BIT0;
esp_netif_t* netif_sta { nullptr };
std::string wifi_result;

void onWifi(void*, esp_event_base_t base, int32_t id, void* data) {
    if ((base == WIFI_EVENT) && (id == WIFI_EVENT_STA_DISCONNECTED)) {
        xEventGroupClearBits(events, Connected);
        const auto* event = static_cast<const wifi_event_sta_disconnected_t*>(data);
        wifi_result = "disconnected, reason " + std::to_string(event->reason);
    } else if ((base == IP_EVENT) && (id == IP_EVENT_STA_GOT_IP)) {
        xEventGroupSetBits(events, Connected);
    }
}

bool connect() {
    netif_sta = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(netif_sta, settings.hostname.c_str());

    if (!settings.ip.empty()) {
        esp_netif_ip_info_t info {};
        if (inet_pton(AF_INET, settings.ip.c_str(), &info.ip)
            && inet_pton(AF_INET, settings.gw.c_str(), &info.gw)
            && inet_pton(AF_INET, settings.mask.c_str(), &info.netmask))
        {
            esp_netif_dhcpc_stop(netif_sta);
            esp_netif_set_ip_info(netif_sta, &info);
            esp_netif_dns_info_t dns {};
            dns.ip.type = ESP_IPADDR_TYPE_V4;
            if (!inet_pton(AF_INET, settings.dns.c_str(), &dns.ip.u_addr.ip4)) {
                dns.ip.u_addr.ip4.addr = info.gw.addr;
            }
            esp_netif_set_dns_info(netif_sta, ESP_NETIF_DNS_MAIN, &dns);
        }
    }

    // same as ESPurna (wifi_esp32.cpp): EU, channels 1..13 scanned actively. The default "01" leaves 12 and 13
    // out, the APs here are on 13
    const wifi_country_t country { "EU", 1, 13, 20, WIFI_COUNTRY_POLICY_AUTO };
    esp_wifi_set_country(&country);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    for (size_t index = 0; index < 2; ++index) {
        if (settings.ssid[index].empty()) {
            continue;
        }

        wifi_config_t config {};
        strncpy(reinterpret_cast<char*>(config.sta.ssid), settings.ssid[index].c_str(), sizeof(config.sta.ssid));
        strncpy(reinterpret_cast<char*>(config.sta.password), settings.pass[index].c_str(),
            sizeof(config.sta.password));
        config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
        config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
        config.sta.pmf_cfg.capable = true;
        config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
        esp_wifi_set_config(WIFI_IF_STA, &config);
        esp_wifi_connect();

        const auto bits = xEventGroupWaitBits(events, Connected, pdFALSE, pdTRUE,
            pdMS_TO_TICKS(ConnectTimeoutMs));
        if (bits & Connected) {
            wifi_result = "connected to " + settings.ssid[index];
            return true;
        }

        esp_wifi_disconnect();
    }

    return false;
}

void accessPoint() {
    esp_netif_create_default_wifi_ap();

    wifi_config_t config {};
    const std::string ssid = settings.hostname + "-SAFEBOOT";
    strncpy(reinterpret_cast<char*>(config.ap.ssid), ssid.c_str(), sizeof(config.ap.ssid));
    config.ap.ssid_len = ssid.size();
    // WPA2 needs 8..63 characters, ESPurna enforces the same for adminPass
    const auto& password = (settings.password.size() >= 8) ? settings.password : std::string(DefaultPassword);
    strncpy(reinterpret_cast<char*>(config.ap.password), password.c_str(), sizeof(config.ap.password));
    config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    config.ap.max_connection = 2;
    config.ap.channel = 1;

    esp_wifi_stop();
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &config);
    esp_wifi_start();
}

// --- web --------------------------------------------------------------------------------------------------------

int64_t last_activity { 0 };
bool restart { false };
std::string auth_header;

bool authenticated(httpd_req_t* req) {
    last_activity = esp_timer_get_time();

    char value[160];
    if ((httpd_req_get_hdr_value_str(req, "Authorization", value, sizeof(value)) == ESP_OK)
        && (auth_header == value))
    {
        return true;
    }

    const std::string realm = "Basic realm=\"" + settings.hostname + " safeboot\"";
    httpd_resp_set_status(req, "401 Unauthorized");
    httpd_resp_set_hdr(req, "WWW-Authenticate", realm.c_str());
    httpd_resp_send(req, "Authentication required", HTTPD_RESP_USE_STRLEN);
    return false;
}

void send(httpd_req_t* req, const std::string& body, const char* status = "200 OK") {
    std::string out;
    out.reserve(2048 + body.size());
    out += "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" "
           "content=\"width=device-width,initial-scale=1\"><title>";
    out += settings.hostname;
    out += " safeboot</title><style>body{font-family:sans-serif;max-width:640px;margin:16px auto;padding:0 16px}"
           "button{margin-top:8px;padding:6px 16px}input[type=text]{width:100%;box-sizing:border-box}"
           "fieldset{margin-top:16px}#log{margin-top:12px}</style></head><body><h2>";
    out += settings.hostname;
    out += " - safeboot</h2>";
    out += body;
    out += "</body></html>";

    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, out.data(), out.size());
}

// a JS string literal, safe inside <script>
std::string jsString(const std::string& value) {
    std::string out = "\"";
    for (const auto c : value) {
        if ((c == '"') || (c == '\\')) {
            out += '\\';
            out += c;
        } else if ((c == '<') || (c == '>') || (static_cast<unsigned char>(c) < 0x20)) {
            char escaped[8];
            snprintf(escaped, sizeof(escaped), "\\x%02x", static_cast<unsigned char>(c));
            out += escaped;
        } else {
            out += c;
        }
    }
    out += '"';
    return out;
}

// The image goes up as the raw request body (no multipart), the page script sends the picked file with fetch().
// Releases: the browser reads firmware.json (GitHub Pages sends it with Access-Control-Allow-Origin: *) and lists the
// -ota.bin files of the ESPurna image in app0 (all ESP32 images when it is not known); the board downloads the picked
// one itself (/url) and checks its SHA-256
constexpr char PageScript[] =
    "const $=i=>document.getElementById(i);"
    // the answers are whole pages: their first paragraph is the result
    "async function msg(r){const d=new DOMParser().parseFromString(await r.text(),'text/html');"
    "const p=d.querySelector('p');return(p||d.body).textContent.trim()}"
    "$('up').onclick=async()=>{const f=$('file').files[0];"
    "const l=$('log');if(!f){l.textContent='Pick a file';return}l.textContent='Uploading...';"
    "try{const r=await fetch('/upload',{method:'POST',body:f});l.textContent=await msg(r);"
    "if(r.ok)setTimeout(()=>location.href='/',30000)}catch(e){l.textContent=e}};"
    "async function get(u,h){const l=$('rlog');l.textContent='Downloading '+u+' (about a minute)...';"
    "for(const b of document.querySelectorAll('#rel button'))b.disabled=true;"
    "try{const r=await fetch('/url',{method:'POST',body:new URLSearchParams({url:u,sha256:h||''})});"
    "l.textContent=await msg(r);if(r.ok)setTimeout(()=>location.href='/',30000)}"
    "catch(e){l.textContent=e}}"
    "fetch(M,{cache:'no-store'}).then(r=>r.json()).then(m=>{const d=$('rel');d.textContent='';"
    "for(const r of m.releases||[]){const im=r.images||{};"
    "for(const n of Object.keys(im)){if(!n.startsWith('esp32')||(I&&n!==I))continue;"
    "for(const f of im[n]){if(f.kind!=='ota')continue;const b=document.createElement('button');"
    "b.textContent=r.tag+': '+f.name+' ('+Math.round(f.size/1024)+' KB)';"
    "b.onclick=()=>{if(confirm('Install '+f.name+' of '+r.tag+'?'))get(new URL(f.url,M).href,f.sha256)};"
    "d.append(b,document.createElement('br'))}}}"
    "if(!d.childNodes.length)d.textContent='No ESP32 image'+(I?' '+I:'')+' in '+M})"
    ".catch(e=>$('rel').textContent='Cannot read '+M+': '+e);";

esp_err_t onRoot(httpd_req_t* req) {
    if (!authenticated(req)) {
        return ESP_OK;
    }

    const auto image = runningImage();
    std::string body = "<p>ESPurna in app0: " + appInfo() + "</p><p>Wi-Fi: " + wifi_result + "</p>";
    if (!last_result.empty()) {
        body += "<p>Last update: " + last_result + "</p>";
    }
    body += "<fieldset><legend>Releases" + (image.empty() ? std::string() : (" of " + image))
          + "</legend><div id=\"rel\">Reading the release list...</div><div id=\"rlog\"></div></fieldset>"
            "<fieldset><legend>Upload an image</legend><input type=\"file\" id=\"file\" accept=\".bin\"><br>"
            "<button id=\"up\">Upload and boot</button><div id=\"log\"></div></fieldset>"
            "<fieldset><legend>Download an image</legend><form method=\"post\" action=\"/url\">"
            "<input type=\"text\" name=\"url\" placeholder=\"https://.../espurna-esp32-matter-ota.bin\">"
            "<br><button>Download and boot</button></form></fieldset>"
            "<form method=\"post\" action=\"/boot\"><button>Boot ESPurna</button></form>";
    body += "<script>const M=" + jsString(settings.manifest) + ",I=" + jsString(image) + ";";
    body += PageScript;
    body += "</script>";
    send(req, body);
    return ESP_OK;
}

esp_err_t onBoot(httpd_req_t* req) {
    if (!authenticated(req)) {
        return ESP_OK;
    }

    std::string error;
    if (!bootApp(error)) {
        send(req, "<p>" + error + "</p><p><a href=\"/\">Back</a></p>", "400 Bad Request");
        return ESP_OK;
    }

    send(req, "<p>Booting ESPurna</p>");
    restart = true;
    return ESP_OK;
}

std::string formBody(httpd_req_t* req) {
    char body[512] {};
    size_t got = 0;
    while ((got < (sizeof(body) - 1)) && (got < req->content_len)) {
        const int len = httpd_req_recv(req, body + got, sizeof(body) - 1 - got);
        if (len == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }
        if (len <= 0) {
            break;
        }
        got += len;
    }

    return std::string(body, got);
}

// application/x-www-form-urlencoded "url=...&sha256=..."
std::string formValue(const std::string& body, const char* key) {
    char encoded[400] {};
    if (httpd_query_key_value(body.c_str(), key, encoded, sizeof(encoded)) != ESP_OK) {
        return std::string();
    }

    std::string out;
    for (size_t i = 0; encoded[i]; ++i) {
        if ((encoded[i] == '%') && encoded[i + 1] && encoded[i + 2]) {
            const char hex[3] { encoded[i + 1], encoded[i + 2], 0 };
            out += static_cast<char>(strtol(hex, nullptr, 16));
            i += 2;
        } else if (encoded[i] == '+') {
            out += ' ';
        } else {
            out += encoded[i];
        }
    }

    return out;
}

esp_err_t onUrl(httpd_req_t* req) {
    if (!authenticated(req)) {
        return ESP_OK;
    }

    const auto body = formBody(req);
    const bool ok = download(formValue(body, "url"), formValue(body, "sha256"));
    storeResult(last_result);
    if (!ok) {
        send(req, "<p>" + last_result + "</p><p><a href=\"/\">Back</a></p>", "400 Bad Request");
        return ESP_OK;
    }

    send(req, "<p>" + last_result + ", booting ESPurna</p>");
    restart = true;
    return ESP_OK;
}

esp_err_t onUpload(httpd_req_t* req) {
    if (!authenticated(req)) {
        return ESP_OK;
    }

    Writer writer;
    std::unique_ptr<char[]> buffer(new char[4096]);
    size_t remaining = req->content_len;
    bool ok = remaining > 0;
    while (ok && remaining) {
        const int len = httpd_req_recv(req, buffer.get(), (remaining < 4096) ? remaining : 4096);
        if (len == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }
        if (len <= 0) {
            writer.error = "connection lost";
            ok = false;
            break;
        }
        ok = writer.write(buffer.get(), len);
        remaining -= len;
        last_activity = esp_timer_get_time();
    }

    if (ok) {
        ok = writer.finish();
    } else {
        writer.abort();
    }

    last_result = ok ? ("uploaded " + std::to_string(writer.written) + " bytes") : writer.error;
    storeResult(last_result);
    httpd_resp_set_status(req, ok ? "200 OK" : "400 Bad Request");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, ok ? (last_result + ", booting ESPurna").c_str() : last_result.c_str());
    restart = ok;
    return ESP_OK;
}

void web() {
    // "admin:<password>" for Basic auth, compared as the whole header value
    const std::string plain = "admin:" + settings.password;
    unsigned char encoded[160];
    size_t len = 0;
    mbedtls_base64_encode(encoded, sizeof(encoded) - 1, &len,
        reinterpret_cast<const unsigned char*>(plain.data()), plain.size());
    auth_header = "Basic " + std::string(reinterpret_cast<char*>(encoded), len);

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.recv_wait_timeout = 15;

    httpd_handle_t server = nullptr;
    if (httpd_start(&server, &config) != ESP_OK) {
        return;
    }

    static const httpd_uri_t uris[] {
        {"/", HTTP_GET, onRoot, nullptr},
        {"/boot", HTTP_POST, onBoot, nullptr},
        {"/url", HTTP_POST, onUrl, nullptr},
        {"/upload", HTTP_POST, onUpload, nullptr},
    };

    for (const auto& uri : uris) {
        httpd_register_uri_handler(server, &uri);
    }
}

} // namespace

extern "C" void app_main() {
    auto result = nvs_flash_init();
    if ((result == ESP_ERR_NVS_NO_FREE_PAGES) || (result == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        // never erase here: the partition holds the ESPurna settings
        esp_restart();
    }

    // Next boot goes back to ESPurna whenever app0 holds a valid image: a crash, a power cut or a reset here must
    // never leave the board in safeboot for good. A finished upload / download sets app0 again anyway
    {
        std::string error;
        bootApp(error);
    }

    readSettings();

    events = xEventGroupCreate();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, onWifi, nullptr);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, onWifi, nullptr);

    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&init);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    if (connect()) {
        // picked in the ESPurna WebUI: a few attempts (Wi-Fi, the server), then the result for ESPurna. A failure
        // before the first written byte leaves ESPurna in app0, it starts again; otherwise the web UI here waits
        const auto request = takeRequest();
        if (!request.url.empty()) {
            bool ok = false;
            for (int attempt = 1; !ok && (attempt <= DownloadAttempts); ++attempt) {
                ok = download(request.url, request.sha256);
                if (!ok && (attempt < DownloadAttempts)) {
                    vTaskDelay(pdMS_TO_TICKS(3000));
                }
            }

            storeResult(ok ? last_result : ("failed: " + last_result));
            std::string error;
            if (ok || bootApp(error)) {
                vTaskDelay(pdMS_TO_TICKS(100));
                esp_restart();
            }
        }
    } else {
        accessPoint();
    }

    web();
    last_activity = esp_timer_get_time();

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(250));

        if (restart) {
            vTaskDelay(pdMS_TO_TICKS(500));
            esp_restart();
        }

        // forgotten in safeboot, or here after a crash loop of ESPurna: give ESPurna another chance
        if ((esp_timer_get_time() - last_activity) > IdleTimeoutUs) {
            std::string error;
            if (bootApp(error)) {
                esp_restart();
            }
            last_activity = esp_timer_get_time();
        }
    }
}
