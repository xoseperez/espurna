/*

TUYA MODULE

Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

// ref: https://docs.tuya.com/en/mcu/mcu-protocol.html

#include "espurna.h"

#if TUYA_SUPPORT

#include "light.h"
#include "relay.h"
#include "rpc.h"
#include "tuya.h"

#include "libs/OnceFlag.h"

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
        if ((lhs.command() != Command::Heartbeat) && (rhs.command() == Command::Heartbeat)) {
            return true;
        }

        return false;
    }

    constexpr unsigned long SerialSpeed { 9600u };

    constexpr unsigned long DiscoveryTimeout { 1500u };

    constexpr unsigned long HeartbeatSlow { 9000u };
    constexpr unsigned long HeartbeatFast { 3000u };
    constexpr unsigned long HeartbeatVeryFast { 200u };

    constexpr unsigned long HeartbeatIncrement { 200u };

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
    OnceFlag configDone;

    bool transportDebug { false };
    bool reportWiFi { false };
    bool filter { false };

    String product;
    std::forward_list<Config> config;

    DpMap switchIds;

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
    DpMap channelIds;
    StateId channelStateId;
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

        explicit TuyaLightProvider(const DpMap& channels, StateId* stateId) :
            _channels(channels),
            _stateId(stateId)
        {}

        void update() override {
        }

        // Channel values > 0 will switch the lights ON anyway
        void state(bool value) override {
            _last_state = value;
            if (*_stateId && !value) {
                send(_stateId->id(), value);
            }
        }

        void channel(size_t channel, float value) override {
            // XXX: can't handle channel values when OFF, and will turn the lights ON
            if (!_last_state) {
                return;
            }

            auto* entry = _channels.find_local(channel);
            if (!entry) {
                return;
            }

            // input dimmer channel value when lights are OFF is 16
            // for the same reason as above, don't send OFF values
            send(entry->dp_id, static_cast<unsigned int>(value));
        }

    private:
        const DpMap& _channels;
        bool _last_state { false };
        StateId* _stateId { nullptr };
    };

#endif

    // --------------------------------------------

    uint8_t getWiFiState() {
        if (wifiConnected()) {
            return 0x04;
        } else if (wifiConnectable()) {
            return 0x01;
        }

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
            addConfig("ledGpio0", String(led));
            addConfig("btnGpio0", String(rst));
            done = true;
        }
    }

    void showProduct() {
        DEBUG_MSG_P(PSTR("[TUYA] Product: %s\n"), product.length() ? product.c_str() : "(unknown)");
    }

    template <typename T>
    void dataframeDebugSend(const char* tag, const T& frame) {
        if (!transportDebug) return;
        StreamString out;
        Output writer(out, frame.length());
        writer.writeHex(frame.serialize());
        DEBUG_MSG("[TUYA] %s: %s\n", tag, out.c_str());
    }

    unsigned long heartbeatInterval(Heartbeat heartbeat) {
        static unsigned long interval { 0ul };

        switch (heartbeat) {
        case Heartbeat::Boot:
            if (interval < HeartbeatFast) {
                interval += HeartbeatIncrement;
            } else {
                interval = HeartbeatFast;
            }
            break;
        case Heartbeat::Fast:
            interval = HeartbeatFast;
            break;
        case Heartbeat::Slow:
            interval = HeartbeatSlow;
            break;
        case Heartbeat::None:
            interval = 0;
            break;
        }

        return interval;
    }

    void sendHeartbeat(Heartbeat heartbeat) {
        static unsigned long interval = 0ul;
        static unsigned long last = millis() + 1ul;

        if (millis() - last > interval) {
            interval = heartbeatInterval(heartbeat);
            last = millis();
            outputFrames.emplace(Command::Heartbeat);
        }
    }

    void sendWiFiStatus() {
        if (reportWiFi) {
            outputFrames.emplace(
                Command::WiFiStatus, std::initializer_list<uint8_t> { getWiFiState() }
            );
        }
    }

    void updateState(const DataProtocol<bool>& proto) {
#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
        if (channelStateId && (channelStateId.id() == proto.id())) {
            // See above. Ignore the selected state ID while we are sending the data,
            // to avoid resetting the state to ON while we are turning OFF
            // (and vice versa)
            if (!channelStateId.filter()) {
                lightState(proto.value());
            }
            return;
        }
#endif

        auto* entry = switchIds.find_dp(proto.id());
        if (!entry) {
            return;
        }

        relayStatus(entry->local_id, proto.value());
    }

    void updateState(const DataProtocol<uint32_t>& proto) {
    }

    // XXX: sometimes we need to ignore incoming state
    // ref: https://github.com/xoseperez/espurna/issues/1729#issuecomment-509234195
    template <typename T>
    void updateState(Type type, const T& frame) {
        if (Type::BOOL == type) {
            DataProtocol<bool> proto(frame.data());
            updateState(proto);
        } else if (Type::INT == type) {
            DataProtocol<uint32_t> proto(frame.data());
            updateState(proto);
        }
    }

    void updateDiscovered(Discovery&& discovery) {
        auto& dps = discovery.get();

        if (configDone) {
            goto error;
        }

        for (auto& dp : dps) {
            switch (dp.type) {
            case Type::BOOL:
                if (!switchIds.add(relayCount(), dp.id)) {
                    DEBUG_MSG_P(PSTR("[TUYA] Switch for DP id=%u already exists\n"), dp.id);
                    goto error;
                }
                if (!relayAdd(std::make_unique<TuyaRelayProvider>(dp.id))) {
                    DEBUG_MSG_P(PSTR("[TUYA] Cannot add relay for DP id=%u\n"), dp.id);
                    goto error;
                }
                break;
            case Type::INT:
#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
                if (!channelIds.add(lightChannels(), dp.id)) {
                    DEBUG_MSG_P(PSTR("[TUYA] Channel for DP id=%u already exists\n"), dp.id);
                    goto error;
                }
                if (!lightAdd()) {
                    DEBUG_MSG_P(PSTR("[TUYA] Cannot add channel for DP id=%u\n"), dp.id);
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

    template <typename T>
    void processDP(State state, const T& frame) {
        auto type = dataType(frame);
        if (Type::UNKNOWN == type) {
            if (frame.length() >= 2) {
                DEBUG_MSG_P(PSTR("[TUYA] Unknown DP id=%u type=%u\n"), frame[0], frame[1]);
            } else {
                DEBUG_MSG_P(PSTR("[TUYA] Invalid DP frame\n"));
            }
            return;
        }

        if (State::DISCOVERY == state) {
            discovery.add(type, frame[0]);
        } else if (!filter) {
            updateState(type, frame);
        }
    }

    void processFrame(State& state, const Transport& buffer) {

        const DataFrameView frame(buffer);

        dataframeDebugSend("<=", frame);

        // initial packet has 0, do the initial setup
        // all after that have 1. might be a good idea to re-do the setup when that happens on boot
        if ((frame.command() == Command::Heartbeat) && (frame.length() == 1)) {
            if (State::BOOT == state) {
                if (!configDone) {
                    DEBUG_MSG_P(PSTR("[TUYA] Attempting to configure the board ...\n"));
#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
                    setupChannels();
#endif
                    setupSwitches();
                }

                DEBUG_MSG_P(PSTR("[TUYA] Starting discovery\n"));
                state = State::QUERY_PRODUCT;
                return;
            }

            sendWiFiStatus();
            return;
        }

        if ((frame.command() == Command::QueryProduct) && frame.length()) {
            if (product.length()) {
                product = "";
            }
            product.reserve(frame.length());
            for (unsigned int n = 0; n < frame.length(); ++n) {
                product += static_cast<char>(frame[n]);
            }
            showProduct();
            state = State::QUERY_MODE;
            return;
        }

        if (frame.command() == Command::QueryMode) {
            // first and second byte are GPIO pin for WiFi status and RST respectively
            if (frame.length() == 2) {
                DEBUG_MSG_P(PSTR("[TUYA] Mode: ESP only, led=GPIO%02u rst=GPIO%02u\n"), frame[0], frame[1]);
                updatePins(frame[0], frame[1]);
            // ... or nothing. we need to report wifi status to the mcu via Command::WiFiStatus
            } else if (!frame.length()) {
                DEBUG_MSG_P(PSTR("[TUYA] Mode: ESP & MCU\n"));
                reportWiFi = true;
                sendWiFiStatus();
            }
            state = State::QUERY_DP;
            return;
        }

        if ((frame.command() == Command::WiFiResetCfg) && !frame.length()) {
            DEBUG_MSG_P(PSTR("[TUYA] WiFi reset request\n"));
            outputFrames.emplace(Command::WiFiResetCfg);
            return;
        }

        if ((frame.command() == Command::WiFiResetSelect) && (frame.length() == 1)) {
            DEBUG_MSG_P(PSTR("[TUYA] WiFi configuration mode request: %s\n"),
                (frame[0] == 0) ? "Smart Config" : "AP");
            outputFrames.emplace(Command::WiFiResetSelect);
            return;
        }

        if ((frame.command() == Command::ReportDP) && frame.length()) {
            processDP(state, frame);
            if (state == State::DISCOVERY) return;
            if (state == State::BOOT) return;
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
                state = State::BOOT;
            case State::BOOT:
                sendHeartbeat(Heartbeat::Boot);
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
                DEBUG_MSG_P(PSTR("[TUYA] Querying DP(s)\n"));
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
                sendHeartbeat(Heartbeat::Slow);
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

    namespace build {

    constexpr unsigned char channelDpId(size_t index) {
        return (index == 0) ? TUYA_CH1_DPID :
            (index == 1) ? TUYA_CH2_DPID :
            (index == 2) ? TUYA_CH3_DPID :
            (index == 3) ? TUYA_CH4_DPID :
            (index == 4) ? TUYA_CH5_DPID : 0u;
    }

    constexpr unsigned char switchDpId(size_t index) {
        return (index == 0) ? TUYA_SW1_DPID :
            (index == 1) ? TUYA_SW2_DPID :
            (index == 2) ? TUYA_SW3_DPID :
            (index == 3) ? TUYA_SW4_DPID :
            (index == 4) ? TUYA_SW5_DPID :
            (index == 5) ? TUYA_SW6_DPID :
            (index == 6) ? TUYA_SW7_DPID :
            (index == 7) ? TUYA_SW8_DPID : 0u;
    }

    constexpr unsigned char channelStateDpId() {
        return TUYA_CH_STATE_DPID;
    }

    } // namespace build

    // Predefined DP<->SWITCH, DP<->CHANNEL associations
    // Respective provider setup should be called before state restore,
    // so we can use dummy values

    void setupSwitches() {
        bool done { false };
        for (size_t id = 0; id < RelaysMax; ++id) {
            auto dp = getSetting({"tuyaSwitch", id}, build::switchDpId(id));
            if (!dp) {
                break;
            }

            if (!switchIds.add(relayCount(), dp)) {
                break;
            }

            if (!relayAdd(std::make_unique<TuyaRelayProvider>(dp))) {
                break;
            }

            done = true;
        }

        if (done) {
            configDone.set();
        }
    }

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM

    void setupChannels() {
        bool done { false };
        for (size_t id = 0; id < Light::ChannelsMax; ++id) {
            auto dp = getSetting({"tuyaChannel", id}, build::channelDpId(id));
            if (!dp) {
                break;
            }

            if (!channelIds.add(lightChannels(), dp)) {
                break;
            }

            if (!lightAdd()) {
                break;
            }

            done = true;
        }

        if (done) {
            channelStateId = getSetting("tuyaChanState", build::channelStateDpId());
            lightSetProvider(std::make_unique<TuyaLightProvider>(channelIds, &channelStateId));
        }

        if (done) {
            configDone.set();
        }
    }

#endif

    void setup() {

        // Print all known DP associations

        #if TERMINAL_SUPPORT

            terminalRegisterCommand(F("TUYA.SHOW"), [](::terminal::CommandContext&& ctx) {
                ctx.output.printf_P(PSTR("Product: %s\n"), product.length() ? product.c_str() : "(unknown)");

                ctx.output.print(F("\nConfig:\n"));
                for (auto& kv : config) {
                    ctx.output.printf_P(PSTR("\"%s\" => \"%s\"\n"), kv.key.c_str(), kv.value.c_str());
                }

                ctx.output.print(F("\nKnown DP(s):\n"));
#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
                if (channelStateId) {
                    ctx.output.printf_P(PSTR("%u (bool) => lights state\n"), channelStateId.id());
                }
                for (auto& entry : channelIds.map()) {
                    ctx.output.printf_P(PSTR("%u (int) => %d (channel)\n"), entry.dp_id, entry.local_id);
                }
#endif
                for (auto& entry : switchIds.map()) {
                    ctx.output.printf_P(PSTR("%u (bool) => %d (relay)\n"), entry.dp_id, entry.local_id);
                }
            });

            terminalRegisterCommand(F("TUYA.SAVE"), [](::terminal::CommandContext&&) {
                for (auto& kv : config) {
                    setSetting(kv.key, kv.value);
                }
            });

        #endif

        // Print all IN and OUT messages
        transportDebug = getSetting("tuyaDebug", 1 == TUYA_DEBUG_ENABLED);

        // Whether to ignore the incoming state messages
        filter = getSetting("tuyaFilter", 1 == TUYA_FILTER_ENABLED);

        // Install main loop method and WiFiStatus ping (only works with specific mode)
        TUYA_SERIAL.begin(SerialSpeed);

        ::espurnaRegisterLoop(loop);
        ::wifiRegister([](wifi::Event event) {
            if ((event == wifi::Event::StationConnected) || (event == wifi::Event::StationDisconnected)) {
                sendWiFiStatus();
            }
        });
    }

}

#endif // TUYA_SUPPORT
