/*

OTA FETCH MODULE (ESP8266)

The image downloads its update by itself, over http:// or https://.
- the WebUI (full image, /ota/fetch) or the page of the minimal image stores the URL and the SHA-256 of the file
  (settings otaFetchUrl, otaFetchSha256), then the board restarts
- 1 MB boards: the full image does not fit next to the next one. The full image only stores the request, the browser
  uploads the minimal image (espurna-esp8266-1m-mini.bin.gz) to /upgrade, the minimal image downloads the full one
- OTA_FETCH_SUPPORT (off in every release image: a full image never has the ~27 KB of heap TLS needs, the unstable
  mode it would boot in for that is softAP only) lets an image take its update at boot itself. The 1 MB updater that
  does this is code/mini-app; 4 MB boards get their update uploaded by the browser
- one attempt per stored URL: it is erased before the download. The result goes to otaFetchResult, the full image
  shows it (/ota/status). The minimal image stays with its page after a failure, a full image restarts normally

TLS: raw BearSSL with one cipher suite (TLS 1.2, RSA key exchange, AES-128-GCM; ~28 KB of code). WiFiClientSecure
(ota_httpupdate.cpp) links every suite and curve, and its CA check needs NTP, too big for the minimal image. The server
certificate is not validated (no CA store, no clock): the file is checked against its SHA-256 instead, which the
browser takes from firmware.json over validated https. SHA-256 is required for every download

*/

#include "espurna.h"

#if defined(ARDUINO_ARCH_ESP8266) && OTA_WEB_SUPPORT

#include "espurna_ota.h"
#include "system.h"

#include <Updater.h>

#if OTA_FETCH_SUPPORT
#include <ESP8266WiFi.h>
#include <StackThunk.h>
#include <bearssl/bearssl.h>
#endif

#include <memory>

#include "libs/PrintString.h"

namespace espurna {
namespace ota {
namespace fetch {
namespace {

namespace keys {

PROGMEM_STRING(Url, "otaFetchUrl");
PROGMEM_STRING(Sha256, "otaFetchSha256");
PROGMEM_STRING(Result, "otaFetchResult");

} // namespace keys

constexpr size_t UrlMax { 240 };

// What the stored settings say, and what this boot took from them
namespace internal {

String result;

} // namespace internal

bool validUrl(const String& url) {
    if ((url.length() < 12) || (url.length() > UrlMax)) {
        return false;
    }

    if (!url.startsWith(F("https://")) && !url.startsWith(F("http://"))) {
        return false;
    }

    // goes into the request line and the JSON status as is
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

void storeResult(const String& value) {
    internal::result = value;
    setSetting(FPSTR(keys::Result), value);
    eepromCommit();
}

void jsonString(String& out, const String& value) {
    out += '"';
    for (const auto c : value) {
        if ((c == '"') || (c == '\\')) {
            out += '\\';
            out += c;
        } else if (static_cast<unsigned char>(c) < 0x20) {
            out += ' ';
        } else if ((c == '<') || (c == '>')) {
            // also used inside <script> of the minimal page
            out += F("\\u003");
            out += (c == '<') ? 'c' : 'e';
        } else {
            out += c;
        }
    }
    out += '"';
}

#if OTA_FETCH_SUPPORT

// -----------------------------------------------------------------------------
// http:// and https:// client
// -----------------------------------------------------------------------------

#ifndef OTA_FETCH_ECDHE
#define OTA_FETCH_ECDHE 0
#endif

constexpr unsigned long Timeout { 20000 };
constexpr size_t OutputSize { 837 }; // same as the Core WiFiClientSecure, requests are short

struct Target {
    bool https { false };
    String host;
    uint16_t port { 80 };
    String path;
};

bool parse(const String& url, Target& out) {
    int pos;
    if (url.startsWith(F("https://"))) {
        out.https = true;
        out.port = 443;
        pos = 8;
    } else if (url.startsWith(F("http://"))) {
        out.https = false;
        out.port = 80;
        pos = 7;
    } else {
        return false;
    }

    int slash = url.indexOf('/', pos);
    if (slash < 0) {
        slash = url.length();
    }

    out.host = url.substring(pos, slash);
    out.path = (slash < static_cast<int>(url.length()))
        ? url.substring(slash)
        : String('/');

    const int colon = out.host.indexOf(':');
    if (colon >= 0) {
        const auto port = out.host.substring(colon + 1).toInt();
        if ((port <= 0) || (port > 65535)) {
            return false;
        }
        out.port = port;
        out.host.remove(colon);
    }

    return out.host.length() > 0;
}

} // namespace
} // namespace fetch
} // namespace ota
} // namespace espurna

// BearSSL needs more stack than the 4 KB of the loop (RSA i15 alone takes ~4.5 KB), the engine steps that
// run the handshake and the record crypto go to the second stack of the Core (StackThunk, 6 KB on the heap
// while a connection is open). Own thunks: the ones of BearSSLHelpers.cpp would link its RSA and EC verifiers
extern "C" {

void espurna_fetch_sendrec_ack(br_ssl_engine_context* cc, size_t len) {
    br_ssl_engine_sendrec_ack(cc, len);
}

void espurna_fetch_recvrec_ack(br_ssl_engine_context* cc, size_t len) {
    br_ssl_engine_recvrec_ack(cc, len);
}

void espurna_fetch_sendapp_ack(br_ssl_engine_context* cc, size_t len) {
    br_ssl_engine_sendapp_ack(cc, len);
}

void espurna_fetch_recvapp_ack(br_ssl_engine_context* cc, size_t len) {
    br_ssl_engine_recvapp_ack(cc, len);
}

void espurna_fetch_flush(br_ssl_engine_context* cc, int force) {
    br_ssl_engine_flush(cc, force);
}

void thunk_espurna_fetch_sendrec_ack(br_ssl_engine_context*, size_t);
void thunk_espurna_fetch_recvrec_ack(br_ssl_engine_context*, size_t);
void thunk_espurna_fetch_sendapp_ack(br_ssl_engine_context*, size_t);
void thunk_espurna_fetch_recvapp_ack(br_ssl_engine_context*, size_t);
void thunk_espurna_fetch_flush(br_ssl_engine_context*, int);

} // extern "C"

make_stack_thunk(espurna_fetch_sendrec_ack);
make_stack_thunk(espurna_fetch_recvrec_ack);
make_stack_thunk(espurna_fetch_sendapp_ack);
make_stack_thunk(espurna_fetch_recvapp_ack);
make_stack_thunk(espurna_fetch_flush);

namespace espurna {
namespace ota {
namespace fetch {
namespace {

// The certificate chain is not validated, only the public key of the server certificate is taken for the key
// exchange (the file is checked with its SHA-256)
struct Insecure {
    const br_x509_class* vtable;
    br_x509_decoder_context decoder;
    bool first;
};

void insecureStartChain(const br_x509_class** ctx, const char*) {
    auto* self = reinterpret_cast<Insecure*>(ctx);
    br_x509_decoder_init(&self->decoder, nullptr, nullptr, nullptr, nullptr);
    self->first = true;
}

void insecureStartCert(const br_x509_class**, uint32_t) {
}

void insecureAppend(const br_x509_class** ctx, const unsigned char* buf, size_t len) {
    auto* self = reinterpret_cast<Insecure*>(ctx);
    if (self->first) {
        br_x509_decoder_push(&self->decoder, buf, len);
    }
}

void insecureEndCert(const br_x509_class** ctx) {
    reinterpret_cast<Insecure*>(ctx)->first = false;
}

unsigned insecureEndChain(const br_x509_class** ctx) {
    auto* self = reinterpret_cast<Insecure*>(ctx);
    return br_x509_decoder_get_pkey(&self->decoder)
        ? 0 : BR_ERR_X509_BAD_SERVER_NAME;
}

const br_x509_pkey* insecureGetPkey(const br_x509_class* const* ctx, unsigned* usages) {
    auto* self = const_cast<Insecure*>(reinterpret_cast<const Insecure*>(ctx));
    if (usages) {
        *usages = BR_KEYTYPE_KEYX | BR_KEYTYPE_SIGN;
    }

    return br_x509_decoder_get_pkey(&self->decoder);
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

// GitHub Pages (Fastly, 2026-11): TLS 1.2, RSA certificate, AES-GCM and ChaCha20 suites only (no CBC), with the
// static RSA key exchange as well. That one is the smallest in code: RSA public key operation only, ~4.5 KB.
// ECDHE (P-256 and the RSA signature check) is another ~12 KB, OTA_FETCH_ECDHE=1 when the server drops static RSA
const uint16_t Suites[] PROGMEM {
#if OTA_FETCH_ECDHE
    BR_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
#endif
    BR_TLS_RSA_WITH_AES_128_GCM_SHA256,
};

// the engine (~4 KB) and its record buffers (~17 KB) are two blocks, a fragmented heap has a better chance
struct Tls {
    br_ssl_client_context sc;
    Insecure x509;
};

constexpr size_t InputSize { BR_SSL_BUFSIZE_INPUT };

class Link {
public:
    Link() = default;
    Link(const Link&) = delete;
    Link& operator=(const Link&) = delete;

    ~Link() {
        close();
    }

    bool open(const Target& target, String& error) {
        _tcp.setTimeout(Timeout);
        _tcp.setNoDelay(true);
        if (!_tcp.connect(target.host.c_str(), target.port)) {
            error = String(F("cannot connect to ")) + target.host;
            return false;
        }

        if (!target.https) {
            return true;
        }

        _tls.reset(new (std::nothrow) Tls);
        _buffers.reset(new (std::nothrow) unsigned char[InputSize + OutputSize]);
        if (!_tls || !_buffers) {
            error = F("out of memory for TLS");
            return false;
        }

        stack_thunk_add_ref();
        _thunk = true;

        auto* eng = &_tls->sc.eng;
        br_ssl_client_zero(&_tls->sc);
        br_ssl_engine_set_versions(eng, BR_TLS12, BR_TLS12);

        uint16_t suites[sizeof(Suites) / sizeof(Suites[0])];
        memcpy_P(suites, Suites, sizeof(Suites));
        br_ssl_engine_set_suites(eng, suites, sizeof(suites) / sizeof(suites[0]));

        br_ssl_client_set_default_rsapub(&_tls->sc);
#if OTA_FETCH_ECDHE
        br_ssl_engine_set_default_rsavrfy(eng);
        br_ssl_engine_set_ec(eng, &br_ec_p256_m15);
#endif
        br_ssl_engine_set_hash(eng, br_sha256_ID, &br_sha256_vtable);
        br_ssl_engine_set_prf_sha256(eng, &br_tls12_sha256_prf);

        // the smallest AES and GHASH of BearSSL (not constant-time, no secret worth a timing attack here)
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

        if (!br_ssl_client_reset(&_tls->sc, target.host.c_str(), 0)) {
            error = tlsError(F("TLS setup"));
            return false;
        }

        // handshake
        if (pump(BR_SSL_SENDAPP) != Pump::Ready) {
            error = tlsError(F("TLS handshake"));
            return false;
        }

        return true;
    }

    bool write(const char* data, size_t len) {
        if (!_tls) {
            return _tcp.write(reinterpret_cast<const uint8_t*>(data), len) == len;
        }

        auto* eng = &_tls->sc.eng;
        while (len) {
            if (pump(BR_SSL_SENDAPP) != Pump::Ready) {
                return false;
            }

            size_t avail;
            auto* buf = br_ssl_engine_sendapp_buf(eng, &avail);
            const size_t size = std::min(avail, len);
            memcpy(buf, data, size);
            thunk_espurna_fetch_sendapp_ack(eng, size);
            data += size;
            len -= size;
        }

        thunk_espurna_fetch_flush(eng, 0);
        return pump(BR_SSL_SENDAPP) == Pump::Ready;
    }

    // > 0 bytes read, 0 - the server closed the connection, < 0 - timeout or error
    int read(uint8_t* out, size_t len) {
        if (!_tls) {
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
        thunk_espurna_fetch_recvapp_ack(eng, size);

        return size;
    }

    void close() {
        _tcp.stop();
        _tls.reset();
        _buffers.reset();
        if (_thunk) {
            stack_thunk_del_ref();
            _thunk = false;
        }
    }

    String tlsError(const __FlashStringHelper* what) const {
        String out(what);
        out += F(" failed, BearSSL error ");
        out += String(_tls ? br_ssl_engine_last_error(&_tls->sc.eng) : -1, 10);
        return out;
    }

private:
    enum class Pump {
        Ready,
        Closed,
        Failed,
    };

    // runs the engine until it is in the `target` state (data can be sent / read), sending and receiving records
    Pump pump(unsigned target) {
        auto* eng = &_tls->sc.eng;
        auto last = millis();

        for (;;) {
            const auto state = br_ssl_engine_current_state(eng);
            if (state & BR_SSL_CLOSED) {
                return (br_ssl_engine_last_error(eng) == BR_ERR_OK)
                    ? Pump::Closed : Pump::Failed;
            }

            if (state & BR_SSL_SENDREC) {
                size_t len;
                auto* buf = br_ssl_engine_sendrec_buf(eng, &len);
                const auto sent = _tcp.write(buf, len);
                if (sent) {
                    thunk_espurna_fetch_sendrec_ack(eng, sent);
                    last = millis();
                    continue;
                }

                if (!_tcp.connected() || (millis() - last > Timeout)) {
                    return Pump::Failed;
                }

                delay(1);
                continue;
            }

            if (state & target) {
                return Pump::Ready;
            }

            if (state & BR_SSL_RECVREC) {
                const int avail = _tcp.available();
                if (avail > 0) {
                    size_t len;
                    auto* buf = br_ssl_engine_recvrec_buf(eng, &len);
                    const int got = _tcp.read(buf, std::min(len, static_cast<size_t>(avail)));
                    if (got > 0) {
                        thunk_espurna_fetch_recvrec_ack(eng, got);
                        last = millis();
                        continue;
                    }
                }

                // closed without close_notify: the end of the body is checked with Content-Length
                if (!_tcp.connected()) {
                    return Pump::Closed;
                }
            }

            if (millis() - last > Timeout) {
                return Pump::Failed;
            }

            delay(1);
        }
    }

    WiFiClient _tcp;
    std::unique_ptr<Tls> _tls;
    std::unique_ptr<unsigned char[]> _buffers;
    bool _thunk { false };
};

// Response head: status line and the two headers that matter
struct Head {
    int status { 0 };
    size_t length { 0 };
    bool length_known { false };
    String location;
};

// Buffered reader of the response, the rest of the buffer after the head is the beginning of the body
class Reader {
public:
    explicit Reader(Link& link) :
        _link(link)
    {}

    bool line(String& out) {
        out = String();
        for (;;) {
            if (_pos >= _len && !fill()) {
                return false;
            }

            const char c = _buffer[_pos++];
            if (c == '\n') {
                if (out.length() && (out[out.length() - 1] == '\r')) {
                    out.remove(out.length() - 1);
                }
                return true;
            }

            if (out.length() > 512) {
                return false;
            }

            out += c;
        }
    }

    bool head(Head& out) {
        String text;
        if (!line(text) || !text.startsWith(F("HTTP/1."))) {
            return false;
        }

        const int space = text.indexOf(' ');
        out.status = (space > 0) ? text.substring(space + 1).toInt() : 0;

        for (;;) {
            if (!line(text)) {
                return false;
            }

            if (!text.length()) {
                return true;
            }

            const int colon = text.indexOf(':');
            if (colon <= 0) {
                continue;
            }

            String name = text.substring(0, colon);
            name.toLowerCase();
            String value = text.substring(colon + 1);
            value.trim();

            if (name.equals(F("content-length"))) {
                out.length = strtoul(value.c_str(), nullptr, 10);
                out.length_known = true;
            } else if (name.equals(F("location"))) {
                out.location = value;
            } else if (name.equals(F("transfer-encoding"))) {
                // never for static files, the length is needed for the update
                out.length_known = false;
            }
        }
    }

    // the body: what is left in the buffer first
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
        const int got = _link.read(reinterpret_cast<uint8_t*>(_buffer), sizeof(_buffer));
        if (got <= 0) {
            return false;
        }

        _pos = 0;
        _len = got;
        return true;
    }

    Link& _link;
    char _buffer[256];
    size_t _pos { 0 };
    size_t _len { 0 };
};

String hex(const uint8_t* data, size_t len) {
    String out;
    out.reserve(len * 2);

    char tmp[3];
    for (size_t index = 0; index < len; ++index) {
        snprintf_P(tmp, sizeof(tmp), PSTR("%02x"), data[index]);
        out += tmp;
    }

    return out;
}

String updateError() {
    PrintString out(64);
    Update.printError(out);
    String result(out.c_str());
    result.trim();
    return result;
}

// The body goes to Update and SHA-256 at once. Update is started for one byte more than the file: it is
// never 'finished' by itself, so a file with the wrong SHA-256 is dropped by end(false) and never booted
bool body(Reader& reader, size_t length, const String& sha256, String& result) {
    // placed at the end of the free space, as far as possible from where eboot unpacks the image (0x0)
    if (!Update.begin(length + 1, U_FLASH)) {
        result = String(F("the file (")) + String(length, 10) + F(" bytes) does not fit, ")
            + String(ESP.getFreeSketchSpace(), 10) + F(" bytes free: ") + updateError();
        return false;
    }

    br_sha256_context sha;
    br_sha256_init(&sha);

    std::unique_ptr<uint8_t[]> buffer(new (std::nothrow) uint8_t[1024]);
    if (!buffer) {
        Update.end(false);
        result = F("out of memory");
        return false;
    }

    size_t done { 0 };
    size_t shown { 0 };
    while (done < length) {
        const int got = reader.read(buffer.get(), std::min<size_t>(1024, length - done));
        if (got <= 0) {
            Update.end(false);
            result = String((got < 0) ? F("timeout") : F("connection closed"))
                + F(" after ") + String(done, 10) + F(" of ") + String(length, 10) + F(" bytes");
            return false;
        }

        if (Update.write(buffer.get(), got) != static_cast<size_t>(got)) {
            result = String(F("write failed: ")) + updateError();
            Update.end(false);
            return false;
        }

        br_sha256_update(&sha, buffer.get(), got);
        done += got;

        if (done - shown >= 65536) {
            DEBUG_MSG_P(PSTR("[OTA] %u of %u bytes\n"), done, length);
            shown = done;
        }
    }

    uint8_t digest[br_sha256_SIZE];
    br_sha256_out(&sha, digest);
    if (!sha256.equals(hex(digest, sizeof(digest)))) {
        Update.end(false);
        result = F("SHA-256 mismatch, not the file of the release list");
        return false;
    }

    if (!Update.end(true)) {
        result = String(F("image check failed: ")) + updateError();
        return false;
    }

    result = String(F("downloaded ")) + String(done, 10) + F(" bytes, SHA-256 checked");
    return true;
}

// a redirect target: absolute, or a path on the same server
String redirect(const Target& from, const String& location) {
    if (location.startsWith(F("https://")) || location.startsWith(F("http://"))) {
        return location;
    }

    if (location.startsWith(F("/"))) {
        String out(from.https ? F("https://") : F("http://"));
        out += from.host;
        if (from.port != (from.https ? 443 : 80)) {
            out += ':';
            out += String(from.port, 10);
        }
        out += location;
        return out;
    }

    return String();
}

bool download(String url, const String& sha256, String& result) {
    for (int redirects = 0; redirects < 4; ++redirects) {
        Target target;
        if (!parse(url, target)) {
            result = F("invalid URL");
            return false;
        }

        DEBUG_MSG_P(PSTR("[OTA] Downloading %s\n"), url.c_str());

        Link link;
        if (!link.open(target, result)) {
            return false;
        }

        String request;
        request.reserve(target.path.length() + target.host.length() + 80);
        request += F("GET ");
        request += target.path;
        request += F(" HTTP/1.1\r\nHost: ");
        request += target.host;
        request += F("\r\nUser-Agent: ESPurna\r\nConnection: close\r\n\r\n");
        if (!link.write(request.c_str(), request.length())) {
            result = F("cannot send the request");
            return false;
        }

        Reader reader(link);
        Head head;
        if (!reader.head(head)) {
            result = F("no HTTP response");
            return false;
        }

        if ((head.status >= 300) && (head.status < 400) && head.location.length()) {
            url = redirect(target, head.location);
            continue;
        }

        if (head.status != 200) {
            result = String(F("HTTP ")) + String(head.status, 10);
            return false;
        }

        if (!head.length_known || !head.length) {
            result = F("no Content-Length in the response");
            return false;
        }

        return body(reader, head.length, sha256, result);
    }

    result = F("too many redirects");
    return false;
}

// -----------------------------------------------------------------------------
// at boot
// -----------------------------------------------------------------------------

constexpr unsigned long WifiWait { 3 * 60 * 1000 };
constexpr unsigned long Settle { 3000 };

struct Pending {
    String url;
    String sha256;
    bool active { false };
    bool forced { false };
    unsigned long start { 0 };
    unsigned long connected { 0 };
};

Pending pending;

void finish(bool ok, const String& message) {
    pending.active = false;

    storeResult(ok ? (String(F("ok: ")) + message) : (String(F("failed: ")) + message));
    DEBUG_MSG_P(PSTR("[OTA] %s %s\n"), ok ? PSTR("Done:") : PSTR("Failed:"), message.c_str());

    if (!ok) {
        eepromRotate(true);
    }

#if SYSTEM_CHECK_ENABLED
    // back to the normal mode with the next boot
    if (pending.forced) {
        systemForceStable();
    }
#endif

    if (ok) {
        prepareReset(CustomResetReason::Ota);
    } else if (pending.forced) {
        prepareReset(CustomResetReason::Stability);
    }
}

void loop() {
    if (!pending.active) {
        return;
    }

    if (!wifiConnected()) {
        pending.connected = 0;
        if (pending.forced && (millis() - pending.start > WifiWait)) {
            finish(false, F("no Wi-Fi connection"));
        }
        return;
    }

    if (!pending.connected) {
        pending.connected = millis();
        return;
    }

    if (millis() - pending.connected < Settle) {
        return;
    }

    pending.active = false;

    // Update writes over the second EEPROM rotation sector at the end of the free space (1 MB layout)
    eepromRotate(false);

    String message;
    const bool ok = download(pending.url, pending.sha256, message);
    pending.url = String();
    finish(ok, message);
}

#endif // OTA_FETCH_SUPPORT

// The request is taken once, a broken URL never keeps the board busy over the next boots
void setup() {
    const auto url = getSetting(FPSTR(keys::Url));
    internal::result = getSetting(FPSTR(keys::Result));
    if (!url.length()) {
        return;
    }

    const auto sha256 = getSetting(FPSTR(keys::Sha256));
    delSetting(FPSTR(keys::Url));
    delSetting(FPSTR(keys::Sha256));

#if OTA_FETCH_SUPPORT
    eepromCommit();

    pending.url = url;
    pending.sha256 = sha256;
    pending.active = true;
    pending.start = millis();

#if WEB_SUPPORT && SYSTEM_CHECK_ENABLED
    // full image: only Wi-Fi and telnet in this boot, the heap stays free for TLS
    systemForceUnstable();
    pending.forced = true;
#endif

    DEBUG_MSG_P(PSTR("[OTA] Update from %s once Wi-Fi is up\n"), url.c_str());
    espurnaRegisterLoop(loop);
#else
    // stored for the minimal image, which never started (its upload failed)
    storeResult(F("failed: the minimal image did not start"));
#endif
}

} // namespace
} // namespace fetch
} // namespace ota
} // namespace espurna

bool otaFetchRequest(const String& url, const String& sha256, String& error) {
    using namespace espurna::ota::fetch;

    if (!validUrl(url)) {
        error = F("invalid URL (http:// or https://, up to 240 characters)");
        return false;
    }

    if (!validSha256(sha256)) {
        error = F("invalid SHA-256 (64 lower case hex digits)");
        return false;
    }

    setSetting(FPSTR(keys::Url), url);
    setSetting(FPSTR(keys::Sha256), sha256);
    delSetting(FPSTR(keys::Result));
    internal::result = String();
    eepromCommit();

    DEBUG_MSG_P(PSTR("[OTA] Stored %s\n"), url.c_str());
    return true;
}

bool otaFetchSelf() {
    return OTA_FETCH_SUPPORT;
}

String otaFetchStatus() {
    using namespace espurna::ota::fetch;

    String out;
    out.reserve(160 + UrlMax);
    out += F("{\"image\":\"" ESPURNA_IMAGE "\",\"fetch\":");
    out += otaFetchSelf() ? '1' : '0';
    out += F(",\"free\":");
    out += String(ESP.getFreeSketchSpace(), 10);
    out += F(",\"url\":");
    jsonString(out, getSetting(FPSTR(keys::Url)));
    out += F(",\"result\":");
    jsonString(out, internal::result);
    out += '}';

    return out;
}

String otaFetchResult() {
    return espurna::ota::fetch::internal::result;
}

void otaFetchSetup() {
    espurna::ota::fetch::setup();
}

#endif
