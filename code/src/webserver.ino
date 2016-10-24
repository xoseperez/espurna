/*

ESPurna
WEBSERVER MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "FS.h"

ESP8266WebServer server(80);

// -----------------------------------------------------------------------------
// WEBSERVER
// -----------------------------------------------------------------------------

String getContentType(String filename) {
    if (server.hasArg("download")) return "application/octet-stream";
    else if (filename.endsWith(".htm")) return "text/html";
    else if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".xml")) return "text/xml";
    else if (filename.endsWith(".pdf")) return "application/x-pdf";
    else if (filename.endsWith(".zip")) return "application/x-zip";
    else if (filename.endsWith(".gz")) return "application/x-gzip";
    return "text/plain";
}

bool handleFileRead(String path) {

    DEBUG_MSG("[WEBSERVER] Request: %s\n", (char *) path.c_str());

    if (path.endsWith("/")) path += "index.html";
    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";
    if (SPIFFS.exists(pathWithGz)) path = pathWithGz;

    if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        size_t sent = server.streamFile(file, contentType);
        size_t contentLength = file.size();
        file.close();
        return true;
    }

    return false;

}

void webServerSetup() {

    // Anything else
    server.onNotFound([]() {

        // Hidden files
        #ifndef DEBUG_PORT
            if (server.uri().startsWith("/.")) {
                server.send(403, "text/plain", "Forbidden");
                return;
            }
        #endif

        // Existing files in SPIFFS
        if (!handleFileRead(server.uri())) {
            server.send(404, "text/plain", "NotFound");
            return;
        }

    });

    // Run server
    server.begin();

}

void webServerLoop() {
    server.handleClient();
}
