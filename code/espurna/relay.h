/*

RELAY MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <Arduino.h>

#include <cstdint>
#include <memory>

#include "system.h"
#include "rpc.h"

constexpr size_t RelaysMax { 32ul };

enum class RelayBoot {
    Off,
    On,
    Same,
    Toggle,
    LockedOff,
    LockedOn
};

enum class RelayLock {
    None,
    Off,
    On
};

enum class RelayType {
    Normal,
    Inverse,
    Latched,
    LatchedInverse
};

enum class RelayMqttTopicMode {
    Normal,
    Inverse
};

enum class RelayProvider {
    None,
    Dummy,
    Gpio,
    Dual,
    Stm
};

enum class RelaySync {
    None,
    ZeroOrOne,
    JustOne,
    All,
    First
};

class RelayProviderBase {
public:
    RelayProviderBase() = default;
    virtual ~RelayProviderBase();

    virtual void dump();

    // whether the provider is ready
    virtual bool setup();

    // status requested at boot
    virtual void boot(bool status);

    // when 'status' was requested, but target status remains the same or is canceled
    virtual void notify(bool status);

    // when relay 'status' is changed from target to current
    virtual void change(bool status) = 0;

    // unique id of the provider
    virtual const char* id() const = 0;
};

PayloadStatus relayParsePayload(const char * payload);

bool relayStatus(size_t id, bool status, bool report, bool group_report);
bool relayStatus(size_t id, bool status);

// gets either current or target status, where current is the status that we are
// actually in and target is the status we would be, eventually, unless
// relayStatus(id, relayStatus()) is called
bool relayStatus(size_t id);
bool relayStatusTarget(size_t id);

void relayToggle(size_t id, bool report, bool group_report);
void relayToggle(size_t id);

size_t relayCount();

const String& relayPayloadOn();
const String& relayPayloadOff();
const String& relayPayloadToggle();

const char* relayPayload(PayloadStatus status);

void relayPulse(size_t id, espurna::duration::Milliseconds, bool);
void relayPulse(size_t id, espurna::duration::Milliseconds);
void relayPulse(size_t id);
void relaySync(size_t id);
void relaySave(bool persist);

using RelayStatusCallback = void(*)(size_t id, bool status);
using RelayProviderBasePtr = std::unique_ptr<RelayProviderBase>;

bool relayAdd(RelayProviderBasePtr&& provider);
void relayOnStatusNotify(RelayStatusCallback);
void relayOnStatusChange(RelayStatusCallback);

void relaySetupDummy(size_t size, bool reconfigure = false);
void relaySetup();
