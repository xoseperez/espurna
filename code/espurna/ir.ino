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

// -----------------------------------------------------------------------------
// PRIVATE
// -----------------------------------------------------------------------------

void _irProcessCode(unsigned long code) {

    static unsigned long last_code;
    boolean found = false;

    // Repeat last valid code
    DEBUG_MSG_P(PSTR("[IR] Received 0x%06X\n"), code);
    if (code == 0xFFFFFFFF) {
        DEBUG_MSG_P(PSTR("[IR] Processing 0x%06X\n"), code);
        code = last_code;
    }

    for (unsigned char i = 0; i < IR_BUTTON_COUNT ; i++) {

        unsigned long button_code = pgm_read_dword(&IR_BUTTON[i][0]);
        if (code == button_code) {

            unsigned long button_mode = pgm_read_dword(&IR_BUTTON[i][1]);
            unsigned long button_value = pgm_read_dword(&IR_BUTTON[i][2]);

            if (button_mode == IR_BUTTON_MODE_STATE) {
                relayStatus(0, button_value);
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
            last_code = code;
            break;

		}

	}

	if (!found) {
		DEBUG_MSG_P(PSTR("[IR] Ignoring code\n"));
	}

}


// -----------------------------------------------------------------------------
// PUBLIC API
// -----------------------------------------------------------------------------

void irSetup() {

    _ir_recv = new IRrecv(IR_PIN);
    _ir_recv->enableIRIn();

    // Register loop
    espurnaRegisterLoop(irLoop);

}

void irLoop() {
    if (_ir_recv->decode(&_ir_results)) {
		unsigned long code = _ir_results.value;
		_irProcessCode(code);
		_ir_recv->resume(); // Receive the next value
	}
}

#endif // IR_SUPPORT
