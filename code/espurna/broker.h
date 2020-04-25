/*

BROKER MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#include <functional>
#include <vector>
#include <utility>

enum class TBrokerType {
    System,
    Status,
    SensorRead,
    SensorReport,
    Datetime,
    Config
};

template <typename... TArgs>
using TBrokerCallback = std::function<void(TArgs...)>;

template <typename... TArgs>
using TBrokerCallbacks = std::vector<TBrokerCallback<TArgs...>>;

template <TBrokerType type, typename... TArgs>
struct TBroker {
    static TBrokerCallbacks<TArgs...> callbacks;

    static void Register(TBrokerCallback<TArgs...> callback) {
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

using StatusBroker = TBroker<TBrokerType::Status, const String&, unsigned char, unsigned int>;

using SensorReadBroker = TBroker<TBrokerType::SensorRead, const String&, unsigned char, double, const char*>;
using SensorReportBroker = TBroker<TBrokerType::SensorReport, const String&, unsigned char, double, const char*>;

using ConfigBroker = TBroker<TBrokerType::Config, const String&, const String&>;
