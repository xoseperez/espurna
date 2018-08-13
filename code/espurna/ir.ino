/*

IR MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2017-2018 by François Déchery

*/

#if IR_SUPPORT

#include <IRremoteESP8266.h>
#include <IRrecv.h>

IRrecv * _ir_recv;
decode_results _ir_results;
unsigned long _ir_last_toggle = 0;

// -----------------------------------------------------------------------------
// PRIVATE
// -----------------------------------------------------------------------------

void _irProcessCode(unsigned long code, unsigned char type) {

    // Check valid code
    unsigned long last_code = 0;
    unsigned long last_time = 0;
    if (code == 0xFFFFFFFF) return;
    if (type == 0xFF) return;
    if ((last_code == code) && (millis() - last_time < IR_DEBOUNCE)) return;
    last_code = code;
    DEBUG_MSG_P(PSTR("[IR] Received 0x%08X (%d)\n"), code, type);

    #if IR_BUTTON_SET > 0

        boolean found = false;

        for (unsigned char i = 0; i < IR_BUTTON_COUNT ; i++) {

            uint32_t button_code = pgm_read_dword(&IR_BUTTON[i][0]);
            if (code == button_code) {

                unsigned long button_mode = pgm_read_dword(&IR_BUTTON[i][1]);
                unsigned long button_value = pgm_read_dword(&IR_BUTTON[i][2]);

                if (button_mode == IR_BUTTON_MODE_STATE) {
                    relayStatus(0, button_value);
                }

                if (button_mode == IR_BUTTON_MODE_TOGGLE) {
                    relayToggle(button_value);
                }

                #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

                    if (button_mode == IR_BUTTON_MODE_BRIGHTER) {
                        lightBrightnessStep(button_value ? 1 : -1);
                        nice_delay(150); //debounce
                    }

                    if (button_mode == IR_BUTTON_MODE_RGB) {
                        lightColor(button_value);
                    }

                    /*
                    #if LIGHT_PROVIDER == LIGHT_PROVIDER_FASTLED
                        if (button_mode == IR_BUTTON_MODE_EFFECT) {
                            _buttonAnimMode(button_value);
                        }
                    #endif
                    */

                    /*
                    if (button_mode == IR_BUTTON_MODE_HSV) {
                        lightColor(button_value);
                    }
                    */

                    lightUpdate(true, true);

                #endif

                found = true;
                break;

    		}

    	}

    	if (!found) {
    		DEBUG_MSG_P(PSTR("[IR] Ignoring code\n"));
    	}

    #endif

    #if MQTT_SUPPORT
        char buffer[16];
        snprintf_P(buffer, sizeof(buffer), "0x%08X", code);
        mqttSend(MQTT_TOPIC_IR, buffer);
    #endif

}

// -----------------------------------------------------------------------------
// PUBLIC API
// -----------------------------------------------------------------------------

void irSetup() {

    _ir_recv = new IRrecv(IR_RECEIVER_PIN);
    _ir_recv->enableIRIn();

    // Register loop
    espurnaRegisterLoop(irLoop);

}

void irLoop() {
    if (_ir_recv->decode(&_ir_results)) {
		_irProcessCode(_ir_results.value, _ir_results.decode_type);
		_ir_recv->resume(); // Receive the next value
	}
}

#endif // IR_SUPPORT
