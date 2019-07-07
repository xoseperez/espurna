// ref: https://docs.tuya.com/en/mcu/mcu-protocol.html

#if TUYA_SUPPORT

#include <functional>
#include <queue>
#include <StreamString.h>

#include "tuya_types.h"
#include "tuya_transport.h"
#include "tuya_dataframe.h"
#include "tuya_protocol.h"
#include "tuya_util.h"

namespace Tuya {

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
    }

    // --------------------------------------------

    States<bool> switchStates(SWITCH_MAX);
    #if LIGHT_PROVIDER == LIGHT_PROVIDER_TUYA
        States<uint32_t> channelStates(DIMMER_MAX);
    #endif

    void pushOrUpdateState(const Type type, const DataFrame& frame) {
        if (Type::BOOL == type) {
            const DataProtocol<bool> proto(frame);
            switchStates.pushOrUpdate(proto.id(), proto.value());
            DEBUG_MSG_P(PSTR("[TUYA] apply BOOL id=%02u value=%s\n"), proto.id(), proto.value() ? "true" : "false");
        } else if (Type::INT == type) {
            #if LIGHT_PROVIDER == LIGHT_PROVIDER_TUYA
                const DataProtocol<uint32_t> proto(frame);
                channelStates.pushOrUpdate(proto.id(), proto.value());
                DEBUG_MSG_P(PSTR("[TUYA] apply  INT id=%02u value=%u\n"), proto.id(), proto.value());
            #endif
        }
    }

    void updateState(const Type type, const DataFrame& frame) {
        if (Type::BOOL == type) {
            const DataProtocol<bool> proto(frame);
            switchStates.update(proto.id(), proto.value());
        } else if (Type::INT == type) {
            #if LIGHT_PROVIDER == LIGHT_PROVIDER_TUYA
                const DataProtocol<uint32_t> proto(frame);
                channelStates.update(proto.id(), proto.value());
            #endif
        }
    }

    void applySwitch() {
        for (unsigned char id=0; id < switchStates.size(); ++id) {
            relayStatus(id, switchStates[id].value);
        }
    }

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_TUYA
        void applyDimmer() {
            for (unsigned char id=0; id < channelStates.size(); ++id) {
                lightChannel(id, channelStates[id].value);
            }
            lightUpdate(true, true);
        }
    #endif

    // --------------------------------------------

    Transport tuyaSerial(TUYA_SERIAL);
    std::queue<StaticDataFrame> outputFrames;

    DiscoveryTimeout discoveryTimeout(DISCOVERY_TIMEOUT);
    bool transportDebug = false;
    bool configDone = false;
    bool reportWiFi = false;
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
        if (!reportWiFi) return;
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
                DEBUG_MSG_P(PSTR("[TUYA] Unknown DP id=%u type=%u\n"), frame[0], frame[1]);
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
            if ((frame[0] == 0) || !configDone) {
                DEBUG_MSG_P(PSTR("[TUYA] Starting configuration ...\n"));
                state = State::QUERY_PRODUCT;
                return;
            } else if (state == State::HEARTBEAT) {
                DEBUG_MSG_P(PSTR("[TUYA] Already configured\n"));
                state = State::IDLE;
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
                reportWiFi = true;
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
        if (id >= switchStates.size()) return;
        outputFrames.emplace(StaticDataFrame{
            Command::SetDP, DataProtocol<bool>(switchStates[id].dp, switchStates[id].value).serialize()
        });
    }

    void tuyaSendSwitch(unsigned char id, bool value) {
        if (id >= switchStates.size()) return;
        if (value == switchStates[id].value) return;
        switchStates[id].value = value;
        tuyaSendSwitch(id);
    }

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_TUYA
        void tuyaSendChannel(unsigned char id) {
            if (id >= channelStates.size()) return;
            outputFrames.emplace(StaticDataFrame{
                Command::SetDP, DataProtocol<uint32_t>(channelStates[id].dp, channelStates[id].value).serialize()
            });
        }

        void tuyaSendChannel(unsigned char id, unsigned int value) {
            if (id >= channelStates.size()) return;
            if (value == channelStates[id].value) return;
            channelStates[id].value = value;
            tuyaSendChannel(id);
        }
    #endif

    void brokerCallback(const unsigned char type, const char * topic, unsigned char id, const char * payload) {

        // Only process status messages
        if (BROKER_MSG_TYPE_STATUS != type) return;

        unsigned char value = atoi(payload);

        #if (RELAY_PROVIDER == RELAY_PROVIDER_LIGHT) && (LIGHT_PROVIDER == LIGHT_PROVIDER_TUYA)
            if (strcmp(MQTT_TOPIC_CHANNEL, topic) == 0) {
                if (lightState(id) != switchStates[id].value) {
                    tuyaSendSwitch(id, value > 0);
                }
                if (lightState(id)) tuyaSendChannel(id, value);
                return;
            }

            if (strcmp(MQTT_TOPIC_RELAY, topic) == 0) {
                if (lightState(id) != switchStates[id].value) {
                    tuyaSendSwitch(id, bool(value));
                }
                if (lightState(id)) tuyaSendChannel(id, value);
                return;
            }
        #else
            if (strcmp(MQTT_TOPIC_CHANNEL, topic) == 0) {
                tuyaSendChannel(id, value);
                return;
            }

            if (strcmp(MQTT_TOPIC_RELAY, topic) == 0) {
                tuyaSendSwitch(id, bool(value));
                return;
            }
        #endif

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
            // parse known data protocols until discovery timeout expires
            case State::DISCOVERY:
            {
                if (discoveryTimeout) {
                    relaySetupDummy(switchStates.size());
                    #if LIGHT_PROVIDER == LIGHT_PROVIDER_TUYA
                        lightSetupChannels(channelStates.size());
                    #endif
                    state = State::IDLE;
                }
                break;
            }
            // initial config is done, only doing heartbeat periodically
            case State::IDLE:
            {
                if (switchStates.changed()) applySwitch();
                #if LIGHT_PROVIDER == LIGHT_PROVIDER_TUYA
                    if (channelStates.changed()) applyDimmer();
                #endif
                sendHeartbeat(Heartbeat::SLOW, state);
                break;
            }
        }

        if (TUYA_SERIAL && !outputFrames.empty()) {
            const DataFrame frame = std::move(outputFrames.front());
            dataframeDebugSend("OUT", frame);
            tuyaSerial.write(frame.serialize());
            outputFrames.pop();
        }

    }

    // Predefined DP<->SWITCH, DP<->CHANNEL associations
    // Respective provider setup should be called before state restore,
    // so we can use dummy values

    void tuyaSetupSwitch() {

        for (unsigned char n = 0; n < switchStates.capacity(); ++n) {
            if (!hasSetting("tuyaSwitch", n)) break;
            uint8_t dp = getSetting("tuyaSwitch", n, 0).toInt();
            switchStates.pushOrUpdate(dp, false);
        }
        relaySetupDummy(switchStates.size());

        if (switchStates.size()) configDone = true;

    }

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_TUYA
        void tuyaSetupLight() {

            for (unsigned char n = 0; n < channelStates.capacity(); ++n) {
                if (!hasSetting("tuyaChannel", n)) break;
                uint8_t dp = getSetting("tuyaChannel", n, 0).toInt();
                channelStates.pushOrUpdate(dp, 0);
            }
            lightSetupChannels(channelStates.size());

            if (channelStates.size()) configDone = true;

        }
    #endif

    void tuyaSetup() {

        // Print all known DP associations

        #if TERMINAL_SUPPORT
            terminalRegisterCommand(F("TUYA.TEST"), [](Embedis* e) {
                sendWiFiStatus();
            });
            terminalRegisterCommand(F("TUYA.INFO"), [](Embedis* e) {
                DEBUG_MSG_P(PSTR("[TUYA] Product: %s\n"), product.c_str());
                if (switchStates.size()) {
                    DEBUG_MSG_P(PSTR("[TUYA] BOOL: \n"));
                    for (unsigned char n=0; n < switchStates.size(); ++n) {
                        DEBUG_MSG_P(PSTR("[TUYA] %02u=%u\n"), switchStates[n].dp, switchStates[n].value);

                    }
                }
                if (switchStates.size()) {
                    DEBUG_MSG_P(PSTR("[TUYA] INT: \n"));
                    for (unsigned char n=0; n < channelStates.size(); ++n) {
                        DEBUG_MSG_P(PSTR("[TUYA] %02u=%03u\n"), channelStates[n].dp, channelStates[n].value);

                    }
                }
            });
        #endif

        // Print all IN and OUT messages

        transportDebug = getSetting("tuyaDebug", 1).toInt() == 1;

        // Install main loop method and WiFiStatus ping (only works with specific mode)

        TUYA_SERIAL.begin(SERIAL_SPEED);

        ::brokerRegister(brokerCallback);
        ::espurnaRegisterLoop(tuyaLoop);
        ::wifiRegister([](justwifi_messages_t code, char * parameter) {
            if ((MESSAGE_CONNECTED == code) || (MESSAGE_DISCONNECTED == code)) {
                sendWiFiStatus();
            }
        });
    }

}

#endif // TUYA_SUPPORT
