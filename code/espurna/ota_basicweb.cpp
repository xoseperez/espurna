/*

Part of the OTA MODULE

Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if !WEB_SUPPORT && OTA_WEB_SUPPORT

// When there's no WEB_SUPPORT, enable a basic server with a form upload and /upgrade endpoint.
// Based on the async-web-server code and the ESP8266HTTPUpdateServer.h bundled with the Core
// effectively repeats what is done in the async variant, but does not involve async-web-server
// (...but, still suffers from a similar std::function API, forcing to self-reference the server object and manage various global state & result objects)
//

#include "espurna_ota.h"

#include <ESP8266WebServer.h>
#include <StreamString.h>

namespace espurna {
namespace ota {
namespace basic_web {
namespace {

// Minimal image (1 MB boards): the release list comes from firmware.json on GitHub Pages (read by the browser,
// Access-Control-Allow-Origin: *), the board downloads the picked .bin.gz itself (/ota/fetch, ota_fetch.cpp). The full
// images of the same flash size are listed, with their SHA-256. Or a file upload, as before
const char HomePageHead[] PROGMEM = R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>ESPurna</title>
<style>body{font-family:sans-serif;max-width:640px;margin:16px auto;padding:0 16px}fieldset{margin-top:12px}
input[type=text]{width:100%;box-sizing:border-box;margin:2px 0}button{margin:3px 0}#log{margin-top:12px}</style>
</head>
<body>
<h3 id="h"></h3>
<div id="info"></div>
<fieldset><legend>Releases</legend><div id="rel">Reading the release list...</div></fieldset>
<fieldset><legend>Download</legend>
<input type="text" id="url" placeholder="https://.../espurna-esp8266-1m.bin.gz">
<input type="text" id="sha" placeholder="SHA-256 of the file">
<button id="go">Download and install</button></fieldset>
<form method="POST" action="/upgrade" enctype="multipart/form-data">
<fieldset>
<legend>Firmware upgrade</legend>
<div>
<input type="file" name="filename">
</div>
<div>
<button type="submit">Upload</button>
</div>
</fieldset>
</form>
<div id="log"></div>
<script>
)";

const char HomePageScript[] PROGMEM = R"(
const $=i=>document.getElementById(i),log=t=>{$('log').textContent=t};
$('h').textContent=S.host+' - '+(S.image||'minimal image');
$('info').textContent=S.result?'Last download: '+S.result:'';
async function go(u,h){if(!confirm('Download and install '+u+' ?'))return;
log('Restarting, then downloading (about a minute)...');
try{const r=await fetch('/ota/fetch',{method:'POST',body:new URLSearchParams({url:u,sha256:h})});
log(await r.text());if(r.ok)setTimeout(()=>location.reload(),90000)}catch(e){log(String(e))}}
$('go').onclick=()=>go($('url').value.trim(),$('sha').value.trim().toLowerCase());
fetch(M,{cache:'no-store'}).then(r=>r.json()).then(m=>{const d=$('rel');d.textContent='';
for(const r of m.releases||[])for(const n of I)for(const f of (r.images||{})[n]||[]){
if(f.kind!=='ota'||!f.sha256)continue;
const ok=Math.ceil((f.size+1)/4096)*4096<=S.free,b=document.createElement('button');
b.textContent=r.tag+': '+f.name+' ('+Math.round(f.size/1024)+' KB)'+(ok?'':', does not fit');b.disabled=!ok;
b.onclick=()=>go(new URL(f.url,M).href,f.sha256);d.append(b,document.createElement('br'))}
if(!d.childNodes.length)d.textContent='No '+I.join(' / ')+' image in '+M})
.catch(e=>{$('rel').textContent='Cannot read '+M+': '+e});
</script>
</body>
</html>
)";

const char UpgradePageHead[] PROGMEM = R"(
<head>
<meta http-equiv="refresh" content="5; url=./">
</head>
)";

namespace build {

// full images of the minimal one's flash size, ref. TWO_STEP of html/src/fwinstall.mjs
PROGMEM_STRING(Images, "['esp8266-1m','esp8266-1m-max']");

PROGMEM_STRING(Manifest, "https://igorufox.github.io/espurna/firmware.json");

} // namespace build

namespace handlers {
namespace internal {

bool denied { false };

struct Result {
    template <typename T>
    void set(int code, T&& value) {
        _code = code;
        _output = std::forward<T>(value);
    }

    void setFromUpdate() {
        if (Update.hasError()) {
            StreamString stream;
            Update.printError(stream);
            set(500, stream);
        }
    }

    template <typename Server>
    void send(Server& server) {
        String output;
        output.reserve(_output.length() + sizeof(UpgradePageHead) + 16);

        output.concat(UpgradePageHead, sizeof(UpgradePageHead) - 1);
        output += F("<code>");
        output += _output;
        output += F("</code>");

        server.send(_code, PSTR("text/html"), output);
    }

    void reset() {
        _code = 200;
        _output = "";
    }

private:
    int _code { 200 };
    String _output;
};

Result result;

} // namespace internal

// same user and realm as the WebUI of the full image, the browser keeps using its credentials
template <typename Server>
bool authenticated(Server& server) {
    const auto password = systemPassword();
    if (server.authenticate("admin", password.c_str())) {
        return true;
    }

    const auto realm = systemHostname();
    server.requestAuthentication(DIGEST_AUTH, realm.c_str());
    return false;
}

template <typename Server>
void result(Server& server) {
    if (internal::denied) {
        internal::denied = false;
        authenticated(server);
        return;
    }

    internal::result.send(server);
}

template <typename Server>
void upload(Server& server) {
    auto& upload = server.upload();

    switch (upload.status) {

    case UPLOAD_FILE_START: {
        if (Update.isRunning()) {
            server.client().stop();
            return;
        }

        internal::result.reset();
        internal::denied = !server.authenticate("admin", systemPassword().c_str());
        if (internal::denied) {
            return;
        }

        const size_t Available { (ESP.getFreeSketchSpace() - 0x1000ul) & 0xfffff000ul };
        if (!Update.begin(Available, U_FLASH)) {
            server.client().stop();
            internal::result.set(500, F("Not enough available space"));
            eepromRotate(true);
        }
        break;
    }

    case UPLOAD_FILE_END:
        if (internal::denied) {
            break;
        }

        // Request body is the file plus a few hundred bytes of multipart framing. When data is lost
        // below HTTP, the form still ends at the final boundary, only with a shorter file. Anything
        // close to a TCP segment missing means the image is broken, never flash it
        if (upload.contentLength > (upload.totalSize + 1024ul)) {
            internal::result.set(400, F("Incomplete upload"));
            Update.end();
            eepromRotate(true);
            break;
        }

        if (otaFinalize(upload.totalSize, CustomResetReason::Ota, true)) {
            internal::result.set(200, F("Rebooting..."));
        } else {
            internal::result.setFromUpdate();
        }
        break;

    case UPLOAD_FILE_WRITE:
        if (!Update.isRunning()) {
            return;
        }

        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            internal::result.set(500, F("Error during write()"));
            server.client().stop();
            Update.end();
            eepromRotate(true);
            return;
        }

        break;

    case UPLOAD_FILE_ABORTED:
        if (internal::denied) {
            break;
        }

        internal::result.set(500, F("Upload aborted"));
        Update.end();
        eepromRotate(true);
        break;

    }
}

template <typename Server>
void home(Server& server) {
    if (!authenticated(server)) {
        return;
    }

    String page;
    page.reserve(sizeof(HomePageHead) + sizeof(HomePageScript) + 512);
    page.concat(HomePageHead, sizeof(HomePageHead) - 1);

    // const S={...status, "host": ...}, I=[images], M="firmware.json URL";
    auto status = otaFetchStatus();
    status.remove(status.length() - 1);
    page += F("const S=");
    page += status;
    page += F(",\"host\":\"");
    page += systemHostname();
    page += F("\"},I=");
    page += FPSTR(build::Images);
    page += F(",M=\"");

    // same setting as in the WebUI of the full image (ADMIN)
    auto manifest = getSetting("webFwUrl", FPSTR(build::Manifest));
    if (!manifest.startsWith(F("https://")) || (manifest.indexOf('"') >= 0) || (manifest.indexOf('<') >= 0)) {
        manifest = FPSTR(build::Manifest);
    }
    page += manifest;
    page += F("\";");

    page.concat(HomePageScript, sizeof(HomePageScript) - 1);
    server.send(200, PSTR("text/html"), page);
}

template <typename Server>
void status(Server& server) {
    if (authenticated(server)) {
        server.send(200, PSTR("application/json"), otaFetchStatus());
    }
}

// the request is stored and taken at boot, once
template <typename Server>
void fetch(Server& server) {
    if (!authenticated(server)) {
        return;
    }

    String error;
    if (!otaFetchRequest(server.arg(F("url")), server.arg(F("sha256")), error)) {
        server.send(400, PSTR("text/plain"), String(F("ERROR: ")) + error);
        return;
    }

    server.send(200, PSTR("text/plain"), String(F("OK, fetch")));
    prepareReset(CustomResetReason::Web);
}

} // namespace handlers

template <typename Server>
void begin(Server& server, uint16_t port) {
    server.on("/", HTTP_GET, [&]() {
        handlers::home(server);
    });

    server.on("/ota/status", HTTP_GET, [&]() {
        handlers::status(server);
    });

    server.on("/ota/fetch", HTTP_POST, [&]() {
        handlers::fetch(server);
    });

    server.on("/upgrade", HTTP_POST,
        [&]() {
            handlers::result(server);
        },
        [&]() {
            handlers::upload(server);
        }
    );

    server.begin(port);
}

namespace build {

constexpr auto DefaultPort = uint16_t{
#if WEB_SSL_ENABLED
        443
#else
        80
#endif
};

constexpr uint16_t port() {
    return (WEB_PORT == 0)
        ? DefaultPort
        : (WEB_PORT);
}

} // namespace build

namespace settings {
namespace keys {

STRING_VIEW_INLINE(Port, "webPort");

} // namespace keys

uint16_t port() {
    return getSetting(keys::Port, build::port());
}

} // namespace settings

namespace internal {

ESP8266WebServer server;

} // namespace internal

void setup() {
    begin(internal::server, settings::port());
    ::espurnaRegisterLoop([]() {
        internal::server.handleClient();
    });
}

} // namespace
} // namespace basic_web
} // namespace ota
} // namespace espurna

void otaWebSetup() {
    espurna::ota::basic_web::setup();
}

#endif

