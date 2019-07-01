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

    constexpr size_t SERIAL_SPEED = 9600;

    size_t getHeartbeatInterval(Heartbeat hb) {
        switch (hb) {
            case Heartbeat::FAST:
                return 3000;
            case Heartbeat::SLOW:
                return 9000;
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

    BufferedTransport tuyaSerial(TUYA_SERIAL);
    std::queue<payload_t> outputData;

    constexpr const unsigned char SWITCH_MAX {8u};
    constexpr const unsigned char DIMMER_MAX {5u};

    template <typename DT>
    class States {

        public:

            States(size_t size) {
                _size = size;
                _states.reserve(size);
            }

            bool update(const DT& data) {
                auto found = std::find_if(_states.begin(), _states.end(), [&data](const DT& internal) {
                    return data.dp == internal.dp;
                });

                if (found != _states.end()) {
                    if (found->value != data.value) {
                        found->value = data.value;
                        _changed = true;
                        return true;
                    }
                }

                return false;
            }

            void pushOrUpdate(const DT& data) {
                if (_states.size() == _size) return;
                if (!update(data)) {
                    _changed = true;
                    _states.push_back(data);
                }
            } 

            bool changed() {
                bool res = _changed;
                if (_changed) _changed = false;
                return res;
            }

            DT& operator[] (const size_t n) {
                return _states[n];
            }

            size_t size() {
                return _states.size();
            }

        private:
            bool _changed = false;
            size_t _size = 0;
            std::vector<DT> _states;
    };

    States<switch_t> switchStates(SWITCH_MAX);
    States<dimmer_t> dimmerStates(DIMMER_MAX);

    void pushOrUpdateState(const Type type, const DataFrame& frame) {
        if (Type::BOOL == type) switchStates.pushOrUpdate(dataParse<switch_t>(frame));
        if (Type::INT == type) dimmerStates.pushOrUpdate(dataParse<dimmer_t>(frame));
    }

    void updateState(const Type type, const DataFrame& frame) {
        if (Type::BOOL == type) switchStates.update(dataParse<switch_t>(frame));
        if (Type::INT == type) dimmerStates.update(dataParse<dimmer_t>(frame));
    }

    void applySwitch() {
        for (unsigned char id=0; id < switchStates.size(); ++id) {
            relayStatus(id, switchStates[id].value);
        }
    }

    void applyDimmer() {
        for (unsigned char id=0; id < dimmerStates.size(); ++id) {
            lightChannel(id, dimmerStates[id].value);
        }
    }

    class DiscoveryTimeout {
        public:
            DiscoveryTimeout(uint32_t start, uint32_t timeout) :
                _start(start),
                _timeout(timeout)
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

    DiscoveryTimeout discoveryTimeout(0, 1500);
    bool transportDebug = false;
    bool configDone = false;
    String product;

    inline void dataframeDebugSend(const char* tag, const DataFrame& frame) {
        if (!transportDebug) return;
        StreamString out;
        out.reserve((frame.length * 2) + 1);
        BufferedTransport writer(out);
        writer.write<PrintHex>(frame);
        DEBUG_MSG("[TUYA] %s: %s\n", tag, out.c_str());
    }

    void sendHeartbeat(Heartbeat hb, State state) {

        static uint32_t last = 0;
        if (millis() - last > getHeartbeatInterval(hb)) {
            outputData.emplace(payload_t{Command::Heartbeat});
            last = millis();
        }

    }

    void sendWiFiStatus() {
        outputData.emplace(payload_t{
            Command::WiFiStatus, {getWiFiState()}
        });
    }

    void processDP(State state, const DataFrame& frame) {

        if (!frame.length) {
            DEBUG_MSG_P(PSTR("[TUYA] DP frame must have data\n"));
            return;
        }

        const Type type{dataType(frame)};
        if (Type::UNKNOWN == type) {
            DEBUG_MSG_P(PSTR("[TUYA] Unknown DP id=%u type=%u\n"), frame.data[0], frame.data[1]);
            return;
        }

        if (state == State::DISCOVERY) {
            discoveryTimeout.feed();
            pushOrUpdateState(type, frame);
        } else {
            updateState(type, frame);
        }

    }

    void processFrame(State& state, const BufferedTransport& buffer) {

        const DataFrame frame {fromTransport(buffer)};

        dataframeDebugSend("IN", frame);

        // initial packet has 0, do the initial setup
        // all after that have 1. might be a good idea to re-do the setup when that happens on boot
        if ((frame & Command::Heartbeat) && (frame.length == 1)) {
            if ((frame.data[0] == 0) && (!configDone)) {
                state = State::QUERY_PRODUCT;
                return;
            }
            sendWiFiStatus();
            return;
        }

        if ((frame & Command::QueryProduct) && frame.length) {
            dataframeDebugSend("Product", frame);
            state = State::QUERY_MODE;
            return;
        }

        if (frame & Command::QueryMode) {
            // first and second byte are GPIO pin for WiFi status and RST respectively
            if (frame.length == 2) {
                DEBUG_MSG_P(PSTR("[TUYA] Mode: ESP only, led=GPIO%02u rst=GPIO%02u\n"), frame.data[0], frame.data[1]);
                updatePins(frame.data[0], frame.data[1]);
            // ... or nothing. we need to report wifi status to the mcu via Command::WiFiStatus
            } else if (!frame.length) {
                DEBUG_MSG_P(PSTR("[TUYA] Mode: ESP & MCU\n"));
                sendWiFiStatus();
            }
            state = State::QUERY_DP;
            return;
        }

        if ((frame & Command::WiFiResetCfg) && !frame.length) {
            DEBUG_MSG_P(PSTR("[TUYA] WiFi reset request\n"));
            outputData.emplace(payload_t{Command::WiFiResetCfg});
            return;
        }

        if ((frame & Command::WiFiResetSelect) && (frame.length == 1)) {
            DEBUG_MSG_P(PSTR("[TUYA] WiFi configuration mode request: %s\n"),
                (frame.data[0] == 0) ? "Smart Config" : "AP");
            outputData.emplace(payload_t{Command::WiFiResetSelect});
            return;
        }

        if ((frame & Command::ReportDP) && frame.length) {
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

    void tuyaSendSwitch(unsigned char id) {
        outputData.emplace(setDP(switchStates[id]));
    }

    void tuyaSendChannel(unsigned char id) {
        outputData.emplace(setDP(dimmerStates[id]));
    } 

    void tuyaSendSwitch(unsigned char id, bool value) {
        if (value == switchStates[id].value) return;
        switchStates[id].value = value;
        outputData.emplace(setDP(switchStates[id]));
    } 

    void tuyaSendChannel(unsigned char id, unsigned int value) {
        if (value == dimmerStates[id].value) return;
        dimmerStates[id].value = value;
        outputData.emplace(setDP(dimmerStates[id]));
    } 

    void tuyaLoop() {

        // TODO: switch heartbeat to polledTimeout

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
                outputData.emplace(payload_t{Command::QueryProduct});
                state = State::IDLE;
                break;
            }
            // whether we control the led&button or not
            case State::QUERY_MODE:
            {
                outputData.emplace(payload_t{Command::QueryMode});
                state = State::IDLE;
                break;
            }
            // full read-out of the data protocol values
            case State::QUERY_DP:
            {
                outputData.emplace(payload_t{Command::QueryDP});
                discoveryTimeout.feed();
                state = State::DISCOVERY;
                break;
            }
            // parse known data protocols until 
            case State::DISCOVERY:
            {
                if (discoveryTimeout) {
                    relaySetupDummy(switchStates.size());
                    lightSetupChannels(dimmerStates.size());
                    state = State::IDLE;
                }
                break;
            }
            // initial config is done, only doing heartbeat periodically
            case State::IDLE:
            {
                if (switchStates.changed()) applySwitch();
                if (dimmerStates.changed()) applyDimmer();
                sendHeartbeat(Heartbeat::SLOW, state);
                break;
            }
        }

        if (!outputData.empty()) {
            const DataFrame frame = fromPayload(outputData.front());
            dataframeDebugSend("OUT", frame);
            tuyaSerial.write(frame);
            outputData.pop();
        }

    }

    void tuyaSetup() {
        TUYA_SERIAL.begin(SERIAL_SPEED);

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
                    DEBUG_MSG_P(PSTR("[TUYA] %u: %u\n"), dimmerStates[n].dp, dimmerStates[n].value);

                }
            }
        });

        transportDebug = getSetting("tuyaDebug", 1).toInt() == 1;

        ::espurnaRegisterLoop(tuyaLoop);
        ::wifiRegister([](justwifi_messages_t code, char * parameter) {
            if ((MESSAGE_CONNECTED == code) || (MESSAGE_DISCONNECTED == code)) {
                sendWiFiStatus();
            }
        });
    }

}

#endif // LIGHT_PROVIDER_TUYA
