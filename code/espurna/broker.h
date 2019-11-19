/*

BROKER MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if BROKER_SUPPORT

#pragma once

#include <functional>
#include <vector>
#include <utility>

enum class TBrokerType {
    SYSTEM,
    STATUS,
    SENSOR,
    DATETIME,
    CONFIG
};

template <typename... TArgs>
using TBrokerCallback = std::function<void(TArgs...)>;

template <typename... TArgs>
using TBrokerCallbacks = std::vector<TBrokerCallback<TArgs...>>;

template <TBrokerType type, typename... TArgs>
struct TBroker {
    using callback_t = TBrokerCallback<TArgs...>;
    using callbacks_t = TBrokerCallbacks<TArgs...>;

    static callbacks_t callbacks;
    static void Register(callback_t callback) {
        callbacks.push_back(callback);
    }
    static void Publish(TArgs... args) {
        for (auto& callback : callbacks) {
            callback(args...);
        }
    }
};

template <TBrokerType type, typename... TArgs>
TBrokerCallbacks<TArgs...> TBroker<type, TArgs...>::callbacks;


// --- Some known types. Bind them here to avoid .ino screwing with order ---

using StatusBroker = TBroker<TBrokerType::STATUS, const String&, unsigned char, unsigned int>;
using SensorBroker = TBroker<TBrokerType::STATUS, const String&, unsigned char, double, const char*>;
using TimeBroker = TBroker<TBrokerType::DATETIME, const String&, time_t, const String&>;
using ConfigBroker = TBroker<TBrokerType::CONFIG, const String&, const String&>;

#endif // BROKER_SUPPORT
