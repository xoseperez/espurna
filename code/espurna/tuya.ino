// ref: https://docs.tuya.com/en/mcu/mcu-protocol.html

#if LIGHT_PROVIDER == LIGHT_PROVIDER_TUYA

#include <queue>
#include <StreamString.h>

#include "tuya_states.h"
#include "tuya_serialbuffer.h"
#include "tuya_dataframe.h"

namespace TuyaDimmer {

    constexpr size_t SERIAL_SPEED = 9600;

    size_t getHeartbeatInterval(Heartbeat hb) {
        switch (hb) {
            case Heartbeat::NONE: return 0;
            case Heartbeat::FAST: return 3000;
            case Heartbeat::SLOW: return 9000;
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

    SerialBuffer serialBuffer(TUYA_SERIAL);
    std::queue<payload_t> outputData;

    uint8_t switchDP = TUYA_SWITCH_DP;
    uint8_t dimmerDP = TUYA_DIMMER_DP;

    void sendHeartbeat(Heartbeat hb, State state) {

        static uint32_t last = 0;
        if (millis() - last > getHeartbeatInterval(hb)) {
            outputData.emplace(payload_t{Command::Heartbeat});
            last = millis();
        }

    }

    // 2 known Data Protocols:
    // - 5 bytes, switch - 0x01(switch) 0x01(bool) 0x00 0x01(8 bits) 0x00/0x01(bool value)
    // - 8 bytes, dimmer - 0x??(percent) 0x02(int) 0x00 0x04(32 bits) 0x00 0x00 0x00 0x00
    void processDP(const DataFrame& frame) {
        if (switchDP == frame.data[0]) {
            if (frame.length != 5) return;
            if ((frame.data[1] != 0x01) || (frame.data[3] == 0x01)) return;
            relayStatus(0, frame.data[4]);
        } else if (dimmerDP == frame.data[0]) {
            if (frame.length != 8) return;
            if ((frame.data[1] != 0x02) || (frame.data[3] == 0x02)) return;
            lightBrightness(frame.data[7]);
            lightUpdate(true, true);
        } else {
            DEBUG_MSG_P(PSTR("[TUYA] Unknown DP id=%u type=%u\n"), frame.data[0], frame.data[1]);
        }
    }

    void processFrame(State& state, const SerialBuffer& buffer) {

        const DataFrame frame(buffer);

        // initial packet has 0
        if ((frame & Command::Heartbeat) && (frame.length == 1) && (frame.data[0] == 0)) {
            if (state == State::HEARTBEAT) {
                state = State::UPDATE_WIFI;
            } else {
                state = State::QUERY_PRODUCT;
            }
            return;
        }

        // all after that have 1. might be a good idea to re-do the setup when that happens on boot
        if ((frame & Command::Heartbeat) && (frame.length == 1) && (frame.data[0] == 1)) {
            if (state == State::HEARTBEAT) {
                state = State::QUERY_PRODUCT;
            } else {
                state = State::UPDATE_WIFI;
            }
            return;
        }

        if ((frame & Command::QueryProduct) && frame.length) {
            // XXX: add debug writer(data, length)
            StreamString buffer;
            buffer.reserve(frame.length + 1);
            frame.printTo(buffer);
            DEBUG_MSG_P(PSTR("[TUYA] Product: %s\n"), buffer.c_str());
            state = State::QUERY_MODE;
            return;
        }

        if (frame & Command::QueryMode) {
            // first and second byte are GPIO pin for WiFi status and RST respectively
            if (frame.length == 2) {
                DEBUG_MSG_P(PSTR("[TUYA] Mode: ESP only, led=GPIO%02u rst=GPIO%02u\n"), frame.data[0], frame.data[1]);
                updatePins(frame.data[0], frame.data[1]);
                state = State::QUERY_DP;
            // ... or nothing. we need to report wifi status to the mcu via Command::WiFiStatus
            } else if (!frame.length) {
                DEBUG_MSG_P(PSTR("[TUYA] Mode: ESP & MCU\n"));
                state = State::UPDATE_WIFI;
            }
            return;
        }

        if ((frame & Command::WiFiStatus) && !frame.length) {
            state = State::QUERY_DP;
            return;
        }

        if ((frame & Command::ReportDP) && frame.length) {
            processDP(frame);
            state = State::IDLE;
            return;
        }

    }

    void processSerial(State& state) {

        while (serialBuffer.available()) {

            serialBuffer.read();

            if (serialBuffer.done()) {
                processFrame(state, serialBuffer);
                serialBuffer.reset();
            }

            if (serialBuffer.full()) {
                serialBuffer.rewind();
                serialBuffer.reset();
            }
        }

    }

    void tuyaSendSwitch(bool state) {
        outputData.emplace(payload_t{Command::SetDP, {
            switchDP, 0x01, 0x00, 0x01, state ? uint8_t(1) : uint8_t(0)
        }});
    }

    void tuyaSendBrightness(unsigned char value) {
        outputData.emplace(payload_t{Command::SetDP, {
            dimmerDP, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, value
        }});
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
                serialBuffer.rewind();
                state = State::HEARTBEAT;
            case State::HEARTBEAT:
                sendHeartbeat(Heartbeat::FAST, state);
                break;
            // general info about the device
            case State::QUERY_PRODUCT:
            {
                outputData.emplace(payload_t{Command::QueryProduct});
                break;
            }
            // whether we control the led&button or not
            case State::QUERY_MODE:
            {
                outputData.emplace(payload_t{Command::QueryMode});
                break;
            }
            // if we don't, send out current wifi status
            case State::UPDATE_WIFI:
            {
                outputData.emplace(payload_t{
                    Command::WiFiStatus, {getWiFiState()}
                });
                break;
            }
            // full read-out of the data protocol values
            case State::QUERY_DP:
            {
                outputData.emplace(payload_t{Command::QueryDP});
                break;
            }
            // initial config is done, only doing heartbeat periodically
            case State::IDLE:
                sendHeartbeat(Heartbeat::SLOW, state);
                if (!outputData.empty()) {
                    //const payload_t& payload = outputData.front();
                    DataFrame frame(outputData.front());
                    frame.printTo(TUYA_SERIAL);
                    outputData.pop();
                }
                break;
        }

    }

    void tuyaSetup() {
        TUYA_SERIAL.begin(SERIAL_SPEED);

        switchDP = getSetting("tuyaSwitchDP", TUYA_SWITCH_DP).toInt();
        dimmerDP = getSetting("tuyaDimmerDP", TUYA_DIMMER_DP).toInt();

        ::espurnaRegisterLoop(tuyaLoop);
    }

}

#endif // LIGHT_PROVIDER_TUYA
