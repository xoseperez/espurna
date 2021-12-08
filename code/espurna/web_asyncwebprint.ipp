/*

Part of the WEBSERVER MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "web.h"
#include "libs/TypeChecks.h"

#include <Schedule.h>

#if WEB_SUPPORT

namespace asyncwebprint {
namespace traits {

template <typename T>
using print_callable_t = decltype(std::declval<T>()(std::declval<Print&>()));

template <typename T>
using is_print_callable = is_detected<print_callable_t, T>;

}
}

template<typename CallbackType>
void AsyncWebPrint::scheduleFromRequest(AsyncWebPrintConfig config, AsyncWebServerRequest* request, CallbackType callback) {
    static_assert(asyncwebprint::traits::is_print_callable<CallbackType>::value, "CallbackType needs to be a callable with void(Print&)");

    // because of async nature of the server, we need to make sure we outlive 'request' object
    auto print = std::shared_ptr<AsyncWebPrint>(new AsyncWebPrint(config, request));

    // attach one ptr to onDisconnect capture, so we can detect disconnection before scheduled function runs
    request->onDisconnect([print, request]() {
#if API_SUPPORT
        // TODO: in case this comes from `apiRegister`'ed endpoint, there's still a lingering ApiRequestHelper that we must remove
        if (request->_tempObject) {
            auto* ptr = reinterpret_cast<ApiRequestHelper*>(request->_tempObject);
            delete ptr;
            request->_tempObject = nullptr;
        }
#endif
        print->setState(AsyncWebPrint::State::Done);
    });

    // attach another capture to the scheduled function, so we execute as soon as we exit next loop()
    schedule_function([callback, print]() {
        if (State::None != print->getState()) return;
        callback(*print.get());
        print->flush();
    });
}

static constexpr AsyncWebPrintConfig AsyncWebPrintDefaults {
    .mimeType = "text/plain",
    .backlog = {
        .count = 2,
        .size = TCP_MSS,
        .timeout = espurna::duration::Seconds(5)
    }
};

template<typename CallbackType>
void AsyncWebPrint::scheduleFromRequest(AsyncWebServerRequest* request, CallbackType callback) {
    AsyncWebPrint::scheduleFromRequest(AsyncWebPrintDefaults, request, callback);
}

#endif
