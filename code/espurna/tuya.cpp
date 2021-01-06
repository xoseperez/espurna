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
#include <forward_list>
#include <StreamString.h>

#include "tuya_types.h"
#include "tuya_transport.h"
#include "tuya_dataframe.h"
#include "tuya_protocol.h"
#include "tuya_util.h"

namespace tuya {

    bool operator<(const DataFrame& lhs, const DataFrame& rhs) {
        if (dataType(lhs) == Type::BOOL) {
            return true;
        }

        return false;
    }

    constexpr size_t SerialSpeed { 9600u };

    constexpr uint32_t DiscoveryTimeout { 1500u };

    constexpr uint32_t HeartbeatSlow { 9000u };
    constexpr uint32_t HeartbeatFast { 3000u };

    struct Config {
        Config(const Config&) = delete;
        Config(Config&& other) noexcept :
            key(std::move(other.key)),
            value(std::move(other.value))
        {}

        Config(String&& key_, String&& value_) noexcept :
            key(std::move(key_)),
            value(std::move(value_))
        {}

        String key;
        String value;
    };

    Transport tuyaSerial(TUYA_SERIAL);
    std::priority_queue<DataFrame> outputFrames;

    template <typename T>
    void send(unsigned char dp, T value) {
        outputFrames.emplace(
            Command::SetDP, DataProtocol<T>(dp, value).serialize()
        );
    }

    // --------------------------------------------

    Discovery discovery(DiscoveryTimeout);
    bool transportDebug { false };
    bool configDone { false };
    bool reportWiFi { false };
    bool filter { false };

    String product;
    std::forward_list<Config> config;

    DpMap switchIds;

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
    DpMap channelIds;
    uint8_t channelStateId { 0u };
#endif

    // --------------------------------------------

    class TuyaRelayProvider : public RelayProviderBase {
    public:
        explicit TuyaRelayProvider(unsigned char dp) :
            _dp(dp)
        {}

        const char* id() const {
            return "tuya";
        }

        void change(bool status) {
            send(_dp, status);
        }
    private:
        unsigned char _dp;
    };

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM

    class TuyaLightProvider : public LightProvider {
    public:
        TuyaLightProvider() = default;
        explicit TuyaLightProvider(const DpMap& channels) :
            _channels(channels)
        {}

        explicit TuyaLightProvider(const DpMap& channels, uint8_t state) :
            _channels(channels),
            _state(state)
        {}

        void update() override {
        }

        void state(bool status) override {
            if (_state && !status) {
                send(_state, status);
            }
        }

        void channel(unsigned char channel, double value) override {
            auto dp = _channels.dp(channel);
            if (!dp) {
                return;
            }

            // tuya dimmer is precious, and can't handle 0...some-kind-of-threshold
            // just ignore it, the associated switch will handle turning it off
            auto rounded = static_cast<unsigned int>(value);
            if (rounded <= 0x10) {
                return;
            }

            send(dp->id, rounded);
        }

    private:
        const DpMap& _channels;
        uint8_t _state { 0 };
    };

#endif

    // --------------------------------------------

    size_t getHeartbeatInterval(Heartbeat hb) {
        switch (hb) {
        case Heartbeat::FAST:
            return HeartbeatFast;
        case Heartbeat::SLOW:
            return HeartbeatSlow;
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

    // --------------------------------------------

    void addConfig(String&& key, String&& value) {
        Config kv{std::move(key), std::move(value)};
        config.push_front(std::move(kv));
    }

    void updatePins(uint8_t led, uint8_t rst) {
        static bool done { false };
        if (!done) {
            addConfig("ledGPIO0", String(led));
            addConfig("btnGPIO0", String(rst));
            done = true;
        }
    }

    void showProduct() {
        DEBUG_MSG_P(PSTR("[TUYA] Product: %s\n"), product.length() ? product.c_str() : "(unknown)");
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
        if (reportWiFi) {
            outputFrames.emplace(
                Command::WiFiStatus, std::initializer_list<uint8_t> { getWiFiState() }
            );
        }
    }

    void updateSwitch(const DataProtocol<bool>& proto) {
        if (filter) {
            return;
        }

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
        if (channelStateId && (channelStateId == proto.id())) {
            lightState(proto.value());
            return;
        }
#endif

        auto* dp = switchIds.id(proto.id());
        if (!dp) {
            return;
        }

        relayStatus(dp->id, proto.value());
    }

    void dump(const char* type, unsigned char id, const char* buf) {
        DEBUG_MSG_P(PSTR("[Tuya] Received %s dp=%u value=%s\n"), type, id, buf);
    }

    void dump(const DataProtocol<bool>& proto) {
        char buf[3] { '#', proto.value() ? 't' : 'f', '\0' };
        dump("boolean", proto.id(), buf);
    }

    void dump(const DataProtocol<uint32_t>& proto) {
        char buf[4 * sizeof(uint32_t)];
        snprintf(buf, sizeof(buf), "%u", proto.value());
        dump("integer", proto.id(), buf);
    }

    // XXX: sometimes we need to ignore incoming state
    // ref: https://github.com/xoseperez/espurna/issues/1729#issuecomment-509234195
    void updateState(Type type, const DataFrame& frame) {
        if (Type::BOOL == type) {
            DataProtocol<bool> proto(frame);
            dump(proto);
            updateSwitch(proto);
        } else if (Type::INT == type) {
            DataProtocol<uint32_t> proto(frame);
            dump(proto);
        }
    }

    void updateDiscovered(Discovery&& discovery) {
        auto& dps = discovery.get();

        for (auto& dp : dps) {
            switch (dp.type) {
            case Type::BOOL:
                if (!relayAdd(std::make_unique<TuyaRelayProvider>(dp.id))) {
                    DEBUG_MSG_P(PSTR("[TUYA] Cannot add relay for DP id=%u\n"), dp.id);
                    goto error;
                }
                if (!switchIds.add((relayCount() - 1), dp.id)) {
                    DEBUG_MSG_P(PSTR("[TUYA] Switch for DP id=%u already exists\n"), dp.id);
                    goto error;
                }
                break;
            case Type::INT:
#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
                if (!lightAdd()) {
                    DEBUG_MSG_P(PSTR("[TUYA] Cannot add channel for DP id=%u\n"), dp.id);
                    goto error;
                }
                if (!channelIds.add((lightChannels() - 1), dp.id)) {
                    DEBUG_MSG_P(PSTR("[TUYA] Channel for DP id=%u already exists\n"), dp.id);
                    goto error;
                }
#endif
                break;
            default:
                break;
            }
        }

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
        if (channelIds.size()) {
           lightSetProvider(std::make_unique<TuyaLightProvider>(channelIds));
        }
#endif

error:
        dps.clear();
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
            discovery.add(type, frame[0]);
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
                discovery.feed();
                state = State::DISCOVERY;
                break;
            }
            // parse known data protocols until discovery timeout expires
            case State::DISCOVERY:
            {
                if (discovery) {
                    DEBUG_MSG_P(PSTR("[TUYA] Discovery finished\n"));
                    updateDiscovered(std::move(discovery));
                    state = State::IDLE;
                }
                break;
            }
            // initial config is done, only doing heartbeat periodically
            case State::IDLE:
            {
                sendHeartbeat(Heartbeat::SLOW, state);
                break;
            }
        }

        if (TUYA_SERIAL && !outputFrames.empty()) {
            auto& frame = outputFrames.top();
            dataframeDebugSend("=>", frame);
            tuyaSerial.write(frame.serialize());
            outputFrames.pop();
        }

    }

    // Predefined DP<->SWITCH, DP<->CHANNEL associations
    // Respective provider setup should be called before state restore,
    // so we can use dummy values

    void setupSwitches() {
        bool done { false };
        for (unsigned char id = 0; id < RelaysMax; ++id) {
            auto dp = getSetting({"tuyaSwitch", id}, 0);
            if (!dp) {
                break;
            }

            if (relayAdd(std::make_unique<TuyaRelayProvider>(dp))) {
                switchIds.add((relayCount() - 1), dp);
                done = true;
            }
        }

        configDone = done;
    }

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM

    void setupChannels() {
        bool done { false };
        for (unsigned char id = 0; id < Light::ChannelsMax; ++id) {
            auto dp = getSetting({"tuyaChannel", id}, 0);
            if (!dp) {
                break;
            }

            if (lightAdd()) {
                channelIds.add((lightChannels() - 1), dp);
                done = true;
            }
        }

        if (done) {
            channelStateId = getSetting("tuyaChanState", 0u);
            lightSetProvider(std::make_unique<TuyaLightProvider>(channelIds, channelStateId));
        }

        configDone = done;
    }

#endif

    void setup() {

        // Print all known DP associations

        #if TERMINAL_SUPPORT

            terminalRegisterCommand(F("TUYA.SHOW"), [](const terminal::CommandContext& ctx) {
                ctx.output.printf_P(PSTR("Product: %s\n"), product.length() ? product.c_str() : "(unknown)");
                ctx.output.println(F("\nConfig:"));
                for (auto& kv : config) {
                    ctx.output.printf_P(PSTR("\"%s\" => \"%s\""), kv.key.c_str(), kv.value.c_str());
                }

                ctx.output.println(F("\nKnown DP(s):"));
                for (auto& entry : switchIds.map()) {
                    ctx.output.printf_P(PSTR("%u (bool) => %d (relay)\n"), entry.dp, entry.id);
                }
#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
                for (auto& entry : channelIds.map()) {
                    ctx.output.printf_P(PSTR("%u (integer) => %d (channel)\n"), entry.dp, entry.id);
                }
#endif
            });

            terminalRegisterCommand(F("TUYA.SAVE"), [](const terminal::CommandContext&) {
                for (auto& kv : config) {
                    setSetting(kv.key, kv.value);
                }
            });

        #endif

        // Filtering for incoming data
        // TODO: see https://github.com/xoseperez/espurna/issues/2222
        //       unlike emulator, this real device sends total garbage which we can safely ignore
        //       (may not be true for all, but that's the one we got the most of :>)
        filter = getSetting("tuyaFilter", false);

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
        setupChannels();
#endif

        setupSwitches();

        // Print all IN and OUT messages
        transportDebug = getSetting("tuyaDebug", true);

        // Install main loop method and WiFiStatus ping (only works with specific mode)
        TUYA_SERIAL.begin(SerialSpeed);

        ::espurnaRegisterLoop(loop);
        ::wifiRegister([](justwifi_messages_t code, char * parameter) {
            if ((MESSAGE_CONNECTED == code) || (MESSAGE_DISCONNECTED == code)) {
                sendWiFiStatus();
            }
        });
    }

}

#endif // TUYA_SUPPORT
