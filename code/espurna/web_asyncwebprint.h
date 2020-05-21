/*

Part of the WEBSERVER MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "web.h"

#if WEB_SUPPORT

#include "libs/TypeChecks.h"

namespace web_type_traits {

template <typename T>
using has_Print_argument_t = decltype(std::declval<T>()(std::declval<Print&>()));

template <typename T>
using has_Print_argument = is_detected<has_Print_argument_t, T>;

}

template<typename CallbackType>
void AsyncWebPrint::scheduleFromRequest(const AsyncWebPrintConfig& config, AsyncWebServerRequest* request, CallbackType callback) {
    static_assert(web_type_traits::has_Print_argument<CallbackType>::value, "CallbackType signature needs to match R(Print&)");
    // because of async nature of the server, we need to make sure we outlive 'request' object
    auto print = std::shared_ptr<AsyncWebPrint>(new AsyncWebPrint(config, request));

    // attach one ptr to onDisconnect capture, so we can detect disconnection before scheduled function runs
    request->onDisconnect([print]() {
        print->setState(AsyncWebPrint::State::Done);
    });

    // attach another capture to the scheduled function, so we execute as soon as we exit next loop()
    schedule_function([callback, print]() {
        if (State::None != print->getState()) return;
        callback(*print.get());
        print->flush();
    });
}

constexpr AsyncWebPrintConfig AsyncWebPrintDefaults {
    /*mimeType       =*/ "text/plain",
    /*backlogCountMax=*/ 2,
    /*backlogSizeMax= */ TCP_MSS,
    /*backlogTimeout= */ 5000
};

template<typename CallbackType>
void AsyncWebPrint::scheduleFromRequest(AsyncWebServerRequest* request, CallbackType callback) {
    static_assert(web_type_traits::has_Print_argument<CallbackType>::value, "CallbackType signature needs to match R(Print&)");
    AsyncWebPrint::scheduleFromRequest(AsyncWebPrintDefaults, request, callback);
}

#endif
