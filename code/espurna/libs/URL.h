// -----------------------------------------------------------------------------
// Parse char string as URL
//
// Adapted from HTTPClient::beginInternal()
// https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/src/ESP8266HTTPClient.cpp
//
// -----------------------------------------------------------------------------

#pragma once

#include <Arduino.h>

#include <cstdint>
#include <utility>

#include "../types.h"

class URL {
public:
    URL() = default;
    URL(const URL&) = default;
    URL(URL&&) = default;

    URL& operator=(const URL&) = default;
    URL& operator=(URL&&) = default;

    explicit URL(espurna::StringView string) {
        _parse(string);
    }

    String protocol;
    String host;
    String path;
    uint16_t port { 0 };

private:
    void _parse(espurna::StringView string) {
        auto buffer = string.toString();

        int index = buffer.indexOf("://");
        if (index > 0) {
            this->protocol = buffer.substring(0, index);
            buffer.remove(0, (index + 3));
        }

        if (this->protocol == "http") {
            this->port = 80;
        } else if (this->protocol == "https") {
            this->port = 443;
        }

        // cut the host part
        String _host;

        index = buffer.indexOf('/');
        if (index >= 0) {
            _host = buffer.substring(0, index);
        } else {
            _host = buffer;
        }

        // store the remaining part as path
        if (index >= 0) {
            buffer.remove(0, index);
            this->path = buffer;
        } else {
            this->path = "/";
        }

        // separate host from port, when present
        index = _host.indexOf(':');
        if (index >= 0) {
            this->port = _host.substring(index + 1).toInt();
            this->host = _host.substring(0, index);
        } else {
            this->host = _host;
        }
    }
};
