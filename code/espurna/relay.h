/*

RELAY MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"
#include "rpc.h"

constexpr size_t RelaysMax = 32;

enum class RelayPulse : uint8_t {
    None,
    Off,
    On
};

enum class RelayLock : uint8_t {
    None,
    Off,
    On
};

enum class RelayType : int {
    Normal,
    Inverse,
    Latched,
    LatchedInverse
};

enum class RelayProvider: int {
    None,
    Dummy,
    Gpio,
    Dual,
    Stm
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

bool relayStatus(unsigned char id, bool status, bool report, bool group_report);
bool relayStatus(unsigned char id, bool status);

// gets either current or target status, where current is the status that we are
// actually in and target is the status we would be, eventually, unless
// relayStatus(id, relayStatus()) is called
bool relayStatus(unsigned char id);
bool relayStatusTarget(unsigned char id);

void relayToggle(unsigned char id, bool report, bool group_report);
void relayToggle(unsigned char id);

unsigned char relayCount();

const String& relayPayloadOn();
const String& relayPayloadOff();
const String& relayPayloadToggle();

const char* relayPayload(PayloadStatus status);

void relayMQTT(unsigned char id);
void relayMQTT();

void relayPulse(unsigned char id);
void relaySync(unsigned char id);
void relaySave(bool persist);

using RelayStatusCallback = void(*)(unsigned char id, bool status);
using RelayProviderBasePtr = std::unique_ptr<RelayProviderBase>;

bool relayAdd(RelayProviderBasePtr&& provider);
void relaySetStatusNotify(RelayStatusCallback);
void relaySetStatusChange(RelayStatusCallback);

void relaySetupDummy(size_t size, bool reconfigure = false);
void relaySetup();
