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

#ifndef LIGHTFOX_PORT
#define LIGHTFOX_PORT 1
#endif

namespace espurna {
namespace hardware {
namespace lightfox {
namespace {

namespace build {

constexpr size_t port() {
    return LIGHTFOX_PORT - 1;
}

} // namespace build

// -----------------------------------------------------------------------------

namespace internal {

Stream* port { nullptr };

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

    espurna::StringView id() const override {
        return STRING_VIEW("lightfox");
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

RelayProviderBasePtr make_relay(size_t index) {
    return std::make_unique<RelayProvider>(index);
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

PROGMEM_STRING(Clear, "LIGHTFOX.CLEAR");

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

class ButtonPin final : public BasePin {
public:
    ButtonPin() = delete;
    explicit ButtonPin(size_t index) :
        _index(index)
    {
        _readings.push_back(Reading{});
    }

    String description() const override {
        String out;

        out += STRING_VIEW("lightfox id:");
        out += _index;
        out += STRING_VIEW(" status:#");
        out += _readings[_index].status ? 't' : 'f';

        return out;
    }

    static void loop() {
        const auto now = TimeSource::now();

        // Emulate 'Click' behaviour by expiring our readings
        // But, unlike previous version, we could make either a switch or a button
        for (auto& reading : _readings) {
            if (reading.status && ((now - reading.last) > ReadInterval)) {
                reading.status = false;
            }
        }

        if (internal::port->available() < 4) {
            return;
        }

        uint8_t bytes[4] = {0};
        internal::port->readBytes(bytes, 4);
        if ((bytes[0] != 0xA0) && (bytes[1] != 0x04) && (bytes[3] != 0xA1)) {
            return;
        }

        // Unlike DUAL, inputs may have different IDs than the outputs
        // ref. https://github.com/foxel/esp-dual-rf-switch
        static constexpr uint8_t Digits { std::numeric_limits<uint8_t>::digits };
        const auto mask = bytes[2];

        for (uint8_t index = 0; index < Digits; ++index) {
            if (((mask & index) > 0) && (index < _readings.size())) {
                _readings[index].status = true;
                _readings[index].last = now;
            }
        }
    }

    unsigned char pin() const override {
        return _index;
    }

    const char* id() const override {
        return "LightfoxPin";
    }

    // Simulate LOW level when the range matches and HIGH when it does not
    int digitalRead() override {
        return _readings[_index].status;
    }

    void pinMode(int8_t) override {
    }

    void digitalWrite(int8_t val) override {
    }

private:
    using TimeSource = time::SystemClock;
    static constexpr TimeSource::duration ReadInterval
        = duration::Milliseconds{ 100 };

    struct Reading {
        bool status { false };
        TimeSource::time_point last;
    };

    size_t _index;
    static std::vector<Reading> _readings;
};

BasePinPtr make_button(size_t index) {
    return std::make_unique<ButtonPin>(index);
}

std::vector<ButtonPin::Reading> ButtonPin::_readings;

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

    ::espurnaRegisterLoop(ButtonPin::loop);
}

} // namespace
} // namespace lightfox
} // namespace hardware
} // namespace espurna

BasePinPtr lightfoxMakeButtonPin(size_t index) {
    return espurna::hardware::lightfox::make_button(index);
}

RelayProviderBasePtr lightfoxMakeRelayProvider(size_t index) {
    return espurna::hardware::lightfox::make_relay(index);
}

void lightfoxSetup() {
    espurna::hardware::lightfox::setup();
}

#endif
