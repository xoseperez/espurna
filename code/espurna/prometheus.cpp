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

namespace espurna {
namespace prometheus {
namespace build {
namespace {

constexpr bool relaySupport() {
    return RELAY_SUPPORT == 1;
}

constexpr bool sensorSupport() {
    return SENSOR_SUPPORT == 1;
}

static_assert(relaySupport() || sensorSupport(), "");

} // namespace
} // namespace build

namespace {

void handler(AsyncWebServerRequest* request) {

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
        for (unsigned char index = 0; index < magnitudeCount(); ++index) {
            auto value = magnitudeValue(index);
            if (std::isnan(value.get()) || std::isinf(value.get())) {
                continue;
            }

            String topic(magnitudeTopicIndex(index));
            topic.replace("/", "");

            response->printf("%s %s\n",
                topic.c_str(),
                value.toString().c_str());
        }
    #endif

    response->write('\n');

    request->send(response);
}

void setup() {
#if API_SUPPORT
    apiRegister(F("metrics"),
        [](ApiRequest& request) {
            request.handle(handler);
            return true;
        },
        nullptr
    );
#else
    webRequestRegister([](AsyncWebServerRequest* request) {
        if (request->url().equals(F(API_BASE_PATH "metrics"))) {
            if (apiAuthenticate(request)) {
                handler(request);
                return true;
            }
            request->send(403);
            return true;
        }

        return false;
    });
#endif
}

} // namespace
} // namespace prometheus
} // namespace espurna

void prometheusSetup() {
    espurna::prometheus::setup();
}

#endif // PROMETHEUS_SUPPORT
