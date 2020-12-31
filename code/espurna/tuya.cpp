/*

TUYA MODULE

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

// ref: https://docs.tuya.com/en/mcu/mcu-protocol.html

#include "tuya.h"

#if TUYA_SUPPORT

#include "broker.h"
#include "light.h"
#include "relay.h"
#include "rpc.h"

#include <functional>
#include <queue>
#include <StreamString.h>

#include "tuya_types.h"
#include "tuya_transport.h"
#include "tuya_dataframe.h"
#include "tuya_protocol.h"
#include "tuya_util.h"

namespace tuya {

    class TuyaProvider : public RelayProviderBase {
    public:
        explicit TuyaProvider(unsigned char id) :
            _id(id)
        {}

        const char* id() const {
            return "tuya";
        }

        void change(bool status) {
            sendSwitch(_id, status);
        }
    private:
        unsigned char _id;
    };

    constexpr size_t SERIAL_SPEED { 9600u };

    constexpr unsigned char SWITCH_MAX { 8u };
    constexpr unsigned char DIMMER_MAX { 5u };

    constexpr uint32_t DISCOVERY_TIMEOUT { 1500u };

    constexpr uint32_t HEARTBEAT_SLOW { 9000u };
    constexpr uint32_t HEARTBEAT_FAST { 3000u };

    // --------------------------------------------

    size_t getHeartbeatInterval(Heartbeat hb) {
        switch (hb) {
            case Heartbeat::FAST:
                return HEARTBEAT_FAST;
            case Heartbeat::SLOW:
                return HEARTBEAT_SLOW;
            case Heartbeat::NONE:
            default:
                return 0;
        }
    }

    uint8_t getWiFiState() {

        uint8_t state = wifiState();
        if (state & WIFI_STATE_SMARTCONFIG) return 0x00;
        if (state & WIFI_STATE_AP) return 0x01;
        if (state & WIFI_STATE_STA) return 0x04;

        return 0x02;
    }

    // TODO: is v2 required to modify pin assigments?
    void updatePins(uint8_t led, uint8_t rst) {
        setSetting("ledGPIO0", led);
        setSetting("btnGPIO0", rst);
        //espurnaReload();
    }

    // --------------------------------------------

    States<bool> switchStates(SWITCH_MAX);
#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
    States<uint32_t> channelStates(DIMMER_MAX);
#endif

    // Handle DP data from the MCU, mapping incoming DP ID to the specific relay / channel ID

    void applySwitch() {
        for (unsigned char id=0; id < switchStates.size(); ++id) {
            relayStatus(id, switchStates[id].value);
        }
    }

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
    void applyChannel() {
        for (unsigned char id=0; id < channelStates.size(); ++id) {
            lightChannel(id, channelStates[id].value);
        }
        lightUpdate(true, true);
    }
#endif

    // --------------------------------------------

    Transport tuyaSerial(TUYA_SERIAL);
    std::queue<DataFrame> outputFrames;

    DiscoveryTimeout discoveryTimeout(DISCOVERY_TIMEOUT);
    bool transportDebug { false };
    bool configDone { false };
    bool reportWiFi { false };
    bool filter { false };

    String product;

    void showProduct() {
        if (product.length()) DEBUG_MSG_P(PSTR("[TUYA] Product: %s\n"), product.c_str());
    }

    // Setup the switch & channel mappings based on either the discovery or user settings

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM

    void updateChannels() {
        if (channelStates.size()) {
            auto size = channelStates.size();
            do {
                if (!::lightAdd()) {
                    return;
                }
            } while (--size);

            lightSetProvider(std::make_unique<LightProvider>(
                []() {
                },
                [](unsigned char id, double value) {
                    sendChannel(id, static_cast<unsigned int>(value));
                }
            ));
            configDone = true;
        }
    }

#endif

    void updateSwitches() {
        if (switchStates.size()) {
            size_t end { switchStates.size() + ::relayCount() };
            if ((end > ::RelaysMax) || (end > std::numeric_limits<decltype(relayCount())>::max())) {
                return;
            }

            for (size_t id = ::relayCount(); id < end; ++id) {
                ::relayAdd(std::make_unique<TuyaProvider>(static_cast<unsigned char>(id)));
            }
            configDone = true;
        }
    }

    void dataframeDebugSend(const char* tag, const DataFrame& frame) {
        if (!transportDebug) return;
        StreamString out;
        Output writer(out, frame.length);
        writer.writeHex(frame.serialize());
        DEBUG_MSG("[TUYA] %s: %s\n", tag, out.c_str());
    }

    void sendHeartbeat(Heartbeat hb, State state) {

        static uint32_t last = 0;
        if (millis() - last > getHeartbeatInterval(hb)) {
            outputFrames.emplace(Command::Heartbeat);
            last = millis();
        }

    }

    void sendWiFiStatus() {
        if (!reportWiFi) return;
        outputFrames.emplace(
            Command::WiFiStatus, std::initializer_list<uint8_t> { getWiFiState() }
        );
    }

    void pushOrUpdateState(const Type type, const DataFrame& frame) {
        if (Type::BOOL == type) {
            const DataProtocol<bool> proto(frame);
            switchStates.pushOrUpdate(proto.id(), proto.value());
            //DEBUG_MSG_P(PSTR("[TUYA] apply BOOL id=%02u value=%s\n"), proto.id(), proto.value() ? "true" : "false");
        } else if (Type::INT == type) {
#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
            const DataProtocol<uint32_t> proto(frame);
            channelStates.pushOrUpdate(proto.id(), proto.value());
            //DEBUG_MSG_P(PSTR("[TUYA] apply  INT id=%02u value=%u\n"), proto.id(), proto.value());
#endif
        }
    }

    // XXX: sometimes we need to ignore incoming state, when not in discovery mode
    // ref: https://github.com/xoseperez/espurna/issues/1729#issuecomment-509234195
    void updateState(const Type type, const DataFrame& frame) {
        if (filter) {
            return;
        }

        if (Type::BOOL == type) {
            const DataProtocol<bool> proto(frame);
            switchStates.update(proto.id(), proto.value());
        } else if (Type::INT == type) {
#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
            const DataProtocol<uint32_t> proto(frame);
            channelStates.update(proto.id(), proto.value());
#endif
        }
    }

    void processDP(State state, const DataFrame& frame) {

        // TODO: do not log protocol errors without transport debug enabled
        if (!frame.length) {
            DEBUG_MSG_P(PSTR("[TUYA] DP frame must have data\n"));
            return;
        }

        const Type type {dataType(frame)};
        if (Type::UNKNOWN == type) {
            if (frame.length >= 2) {
                DEBUG_MSG_P(PSTR("[TUYA] Unknown DP id=%u type=%u\n"), frame[0], frame[1]);
            } else {
                DEBUG_MSG_P(PSTR("[TUYA] Invalid DP frame\n"));
            }
            return;
        }

        if (State::DISCOVERY == state) {
            discoveryTimeout.feed();
            pushOrUpdateState(type, frame);
        } else {
            updateState(type, frame);
        }

    }

    void processFrame(State& state, const Transport& buffer) {

        const DataFrame frame(buffer);

        dataframeDebugSend("<=", frame);

        // initial packet has 0, do the initial setup
        // all after that have 1. might be a good idea to re-do the setup when that happens on boot
        if (frame.commandEquals(Command::Heartbeat) && (frame.length == 1)) {
            if (State::HEARTBEAT == state) {
                if ((frame[0] == 0) || !configDone) {
                    DEBUG_MSG_P(PSTR("[TUYA] Starting configuration ...\n"));
                    state = State::QUERY_PRODUCT;
                    return;
                } else {
                    DEBUG_MSG_P(PSTR("[TUYA] Already configured\n"));
                    state = State::IDLE;
                }
            }
            sendWiFiStatus();
            return;
        }

        if (frame.commandEquals(Command::QueryProduct) && frame.length) {
            if (product.length()) {
                product = "";
            }
            product.reserve(frame.length);
            for (unsigned int n = 0; n < frame.length; ++n) {
                product += static_cast<char>(frame[n]);
            }
            showProduct();
            state = State::QUERY_MODE;
            return;
        }

        if (frame.commandEquals(Command::QueryMode)) {
            // first and second byte are GPIO pin for WiFi status and RST respectively
            if (frame.length == 2) {
                DEBUG_MSG_P(PSTR("[TUYA] Mode: ESP only, led=GPIO%02u rst=GPIO%02u\n"), frame[0], frame[1]);
                updatePins(frame[0], frame[1]);
            // ... or nothing. we need to report wifi status to the mcu via Command::WiFiStatus
            } else if (!frame.length) {
                DEBUG_MSG_P(PSTR("[TUYA] Mode: ESP & MCU\n"));
                reportWiFi = true;
                sendWiFiStatus();
            }
            state = State::QUERY_DP;
            return;
        }

        if (frame.commandEquals(Command::WiFiResetCfg) && !frame.length) {
            DEBUG_MSG_P(PSTR("[TUYA] WiFi reset request\n"));
            outputFrames.emplace(Command::WiFiResetCfg);
            return;
        }

        if (frame.commandEquals(Command::WiFiResetSelect) && (frame.length == 1)) {
            DEBUG_MSG_P(PSTR("[TUYA] WiFi configuration mode request: %s\n"),
                (frame[0] == 0) ? "Smart Config" : "AP");
            outputFrames.emplace(Command::WiFiResetSelect);
            return;
        }

        if (frame.commandEquals(Command::ReportDP) && frame.length) {
            processDP(state, frame);
            if (state == State::DISCOVERY) return;
            if (state == State::HEARTBEAT) return;
            state = State::IDLE;
            return;
        }

    }

    void processSerial(State& state) {

        while (tuyaSerial.available()) {

            tuyaSerial.read();

            if (tuyaSerial.done()) {
                processFrame(state, tuyaSerial);
                tuyaSerial.reset();
            }

            if (tuyaSerial.full()) {
                tuyaSerial.rewind();
                tuyaSerial.reset();
            }
        }

    }

    // Push local state data, mapping it to the appropriate DP

    void sendSwitch(unsigned char id) {
        if (id >= switchStates.size()) return;
        outputFrames.emplace(
            Command::SetDP, DataProtocol<bool>(switchStates[id].dp, switchStates[id].value).serialize()
        );
    }

    void sendSwitch(unsigned char id, bool value) {
        if (id >= switchStates.size()) return;
        if (value == switchStates[id].value) return;
        switchStates[id].value = value;
        sendSwitch(id);
    }

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM

    void sendChannel(unsigned char id) {
        if (id >= channelStates.size()) return;
        outputFrames.emplace(
            Command::SetDP, DataProtocol<uint32_t>(channelStates[id].dp, channelStates[id].value).serialize()
        );
    }

    void sendChannel(unsigned char id, unsigned int value) {
        if (id >= channelStates.size()) return;
        if (value == channelStates[id].value) return;
        channelStates[id].value = value;
        sendChannel(id);
    }
#endif

    // Main loop state machine. Process input data and manage output queue

    void loop() {

        static State state = State::INIT;

        // running this before anything else to quickly switch to the required state
        processSerial(state);

        // go through the initial setup step-by-step, as described in
        // https://docs.tuya.com/en/mcu/mcu-protocol.html#21-basic-protocol
        switch (state) {
            // flush serial buffer before transmitting anything
            // send fast heartbeat until mcu responds with something
            case State::INIT:
                tuyaSerial.rewind();
                state = State::HEARTBEAT;
            case State::HEARTBEAT:
                sendHeartbeat(Heartbeat::FAST, state);
                break;
            // general info about the device (which we don't care about)
            case State::QUERY_PRODUCT:
            {
                outputFrames.emplace(Command::QueryProduct);
                state = State::IDLE;
                break;
            }
            // whether we control the led & button or not
            // TODO: make updatePins() do something!
            case State::QUERY_MODE:
            {
                outputFrames.emplace(Command::QueryMode);
                state = State::IDLE;
                break;
            }
            // full read-out of the data protocol values
            case State::QUERY_DP:
            {
                DEBUG_MSG_P(PSTR("[TUYA] Starting discovery\n"));
                outputFrames.emplace(Command::QueryDP);
                discoveryTimeout.feed();
                state = State::DISCOVERY;
                break;
            }
            // parse known data protocols until discovery timeout expires
            case State::DISCOVERY:
            {
                if (discoveryTimeout) {
                    DEBUG_MSG_P(PSTR("[TUYA] Discovery finished\n"));
#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
                    updateChannels();
#endif
                    updateSwitches();
                    state = State::IDLE;
                }
                break;
            }
            // initial config is done, only doing heartbeat periodically
            case State::IDLE:
            {
                if (switchStates.changed()) applySwitch();
#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
                if (channelStates.changed()) applyChannel();
#endif
                sendHeartbeat(Heartbeat::SLOW, state);
                break;
            }
        }

        if (TUYA_SERIAL && !outputFrames.empty()) {
            const DataFrame frame = std::move(outputFrames.front());
            dataframeDebugSend("=>", frame);
            tuyaSerial.write(frame.serialize());
            outputFrames.pop();
        }

    }

    // Predefined DP<->SWITCH, DP<->CHANNEL associations
    // Respective provider setup should be called before state restore,
    // so we can use dummy values

    void setupSwitches() {
        for (unsigned char id = 0; id < switchStates.capacity(); ++id) {
            auto dp = getSetting({"tuyaSwitch", id}, 0);
            if (!dp) {
                break;
            }

            switchStates.pushOrUpdate(dp, false);
        }

        updateSwitches();
    }

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM

    void setupChannels() {
        for (unsigned char id = 0; id < channelStates.capacity(); ++id) {
            auto dp = getSetting({"tuyaChannel", id}, 0);
            if (!dp) {
                break;
            }

            channelStates.pushOrUpdate(dp, 0);
        }

        updateChannels();
    }

#endif

    void setup() {

        // Print all known DP associations

        #if TERMINAL_SUPPORT

            terminalRegisterCommand(F("TUYA.SHOW"), [](const terminal::CommandContext&) {
                static const char fmt[] PROGMEM = "%12s%u => dp=%u value=%u\n";
                showProduct();

                for (unsigned char id=0; id < switchStates.size(); ++id) {
                    DEBUG_MSG_P(fmt, "tuyaSwitch", id, switchStates[id].dp, switchStates[id].value);
                }

                #if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
                    for (unsigned char id=0; id < channelStates.size(); ++id) {
                        DEBUG_MSG_P(fmt, "tuyaChannel", id, channelStates[id].dp, channelStates[id].value);
                    }
                #endif
            });

            terminalRegisterCommand(F("TUYA.SAVE"), [](const terminal::CommandContext&) {
                DEBUG_MSG_P(PSTR("[TUYA] Saving current configuration ...\n"));
                for (unsigned char n=0; n < switchStates.size(); ++n) {
                    setSetting({"tuyaSwitch", n}, switchStates[n].dp);
                }
                #if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
                    for (unsigned char n=0; n < channelStates.size(); ++n) {
                        setSetting({"tuyaChannel", n}, channelStates[n].dp);
                    }
                #endif
            });

        #endif

        // Filtering for incoming data
        // TODO: see https://github.com/xoseperez/espurna/issues/2222
        //       unlike emulator, this real device sends total garbage which we can safely ignore 
        //       (may not be true for all, but that's the one we got the most of :>)
        filter = getSetting("tuyaFilter", true);

        setupSwitches();

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
        setupChannels();
#endif

        // Print all IN and OUT messages
        transportDebug = getSetting("tuyaDebug", true);

        // Install main loop method and WiFiStatus ping (only works with specific mode)
        TUYA_SERIAL.begin(SERIAL_SPEED);

        ::espurnaRegisterLoop(loop);
        ::wifiRegister([](justwifi_messages_t code, char * parameter) {
            if ((MESSAGE_CONNECTED == code) || (MESSAGE_DISCONNECTED == code)) {
                sendWiFiStatus();
            }
        });
    }

}

#endif // TUYA_SUPPORT
