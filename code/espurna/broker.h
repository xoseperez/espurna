/*

BROKER MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <functional>
#include <utility>
#include <vector>
#include <tuple>

// Example usage:
//
// module.h
// BrokerDeclare(CustomBroker, void(int));
//
// module.cpp
// BrokerBind(CustomBroker);
//
// other.cpp
// #include "module.h"
// void func() {
//     CustomBroker::Register([](int arg) { Serial.println(arg); }
//     CustomBroker::Publish(12345);
// }

template <typename Func>
struct TBroker {};

template <typename R, typename ...Args>
struct TBroker<R(Args...)> {

    using TArgs = typename std::tuple<Args...>;
    using TCallback = std::function<R(Args...)>;
    using TCallbacks = std::vector<TCallback>;

    TBroker(const TBroker&) = delete;
    TBroker& operator=(const TBroker&) = delete;

    TBroker() = default;

    // TODO: https://source.chromium.org/chromium/chromium/src/+/master:base/callback_list.h
    // Consider giving out 'subscription' / 'token', so that the caller can remove callback later

    void Register(TCallback callback) {
        callbacks.push_back(callback);
    }

    void Publish(Args... args) {
        for (auto& callback : callbacks) {
            callback(args...);
        }
    }

    protected:

    TCallbacks callbacks;

};

// TODO: since 1.14.0 we intoduced static syntax for Brokers, ::Register & ::Publish.
// Preserve it (up to a point) when creating module-level objects.
// Provide a helper namespace with Register & Publish, instance and
// To help out VS Code with argument discovery, put TArgs as the first template parameter.

#define BrokerDeclare(Name, Signature) \
namespace Name { \
using type = TBroker<Signature>; \
extern type Instance; \
template<typename S = type::TArgs, typename ...Args> \
inline void Register(Args&&... args) { \
    Instance.Register(std::forward<Args>(args)...); \
}\
\
template<typename S = type::TArgs, typename ...Args> \
inline void Publish(Args&&... args) { \
    Instance.Publish(std::forward<Args>(args)...); \
}\
}

#define BrokerBind(Name) \
namespace Name { \
Name::type Instance; \
}

