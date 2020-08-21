/*

PROMETHEUS METRICS MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if WEB_SUPPORT && PROMETHEUS_SUPPORT

#include "prometheus.h"

#include "relay.h"
#include "sensor.h"
#include "web.h"

void _prometheusRequestHandler(AsyncWebServerRequest* request) {
    static_assert(RELAY_SUPPORT || SENSOR_SUPPORT, "");

    AsyncResponseStream *response = request->beginResponseStream("text/plain");

    #if RELAY_SUPPORT
        for (unsigned char index = 0; index < relayCount(); ++index) {
            response->printf("relay%u %d\n", index, static_cast<int>(relayStatus(index)));
        }
    #endif

    #if SENSOR_SUPPORT
        char buffer[64] { 0 };
        for (unsigned char index = 0; index < magnitudeCount(); ++index) {
            String topic(magnitudeTopicIndex(index));
            topic.replace("/", "");

            dtostrf(magnitudeValue(index), 1, 3, buffer);

            response->printf("%s %s\n", topic.c_str(), buffer);
        }
    #endif

    response->write('\n');

    request->send(response);
}

bool _prometheusRequestCallback(AsyncWebServerRequest* request) {
    if (request->url().equals(F("/metrics"))) {
        _prometheusRequestHandler(request);
        return true;
    }

    return false;
}

void prometheusSetup() {
    webRequestRegister(_prometheusRequestCallback);
}

#endif // PROMETHEUS_SUPPORT
