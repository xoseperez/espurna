/*

RF MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if RF_SUPPORT

#include <RCSwitch.h>

RCSwitch * _rfModem;

unsigned long _rf_learn_start = 0;
unsigned char _rf_learn_id = 0;
bool _rf_learn_status = true;
bool _rf_learn_active = false;

// -----------------------------------------------------------------------------
// RF
// -----------------------------------------------------------------------------

#if RF_BUTTON_SET > 0
unsigned long _rf_home_code;
void _rfProcessCode(unsigned long code) {

    static unsigned long _rf_last_toggle = 0;
    boolean found = false;

    // Repeat last valid code
    DEBUG_MSG_P(PSTR("[RF] Trying to match code 0x%06X with RF remote\n"), code);

    #if defined(RF_BUTTON_LEARN_CODE) && defined(RF_BUTTON_LEARN_MASK)
    // Learning of a Home Code by pressing a defined button for a while until the code was received ten times in series
    static unsigned long _rf_last_learn = 0;
    static unsigned char _rf_learn_count = 0;

    if ( (code & RF_BUTTON_LEARN_MASK) == RF_BUTTON_LEARN_CODE) { // check if code matches the defined learn button
        if ( (millis() - _rf_last_learn) < 1000 ) { // check if time since last press is less than a second
            _rf_learn_count++;
        } else {
            _rf_learn_count = 1;
        }
        _rf_last_learn = millis();

        if (_rf_learn_count == 10) {
            _rf_home_code = code & (0xFFFFFF ^ RF_BUTTON_LEARN_MASK);
            setSetting("rfHomeCode", _rf_home_code);
            DEBUG_MSG_P(PSTR("[RF] RF remote home code learned 0x%06X\n"), _rf_home_code);
        }
    } else { // reset counter if other button
        _rf_learn_count = 0;
        _rf_last_learn = 0;
    }
    #endif

    for (unsigned char i = 0; i < RF_BUTTON_COUNT ; i++) {

        unsigned long button_code = pgm_read_dword(&RF_BUTTON[i][0]) | _rf_home_code;
        if (code == button_code) {

            unsigned long button_mode = pgm_read_dword(&RF_BUTTON[i][1]);
            unsigned long button_value = pgm_read_dword(&RF_BUTTON[i][2]);

            if (button_mode == RF_BUTTON_MODE_STATE) {
                relayStatus(0, button_value);
            }

            if (button_mode == RF_BUTTON_MODE_TOGGLE) {

                if (millis() - _rf_last_toggle > 250){
                    relayToggle(button_value);
                    _rf_last_toggle = millis();
                } else {
                    DEBUG_MSG_P(PSTR("[RF] Ignoring repeated code\n"));
                }
            }

            #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

                if (button_mode == RF_BUTTON_MODE_BRIGHTER) {
                    lightBrightnessStep(button_value ? 1 : -1);
                    nice_delay(150); //debounce
                }

                if (button_mode == RF_BUTTON_MODE_RGB) {
                    lightColor(button_value);
                }

                // Mode Buttons used for color temperature
                if (button_mode == RF_BUTTON_MODE_MODE) {
                    _fromMireds(_light_mireds + (button_value ? 50 : -50));
                    nice_delay(150); //debounce
                }

                // Speed Buttons used for color Saturation
                if (button_mode == RF_BUTTON_MODE_SPEED) {
                    String hsv = lightColor(false);                           // 240,100,90
                    String h = hsv.substring(0,hsv.indexOf(",")+1);            // 240,
                    String s = hsv.substring(h.length(),hsv.lastIndexOf(",")); // 100
                    String v = hsv.substring(hsv.lastIndexOf(","));            // ,90
                    int saturation = s.toInt();
                    saturation = constrain( (saturation + (button_value ? 10 : -10)) , 0, 100);
                    hsv = h;
                    hsv.concat(saturation);
                    hsv.concat(v);
                    lightColor(hsv.c_str(), false);
                }

                lightUpdate(true, true);

            #endif

            found = true;
            break;

        }

    }

    if (!found) {
        DEBUG_MSG_P(PSTR("[RF] Ignoring code\n"));
    }

}
#endif

unsigned long _rfRetrieve(unsigned char id, bool status) {
    String code = getSetting(status ? "rfbON" : "rfbOFF", id, "0");
    return strtoul(code.c_str(), 0, 16);
}

void _rfStore(unsigned char id, bool status, unsigned long code) {
    DEBUG_MSG_P(PSTR("[RF] Storing %d-%s => %X\n"), id, status ? "ON" : "OFF", code);
    char buffer[20];
    snprintf_P(buffer, sizeof(buffer), PSTR("%X"), code);
    setSetting(status ? "rfbON" : "rfbOFF", id, buffer);
}

void _rfLearn(unsigned char id, bool status) {
    _rf_learn_start = millis();
    _rf_learn_id = id;
    _rf_learn_status = status;
    _rf_learn_active = true;
}

void _rfForget(unsigned char id, bool status) {

    delSetting(status ? "rfbON" : "rfbOFF", id);

    // Websocket update
    #if WEB_SUPPORT
        char wsb[100];
        snprintf_P(wsb, sizeof(wsb), PSTR("{\"rfb\":[{\"id\": %d, \"status\": %d, \"data\": \"\"}]}"), id, status ? 1 : 0);
        wsSend(wsb);
    #endif

}


bool _rfMatch(unsigned long code, unsigned char& relayID, unsigned char& value) {

    bool found = false;
    DEBUG_MSG_P(PSTR("[RF] Trying to match code %X\n"), code);

    for (unsigned char i=0; i<relayCount(); i++) {

        unsigned long code_on = _rfRetrieve(i, true);
        unsigned long code_off = _rfRetrieve(i, false);

        if (code == code_on) {
            DEBUG_MSG_P(PSTR("[RF] Match ON code for relay %d\n"), i);
            value = 1;
            found = true;
        }

        if (code == code_off) {
            DEBUG_MSG_P(PSTR("[RF] Match OFF code for relay %d\n"), i);
            if (found) value = 2;
            found = true;
        }

        if (found) {
            relayID = i;
            return true;
        }

    }

    return false;

}

// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

void _rfWebSocketOnSend(JsonObject& root) {
    char buffer[20];
    root["rfbVisible"] = 1;
    root["rfbCount"] = relayCount();
    JsonArray& rfb = root.createNestedArray("rfb");
    for (byte id=0; id<relayCount(); id++) {
        for (byte status=0; status<2; status++) {
            JsonObject& node = rfb.createNestedObject();
            snprintf_P(buffer, sizeof(buffer), PSTR("%X"), _rfRetrieve(id, status == 1));
            node["id"] = id;
            node["status"] = status;
            node["data"] = String(buffer);
        }
    }
}

void _rfWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "rfblearn") == 0) _rfLearn(data["id"], data["status"]);
    if (strcmp(action, "rfbforget") == 0) _rfForget(data["id"], data["status"]);
    if (strcmp(action, "rfbsend") == 0) _rfStore(data["id"], data["status"], data["data"].as<long>());
}

// -----------------------------------------------------------------------------

void rfLoop() {

    if (_rfModem->available()) {

        static unsigned long last = 0;
        if (millis() - last > RF_DEBOUNCE) {
            last = millis();

            if (_rfModem->getReceivedValue() > 0) {

                unsigned long rf_code = _rfModem->getReceivedValue();

                DEBUG_MSG_P(PSTR("[RF] Received code: %X\n"), rf_code);

                if (_rf_learn_active) {

                    _rf_learn_active = false;

                    _rfStore(_rf_learn_id, _rf_learn_status, rf_code);

                    // Websocket update
                    #if WEB_SUPPORT
                        char wsb[100];
                        snprintf_P(
                            wsb, sizeof(wsb),
                            PSTR("{\"rfb\":[{\"id\": %d, \"status\": %d, \"data\": \"%X\"}]}"),
                            _rf_learn_id, _rf_learn_status ? 1 : 0, rf_code);
                        wsSend(wsb);
                    #endif

                } else {

                    unsigned char id;
                    unsigned char value;
                    if (_rfMatch(rf_code, id, value)) {
                        if (2 == value) {
                            relayToggle(id);
                        } else {
                            relayStatus(id, 1 == value);
                        }
                    } else {
                        #if RF_BUTTON_SET > 0
                            _rfProcessCode(rf_code);
                        #endif 
                    }

                }

            }

        }

        _rfModem->resetAvailable();

    }

    if (_rf_learn_active && (millis() - _rf_learn_start > RF_LEARN_TIMEOUT)) {
        _rf_learn_active = false;
    }

}

void rfSetup() {

    #if RF_ENABLE_PIN
        pinMode(RF_ENABLE_PIN, OUTPUT);
        #if RF_ENABLE_PIN_INVERSE == 1
            digitalWrite(RF_ENABLE_PIN,LOW);
            DEBUG_MSG_P(PSTR("[RF] RF enable PIN on GPIO %u set to LOW\n"), RF_ENABLE_PIN);
        #else
            digitalWrite(RF_ENABLE_PIN,HIGH);
            DEBUG_MSG_P(PSTR("[RF] RF enable PIN on GPIO %u set to HIGH\n"), RF_ENABLE_PIN);
        #endif
    #endif

    _rfModem = new RCSwitch();
    _rfModem->enableReceive(digitalPinToInterrupt(RF_PIN));
    DEBUG_MSG_P(PSTR("[RF] RF receiver on GPIO %u\n"), RF_PIN);

    #if WEB_SUPPORT
        wsOnSendRegister(_rfWebSocketOnSend);
        wsOnActionRegister(_rfWebSocketOnAction);
    #endif

    #if RF_BUTTON_SET > 0
        _rf_home_code = getSetting("rfHomeCode", 0x000000).toInt();
        DEBUG_MSG_P(PSTR("[RF] RF home code 0x%06X\n"), _rf_home_code);
    #endif

    // Register loop
    espurnaRegisterLoop(rfLoop);

}

#endif
