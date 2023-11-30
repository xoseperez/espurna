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
    return 1 == RELAY_SUPPORT;
}

constexpr bool sensorSupport() {
    return 1 == SENSOR_SUPPORT;
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
    auto *response = request->beginResponseStream("text/plain");

    if (build::relaySupport()) {
        for (size_t index = 0; index < relayCount(); ++index) {
            response->printf_P(PSTR("relay%u %d\n"), index, relayStatus(index) ? 1 : 0);
        }
    }

    if (build::sensorSupport()) {
        for (size_t index = 0; index < magnitudeCount(); ++index) {
            auto value = magnitudeValue(index);
            if (value) {
                value.topic.replace("/", "");
                response->printf_P(PSTR("%s %s\n"),
                    value.topic.c_str(), value.repr.c_str());
            }
        }
    }

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
