/*

LightFox module

Copyright (C) 2019 by Andrey F. Kupreychik <foxle@quickfox.ru>

*/

#include "espurna.h"

#ifdef FOXEL_LIGHTFOX_DUAL

static_assert(1 == (RELAY_SUPPORT), "");
static_assert(1 == (BUTTON_SUPPORT), "");

#include "button.h"
#include "lightfox.h"
#include "relay.h"
#include "terminal.h"
#include "ws.h"

#include <array>
#include <vector>

#ifndef LIGHTFOX_BUTTONS
#define LIGHTFOX_BUTTONS 4
#endif

#ifndef LIGHTFOX_RELAYS
#define LIGHTFOX_RELAYS 2
#endif

#ifndef LIGHTFOX_PORT
#define LIGHTFOX_PORT 1
#endif

namespace espurna {
namespace hardware {
namespace lightfox {
namespace {

namespace build {

constexpr size_t buttons() {
    return LIGHTFOX_BUTTONS;
}

constexpr size_t relays() {
    return LIGHTFOX_RELAYS;
}

constexpr size_t port() {
    return LIGHTFOX_PORT - 1;
}

} // namespace build

// -----------------------------------------------------------------------------

namespace internal {

Stream* port { nullptr };

size_t button_offset { 0 };
size_t buttons { 0 };

} // namespace internal

constexpr uint8_t CodeStart { 0xa0 };
constexpr uint8_t CodeLearn { 0xf1 };
constexpr uint8_t CodeClear { 0xf2 };
constexpr uint8_t CodeStop { 0xa1 };

void send(uint8_t code) {
    const std::array<uint8_t, 6> data {
        CodeStart,
        code,
        0x00,
        CodeStop,
        static_cast<uint8_t>('\r'),
        static_cast<uint8_t>('\n')
    };

    DEBUG_MSG_P(PSTR("[LIGHTFOX] Send %02X\n"), code);
    internal::port->write(data.begin(), data.size());
    internal::port->flush();
}

void learn() {
    send(CodeLearn);
}

void clear() {
    send(CodeClear);
}

class RelayProvider : public RelayProviderBase {
public:
    RelayProvider() = delete;
    explicit RelayProvider(size_t id) :
        _id(id)
    {
        _instances.push_back(this);
    }

    ~RelayProvider() override {
        _instances.erase(
            std::remove(_instances.begin(), _instances.end(), this),
            _instances.end());
    }

    const char* id() const override {
        return "lightfox";
    }

    bool setup() override {
        return true;
    }

    // we apply relay statuses in bulk
    void change(bool) override {
        espurnaRegisterOnce(flush);
    }

    size_t relayId() const {
        return _id;
    }

    static std::vector<RelayProvider*>& instances() {
        return _instances;
    }

    static void flush();

private:
    size_t _id;
    static std::vector<RelayProvider*> _instances;
};

std::vector<RelayProvider*> RelayProvider::_instances;

void RelayProvider::flush() {
    size_t mask { 0ul };
    for (size_t index = 0; index < _instances.size(); ++index) {
        bool status { relayStatus(_instances[index]->relayId()) };
        mask |= (status ? 1ul : 0ul << index);
    }

    DEBUG_MSG_P(PSTR("[LIGHTFOX] DUAL mask: 0x%02X\n"), mask);
    uint8_t buffer[4] { 0xa0, 0x04, static_cast<uint8_t>(mask), 0xa1 };
    internal::port->write(buffer, sizeof(buffer));
    internal::port->flush();
}

// -----------------------------------------------------------------------------

#if WEB_SUPPORT
namespace web {

PROGMEM_STRING(Module, "lightfox");

void onVisible(JsonObject& root) {
    wsPayloadModule(root, Module);
}

void onAction(uint32_t client_id, const char* action, JsonObject& data) {
    STRING_VIEW_INLINE(Learn, "lightfoxLearn");
    if (Learn == action) {
        learn();
        return;
    }

    STRING_VIEW_INLINE(Clear, "lightfoxClear");
    if (Clear == action) {
        clear();
        return;
    }
}

void setup() {
    wsRegister()
        .onVisible(onVisible)
        .onAction(onAction);
}

} // namespace web
#endif

#if TERMINAL_SUPPORT
namespace terminal {

PROGMEM_STRING(Learn, "LIGHTFOX.LEARN");

static void learn(::terminal::CommandContext&& ctx) {
    lightfox::learn();
    terminalOK(ctx);
}

PROGMEM_STRING(Clear, "LIGHTFOX.LEARN");

static void clear(::terminal::CommandContext&& ctx) {
    lightfox::clear();
    terminalOK(ctx);
}

static constexpr ::terminal::Command Commands[] PROGMEM {
    {Learn, learn},
    {Clear, clear},
};

void setup() {
    espurna::terminal::add(Commands);
}

} // namespace terminal
#endif

// -----------------------------------------------------------------------------

void loop() {
    if (internal::port->available() < 4) {
        return;
    }

    unsigned char bytes[4] = {0};
    internal::port->readBytes(bytes, 4);
    if ((bytes[0] != 0xA0) && (bytes[1] != 0x04) && (bytes[3] != 0xA1)) {
        return;
    }

    // Unlike DUAL, inputs may have different IDs than the outputs
    // ref. https://github.com/foxel/esp-dual-rf-switch
    constexpr unsigned long InputsMask { 0xf };
    unsigned long mask { static_cast<unsigned long>(bytes[2]) & InputsMask };
    unsigned long id { 0 };

    for (size_t button = 0; id < internal::buttons; ++button) {
        if (mask & (1ul << button)) {
            buttonEvent(button + internal::button_offset, ButtonEvent::Click);
        }
    }
}

void setup() {
    const auto port = uartPort(build::port());
    if (!port) {
        return;
    }

    internal::port = port->stream;

#if WEB_SUPPORT
    web::setup();
#endif
#if TERMINAL_SUPPORT
    terminal::setup();
#endif

    for (size_t relay = 0; relay < build::relays(); ++relay) {
        size_t relayId { relayCount() };
        if (!relayAdd(std::make_unique<RelayProvider>(relayId))) {
            break;
        }
    }

    internal::button_offset = buttonCount();
    for (size_t index = 0; index < build::buttons(); ++index) {
        if (buttonAdd()) {
            ++internal::buttons;
        }
    }

    ::espurnaRegisterLoop(lightfox::loop);
}

} // namespace
} // namespace lightfox
} // namespace hardware
} // namespace espurna

void lightfoxSetup() {
    espurna::hardware::lightfox::setup();
}

#endif
