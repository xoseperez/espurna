/*

Part of the WEBSERVER MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "web.h"
#include "api.h"
#include "libs/TypeChecks.h"

namespace espurna {
namespace web {
namespace print {
namespace traits {

template <typename T>
using print_callable_t = decltype(std::declval<T>()(std::declval<Print&>()));

template <typename T>
using is_print_callable = is_detected<print_callable_t, T>;

} // namespace traits

void RequestPrint::_onDisconnect() {
#if API_SUPPORT
    // TODO: in case this comes from `apiRegister`'ed endpoint, there's still a lingering ApiRequestHelper that we must remove
    auto* req = request();
    if (req->_tempObject) {
        auto* ptr = reinterpret_cast<espurna::api::Request*>(req->_tempObject);
        delete ptr;
        req->_tempObject = nullptr;
    }
#endif
    state(State::Done);
}

template <typename CallbackType>
void RequestPrint::_callback(CallbackType&& callback) {
    if (State::None != state()) {
        return;
    }

    callback(*this);
    flush();
}

template<typename CallbackType>
void RequestPrint::scheduleFromRequest(Config config, AsyncWebServerRequest* request, CallbackType callback) {
    static_assert(
        traits::is_print_callable<CallbackType>::value,
        "CallbackType needs to be a callable with void(Print&)");

    // because of async nature of the server, we need to make sure we outlive 'request' object
    auto print = std::shared_ptr<RequestPrint>(
        new RequestPrint(config, request));

    // attach one ptr to onDisconnect capture, so we can detect disconnection before scheduled function runs
    request->onDisconnect(
        [print]() {
            print->_onDisconnect();
        });

    // attach another capture to the scheduled function, so we execute as soon as we exit next loop()
    espurnaRegisterOnce(
        [callback, print]() {
            print->_callback(callback);
        });
}

static constexpr auto DefaultConfig = Config{
    .mimeType = "text/plain",
    .backlog = {
        .count = 2,
        .size = TCP_MSS,
        .timeout = duration::Seconds(5)
    },
};

template <typename CallbackType>
void RequestPrint::scheduleFromRequest(AsyncWebServerRequest* request, CallbackType callback) {
    RequestPrint::scheduleFromRequest(DefaultConfig, request, callback);
}

} // namespace print
} // namespace web
} // namespace espurna
