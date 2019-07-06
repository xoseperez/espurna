// ref: https://docs.tuya.com/en/mcu/mcu-protocol.html

#if LIGHT_PROVIDER == LIGHT_PROVIDER_TUYA

#include <functional>
#include <queue>
#include <StreamString.h>

#include "tuya_types.h"
#include "tuya_transport.h"
#include "tuya_dataframe.h"
#include "tuya_protocol.h"

namespace TuyaDimmer {

    constexpr const size_t SERIAL_SPEED = 9600;

    constexpr const unsigned char SWITCH_MAX {8u};
    constexpr const unsigned char DIMMER_MAX {5u};

    constexpr const uint32_t DISCOVERY_TIMEOUT = 1500;

    constexpr const uint32_t HEARTBEAT_SLOW = 9000;
    constexpr const uint32_t HEARTBEAT_FAST = 3000;

    // --------------------------------------------

    size_t getHeartbeatInterval(Heartbeat hb) {
        switch (hb) {
            case Heartbeat::FAST:
                return HEARTBEAT_FAST;
            case Heartbeat::SLOW:
                return HEARTBEAT_FAST;
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
    }

    // --------------------------------------------

    template <typename T>
    class States {

        public:

            struct Container {
                uint8_t dp;
                T value;
            };


            States(size_t size) :
                _size(size),
                _states(size)
            {}

            bool update(const uint8_t dp, const T value) {
                auto found = std::find_if(_states.begin(), _states.end(), [dp](const Container& internal) {
                    return dp == internal.dp;
                });

                if (found != _states.end()) {
                    if (found->value != value) {
                        found->value = value;
                        _changed = true;
                        return true;
                    }
                }

                return false;
            }

            void pushOrUpdate(const uint8_t dp, const T value) {
                if (_states.size() == _size) return;
                if (!update(dp, value)) {
                    _changed = true;
                    _states.emplace_back(States::Container{dp, value});
                }
            }

            bool changed() {
                bool res = _changed;
                if (_changed) _changed = false;
                return res;
            }

            Container& operator[] (const size_t n) {
                return _states[n];
            }

            size_t size() {
                return _states.size();
            }

        private:
            bool _changed = false;
            size_t _size = 0;
            std::vector<Container> _states;
    };

    States<bool> switchStates(SWITCH_MAX);
    States<uint32_t> channelStates(DIMMER_MAX);

    void pushOrUpdateState(const Type type, const DataFrame& frame) {
        if (Type::BOOL == type) {
            const DataProtocol<bool> proto(frame);
            switchStates.pushOrUpdate(proto.id(), proto.value());
        } else if (Type::INT == type) {
            const DataProtocol<uint32_t> proto(frame);
            channelStates.pushOrUpdate(proto.id(), proto.value());
        }
    }

    void updateState(const Type type, const DataFrame& frame) {
        if (Type::BOOL == type) {
            const DataProtocol<bool> proto(frame);
            switchStates.update(proto.id(), proto.value());
        } else if (Type::INT == type) {
            const DataProtocol<uint32_t> proto(frame);
            channelStates.update(proto.id(), proto.value());
        }
    }

    void applySwitch() {
        for (unsigned char id=0; id < switchStates.size(); ++id) {
            relayStatus(id, switchStates[id].value);
        }
    }

    void applyDimmer() {
        for (unsigned char id=0; id < channelStates.size(); ++id) {
            lightChannel(id, channelStates[id].value);
        }
    }

    class DiscoveryTimeout {
        public:
            DiscoveryTimeout(uint32_t start, uint32_t timeout) :
                _start(start),
                _timeout(timeout)
            {}

            DiscoveryTimeout(uint32_t timeout) :
                DiscoveryTimeout(millis(), timeout)
            {}

            operator bool() {
                return (millis() - _start > _timeout);
            }

            void feed() {
                _start = millis();
            }

        private:
            uint32_t _start;
            const uint32_t _timeout;
    };

    // --------------------------------------------

    Transport tuyaSerial(TUYA_SERIAL);
    std::queue<StaticDataFrame> outputFrames;

    DiscoveryTimeout discoveryTimeout(DISCOVERY_TIMEOUT);
    bool transportDebug = false;
    bool configDone = false;
    String product;

    inline void dataframeDebugSend(const char* tag, const DataFrame& frame) {
        if (!transportDebug) return;
        StreamString out;
        Output writer(out, frame.length);
        writer.writeHex(frame.serialize());
        DEBUG_MSG("[TUYA] %s: %s\n", tag, out.c_str());
    }

    void sendHeartbeat(Heartbeat hb, State state) {

        static uint32_t last = 0;
        if (millis() - last > getHeartbeatInterval(hb)) {
            outputFrames.emplace(StaticDataFrame{Command::Heartbeat});
            last = millis();
        }

    }

    void sendWiFiStatus() {
        outputFrames.emplace(StaticDataFrame{
            Command::WiFiStatus, {getWiFiState()}
        });
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
                DEBUG_MSG_P(PSTR("[TUYA] Unknown DP id=%u type=%u\n"), frame.cbegin()[0], frame.cbegin()[1]);
            } else {
                DEBUG_MSG_P(PSTR("[TUYA] Invalid DP frame\n"));
            }
            return;
        }

        if (state == State::DISCOVERY) {
            discoveryTimeout.feed();
            pushOrUpdateState(type, frame);
        } else {
            updateState(type, frame);
        }

    }

    void processFrame(State& state, const Transport& buffer) {

        const DataFrame frame {fromTransport(buffer)};

        dataframeDebugSend("IN", frame);

        // initial packet has 0, do the initial setup
        // all after that have 1. might be a good idea to re-do the setup when that happens on boot
        if (frame.commandEquals(Command::Heartbeat) && (frame.length == 1)) {
            if ((frame[0] == 0) && (!configDone)) {
                state = State::QUERY_PRODUCT;
                return;
            }
            sendWiFiStatus();
            return;
        }

        if (frame.commandEquals(Command::QueryProduct) && frame.length) {
            dataframeDebugSend("Product", frame);
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
                sendWiFiStatus();
            }
            state = State::QUERY_DP;
            return;
        }

        if (frame.commandEquals(Command::WiFiResetCfg) && !frame.length) {
            DEBUG_MSG_P(PSTR("[TUYA] WiFi reset request\n"));
            outputFrames.emplace(StaticDataFrame{Command::WiFiResetCfg});
            return;
        }

        if (frame.commandEquals(Command::WiFiResetSelect) && (frame.length == 1)) {
            DEBUG_MSG_P(PSTR("[TUYA] WiFi configuration mode request: %s\n"),
                (frame[0] == 0) ? "Smart Config" : "AP");
            outputFrames.emplace(StaticDataFrame{Command::WiFiResetSelect});
            return;
        }

        if (frame.commandEquals(Command::ReportDP) && frame.length) {
            processDP(state, frame);
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

    // Queue data when sending local state

    void tuyaSendSwitch(unsigned char id) {
        outputFrames.emplace(StaticDataFrame{
            Command::SetDP, DataProtocol<bool>(switchStates[id].dp, switchStates[id].value).serialize()
        });
    }

    void tuyaSendChannel(unsigned char id) {
        outputFrames.emplace(StaticDataFrame{
            Command::SetDP, DataProtocol<uint32_t>(channelStates[id].dp, channelStates[id].value).serialize()
        });
    }

    void tuyaSendSwitch(unsigned char id, bool value) {
        if (value == switchStates[id].value) return;
        switchStates[id].value = value;
        tuyaSendSwitch(id);
    }

    void tuyaSendChannel(unsigned char id, unsigned int value) {
        if (value == channelStates[id].value) return;
        channelStates[id].value = value;
        tuyaSendChannel(id);
    }

    // Main loop state machine. Process input data and manage output queue

    void tuyaLoop() {

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
            // general info about the device
            case State::QUERY_PRODUCT:
            {
                outputFrames.emplace(StaticDataFrame{Command::QueryProduct});
                state = State::IDLE;
                break;
            }
            // whether we control the led&button or not
            case State::QUERY_MODE:
            {
                outputFrames.emplace(StaticDataFrame{Command::QueryMode});
                state = State::IDLE;
                break;
            }
            // full read-out of the data protocol values
            case State::QUERY_DP:
            {
                outputFrames.emplace(StaticDataFrame{Command::QueryDP});
                discoveryTimeout.feed();
                state = State::DISCOVERY;
                break;
            }
            // parse known data protocols until
            case State::DISCOVERY:
            {
                if (discoveryTimeout) {
                    relaySetupDummy(switchStates.size());
                    lightSetupChannels(channelStates.size());
                    state = State::IDLE;
                }
                break;
            }
            // initial config is done, only doing heartbeat periodically
            case State::IDLE:
            {
                if (switchStates.changed()) applySwitch();
                if (channelStates.changed()) applyDimmer();
                sendHeartbeat(Heartbeat::SLOW, state);
                break;
            }
        }

        if (!outputFrames.empty()) {
            const DataFrame frame = std::move(outputFrames.front());
            outputFrames.pop();
            dataframeDebugSend("OUT", frame);
            tuyaSerial.write(frame);
        }

    }

    void tuyaSetup() {

        // Print all known DP associations

        #if TERMINAL_SUPPORT
            terminalRegisterCommand(F("TUYA.INFO"), [](Embedis* e) {
                DEBUG_MSG_P(PSTR("[TUYA] Product: %s\n"), product.c_str());
                if (switchStates.size()) {
                    DEBUG_MSG_P(PSTR("[TUYA] BOOL: %s\n"));
                    for (unsigned char n=0; n < switchStates.size(); ++n) {
                        DEBUG_MSG_P(PSTR("[TUYA] %u: %s\n"), switchStates[n].dp, switchStates[n].value ? "ON" : "OFF");

                    }
                }
                if (switchStates.size()) {
                    DEBUG_MSG_P(PSTR("[TUYA] INT: %s\n"));
                    for (unsigned char n=0; n < switchStates.size(); ++n) {
                        DEBUG_MSG_P(PSTR("[TUYA] %u: %u\n"), channelStates[n].dp, channelStates[n].value);

                    }
                }
            });
        #endif

        // Print all IN and OUT messages

        transportDebug = getSetting("tuyaDebug", 1).toInt() == 1;

        // Predefined DP<->SWITCH, DP<->CHANNEL associations

        for (unsigned char n=0; n<relayCount(); ++n) {
            if (!hasSetting("tuyaSwitch", n)) break;
            uint8_t dp = getSetting("tuyaSwitch", n).toInt();
            switchStates.pushOrUpdate(dp, relayStatus(n))
        }

        for (unsigned char n=0; n<channelCount(); ++n) {
            if (!hasSetting("tuyaChannel", n)) break;
            uint8_t dp = getSetting("tuyaChannel", n).toInt();
            channelStates.pushOrUpdate(dp, lightChannel(n))
        }

        // Install main loop method and WiFiStatus ping (only works with specific mode)

        TUYA_SERIAL.begin(SERIAL_SPEED);

        ::espurnaRegisterLoop(tuyaLoop);
        ::wifiRegister([](justwifi_messages_t code, char * parameter) {
            if ((MESSAGE_CONNECTED == code) || (MESSAGE_DISCONNECTED == code)) {
                sendWiFiStatus();
            }
        });
    }

}

#endif // LIGHT_PROVIDER_TUYA
