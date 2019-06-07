// ref: https://docs.tuya.com/en/mcu/mcu-protocol.html

#if LIGHT_PROVIDER == LIGHT_PROVIDER_TUYA

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

    // TODO: attach to Serial object, remove other Serial use in code
    // TODO: attach to arbitrary stream object, allow to mock or use SoftwareSerial
    SerialBuffer serialBuffer;

    void sendHeartbeat(Heartbeat hb, State state) {

        static uint32_t last = 0;
        if (millis() - last > getHeartbeatInterval(hb)) {
            DataFrame frame(Command::Heartbeat);
            frame.printTo(Serial);
            last = millis();
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
            char* buffer = new char[frame.length + 1];
            os_memset(buffer, 0, frame.length + 1);
            os_memcpy(buffer, frame.data, frame.length);
            DEBUG_MSG_P(PSTR("[TUYA] Product: %s\n"), buffer);
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
        }

        // 2 known Data Protocols:
        // - 5 bytes, switch - 0x01(switch) 0x01(bool) 0x00 0x01(8 bits) 0x00/0x01(bool value)
        // - 8 bytes, dimmer - 0x02(percent) 0x02(int) 0x00 0x04(32 bits) 0x00 0x00 0x00 0x00
        if ((frame & Command::ReportDP) && frame.length) {
            switch (frame.data[0]) {
                case 0x01:
                    if (frame.length != 5) break;
                    if ((frame.data[1] != 0x01) || (frame.data[3] == 0x01)) break;
                    relayStatus(0, frame.data[4]);
                    break;
                case 0x02:
                    if (frame.length != 8) break;
                    if ((frame.data[1] != 0x02) || (frame.data[3] == 0x02)) break;
                    lightBrightness(frame.data[7]);
                    break;
                default:
                    DEBUG_MSG_P(PSTR("[TUYA] Unknown DP id=%u type=%u\n"), frame.data[0], frame.data[1]);
            }
            state = State::IDLE;
        }

    }

    void processSerial(State& state) {

        while (Serial.available()) {

            serialBuffer.read(Serial.read());

            if (serialBuffer.done()) {
                processFrame(state, serialBuffer);
                serialBuffer.reset();
            }

            if (serialBuffer.full()) {
                while(Serial.read() != -1);
                serialBuffer.reset();
            }
        }

    }

    void tuyaLoop() {

        // TODO: switch heartbeat to polledTimeout

        static State state = State::INIT;

        // running this before anything else to quickly switch to the required state
        processSerial(state);

        // go through the initial setup step-by-step, as described in
        // https://docs.tuya.com/en/mcu/mcu-protocol.html#21-basic-protocol
        switch (state) {
            // send fast heartbeat until mcu responds with something
            case State::INIT:
                // flush serial buffer before transmitting anything
                while (Serial.read() != -1);
            case State::HEARTBEAT:
                sendHeartbeat(Heartbeat::FAST, state);
                break;
            case State::QUERY_PRODUCT:
            {
                DataFrame frame(Command::QueryProduct);
                frame.printTo(Serial);
                break;
            }
            // whether we control the led&button or not
            case State::QUERY_MODE:
            {
                DataFrame frame(Command::QueryMode);
                frame.printTo(Serial);
                break;
            }
            // full read-out of the data protocol values
            case State::QUERY_DP:
            {
                DataFrame frame(Command::QueryDP);
                frame.printTo(Serial);
                break;
            }
            // initial config is done, only doing heartbeat periodically
            case State::IDLE:
                sendHeartbeat(Heartbeat::SLOW, state);
                break;
        }

    }

}

void tuyaSetup() {
    Serial.begin(TuyaDimmer::SERIAL_SPEED);
    espurnaRegisterLoop(TuyaDimmer::tuyaLoop);
}

void tuyaSendSwitch(bool state) {
    uint8_t data[] = {0x01,0x01,0x00,0x01, state ? uint8_t(1) : uint8_t(0)};
    TuyaDimmer::DataFrame frame(TuyaDimmer::Command::SetDP, data, 5);
    frame.printTo(Serial);
}

void tuyaSendBrightness(uint8_t value) {
    uint8_t data[] = {0x02,0x02,0x00,0x04,0x00,0x00,0x00,value};
    TuyaDimmer::DataFrame frame(TuyaDimmer::Command::SetDP, data, 8);
    frame.printTo(Serial);
} 

#endif // LIGHT_PROVIDER_TUYA
