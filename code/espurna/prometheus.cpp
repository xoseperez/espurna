/*

PROMETHEUS METRICS MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if WEB_SUPPORT && PROMETHEUS_SUPPORT

#include "prometheus.h"

#include "api.h"
#include "relay.h"
#include "sensor.h"
#include "web.h"

#include <cmath>

void _prometheusRequestHandler(AsyncWebServerRequest* request) {
    static_assert(RELAY_SUPPORT || SENSOR_SUPPORT, "");

    // TODO: Add more stuff?
    // Note: Response 'stream' backing buffer is customizable. Default is 1460 bytes (see ESPAsyncWebServer.h)
    //       In case printf overflows, memory of CurrentSize+N{overflow} will be allocated to replace
    //       the existing buffer. Previous buffer will be copied into the new and destroyed after that.
    AsyncResponseStream *response = request->beginResponseStream("text/plain");

    #if RELAY_SUPPORT
        for (unsigned char index = 0; index < relayCount(); ++index) {
            response->printf("relay%u %d\n", index, static_cast<int>(relayStatus(index)));
        }
    #endif

    #if SENSOR_SUPPORT
        char buffer[64] { 0 };
        for (unsigned char index = 0; index < magnitudeCount(); ++index) {
            auto value = magnitudeValue(index);
            if (std::isnan(value.get()) || std::isinf(value.get())) {
                continue;
            }

            String topic(magnitudeTopicIndex(index));
            topic.replace("/", "");

            magnitudeFormat(value, buffer, sizeof(buffer));
            response->printf("%s %s\n", topic.c_str(), buffer);
        }
    #endif

    response->write('\n');

    request->send(response);
}

bool _prometheusRequestCallback(AsyncWebServerRequest* request) {
    if (request->url().equals(F("/api/metrics"))) {
        webLog(request);
        if (apiAuthenticate(request)) {
            _prometheusRequestHandler(request);
        }
        return true;
    }

    return false;
}

void prometheusSetup() {
    webRequestRegister(_prometheusRequestCallback);
}

#endif // PROMETHEUS_SUPPORT
