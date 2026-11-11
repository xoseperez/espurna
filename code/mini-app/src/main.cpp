/*

ESPurna mini (ESP8266, 1 MB boards)

The first step of the two-step update of 1 MB boards. A full image (~800 KB) leaves ~200 KB of free space, too
little for the next full image, so the full image takes this one (espurna-esp8266-1m-mini.bin.gz, uploaded by the
WebUI to /upgrade), boots it, and this one downloads the full image (.bin.gz, 1 MB layout) and writes it. Same idea as
code/safeboot-app of ESP32, but here the mini replaces the full image: there is no ESPurna to go back to until a
full image is written. Plain Arduino core (core 3.1.2 + NONOS SDK 3.0.5, the 1 MB layout of ESPurna), no ESPurna code,
every byte counts (the gzip of this image has to fit next to the biggest full image):
- the ESPurna settings are read in place: the EEPROM_Rotate pool of ESPurna (sectors 251 and 250, the copy with
  the valid sum and the newest counter), the Embedis key-value records. Wi-Fi ssid0/pass0, ssid1/pass1, static
  ip0/gw0/mask0/dns0, hostname, adminPass, webFwUrl; no settings of its own
- the request of the full image (POST /ota/fetch there, ota_fetch.cpp): otaFetchUrl + otaFetchSha256. Taken once:
  erased (and a provisional "failed: ..." result stored) before anything else, then Wi-Fi (up to 3 minutes), the
  download (http:// or https://), SHA-256 check, Update (gzip is unpacked by eboot at the next boot). The result goes
  to otaFetchResult ("ok: ..." / "failed: ..."), which the full image shows in /ota/status; reboot on success
- settings writes: the same record edits as ESPurna (settings_embedis.h, set / del), always into the first sector
  of the pool (251) with the counter increased, rotation off as ESPurna does around an update: the image being
  downloaded overwrites the second one (250, the last sector of the free space). Records that do not end the way
  ESPurna leaves them (0xffff after the last one) are never written over
- otherwise, and after a failure: a page on port 80 (user admin + adminPass, HTTP Digest with realm = hostname as
  the ESPurna WebUI, so the browser answers with what it has). Status, last result, the releases of webFwUrl (read by
  the browser, GitHub Pages sends CORS *) with the 1 MB images that fit, a click stores the request and reboots into
  it; a file upload to /upgrade (.bin or .bin.gz, raw body or multipart as ESPurna's /upgrade); GET /ota/status and
  POST /ota/fetch in the shape of the full image (fwinstall.mjs)
- no Wi-Fi in 30 s: own access point <hostname>-MINI (WPA2, admin password), 192.168.4.1. Every 5 minutes without
  anybody on it, the board tries the Wi-Fi again (a router back after a power cut); 5 minutes without the Wi-Fi:
  the access point again
- log on the serial TX pin (GPIO1, 115200), written to the UART registers (no Serial, no printf of newlib)

TLS (MINI_TLS=1): raw BearSSL, TLS 1.2, static RSA key exchange + AES-128-GCM (what GitHub Pages offers that takes the
least code), the client of ota_fetch.cpp. The server certificate is not validated (no CA store, no clock): only its
RSA key is taken, the file is checked against its SHA-256 instead, which the browser took from firmware.json over
validated https. SHA-256 is required. MINI_TLS=0: http:// only (26 KB less of the .bin, 20 KB less of the .bin.gz)

*/

#include <Arduino.h>
#include <EEPROM_Rotate.h>
#include <ESP8266WiFi.h>
#include <MD5Builder.h>
#include <eboot_command.h>
#include <spi_flash.h>
#include <lwip/ip4_addr.h>

#ifndef MINI_TLS
#define MINI_TLS 1
#endif

#ifndef MINI_LOG
#define MINI_LOG 1
#endif

#include <bearssl/bearssl.h>

#if MINI_TLS
#include <StackThunk.h>
#endif

#include <memory>

// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------

namespace {

#if MINI_LOG
void put(char c) {
    while (((USS(0) >> USTXC) & 0xff) >= 0x7f) {
    }
    USF(0) = c;
}

// format in flash or RAM
void logv(const char* fmt, va_list args) {
    char format[96];
    strncpy_P(format, fmt, sizeof(format) - 1);
    format[sizeof(format) - 1] = '\0';

    char out[200];
    ets_vsnprintf(out, sizeof(out), format, args);
    for (const char* ptr = "[MINI] "; *ptr; ++ptr) {
        put(*ptr);
    }
    for (const char* ptr = out; *ptr; ++ptr) {
        put(*ptr);
    }
    put('\r');
    put('\n');
}

void logf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logv(fmt, args);
    va_end(args);
}

#define LOG(fmt, ...) logf(PSTR(fmt), ##__VA_ARGS__)
#else
#define LOG(...) do {} while (false)
#endif

} // namespace

// The SDK formatter (ets_vsnprintf, linked anyway; %d %u %x %s %c) instead of the one of newlib (~3 KB): the Core
// uses vsnprintf for its panic / postmortem output only
extern "C" int vsnprintf(char* buffer, size_t size, const char* fmt, va_list args) {
    return ets_vsnprintf(buffer, size, fmt, args);
}

namespace {

constexpr char Image[] = "esp8266-1m-mini";
constexpr char DefaultPassword[] = "fibonacci";
constexpr char DefaultManifest[] = "https://igorufox.github.io/espurna/firmware.json";

constexpr unsigned long ConnectWindow { 30000 };
constexpr unsigned long RequestConnectWindow { 3 * 60 * 1000 };
constexpr unsigned long FallbackAfter { 5 * 60 * 1000 };
constexpr unsigned long Timeout { 20000 };

namespace keys {

constexpr char Url[] = "otaFetchUrl";
constexpr char Sha256[] = "otaFetchSha256";
constexpr char Result[] = "otaFetchResult";

} // namespace keys

// decimal, after spaces (atoi of newlib links strtol and errno)
long number(const char* text) {
    while (*text == ' ') {
        ++text;
    }

    long out { 0 };
    while ((*text >= '0') && (*text <= '9')) {
        out = (out * 10) + (*text++ - '0');
    }

    return out;
}

int digit(char c) {
    return (c <= '9') ? (c - '0') : ((c | 0x20) - 'a' + 10);
}

String hex(const uint8_t* data, size_t len) {
    static const char Digits[] = "0123456789abcdef";
    String out;
    out.reserve(len * 2);
    for (size_t index = 0; index < len; ++index) {
        out += Digits[data[index] >> 4];
        out += Digits[data[index] & 15];
    }

    return out;
}

// -----------------------------------------------------------------------------
// ESPurna settings
// -----------------------------------------------------------------------------

// One 4 KB sector of the EEPROM_Rotate pool (storage_eeprom.h, arch/esp8266/storage_eeprom_esp8266.cpp): sum and
// counter of EEPROM_Rotate at 11..13, the crash record up to 256 (crash.cpp), the Embedis records from the end of the
// sector down to 256 (settings.cpp). A record, in the address order: value, value length (2 bytes, big endian), key,
// key length; 0xffff instead of the key length is the free space
constexpr uint16_t RotateOffset { 11 };
constexpr uint16_t ReservedEnd { RotateOffset + 3 };
constexpr uint16_t KvBegin { 256 };
constexpr uint16_t KvEnd { SPI_FLASH_SEC_SIZE };

struct Record {
    uint16_t begin;
    uint16_t end;
    uint16_t key;
    uint16_t key_len;
    uint16_t value_len;
};

struct Pair {
    const char* key;
    String value;
};

class Storage : public EEPROM_Rotate {
public:
    void load() {
        // as ESPurna: 2 sectors (251, 250) with the 1 MB layout
        _pool_size = (last() > 250) ? 2 : 1;
        _offset = RotateOffset;
        begin(KvEnd);
        _valid = _checkCRC();
        LOG("Settings in sector %d (pool of %d): %s", current(), _pool_size, _valid ? "valid" : "none");
    }

    String get(const char* key) const {
        String out;
        uint16_t pos { KvEnd };
        Record record;
        bool clean;
        while (_valid && next(pos, record, clean)) {
            if (match(record, key)) {
                out.concat(reinterpret_cast<const char*>(_data + record.begin), record.value_len);
                break;
            }
        }

        return out;
    }

    // ESPurna set() and del() at once: the records of `drop` and of the keys of `pairs` go, the others move towards
    // the end of the sector in the same order, the pairs follow them
    bool write(const Pair* pairs, size_t count, const char* drop = nullptr, const char* drop2 = nullptr) {
        if (!_valid) {
            // no valid copy, ESPurna starts with empty settings as well
            memset(_data + ReservedEnd, 0xff, _size - ReservedEnd);
        }

        auto dropped = [&](const Record& record) {
            if ((drop && match(record, drop)) || (drop2 && match(record, drop2))) {
                return true;
            }

            for (size_t index = 0; index < count; ++index) {
                if (match(record, pairs[index].key)) {
                    return true;
                }
            }

            return false;
        };

        // nothing is changed unless the records end with the free space marker and the new ones fit
        uint16_t pos { KvEnd };
        size_t kept { 0 };
        Record record;
        bool clean { false };
        while (next(pos, record, clean)) {
            if (!dropped(record)) {
                kept += record.end - record.begin;
            }
        }

        size_t need { 0 };
        for (size_t index = 0; index < count; ++index) {
            need += 4 + strlen(pairs[index].key) + pairs[index].value.length();
        }

        if (!clean || ((kept + need + 2) > static_cast<size_t>(KvEnd - KvBegin))) {
            LOG("Settings not written: %s", clean ? "no room" : "unknown layout");
            return false;
        }

        const uint16_t low { pos };
        uint16_t out { KvEnd };
        pos = KvEnd;
        while (next(pos, record, clean)) {
            if (!dropped(record)) {
                const uint16_t size = record.end - record.begin;
                memmove(_data + out - size, _data + record.begin, size);
                out -= size;
            }
        }

        memset(_data + low, 0xff, out - low);

        for (size_t index = 0; index < count; ++index) {
            const auto& pair = pairs[index];
            const size_t key_len = strlen(pair.key);
            const size_t value_len = pair.value.length();

            out -= 4 + key_len + value_len;
            auto* ptr = _data + out;
            memcpy(ptr, pair.value.c_str(), value_len);
            ptr += value_len;
            *ptr++ = value_len >> 8;
            *ptr++ = value_len & 0xff;
            memcpy(ptr, pair.key, key_len);
            ptr += key_len;
            *ptr++ = key_len >> 8;
            *ptr = key_len & 0xff;
        }

        _data[out - 2] = 0xff;
        _data[out - 1] = 0xff;

        // the first sector of the pool, counter + 1: newer than the second one whatever is left of it
        _dirty = true;
        rotate(false);
        const bool ok = commit();
        _valid = _valid || ok;

        LOG("Settings written to sector %d: %s", current(), ok ? "ok" : "FAILED");
        return ok;
    }

private:
    uint16_t length(uint16_t pos) const {
        return (_data[pos - 2] << 8) | _data[pos - 1];
    }

    bool match(const Record& record, const char* key) const {
        const size_t len = strlen(key);
        return (record.key_len == len) && (memcmp(_data + record.key, key, len) == 0);
    }

    // the record below `pos`. false at the end: `clean` when it is the free space marker
    bool next(uint16_t& pos, Record& out, bool& clean) const {
        clean = false;
        if (pos < (KvBegin + 2)) {
            return false;
        }

        const uint16_t key_len = length(pos);
        if (key_len == 0xffff) {
            clean = true;
            return false;
        }

        if (!key_len || ((pos - KvBegin) < (key_len + 4))) {
            return false;
        }

        const uint16_t key = pos - 2 - key_len;
        const uint16_t value_len = length(key);
        if ((value_len == 0xffff) || ((key - 2 - KvBegin) < value_len)) {
            return false;
        }

        out.end = pos;
        out.key = key;
        out.key_len = key_len;
        out.value_len = value_len;
        out.begin = key - 2 - value_len;
        pos = out.begin;

        return true;
    }

    bool _valid { false };
};

Storage storage;

struct Config {
    String ssid[2];
    String pass[2];
    String ip;
    String gw;
    String mask;
    String dns;
    String hostname;
    String password;
    String manifest;
};

Config config;

String last_result;

void storeResult(const String& value) {
    last_result = value;
    const Pair pair { keys::Result, value };
    storage.write(&pair, 1);
    LOG("Result: %s", value.c_str());
}

bool validUrl(const String& url) {
    if ((url.length() < 12) || (url.length() > 240) || (!url.startsWith("https://") && !url.startsWith("http://"))) {
        return false;
    }

    for (const auto c : url) {
        if ((c <= ' ') || (c == '"') || (c == '\\') || (c == '<') || (c == '>') || (c > '~')) {
            return false;
        }
    }

    return true;
}

bool validSha256(const String& value) {
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

// -----------------------------------------------------------------------------
// image writer
// -----------------------------------------------------------------------------

} // namespace

extern "C" uint32_t _FS_start;

namespace {

// Instead of the Updater of the Core (~4 KB with its MD5, signature and LED code), the same placement and eboot
// command: the file goes to the end of the free space (right below the EEPROM sector 251, over 250), sector by sector,
// SHA-256 on the way. end() reads the written file back and compares its SHA-256 (a staged image with holes in flash
// once bricked a 1 MB board, the stream checksum had said OK), only then eboot is told to copy it (gzip unpacked)
class Writer {
public:
    // nullptr or the error
    const char* begin(size_t size) {
        const uint32_t end = reinterpret_cast<uint32_t>(&_FS_start) - 0x40200000;
        const uint32_t used = (ESP.getSketchSize() + SPI_FLASH_SEC_SIZE - 1) & ~(SPI_FLASH_SEC_SIZE - 1);
        const uint32_t rounded = (size + SPI_FLASH_SEC_SIZE - 1) & ~(SPI_FLASH_SEC_SIZE - 1);
        if (!size || (rounded > end - used)) {
            return "the file does not fit in the free space";
        }

        _buffer.reset(new (std::nothrow) uint8_t[SPI_FLASH_SEC_SIZE]);
        if (!_buffer) {
            return "out of memory";
        }

        _start = end - rounded;
        _size = size;
        done = 0;
        _written = 0;
        _len = 0;
        br_sha256_init(&_sha);
        return nullptr;
    }

    const char* write(const uint8_t* data, size_t len) {
        if (done + len > _size) {
            return "more data than announced";
        }

        br_sha256_update(&_sha, data, len);
        done += len;
        while (len) {
            const size_t size = std::min(len, SPI_FLASH_SEC_SIZE - _len);
            memcpy(_buffer.get() + _len, data, size);
            _len += size;
            data += size;
            len -= size;
            if ((_len == SPI_FLASH_SEC_SIZE) || (_written + _len == _size)) {
                const char* error = flush();
                if (error) {
                    return error;
                }
            }
        }

        return nullptr;
    }

    // the whole file arrived (and its SHA-256 is `sha256`, when given)
    const char* end(const String& sha256) {
        uint8_t digest[br_sha256_SIZE];
        br_sha256_out(&_sha, digest);
        if ((done != _size) || (sha256.length() && !sha256.equals(hex(digest, sizeof(digest))))) {
            return "SHA-256 mismatch, not the file of the release list";
        }

        br_sha256_init(&_sha);
        for (uint32_t offset = 0; offset < _size; offset += SPI_FLASH_SEC_SIZE) {
            const uint32_t size = std::min<uint32_t>(_size - offset, SPI_FLASH_SEC_SIZE);
            if (spi_flash_read(_start + offset, reinterpret_cast<uint32_t*>(_buffer.get()), (size + 3) & ~3)) {
                return "flash read failed";
            }

            if (!offset) {
                _buffer[2] = _mode;
            }
            br_sha256_update(&_sha, _buffer.get(), size);
            yield();
        }

        uint8_t check[br_sha256_SIZE];
        br_sha256_out(&_sha, check);
        if (memcmp(digest, check, sizeof(digest))) {
            return "the file read back from flash differs, not booted";
        }

        eboot_command command {};
        command.action = ACTION_COPY_RAW;
        command.args[0] = _start;
        command.args[1] = 0;
        command.args[2] = _size;
        eboot_command_write(&command);
        _buffer.reset();
        return nullptr;
    }

    size_t done { 0 };

private:
    const char* flush() {
        const uint32_t address = _start + _written;
        if (!_written) {
            // ESP8266 application or gzip (unpacked by eboot)
            const bool gzip = (_len > 1) && (_buffer[0] == 0x1f) && (_buffer[1] == 0x8b);
            if (!gzip && ((_len < 4) || (_buffer[0] != 0xe9))) {
                return "not an ESP8266 image (no 0xE9 or gzip magic)";
            }

            // a plain image gets the flash mode of the running one (as Updater); the hash is over the original byte
            _mode = _buffer[2];
            if (!gzip) {
                _buffer[2] = ESP.getFlashChipMode();
            }
        }

        memset(_buffer.get() + _len, 0xff, ((_len + 3) & ~3) - _len);
        const bool ok = !spi_flash_erase_sector(address / SPI_FLASH_SEC_SIZE)
            && !spi_flash_write(address, reinterpret_cast<uint32_t*>(_buffer.get()), (_len + 3) & ~3);
        _written += _len;
        _len = 0;
        return ok ? nullptr : "flash write failed";
    }

    std::unique_ptr<uint8_t[]> _buffer;
    br_sha256_context _sha;
    uint32_t _start { 0 };
    size_t _size { 0 };
    size_t _written { 0 };
    size_t _len { 0 };
    uint8_t _mode { 0 };
};

// -----------------------------------------------------------------------------
// http:// and https:// client (ota_fetch.cpp of ESPurna)
// -----------------------------------------------------------------------------

struct Target {
    bool https { false };
    String host;
    uint16_t port { 80 };
    String path;
};

bool parse(const String& url, Target& out) {
    out.https = url.startsWith("https://");
    if (!out.https && !url.startsWith("http://")) {
        return false;
    }

    out.port = out.https ? 443 : 80;
    const int pos = out.https ? 8 : 7;

    int slash = url.indexOf('/', pos);
    if (slash < 0) {
        slash = url.length();
    }

    out.host = url.substring(pos, slash);
    out.path = url.substring(slash);
    if (!out.path.length()) {
        out.path = "/";
    }

    const int colon = out.host.indexOf(':');
    if (colon >= 0) {
        const auto port = number(out.host.c_str() + colon + 1);
        if ((port <= 0) || (port > 65535)) {
            return false;
        }
        out.port = port;
        out.host.remove(colon);
    }

    return out.host.length() > 0;
}

} // namespace

#if MINI_TLS

// BearSSL needs more stack than the 4 KB of the loop, the engine steps go to the second stack of the Core
// (StackThunk, 6 KB on the heap while a connection is open). Own thunks: the ones of BearSSLHelpers.cpp would link
// its RSA and EC verifiers
extern "C" {

void mini_sendrec_ack(br_ssl_engine_context* cc, size_t len) {
    br_ssl_engine_sendrec_ack(cc, len);
}

void mini_recvrec_ack(br_ssl_engine_context* cc, size_t len) {
    br_ssl_engine_recvrec_ack(cc, len);
}

void mini_sendapp_ack(br_ssl_engine_context* cc, size_t len) {
    br_ssl_engine_sendapp_ack(cc, len);
}

void mini_recvapp_ack(br_ssl_engine_context* cc, size_t len) {
    br_ssl_engine_recvapp_ack(cc, len);
}

void mini_flush(br_ssl_engine_context* cc, int force) {
    br_ssl_engine_flush(cc, force);
}

void thunk_mini_sendrec_ack(br_ssl_engine_context*, size_t);
void thunk_mini_recvrec_ack(br_ssl_engine_context*, size_t);
void thunk_mini_sendapp_ack(br_ssl_engine_context*, size_t);
void thunk_mini_recvapp_ack(br_ssl_engine_context*, size_t);
void thunk_mini_flush(br_ssl_engine_context*, int);

} // extern "C"

make_stack_thunk(mini_sendrec_ack);
make_stack_thunk(mini_recvrec_ack);
make_stack_thunk(mini_sendapp_ack);
make_stack_thunk(mini_recvapp_ack);
make_stack_thunk(mini_flush);

#endif

namespace {

#if MINI_TLS

// The certificate chain is not validated. The server certificate (the first one) is kept, its RSA key is taken from
// SubjectPublicKeyInfo: the rsaEncryption OID, then BIT STRING { SEQUENCE { INTEGER n, INTEGER e } }. Smaller than
// the X.509 decoder of BearSSL
constexpr size_t CertMax { 4096 };

struct Insecure {
    const br_x509_class* vtable;
    uint8_t* cert;
    size_t size;
    size_t len;
    br_x509_pkey pkey;
};

bool tag(const uint8_t* der, size_t len, size_t& pos, uint8_t type, size_t& out) {
    if ((pos + 2 > len) || (der[pos] != type)) {
        return false;
    }

    out = der[pos + 1];
    pos += 2;
    if (out & 0x80) {
        size_t bytes = out & 0x7f;
        if ((bytes > 2) || (pos + bytes > len)) {
            return false;
        }

        out = 0;
        while (bytes--) {
            out = (out << 8) | der[pos++];
        }
    }

    return pos + out <= len;
}

bool rsaKey(const uint8_t* der, size_t len, br_rsa_public_key& out) {
    static const uint8_t Oid[] { 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01 };
    for (size_t pos = 0; pos + sizeof(Oid) < len; ++pos) {
        if (memcmp(der + pos, Oid, sizeof(Oid)) != 0) {
            continue;
        }

        pos += sizeof(Oid);
        if ((pos + 2 <= len) && (der[pos] == 0x05) && !der[pos + 1]) {
            pos += 2;
        }

        size_t size;
        if (!tag(der, len, pos, 0x03, size) || der[pos++] || !tag(der, len, pos, 0x30, size)
            || !tag(der, len, pos, 0x02, size))
        {
            return false;
        }

        out.n = const_cast<uint8_t*>(der + pos);
        out.nlen = size;
        pos += size;
        while (out.nlen && !*out.n) {
            ++out.n;
            --out.nlen;
        }

        if (!tag(der, len, pos, 0x02, size)) {
            return false;
        }

        out.e = const_cast<uint8_t*>(der + pos);
        out.elen = size;
        return out.nlen && size;
    }

    return false;
}

void insecureStartChain(const br_x509_class** ctx, const char*) {
    auto* self = reinterpret_cast<Insecure*>(ctx);
    self->size = 0;
    self->len = 0;
}

void insecureStartCert(const br_x509_class** ctx, uint32_t length) {
    auto* self = reinterpret_cast<Insecure*>(ctx);
    if (!self->size && !self->cert && (length <= CertMax)) {
        self->cert = static_cast<uint8_t*>(malloc(length));
        self->size = self->cert ? length : 0;
    }
}

void insecureAppend(const br_x509_class** ctx, const unsigned char* buf, size_t len) {
    auto* self = reinterpret_cast<Insecure*>(ctx);
    if (self->cert && (self->len + len <= self->size)) {
        memcpy(self->cert + self->len, buf, len);
        self->len += len;
    }
}

void insecureEndCert(const br_x509_class**) {
}

unsigned insecureEndChain(const br_x509_class** ctx) {
    auto* self = reinterpret_cast<Insecure*>(ctx);
    self->pkey.key_type = BR_KEYTYPE_RSA;
    return (self->cert && (self->len == self->size) && rsaKey(self->cert, self->len, self->pkey.key.rsa))
        ? 0 : BR_ERR_X509_UNSUPPORTED;
}

const br_x509_pkey* insecureGetPkey(const br_x509_class* const* ctx, unsigned* usages) {
    if (usages) {
        *usages = BR_KEYTYPE_KEYX | BR_KEYTYPE_SIGN;
    }

    return &reinterpret_cast<const Insecure*>(ctx)->pkey;
}

const br_x509_class InsecureClass PROGMEM {
    sizeof(Insecure),
    insecureStartChain,
    insecureStartCert,
    insecureAppend,
    insecureEndCert,
    insecureEndChain,
    insecureGetPkey,
};

// GitHub Pages (Fastly, 2026-11): TLS 1.2, RSA certificate, AES-GCM and ChaCha20 suites, static RSA key exchange too
const uint16_t Suites[] PROGMEM {
    BR_TLS_RSA_WITH_AES_128_GCM_SHA256,
};

struct Tls {
    ~Tls() {
        free(x509.cert);
    }

    br_ssl_client_context sc;
    Insecure x509;
};

constexpr size_t InputSize { BR_SSL_BUFSIZE_INPUT };
constexpr size_t OutputSize { 837 };

#endif

class Link {
public:
    Link() = default;
    Link(const Link&) = delete;
    Link& operator=(const Link&) = delete;

    ~Link() {
        _tcp.stop();
#if MINI_TLS
        if (_tls) {
            stack_thunk_del_ref();
        }
#endif
    }

    bool open(const Target& target, String& error) {
        _tcp.setTimeout(Timeout);
        if (!_tcp.connect(target.host.c_str(), target.port)) {
            error = String("cannot connect to ") + target.host;
            return false;
        }

        if (!target.https) {
            return true;
        }

#if MINI_TLS
        _buffers.reset(new (std::nothrow) unsigned char[InputSize + OutputSize]);
        _tls.reset(_buffers ? new (std::nothrow) Tls() : nullptr);
        if (!_tls) {
            error = "out of memory for TLS";
            return false;
        }

        stack_thunk_add_ref();

        auto* eng = &_tls->sc.eng;
        br_ssl_client_zero(&_tls->sc);
        br_ssl_engine_set_versions(eng, BR_TLS12, BR_TLS12);

        uint16_t suites[sizeof(Suites) / sizeof(Suites[0])];
        memcpy_P(suites, Suites, sizeof(Suites));
        br_ssl_engine_set_suites(eng, suites, sizeof(suites) / sizeof(suites[0]));

        br_ssl_client_set_default_rsapub(&_tls->sc);
        br_ssl_engine_set_hash(eng, br_sha256_ID, &br_sha256_vtable);
        br_ssl_engine_set_prf_sha256(eng, &br_tls12_sha256_prf);

        // the smallest AES and GHASH of BearSSL
        br_ssl_engine_set_gcm(eng, &br_sslrec_in_gcm_vtable, &br_sslrec_out_gcm_vtable);
        br_ssl_engine_set_aes_ctr(eng, &br_aes_small_ctr_vtable);
        br_ssl_engine_set_ghash(eng, &br_ghash_ctmul);

        _tls->x509.vtable = &InsecureClass;
        br_ssl_engine_set_x509(eng, &_tls->x509.vtable);
        br_ssl_engine_set_buffers_bidi(eng,
            _buffers.get(), InputSize,
            _buffers.get() + InputSize, OutputSize);

        uint8_t seed[32];
        ESP.random(seed, sizeof(seed));
        br_ssl_engine_inject_entropy(eng, seed, sizeof(seed));

        if (!br_ssl_client_reset(&_tls->sc, target.host.c_str(), 0) || (pump(BR_SSL_SENDAPP) != Pump::Ready)) {
            error = String("TLS handshake failed, BearSSL error ") + br_ssl_engine_last_error(eng);
            return false;
        }

        return true;
#else
        error = "https needs MINI_TLS=1";
        return false;
#endif
    }

    bool write(const char* data, size_t len) {
#if MINI_TLS
        if (_tls) {
            auto* eng = &_tls->sc.eng;
            while (len) {
                if (pump(BR_SSL_SENDAPP) != Pump::Ready) {
                    return false;
                }

                size_t avail;
                auto* buf = br_ssl_engine_sendapp_buf(eng, &avail);
                const size_t size = std::min(avail, len);
                memcpy(buf, data, size);
                thunk_mini_sendapp_ack(eng, size);
                data += size;
                len -= size;
            }

            thunk_mini_flush(eng, 0);
            return pump(BR_SSL_SENDAPP) == Pump::Ready;
        }
#endif
        return _tcp.write(reinterpret_cast<const uint8_t*>(data), len) == len;
    }

    // > 0 bytes read, 0 - the server closed the connection, < 0 - timeout or error
    int read(uint8_t* out, size_t len) {
#if MINI_TLS
        if (_tls) {
            switch (pump(BR_SSL_RECVAPP)) {
            case Pump::Ready:
                break;
            case Pump::Closed:
                return 0;
            case Pump::Failed:
                return -1;
            }

            auto* eng = &_tls->sc.eng;
            size_t avail;
            auto* buf = br_ssl_engine_recvapp_buf(eng, &avail);
            const size_t size = std::min(avail, len);
            memcpy(out, buf, size);
            thunk_mini_recvapp_ack(eng, size);

            return size;
        }
#endif
        const auto start = millis();
        for (;;) {
            const int avail = _tcp.available();
            if (avail > 0) {
                return _tcp.read(out, std::min(len, static_cast<size_t>(avail)));
            }

            if (!_tcp.connected()) {
                return 0;
            }

            if (millis() - start > Timeout) {
                return -1;
            }

            delay(1);
        }
    }

private:
#if MINI_TLS
    enum class Pump {
        Ready,
        Closed,
        Failed,
    };

    // runs the engine until it is in the `target` state, sending and receiving records
    Pump pump(unsigned target) {
        auto* eng = &_tls->sc.eng;
        auto last = millis();

        for (;;) {
            const auto state = br_ssl_engine_current_state(eng);
            if (state & BR_SSL_CLOSED) {
                return (br_ssl_engine_last_error(eng) == BR_ERR_OK) ? Pump::Closed : Pump::Failed;
            }

            if (state & BR_SSL_SENDREC) {
                size_t len;
                auto* buf = br_ssl_engine_sendrec_buf(eng, &len);
                const auto sent = _tcp.write(buf, len);
                if (sent) {
                    thunk_mini_sendrec_ack(eng, sent);
                    last = millis();
                    continue;
                }
            } else if (state & target) {
                return Pump::Ready;
            } else if (state & BR_SSL_RECVREC) {
                const int avail = _tcp.available();
                if (avail > 0) {
                    size_t len;
                    auto* buf = br_ssl_engine_recvrec_buf(eng, &len);
                    const int got = _tcp.read(buf, std::min(len, static_cast<size_t>(avail)));
                    if (got > 0) {
                        thunk_mini_recvrec_ack(eng, got);
                        last = millis();
                        continue;
                    }
                }

                // closed without close_notify: the end of the body is checked with Content-Length
                if (!_tcp.connected()) {
                    return Pump::Closed;
                }
            }

            if (!_tcp.connected() || (millis() - last > Timeout)) {
                return Pump::Failed;
            }

            delay(1);
        }
    }

    std::unique_ptr<unsigned char[]> _buffers;
    std::unique_ptr<Tls> _tls;
#endif
    WiFiClient _tcp;
};

// Buffered reader of the response, the rest of the buffer after the head is the beginning of the body
class Reader {
public:
    explicit Reader(Link& link) :
        _link(link)
    {}

    // a line without CR LF into `out`, false when longer
    bool line(char* out, size_t size) {
        size_t len { 0 };
        for (;;) {
            if ((_pos >= _len) && !fill()) {
                return false;
            }

            const char c = _buffer[_pos++];
            if (c == '\n') {
                if (len && (out[len - 1] == '\r')) {
                    --len;
                }
                out[len] = '\0';
                return true;
            }

            if (len + 1 >= size) {
                return false;
            }

            out[len++] = c;
        }
    }

    int read(uint8_t* out, size_t len) {
        if (_pos < _len) {
            const size_t size = std::min(len, _len - _pos);
            memcpy(out, _buffer + _pos, size);
            _pos += size;
            return size;
        }

        return _link.read(out, len);
    }

private:
    bool fill() {
        const int got = _link.read(_buffer, sizeof(_buffer));
        if (got <= 0) {
            return false;
        }

        _pos = 0;
        _len = got;
        return true;
    }

    Link& _link;
    uint8_t _buffer[256];
    size_t _pos { 0 };
    size_t _len { 0 };
};

// The body goes to the writer, which boots nothing unless the whole file arrived with the right SHA-256
bool body(Reader& reader, size_t length, const String& sha256, String& result) {
    Writer writer;
    const char* error = writer.begin(length);

    uint8_t buffer[512];
    while (!error && (writer.done < length)) {
        const int got = reader.read(buffer, std::min(sizeof(buffer), length - writer.done));
        if (got <= 0) {
            result = String((got < 0) ? "timeout" : "connection closed") + " after " + writer.done + " of " + length
                + " bytes";
            return false;
        }

        error = writer.write(buffer, got);
        if (!(writer.done & 0xffff)) {
            LOG("%d of %d bytes", writer.done, length);
        }
    }

    if (!error) {
        error = writer.end(sha256);
    }

    if (error) {
        result = error;
        return false;
    }

    result = String("downloaded ") + length + " bytes, SHA-256 checked";
    return true;
}

bool download(String url, const String& sha256, String& result) {
    char line[256];
    for (int redirects = 0; redirects < 4; ++redirects) {
        Target target;
        if (!parse(url, target)) {
            result = "invalid URL";
            return false;
        }

        LOG("Downloading %s, heap %d", url.c_str(), ESP.getFreeHeap());

        Link link;
        if (!link.open(target, result)) {
            return false;
        }

        const String request = String("GET ") + target.path + " HTTP/1.1\r\nHost: " + target.host
            + "\r\nUser-Agent: ESPurna-mini\r\nConnection: close\r\n\r\n";
        if (!link.write(request.c_str(), request.length())) {
            result = "cannot send the request";
            return false;
        }

        Reader reader(link);
        if (!reader.line(line, sizeof(line)) || strncmp(line, "HTTP/1.", 7)) {
            result = "no HTTP response";
            return false;
        }

        const char* space = strchr(line, ' ');
        const int status = space ? number(space + 1) : 0;
        long length { -1 };
        String location;
        for (;;) {
            if (!reader.line(line, sizeof(line))) {
                result = "no HTTP response";
                return false;
            }

            if (!line[0]) {
                break;
            }

            if (!strncasecmp_P(line, PSTR("content-length:"), 15)) {
                length = number(line + 15);
            } else if (!strncasecmp_P(line, PSTR("location:"), 9)) {
                location = line + 9;
                location.trim();
            } else if (!strncasecmp_P(line, PSTR("transfer-encoding:"), 18)) {
                // never for static files, the length is needed for the update
                length = -1;
            }
        }

        if ((status >= 300) && (status < 400) && location.length()) {
            // absolute, or a path on the same server
            if (location[0] == '/') {
                location = String(target.https ? "https://" : "http://") + target.host + ':' + target.port
                    + location;
            }
            url = location;
            continue;
        }

        if (status != 200) {
            result = String("HTTP ") + status;
            return false;
        }

        if (length <= 0) {
            result = "no Content-Length in the response";
            return false;
        }

        return body(reader, length, sha256, result);
    }

    result = "too many redirects";
    return false;
}

// -----------------------------------------------------------------------------
// Wi-Fi
// -----------------------------------------------------------------------------

bool ap_mode { false };
bool joined { false };
unsigned long wifi_since { 0 };

uint32_t ip(const String& text) {
    ip4_addr_t out;
    return ip4addr_aton(text.c_str(), &out) ? out.addr : 0;
}

String wifiText() {
    if (!ap_mode && (WiFi.status() != WL_CONNECTED)) {
        return "not connected";
    }

    String out(ap_mode ? "own access point " : "connected to ");
    char addr[16];
    ip4_addr_t value;
    if (ap_mode) {
        out += WiFi.softAPSSID();
        value.addr = WiFi.softAPIP();
    } else {
        out += WiFi.SSID();
        value.addr = WiFi.localIP();
    }

    out += ", ";
    out += ip4addr_ntoa_r(&value, addr, sizeof(addr));
    return out;
}

// ssid0 / ssid1 in turns (15 s each when both are set) until connected or `window` is over
bool join(unsigned long window) {
    ap_mode = false;
    joined = true;
    wifi_since = millis();
    WiFi.mode(WIFI_STA);

    const size_t networks = (config.ssid[0].length() ? 1 : 0) + (config.ssid[1].length() ? 1 : 0);
    if (!networks) {
        LOG("No Wi-Fi in the settings");
        return false;
    }

    WiFi.hostname(config.hostname);
    if (ip(config.ip) && ip(config.gw) && ip(config.mask)) {
        const auto dns = ip(config.dns);
        WiFi.config(ip(config.ip), ip(config.gw), ip(config.mask), dns ? dns : ip(config.gw));
    }

    const unsigned long slice = (networks > 1) ? 15000 : window;
    size_t index = config.ssid[0].length() ? 0 : 1;
    for (;;) {
        LOG("Connecting to %s", config.ssid[index].c_str());
        WiFi.begin(config.ssid[index], config.pass[index]);
        const auto attempt = millis();
        while (millis() - attempt < slice) {
            if (WiFi.status() == WL_CONNECTED) {
                LOG("Connected");
                return true;
            }

            if (millis() - wifi_since > window) {
                WiFi.disconnect();
                return false;
            }

            delay(100);
        }

        WiFi.disconnect();
        if (networks > 1) {
            index ^= 1;
        }
    }
}

void accessPoint() {
    // WPA2 needs 8..63 characters, ESPurna enforces the same for adminPass; SSID up to 32
    const String ssid = config.hostname.substring(0, 27) + "-MINI";
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, (config.password.length() >= 8) ? config.password : String(DefaultPassword), 1, false, 2);
    ap_mode = true;
    wifi_since = millis();
    LOG("Access point %s", ssid.c_str());
}

// -----------------------------------------------------------------------------
// web
// -----------------------------------------------------------------------------

WiFiServer server(80);
char nonce[33];
char ha1[33];
bool restart { false };
unsigned long last_activity { 0 };

// the request head: line buffer, then what is kept of it
char line[512];
char method[8];
char target[256];
char auth[512];
char type[160];
long length;

// a line without its CR LF; `raw` - bytes taken from the connection. false on timeout or a too long line
bool readLine(WiFiClient& client, size_t& raw) {
    size_t len { 0 };
    raw = 0;

    const auto start = millis();
    for (;;) {
        const int c = client.read();
        if (c < 0) {
            if (!client.connected() || (millis() - start > 5000)) {
                return false;
            }
            delay(1);
            continue;
        }

        ++raw;
        if (c == '\n') {
            if (len && (line[len - 1] == '\r')) {
                --len;
            }
            line[len] = '\0';
            return true;
        }

        if (len + 1 >= sizeof(line)) {
            return false;
        }

        line[len++] = c;
    }
}

void copyValue(char* out, size_t size, const char* value) {
    while (*value == ' ') {
        ++value;
    }

    strncpy(out, value, size - 1);
    out[size - 1] = '\0';
}

bool readHead(WiFiClient& client) {
    size_t raw;
    auth[0] = type[0] = '\0';
    length = -1;
    if (!readLine(client, raw)) {
        return false;
    }

    char* path = strchr(line, ' ');
    if (!path || ((path - line) >= static_cast<int>(sizeof(method)))) {
        return false;
    }
    *path++ = '\0';
    strcpy(method, line);

    char* version = strchr(path, ' ');
    if (version) {
        *version = '\0';
    }
    copyValue(target, sizeof(target), path);

    for (;;) {
        if (!readLine(client, raw)) {
            return false;
        }

        if (!line[0]) {
            return true;
        }

        if (!strncasecmp_P(line, PSTR("authorization:"), 14)) {
            copyValue(auth, sizeof(auth), line + 14);
        } else if (!strncasecmp_P(line, PSTR("content-length:"), 15)) {
            length = number(line + 15);
        } else if (!strncasecmp_P(line, PSTR("content-type:"), 13)) {
            copyValue(type, sizeof(type), line + 13);
        }
    }
}

bool isPath(const char* path) {
    const size_t len = strlen(path);
    return !strncmp(target, path, len) && (!target[len] || (target[len] == '?'));
}

// name=value or name="value" of a Digest header or of Content-Type parameters, into `line`
const char* param(const char* header, const char* name) {
    const size_t len = strlen(name);
    for (const char* pos = header; (pos = strstr(pos, name)); pos += len) {
        const char before = (pos == header) ? ' ' : pos[-1];
        if (((before != ' ') && (before != ',') && (before != ';')) || (pos[len] != '=')) {
            continue;
        }

        const char* value = pos + len + 1;
        const bool quoted = (*value == '"');
        value += quoted;

        size_t out { 0 };
        while (*value && (out + 1 < sizeof(line)) && (quoted ? (*value != '"') : ((*value != ',') && (*value != ' ')))) {
            line[out++] = *value++;
        }
        line[out] = '\0';
        return line;
    }

    line[0] = '\0';
    return line;
}

void md5Add(MD5Builder& md5, const char* value) {
    md5.add(value);
    md5.add(":");
}

// lower case hex, as getChars() (which formats with sprintf)
void md5Hex(MD5Builder& md5, char* out) {
    uint8_t digest[16];
    md5.calculate();
    md5.getBytes(digest);
    strcpy(out, hex(digest, sizeof(digest)).c_str());
}

// Digest as the ESPurna WebUI (realm = hostname, qop=auth). `stale`: the right password with the nonce of another
// boot or of ESPurna itself, the browser repeats the request with the new nonce without asking
bool authorized(bool& stale) {
    stale = false;
    if (strncmp(auth, "Digest ", 7) || strcmp(param(auth, "username"), "admin") || strcmp(param(auth, "uri"), target)) {
        return false;
    }

    char ha2[33];
    MD5Builder md5;
    md5.begin();
    md5Add(md5, method);
    md5.add(target);
    md5Hex(md5, ha2);

    md5.begin();
    md5Add(md5, ha1);
    md5Add(md5, param(auth, "nonce"));
    stale = strcmp(line, nonce) != 0;
    if (param(auth, "qop")[0]) {
        md5Add(md5, param(auth, "nc"));
        md5Add(md5, param(auth, "cnonce"));
        md5Add(md5, param(auth, "qop"));
    }
    md5.add(ha2);
    md5Hex(md5, ha2);

    const bool ok = !strcasecmp(param(auth, "response"), ha2);
    stale = stale && ok;
    return ok && !stale;
}

// Connection: close, the body ends with the connection (no Content-Length)
void respond(WiFiClient& client, const char* status, const char* content, const String& body, int challenge = 0) {
    String head("HTTP/1.1 ");
    head += status;
    head += "\r\nContent-Type: ";
    head += content;
    head += "\r\nCache-Control: no-store\r\nConnection: close\r\n";
    if (challenge) {
        head += "WWW-Authenticate: Digest realm=\"";
        head += config.hostname;
        head += "\", qop=\"auth\", nonce=\"";
        head += nonce;
        head += (challenge > 1) ? "\", stale=TRUE\r\n" : "\"\r\n";
    }
    head += "\r\n";
    head += body;

    client.write(head.c_str(), head.length());
}

void text(WiFiClient& client, const char* status, const String& body) {
    respond(client, status, "text/plain", body);
}

constexpr char Ok[] = "200 OK";
constexpr char BadRequest[] = "400 Bad Request";

void json(String& out, const String& value) {
    out += '"';
    for (const auto c : value) {
        if ((c == '"') || (c == '\\')) {
            out += '\\';
            out += c;
        } else if ((c == '<') || (c == '>') || (static_cast<unsigned char>(c) < 0x20)) {
            // also used inside <script>
            out += ' ';
        } else {
            out += c;
        }
    }
    out += '"';
}

// the /ota/status of the full image (ota_fetch.cpp): fwinstall.mjs tells the images apart by "image"
String status() {
    String out("{\"image\":\"");
    out += Image;
    out += "\",\"fetch\":1,\"free\":";
    out += ESP.getFreeSketchSpace();
    out += ",\"url\":";
    json(out, storage.get(keys::Url));
    out += ",\"result\":";
    json(out, last_result);
    out += '}';
    return out;
}

// Page: the browser reads the release list (GitHub Pages sends Access-Control-Allow-Origin: *) and shows the 1 MB
// images that fit next to this one (Update for one byte more, in whole sectors); the board downloads the picked
// file itself after a reboot (/ota/fetch), or takes an uploaded file (/upgrade, the file as the body)
const char PageHead[] PROGMEM =
    "<!DOCTYPE html><html><head><meta charset=utf-8><meta name=viewport content=\"width=device-width,initial-scale=1\">"
    "<title>ESPurna mini</title><style>body{font-family:sans-serif;max-width:640px;margin:16px auto;padding:0 16px}"
    "button{margin-top:8px}fieldset{margin-top:16px}</style></head><body><h2 id=h></h2><p id=st></p>"
    "<fieldset><legend>Releases</legend><div id=rel>Reading the release list...</div></fieldset>"
    "<fieldset><legend>Upload an image (.bin or .bin.gz)</legend><input type=file id=file accept=.bin,.gz><br>"
    "<button id=up>Upload and boot</button></fieldset><p id=log></p><script>";

const char PageScript[] PROGMEM =
    "const $=i=>document.getElementById(i),L=$('log'),I=S.image,F=S.free;"
    "$('h').textContent=document.title=H+' - ESPurna mini';"
    "$('st').textContent='Wi-Fi: '+W+'. Free: '+F+' bytes. Last update: '+(S.result||'none');"
    "function wait(){let away=0,t=0;const tick=async()=>{t+=3;"
    "try{const r=await fetch('/ota/status',{cache:'no-store'});"
    "const s=r.status===404?{}:await r.json();"
    "if(s.image!==I){L.textContent='ESPurna is back '+(s.result||'');setTimeout(()=>location.href='/',3000);return}"
    "if(away&&s.result){L.textContent=s.result;return}}catch(e){away=1}"
    "L.textContent='Waiting for the board ('+t+' s)...';setTimeout(tick,3000)};setTimeout(tick,3000)}"
    "function lock(v){for(const b of document.querySelectorAll('button'))b.disabled=v}"
    "async function post(u,b){lock(true);try{const r=await fetch(u,{method:'POST',body:b});"
    "L.textContent=await r.text();if(r.ok){wait();return}}catch(e){L.textContent=e}lock(false)}"
    "$('up').onclick=()=>{const f=$('file').files[0];if(!f){L.textContent='Pick a file';return}"
    "L.textContent='Uploading '+f.name+'...';post('/upgrade',f)};"
    "fetch(M,{cache:'no-store'}).then(r=>r.json()).then(m=>{const d=$('rel');d.textContent='';"
    "for(const r of m.releases||[]){const im=r.images||{};"
    "for(const n of Object.keys(im)){if(!n.startsWith('esp8266-1m')||n===I)continue;"
    "for(const f of im[n]){if(f.kind!=='ota'||!f.sha256||(f.size-f.size%4096+4096)>F)continue;"
    "const b=document.createElement('button');b.textContent=r.tag+': '+f.name+' ('+Math.round(f.size/1024)+' KB)';"
    "b.onclick=()=>{if(confirm('Install '+f.name+' of '+r.tag+'?')){L.textContent='Storing the request...';"
    "post('/ota/fetch',new URLSearchParams({url:new URL(f.url,M).href,sha256:f.sha256}))}};"
    "d.append(b,document.createElement('br'))}}}"
    "if(!d.childNodes.length)d.textContent='No 1 MB image that fits in '+F+' bytes in '+M})"
    ".catch(e=>$('rel').textContent='Cannot read '+M+': '+e)</script></body></html>";

void onRoot(WiFiClient& client) {
    String body(FPSTR(PageHead));
    body += "const S=";
    body += status();
    body += ",H=";
    json(body, config.hostname);
    body += ",M=";
    json(body, config.manifest);
    body += ",W=";
    json(body, wifiText());
    body += ';';
    body += FPSTR(PageScript);
    respond(client, Ok, "text/html", body);
}

// application/x-www-form-urlencoded or multipart/form-data (FormData of fwinstall.mjs)
String formField(const char* body, bool multipart, const char* name) {
    String out;
    const size_t len = strlen(name);
    for (const char* pos = body; (pos = strstr(pos, name)); pos += len) {
        if (multipart) {
            if ((pos < body + 6) || strncmp(pos - 6, "name=\"", 6) || (pos[len] != '"')) {
                continue;
            }

            const char* value = strstr(pos, "\r\n\r\n");
            const char* end = value ? strstr(value + 4, "\r\n--") : nullptr;
            if (end) {
                out.concat(value + 4, end - value - 4);
            }
            break;
        }

        if (((pos != body) && (pos[-1] != '&')) || (pos[len] != '=')) {
            continue;
        }

        for (const char* value = pos + len + 1; *value && (*value != '&'); ++value) {
            if ((*value == '%') && value[1] && value[2]) {
                out += static_cast<char>((digit(value[1]) << 4) | digit(value[2]));
                value += 2;
            } else {
                out += (*value == '+') ? ' ' : *value;
            }
        }
        break;
    }

    return out;
}

// url + sha256, the same checks as the full image. Stored, then the reboot takes it (one attempt)
void onFetch(WiFiClient& client) {
    std::unique_ptr<char[]> body((length > 0) && (length < 1024) ? new (std::nothrow) char[length + 1] : nullptr);
    if (!body) {
        text(client, BadRequest, "ERROR: Invalid request");
        return;
    }

    const auto got = client.readBytes(body.get(), length);
    body[got] = '\0';

    const bool multipart = !strncmp(type, "multipart/form-data", 19);
    const auto url = formField(body.get(), multipart, "url");
    const auto sha256 = formField(body.get(), multipart, "sha256");
    if (!validUrl(url) || !validSha256(sha256)) {
        text(client, BadRequest, "ERROR: invalid URL or SHA-256");
        return;
    }

    const Pair pairs[] {
        {keys::Url, url},
        {keys::Sha256, sha256},
    };
    if (!storage.write(pairs, 2, keys::Result)) {
        text(client, "500 Internal Server Error", "ERROR: cannot write the settings");
        return;
    }

    last_result = String();
    text(client, Ok, "OK, fetch");
    restart = true;
}

// .bin or .bin.gz: the file as the body (page of this image), or multipart/form-data as ESPurna's /upgrade
// (scripts, "--boundary" part head, then the file, then "\r\n--boundary--\r\n")
void onUpgrade(WiFiClient& client) {
    size_t left = (length > 0) ? length : 0;
    size_t size = left;

    String boundary;
    if (!strncmp(type, "multipart/form-data", 19)) {
        boundary = param(type, "boundary");
    }

    if (boundary.length()) {
        // part head: --boundary, headers, empty line
        for (bool first = true;; first = false) {
            size_t raw;
            if (!readLine(client, raw) || (raw > left)
                || (first && ((line[0] != '-') || (line[1] != '-') || strcmp(line + 2, boundary.c_str()))))
            {
                left = 0;
                break;
            }

            left -= raw;
            if (!first && !line[0]) {
                break;
            }
        }

        const size_t tail = boundary.length() + 8;
        size = (left > tail) ? (left - tail) : 0;
    }

    LOG("Upload of %d bytes", size);

    Writer writer;
    const char* error = size ? writer.begin(size) : "invalid request";

    uint8_t buffer[512];
    while (!error && (writer.done < size)) {
        const auto got = client.readBytes(buffer, std::min(sizeof(buffer), size - writer.done));
        error = got ? writer.write(buffer, got) : "connection lost";
    }

    if (!error && boundary.length()) {
        const auto got = client.readBytes(buffer, 4 + boundary.length());
        if ((got != (4 + boundary.length())) || memcmp(buffer, "\r\n--", 4)
            || memcmp(buffer + 4, boundary.c_str(), boundary.length()))
        {
            error = "the upload does not end with its boundary";
        }
    }

    if (!error) {
        error = writer.end(String());
    }

    if (error) {
        LOG("Upload failed: %s", error);
        text(client, BadRequest, String("ERROR: ") + error);
        return;
    }

    // the full image shows it in /ota/status instead of an earlier failure
    storeResult(String("ok: uploaded ") + size + " bytes");
    text(client, Ok, "OK");
    restart = true;
}

void handle(WiFiClient& client) {
    client.setTimeout(15000);
    if (!readHead(client)) {
        text(client, BadRequest, "ERROR: Invalid request");
        return;
    }

    const bool get = !strcmp(method, "GET");
    const bool post = !strcmp(method, "POST");

    bool stale;
    if (!authorized(stale)) {
        respond(client, "401 Unauthorized", "text/plain", "Authentication required", stale ? 2 : 1);
    } else if (get && isPath("/")) {
        onRoot(client);
    } else if (get && isPath("/ota/status")) {
        respond(client, Ok, "application/json", status());
    } else if (post && isPath("/ota/fetch")) {
        onFetch(client);
    } else if (post && isPath("/upgrade")) {
        onUpgrade(client);
    } else {
        text(client, "404 Not Found", "Not found");
    }

    // what is left of a short body, so that the client reads the answer instead of a reset
    const auto start = millis();
    while (client.connected() && client.available() && (millis() - start < 500)) {
        client.read();
    }
}

// The request of the full image, taken once: erased before anything else (a power cut or a crash during the
// download leaves the provisional result), then Wi-Fi and the download
void takeRequest() {
    const auto url = storage.get(keys::Url);
    if (!url.length()) {
        return;
    }

    const auto sha256 = storage.get(keys::Sha256);
    LOG("Request %s", url.c_str());

    const Pair pair { keys::Result, "failed: the minimal image restarted during the download (power cut or crash)" };
    if (!storage.write(&pair, 1, keys::Url, keys::Sha256)) {
        // never retried over the next boots
        last_result = "failed: the request could not be erased from the settings";
        return;
    }

    String message;
    bool ok { false };
    if (!validUrl(url) || !validSha256(sha256)) {
        message = "invalid URL or SHA-256 in the request";
    } else if (!join(RequestConnectWindow)) {
        message = "no Wi-Fi connection";
    } else {
        ok = download(url, sha256, message);
    }

    storeResult(String(ok ? "ok: " : "failed: ") + message);
    if (ok) {
        delay(100);
        ESP.restart();
    }
}

} // namespace

void setup() {
#if MINI_LOG
    USD(0) = ESP8266_CLOCK / 115200;
#endif
    LOG("ESPurna mini, built " __DATE__ " " __TIME__ ", %d bytes free next to it", ESP.getFreeSketchSpace());

    storage.load();
    config.ssid[0] = storage.get("ssid0");
    config.pass[0] = storage.get("pass0");
    config.ssid[1] = storage.get("ssid1");
    config.pass[1] = storage.get("pass1");
    config.ip = storage.get("ip0");
    config.gw = storage.get("gw0");
    config.mask = storage.get("mask0");
    config.dns = storage.get("dns0");
    config.hostname = storage.get("hostname");
    config.password = storage.get("adminPass");
    config.manifest = storage.get("webFwUrl");
    last_result = storage.get(keys::Result);

    if (!config.hostname.length()) {
        // ESPURNA-<the last 3 bytes of the MAC>, as ESPurna (arch/esp8266/system_esp8266.cpp)
        const uint32_t reg0 = READ_PERI_REG(0x3ff00050);
        const uint32_t reg1 = READ_PERI_REG(0x3ff00054);
        const uint8_t id[3] { static_cast<uint8_t>(reg1 >> 8), static_cast<uint8_t>(reg1),
            static_cast<uint8_t>(reg0 >> 24) };
        config.hostname = String("ESPURNA-") + hex(id, sizeof(id));
        config.hostname.toUpperCase();
    }

    if (!config.password.length()) {
        config.password = DefaultPassword;
    }

    if (!config.manifest.length()) {
        config.manifest = DefaultManifest;
    }

    {
        uint8_t random[16];
        ESP.random(random, sizeof(random));
        strcpy(nonce, hex(random, sizeof(random)).c_str());

        MD5Builder md5;
        md5.begin();
        md5Add(md5, "admin");
        md5Add(md5, config.hostname.c_str());
        md5.add(config.password);
        md5Hex(md5, ha1);
    }

    WiFi.persistent(false);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);

    takeRequest();

    // not twice: a request that found no Wi-Fi waited 3 minutes already
    if ((WiFi.status() != WL_CONNECTED) && (joined || !join(ConnectWindow))) {
        accessPoint();
    }

    server.begin();
    last_activity = millis();
    LOG("Web page on port 80, heap %d", ESP.getFreeHeap());
}

void loop() {
    if (restart) {
        delay(500);
        ESP.restart();
    }

    auto client = server.accept();
    if (client) {
        handle(client);
        client.stop();
        last_activity = millis();
    }

    const auto now = millis();
    if (ap_mode) {
        // a router back after a power cut: the Wi-Fi again, unless somebody uses the access point
        if ((now - wifi_since > FallbackAfter) && (now - last_activity > FallbackAfter)
            && !WiFi.softAPgetStationNum() && !join(ConnectWindow))
        {
            accessPoint();
        }
    } else if (WiFi.status() == WL_CONNECTED) {
        wifi_since = now;
    } else if (now - wifi_since > FallbackAfter) {
        accessPoint();
    }
}
