/*

LightFox module

Copyright (C) 2019 by Andrey F. Kupreychik <foxle@quickfox.ru>

*/

#include "espurna.h"

#ifdef FOXEL_LIGHTFOX_DUAL

#include "button.h"
#include "lightfox.h"
#include "relay.h"
#include "terminal.h"
#include "ws.h"

#include <bitset>
#include <vector>

static_assert(1 == (RELAY_SUPPORT), "");
static_assert(1 == (BUTTON_SUPPORT), "");

constexpr size_t _lightfoxBuildButtons() {
    return LIGHTFOX_BUTTONS;
}

constexpr size_t _lightfoxBuildRelays() {
    return LIGHTFOX_RELAYS;
}

// -----------------------------------------------------------------------------
// PROTOCOL
// -----------------------------------------------------------------------------

constexpr uint8_t CodeStart { 0xa0 };
constexpr uint8_t CodeLearn { 0xf1 };
constexpr uint8_t CodeClear { 0xf2 };
constexpr uint8_t CodeStop { 0xa1 };

void _lightfoxSend(uint8_t code) {
    uint8_t data[6] {
        CodeStart,
        code,
        0x00,
        CodeStop,
        static_cast<uint8_t>('\r'),
        static_cast<uint8_t>('\n')
    };
    Serial.write(data, sizeof(data));
    Serial.flush();
    DEBUG_MSG_P(PSTR("[LIGHTFOX] Code %02X sent\n"), code);
}

void lightfoxLearn() {
    _lightfoxSend(CodeLearn);
}

void lightfoxClear() {
    _lightfoxSend(CodeClear);
}

class LightfoxProvider : public RelayProviderBase {
public:
    LightfoxProvider() = delete;
    explicit LightfoxProvider(size_t id) :
        _id(id)
    {
        _instances.push_back(this);
    }

    ~LightfoxProvider() {
        _instances.erase(
            std::remove(_instances.begin(), _instances.end(), this),
            _instances.end());
    }

    const char* id() const override {
        return "lightfox";
    }

    bool setup() override {
        static bool once { false };
        if (!once) {
            once = true;
            Serial.begin(SERIAL_BAUDRATE);
        }
        return true;
    }

    void change(bool) override {
        static bool scheduled { false };
        if (!scheduled) {
            schedule_function([]() {
                flush();
                scheduled = false;
            });
        }
    }

    size_t relayId() const {
        return _id;
    }

    static std::vector<LightfoxProvider*>& instances() {
        return _instances;
    }

    static void flush() {
        size_t mask { 0ul };
        for (size_t index = 0; index < _instances.size(); ++index) {
            bool status { relayStatus(_instances[index]->relayId()) };
            mask |= (status ? 1ul : 0ul << index);
        }

        DEBUG_MSG_P(PSTR("[LIGHTFOX] Sending DUAL mask: 0x%02X\n"), mask);

        uint8_t buffer[4] { 0xa0, 0x04, static_cast<uint8_t>(mask), 0xa1 };
        Serial.write(buffer, sizeof(buffer));
        Serial.flush();
    }

private:
    size_t _id;
    static std::vector<LightfoxProvider*> _instances;
};

std::vector<LightfoxProvider*> LightfoxProvider::_instances;

size_t _lightfox_button_offset { 0 };
size_t _lightfox_buttons { 0 };

// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

void _lightfoxWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, "lightfox");
}

void _lightfoxWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "lightfoxLearn") == 0) {
        lightfoxLearn();
    } else if (strcmp(action, "lightfoxClear") == 0) {
        lightfoxClear();
    }
}

#endif

// -----------------------------------------------------------------------------
// TERMINAL
// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT

void _lightfoxInitCommands() {

    terminalRegisterCommand(F("LIGHTFOX.LEARN"), [](::terminal::CommandContext&& ctx) {
        lightfoxLearn();
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("LIGHTFOX.CLEAR"), [](::terminal::CommandContext&& ctx) {
        lightfoxClear();
        terminalOK(ctx);
    });
}

#endif

// -----------------------------------------------------------------------------
// SETUP & LOOP
// -----------------------------------------------------------------------------

void _lightfoxInputLoop() {
    if (Serial.available() < 4) {
        return;
    }

    unsigned char bytes[4] = {0};
    Serial.readBytes(bytes, 4);
    if ((bytes[0] != 0xA0) && (bytes[1] != 0x04) && (bytes[3] != 0xA1)) {
        return;
    }

    // Unlike DUAL, inputs may have different IDs than the outputs
    // ref. https://github.com/foxel/esp-dual-rf-switch
    constexpr unsigned long InputsMask { 0xf };
    unsigned long mask { static_cast<unsigned long>(bytes[2]) & InputsMask };
    unsigned long id { 0 };

    for (size_t button = 0; id < _lightfox_buttons; ++button) {
        if (mask & (1ul << button)) {
            buttonEvent(button + _lightfox_button_offset, ButtonEvent::Click);
        }
    }
}

void lightfoxSetup() {

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_lightfoxWebSocketOnVisible)
            .onAction(_lightfoxWebSocketOnAction);
    #endif

    #if TERMINAL_SUPPORT
        _lightfoxInitCommands();
    #endif

    for (size_t relay = 0; relay < _lightfoxBuildRelays(); ++relay) {
        size_t relayId { relayCount() };
        if (!relayAdd(std::make_unique<LightfoxProvider>(relayId))) {
            break;
        }
    }

    _lightfox_button_offset = buttonCount();
    for (size_t index = 0; index < _lightfoxBuildButtons(); ++index) {
        if (buttonAdd()) {
            ++_lightfox_buttons;
        }
    }

    espurnaRegisterLoop(_lightfoxInputLoop);

}

#endif
